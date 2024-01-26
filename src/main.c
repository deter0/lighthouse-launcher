#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include <dirent.h>

#include "raylib.h"

#define SV_IMPLEMENTATION
#include "sv/sv.h"

#include "desktop_file_parser.h"
#include "slurp.h"
#include "trie.h"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 500

#define SEARCH_BUFFER_MAX_LEN 32

// #define USE_SDF_FONTS
Shader sdf_shader;

#define ARRAY_LEN(xs) (sizeof(xs)/sizeof(xs[0]))

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

Font load_sdf_font(const char *font_path) {
	Font fontSDF = { 0 };
	
	int font_file_size = 0;
	unsigned char *font_file_data = LoadFileData(font_path, &font_file_size);

	fontSDF.baseSize = 32;
	fontSDF.glyphCount = 95;
	
	fontSDF.glyphs = LoadFontData(font_file_data, font_file_size, fontSDF.baseSize, 0, 0, FONT_SDF);
	
	Image atlas = GenImageFontAtlas(fontSDF.glyphs, &fontSDF.recs, fontSDF.glyphCount, fontSDF.baseSize, 0, 1);
	fontSDF.texture = LoadTextureFromImage(atlas);
	
	UnloadImage(atlas);
	UnloadFileData(font_file_data);

	SetTextureFilter(fontSDF.texture, TEXTURE_FILTER_BILINEAR);    // Required for SDF font

	return fontSDF;
}

char modifiers[] = {
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

const char *get_icon_file_path(const char *icon_name) {

}

int main(void) {
	LighthouseDesktopEntries entries = get_application_files("/usr/share/applications");

	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lighthouse Launcher");
	SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

	SetWindowMaxSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	TrieNode *app_name_search_trie = trie_alloc_node();
	for (size_t i = 0; i < entries.entries_count; i++) {
		trie_push_text(app_name_search_trie, TextToLower(entries.entries[i].name), (void*)&entries.entries[i]);
	}

	// Load SDF required shader (we use default vertex shader)

	Font prompt_ttf = { 0 };
#ifdef USE_SDF_FONTS
	prompt_ttf = load_sdf_font("./res/Prompt-Regular.ttf");
	sdf_shader = LoadShader(0, "./res/sdf_font.fs");
#else
	prompt_ttf = LoadFontEx("./res/Prompt-Regular.ttf", 32, NULL, 0);
	sdf_shader = LoadShader(0, 0);
#endif

	SetTargetFPS(60);

	const float text_size = 32.f;
	const float padding = 4.f;
	
	bool quit = false;

	int mouse_x = GetMouseX(), mouse_y = GetMouseY();
	DisableCursor();

	char search_buffer[SEARCH_BUFFER_MAX_LEN] = { 0 };
	int search_buffer_len = 0;

	WordPool trie_search_result = { 0 };
	trie_search(app_name_search_trie, "", &trie_search_result);

	int selected_entry_index = 0;
	
	while (!WindowShouldClose() && !quit) {
		if (IsKeyReleased(KEY_ESCAPE)) {
			CloseWindow();	
		}

		BeginDrawing();
		
		ClearBackground(BLACK);

		int char_pressed = 0;
		bool update_results = false;
		
		while ((char_pressed = GetCharPressed()) != 0) {
			printf("Typed: %c\n", (char)char_pressed);

			if (search_buffer_len + 1 < SEARCH_BUFFER_MAX_LEN) {
				search_buffer[search_buffer_len++] = (char)char_pressed;
				search_buffer[search_buffer_len] = 0;
				assert(search_buffer_len < SEARCH_BUFFER_MAX_LEN);

				update_results = true;
			}
		}

		if ((IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE)) && search_buffer_len > 0) {
			if (IsKeyDown(KEY_LEFT_CONTROL)) { // Delete words
				if (search_buffer_len > 0) {
					bool first_non_space_char_found = false;
					
					for (int i = search_buffer_len - 1; i >= 0; i--) {
						if (!isspace(search_buffer[i])) {
							first_non_space_char_found = true;
						}
							
						if (i == 0) {
							search_buffer[i] = 0;
							search_buffer_len = i;
							break;
						} else if (search_buffer[i] == ' ' && first_non_space_char_found) {
							search_buffer[i + 1] = 0;
							search_buffer_len = i + 1;
							break;
						}
					}
				}
			} else {
				search_buffer[--search_buffer_len] = 0;
				assert(search_buffer_len >= 0);
			}
			update_results = true;
		}

		if (IsKeyPressed(KEY_UP)) {
			selected_entry_index += 1;
		}
		if (IsKeyPressed(KEY_DOWN)) {
			selected_entry_index -= 1;
		}
		if (IsKeyPressed(KEY_ENTER)) {
			TrieWord current_selected_word = trie_search_result.words[selected_entry_index];
			assert(current_selected_word.user_ptr != NULL);

			LighthouseDesktopEntry *selected_entry = (LighthouseDesktopEntry*)current_selected_word.user_ptr;
			printf("Opening: `%s`, Command: `%s`\n", selected_entry->name, selected_entry->exec);

			int status = execute_desktop_file(selected_entry);
			printf("Executed program stripped of arguments, status: %d.\n", status);

			quit = true;
		}
		selected_entry_index = (size_t)selected_entry_index > trie_search_result.words_count ? (int)trie_search_result.words_count - 1 : selected_entry_index;

		if (update_results) {
			trie_search_result.words_count = 0;
			
			trie_search(app_name_search_trie, TextToLower(search_buffer), &trie_search_result);

			for (size_t i = 0; i < trie_search_result.words_count; i++) {
				printf("Suggestion: %s\n", trie_search_result.words[i].word);
			}
		}

		static int height = text_size + padding * 2;
		DrawRectangle(0, GetRenderHeight() - height, GetRenderWidth(), height, WHITE);

		Vector2 search_buffer_dims = MeasureTextEx(prompt_ttf, search_buffer, text_size, 0.f);
		Vector2 search_buffer_start = (Vector2){ padding, GetRenderHeight() - text_size - padding };

		// Draw Cursor
		DrawRectangle(search_buffer_start.x + search_buffer_dims.x, search_buffer_start.y, 1, text_size, BLACK);

		BeginShaderMode(sdf_shader);
			DrawTextEx(prompt_ttf, search_buffer, search_buffer_start, text_size, 0.f, BLACK);
		EndShaderMode();
		
		BeginShaderMode(sdf_shader);
			for (size_t i = 0; i < trie_search_result.words_count; i++) {
				LighthouseDesktopEntry *entry = (LighthouseDesktopEntry*)trie_search_result.words[i].user_ptr;
				Color text_color = { 111, 111, 111, 255 };

				if ((int)i == selected_entry_index) {
					text_color = WHITE;
				}
				
				DrawTextEx(prompt_ttf, entry->name, (Vector2){0, GetRenderHeight() - height - (text_size * (i+1))}, text_size, 0.f, text_color);
			}
		EndShaderMode();
		
		EndDrawing();

		if (!IsWindowFocused()) {
			quit = true;
		}
	}

	EnableCursor();
	SetMousePosition(mouse_x, mouse_y);
	
	CloseWindow();

	free(entries.entries);

	return 0;
}
