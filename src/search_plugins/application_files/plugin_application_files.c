#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include <ctype.h>
#include <dirent.h>

#include "../../search_plugin.h"

#include "../../common/trie.h"
#include "../../common/slurp.h"

#include "./desktop_file_parser.h"

// Lighthouse Desktop File Search Plugin
// Jan 26, 2024                   deter0

#ifndef ARRAY_LEN
#define ARRAY_LEN(xs) (sizeof(xs)/sizeof(xs[0]))
#endif

typedef struct {
	LighthouseDesktopEntry *entries;
	size_t entries_allocated;
	size_t entries_count;
} LighthouseDesktopEntries;

TrieNode *app_name_search_trie = NULL;
LighthouseDesktopEntries entries = { 0 };

static const char *get_filename_ext(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if(!dot || dot == filename) return "";
  return dot + 1;
}

LighthouseDesktopEntries collect_application_files(const char *directory) {
	DIR *fd;
	struct dirent *dir_entry;

	LighthouseDesktopEntries desktop_entries = { 0 };
	desktop_entries.entries_allocated = 128;
	desktop_entries.entries = malloc(sizeof(LighthouseDesktopEntry) * desktop_entries.entries_allocated);
	assert(desktop_entries.entries != NULL);

	if ((fd = opendir(directory)) == NULL) {
		fprintf(stderr, "Error: opening folder: %s:%s.\n", directory, strerror(errno));
		return desktop_entries;
	}
	
	while ((dir_entry = readdir(fd)) != NULL) {
		if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
			continue;
		}
		
		char full_name[4096] = { 0 };
		snprintf(full_name, 4096, "%s/%s", directory, dir_entry->d_name);
		
		if (dir_entry->d_type == DT_DIR) {
			printf("%s is a dir. skipping.\n", full_name);
		} else {
			const char *extension = get_filename_ext(full_name);
			if (strcmp(extension, "desktop") == 0) {
				//printf("Parsing Desktop File: %s\n", full_name);
				LighthouseDesktopEntry desktop_entry = get_desktop_file_info(full_name);
				//printf("\tName: '%s', Categories: '%s', Generic Name: '%s', Icon: '%s'\n", desktop_entry.name, desktop_entry.categories, desktop_entry.generic_name, desktop_entry.icon);

				LighthouseDesktopEntry *entry_in_list = &desktop_entries.entries[desktop_entries.entries_count++];
				memcpy(entry_in_list, &desktop_entry, sizeof(LighthouseDesktopEntry));

				if (desktop_entries.entries_count >= desktop_entries.entries_allocated) {
					desktop_entries.entries_allocated = desktop_entries.entries_allocated * 2 + 1;
					desktop_entries.entries = realloc(desktop_entries.entries, sizeof(LighthouseDesktopEntry) * desktop_entries.entries_allocated);
					assert(desktop_entries.entries);
				}
			} else {
				printf("Skipped File: %s\n", full_name);
			}
		}
	}

	closedir(fd);

	return desktop_entries;
}


static const char modifiers[] = {
	'f',
	'F',
	'u',
	'U',
	'd',
	'D',
	'n',
	'N',
	'i',
	'c',
	'k',
	'v',
	'm'
};

int execute_desktop_file(LighthouseDesktopEntry *entry) {
	assert(entry);

	char exec_string[MAX_SMALL_STRING_LEN];
	memcpy(exec_string, entry->exec, MAX_SMALL_STRING_LEN);

	size_t exec_string_len = strlen(exec_string);
	assert(exec_string_len < MAX_SMALL_STRING_LEN);

	char exec_string_stripped[MAX_SMALL_STRING_LEN] = { 0 };
	size_t exec_stripped_len = 0;

	size_t modifiers_count = 0;
	for (size_t j = 0; j <= exec_string_len; j++) {
		if (exec_string[j] == '%' && (j + 1) < exec_string_len) {
			char after_percent = exec_string[j + 1];
			bool valid_modifier_found = false;

			for (size_t i = 0; i < ARRAY_LEN(modifiers); i++) {
				if (after_percent == modifiers[i]) {
					valid_modifier_found = true;
					modifiers_count++;
					j++; // Skip character
					break;
				}
			}

			if (!valid_modifier_found) {
				fprintf(stderr, "Exec string modifier unknown: %%%c.\n", after_percent);  
			}
		} else {
			exec_string_stripped[exec_stripped_len++] = exec_string[j];
		}
	}

	printf("Modifiers Count: %zu\n", modifiers_count);
	printf("New String: `%s`\n", exec_string_stripped);

	assert(strlen(exec_string_stripped) == exec_string_len-2*modifiers_count);

	return execl("/bin/sh", "sh", "-c", exec_string_stripped , "&", (char *) NULL);
	// return system(exec_string_new);
}


bool search_plugin_init(void) {
	if ((app_name_search_trie = trie_alloc_node()) == NULL) {
		return false;
	}
	
  entries = collect_application_files("/usr/share/applications");
  for (size_t i = 0; i < entries.entries_count; i++) {
		char lower_string[MAX_SMALL_STRING_LEN] = { 0 };
		for (size_t j = 0; j <= strlen(entries.entries[i].name); j++) {
			lower_string[j] = tolower(entries.entries[i].name[j]);
		}
		
		trie_push_text(app_name_search_trie, lower_string, (void*)&entries.entries[i]);
	}

	return true;
}

// TODO(deter): For optimizations we can pass max-results from lighthouse main to here? we could break early then
SearchPluginResult *search_plugin_query(const char *query) {
	char *query_lower = strdup(query);
	for(size_t i = 0; query_lower[i]; i++){
		query_lower[i] = tolower(query_lower[i]);
	}
	printf("Query: %s, Lower: %s\n", query, query_lower);

	WordPool trie_search_result = { 0 };

  // TrieWord current_selected_word = trie_search_result.words[selected_entry_index];
  // assert(current_selected_word.user_ptr != NULL);

  // LighthouseDesktopEntry *selected_entry = (LighthouseDesktopEntry*)current_selected_word.user_ptr;
  // printf("Opening: `%s`, Command: `%s`\n", selected_entry->name, selected_entry->exec);

  // int status = execute_desktop_file(selected_entry);
  // printf("Executed program stripped of arguments, status: %d.\n", status);
  
  trie_search(app_name_search_trie, query_lower, &trie_search_result);

  SearchPluginResult *plugin_results = calloc(trie_search_result.words_count, sizeof(*plugin_results));
  for (size_t i = 0; i < trie_search_result.words_count; i++) {
    SearchPluginResult *result = &plugin_results[i];
    result->results_count = trie_search_result.words_count;
    result->user_ptr = trie_search_result.words[i].user_ptr;
    memcpy(result->name, trie_search_result.words[i].word, MAX_SMALL_STRING_LEN);
  }

  return plugin_results;
}

#ifdef PLUGIN_APPLICATION_FILES_TEST

#define SV_IMPLEMENTATION
#include "sv/sv.h"

int main() {
	search_plugin_init();

	{
		SearchPluginResult *results = search_plugin_query("");
		for (size_t i = 0; i < results[0].results_count; i++) {
			SearchPluginResult *result = &results[i];
			printf("%s\n", ((LighthouseDesktopEntry*)result->user_ptr)->name);
		}
		assert(results[0].results_count > 0);
		free(results);
	}
	printf("-------------------\n");

	{
		SearchPluginResult *results = search_plugin_query("vim");
		for (size_t i = 0; i < results[0].results_count; i++) {
			SearchPluginResult *result = &results[i];
			printf("%s\n", result->name);
		}
		assert(results[0].results_count == 1);
		assert(strcmp(results[0].name, "vim") == 0);
		free(results);
	}
	printf("-------------------\n");
	
	{
		SearchPluginResult *results = search_plugin_query("VIM");
		for (size_t i = 0; i < results[0].results_count; i++) {
			SearchPluginResult *result = &results[i];
			printf("%s\n", result->name);
		}
		assert(results[0].results_count == 1);
		assert(strcmp(results[0].name, "vim") == 0);
		free(results);
	}
	printf("-------------------\n");

	return 0;
}

#endif

