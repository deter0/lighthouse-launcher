#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <dirent.h>

#include "raylib.h"

#define SV_IMPLEMENTATION
#include "sv/sv.h"

#include "desktop_file_parser.h"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 400

const char *get_filename_ext(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if(!dot || dot == filename) return "";
  return dot + 1;
}

typedef struct {
	LighthouseDesktopEntry *entries;
	size_t entries_allocated;
	size_t entries_count;
} LighthouseDesktopEntries;

LighthouseDesktopEntries get_application_files(const char *directory) {
	DIR *fd; // File Descriptor
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
				printf("Parsing Desktop File: %s\n", full_name);
				LighthouseDesktopEntry desktop_entry = get_desktop_file_info(full_name);
				printf("\tName: '%s', Categories: '%s', Generic Name: '%s', Icon: '%s'\n", desktop_entry.name, desktop_entry.categories, desktop_entry.generic_name, desktop_entry.icon);

				LighthouseDesktopEntry *entry_in_list = &desktop_entries.entries[desktop_entries.entries_count++];
				memcpy(entry_in_list, &desktop_entry, sizeof(LighthouseDesktopEntry));

				if (desktop_entries.entries_count >= desktop_entries.entries_allocated) {
					desktop_entries.entries_allocated *= 2;
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

int main(void) {
	get_application_files("/usr/share/applications");
	return 0;
	
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lighthouse Launcher");
	SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

	SetWindowMaxSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	while (!WindowShouldClose()) {
		if (IsKeyReleased(KEY_ESCAPE)) {
			CloseWindow();	
		}

		
		
		BeginDrawing();	
		ClearBackground(RED);
		EndDrawing();
	}

	CloseWindow();

	return 0;
}
