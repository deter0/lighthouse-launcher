#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <dirent.h>

#include "raylib.h"

#define _GNU_SOURCE
#include <dlfcn.h>

#ifndef SV_IMPLEMENTATION
#define SV_IMPLEMENTATION
#endif

#include "sv/sv.h"

#include "ui_provider.h"
#include "image_cache.h"
#include "search_plugin.h"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 500

#define SEARCH_BUFFER_MAX_LEN MAX_SMALL_STRING_LEN
static char search_buffer[SEARCH_BUFFER_MAX_LEN] = { 0 };
static int search_buffer_len = 0;

#define MAX_SEARCH_PLUGINS 8
static SearchPlugin search_plugins[MAX_SEARCH_PLUGINS] = { 0 };
static size_t search_plugins_count = 0;

#define ARRAY_LEN(xs) (sizeof(xs)/sizeof(xs[0]))

static UIProvider default_ui_provider = { 0 };

#define LOAD_FUNC(HANDLE, FUNC_NAME, PROVIDER)  PROVIDER->FUNC_NAME = dlsym(HANDLE, #FUNC_NAME); \
	if (!PROVIDER->FUNC_NAME) { \
		fprintf(stderr, "\tYour UI theme does not have an `" #FUNC_NAME "` function.\n"); \
		fprintf(stderr, "\t\tdlerror: `%s`.\n", dlerror()); \
		return false; \
	}

bool load_ui_functions(const char *ui_so_path, UIProvider *ui_provider) {
	void *ui_so_handle = dlopen(ui_so_path, RTLD_NOW | RTLD_GLOBAL);
	if (!ui_so_handle) {
		fprintf(stderr, "Error loading ui theme: `%s`.\n", ui_so_path);
		fprintf(stderr, "\t\tdlerror: `%s`.\n", dlerror());
		return false;
	}

	printf("Loading functions from UI .so file: `%s`.\n", ui_so_path);

	LOAD_FUNC(ui_so_handle, ui_init, ui_provider);
	LOAD_FUNC(ui_so_handle, ui_get_background_color, ui_provider);
	LOAD_FUNC(ui_so_handle, ui_draw_user_input_field, ui_provider);
	LOAD_FUNC(ui_so_handle, ui_draw_entry, ui_provider);
	LOAD_FUNC(ui_so_handle, ui_draw_entry_group, ui_provider);
	
	return true;
}

bool load_plugin(const char *plugin_so_path, SearchPlugin *new_plugin) {
	void *plugin_so_handle = dlopen(plugin_so_path, RTLD_NOW | RTLD_GLOBAL);
	if (!plugin_so_handle) {
		fprintf(stderr, "Error opening search plugin: `%s`.\n", plugin_so_path);
		fprintf(stderr, "\tdlerror: `%s`.\n", dlerror());
		return false;
	}

	LOAD_FUNC(plugin_so_handle, search_plugin_init, new_plugin);
	LOAD_FUNC(plugin_so_handle, search_plugin_query, new_plugin);
	LOAD_FUNC(plugin_so_handle, search_plugin_execute, new_plugin);

	return true;
}

static const char *get_filename_ext(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if(!dot || dot == filename) return "";
  return dot + 1;
}

void load_search_plugins_from_dir(const char *directory_path) {
	DIR *plugin_dir;
	struct dirent *plugin_dir_entry;
	
	if ((plugin_dir = opendir(directory_path)) == NULL) {
		fprintf(stderr, "Error: opening search plugin directory: %s:%s.\n", directory_path, strerror(errno));
	}

	while ((plugin_dir_entry = readdir(plugin_dir)) != NULL) {
		if (strcmp(plugin_dir_entry->d_name, ".") == 0 || strcmp(plugin_dir_entry->d_name, "..") == 0) {
			continue;
		}
		
		char file_full_path[4096] = { 0 };
		snprintf(file_full_path, 4096, "%s/%s", directory_path, plugin_dir_entry->d_name);

		if (plugin_dir_entry->d_type != DT_DIR) {
			const char *extension = get_filename_ext(file_full_path);
			if (strcmp(extension, "so") == 0) {
				printf(".SO plugin: `%s`\n", file_full_path);

				int index = strlen(file_full_path) - 3 - 2;
				bool is_ui_file = false;
				if (index > 0 && index < (int)strlen(file_full_path)) {
					const char *ending = file_full_path + (strlen(file_full_path) - 3 - 2);
					if (strcmp(ending, "ui.so") == 0) {
						is_ui_file = true;
						printf("\tFlagged as UI file skipping in plugin load (`%s`)\n", file_full_path);
					}
				}

				if (!is_ui_file) {
					SearchPlugin new_plugin = { 0 };
					
					size_t plugin_name_str_size = strlen(plugin_dir_entry->d_name);
					memcpy(new_plugin.plugin_file_name, plugin_dir_entry->d_name, plugin_name_str_size < MAX_SMALL_STRING_LEN ? plugin_name_str_size : MAX_SMALL_STRING_LEN - 1);

					assert(load_plugin(file_full_path, &new_plugin) != false);
					printf("\t✅ Plugin Loaded!\n");

					memcpy(&search_plugins[search_plugins_count++], &new_plugin, sizeof(new_plugin));
				}
			}
		}
	}
}

static void search_buffer_backspace(bool ctrl);

int search_results_sort_cmp(const void *aa, const void *bb) {
	SearchPluginResult *a = (SearchPluginResult*)aa;
	SearchPluginResult *b = (SearchPluginResult*)bb;

	// printf("Comparing: %s <=> %s.\n", a->name, b->name);
	
	if (a->score <= b->score) {
		return 1;
	} else {
		return -1;
	}
}

int main(void) {
	assert(load_ui_functions("./plugins/default_ui.so", &default_ui_provider) != false);
	load_search_plugins_from_dir("./plugins");
	
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lighthouse Launcher");
	SetWindowState(FLAG_WINDOW_TOPMOST);

	printf("Init Search Plugins.\n");
	for (size_t i = 0; i < search_plugins_count; i++) {
		printf("\tInitializing Search Plugin: `%s`.\n", search_plugins[i].plugin_file_name);
		printf("\n==================== PLUGIN LOG ====================\n");  
		SearchPluginMetadata metadata = search_plugins[i].search_plugin_init();
		printf("==================== END    LOG ====================\n\n");
		if (metadata.init_status != false) {
			printf("\t✅ Initalized Search Plugin Succesfully.\n\n");
			search_plugins[i].plugin_metadata = metadata;
		} else {
			printf("\t❌ Failed to Initalize Search Plugin.\n\n");
		}
	}
	
	SetWindowMaxSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	bool quit = false;
	
	SetTargetFPS(60);
	DisableCursor();
	
	default_ui_provider.ui_init();

	#define CURRENT_RESULTS_MAX 8
	SearchPluginResult *current_results[8] = { 0 };
	size_t current_results_count = 0;

	SearchPluginResult *selected_result = NULL;
	SearchPluginResult *garbage = NULL;

	ImageCache icon_cache = create_image_cache();

	while (!WindowShouldClose() && !quit) {
		if (IsKeyReleased(KEY_ESCAPE)) {
			CloseWindow();	
		}

		BeginDrawing();
		ClearBackground(default_ui_provider.ui_get_background_color());

		DrawFPS(20, 20);

		bool requery_plugins = false;
		
		int char_input = 0;
		while ((char_input = GetCharPressed()) != 0) {
			if (search_buffer_len < SEARCH_BUFFER_MAX_LEN - 1) {
				search_buffer[search_buffer_len++] = (char)char_input;
				search_buffer[search_buffer_len] = 0;
				assert(search_buffer_len < SEARCH_BUFFER_MAX_LEN);

				requery_plugins = true;
			}
		}

		if ((IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE)) && search_buffer_len > 0) {
			search_buffer_backspace(IsKeyDown(KEY_LEFT_CONTROL));
			requery_plugins = true;
		}

		if (IsKeyPressed(KEY_ENTER) && selected_result) {
			assert(selected_result->plugin);
			selected_result->plugin->search_plugin_execute(selected_result);
			quit = true;
		}
		
		if (requery_plugins) {
			current_results_count = 0;

			if (garbage) free(garbage);
			
			for (size_t j = 0; j < search_plugins_count; j++) {
				SearchPlugin *plugin = &search_plugins[j];

				SearchPluginResult *results = plugin->search_plugin_query(search_buffer);
				for (size_t i = 0; i < results->results_count; i++) {
					results[i].plugin = plugin;
					if (current_results_count < CURRENT_RESULTS_MAX) {
						current_results[current_results_count++] = &results[i];
					} else {
						break;
					}
				}

				garbage = results;
			}

			if (current_results_count > 1)
				qsort(*current_results, current_results_count, sizeof(SearchPluginResult), search_results_sort_cmp);
		}

		default_ui_provider.ui_draw_user_input_field(search_buffer);
		default_ui_provider.ui_draw_entry(NULL, NULL, NULL, 0);

		for (size_t i = 0; i < current_results_count; i++) {
			Texture2D *icon = NULL;
			if (strlen(current_results[i]->icon_path)>0 && *current_results[i]->icon_path != '!') {
				icon = image_cache_get_if_not_put(&icon_cache, current_results[i]->icon_path, 28, 28);
			}
			if (i == 0) selected_result = current_results[i];
			
			assert(current_results[i]->plugin && current_results[i]->name);

			// printf("Name: %s\n", current_results[i]->name);
			// printf("Plugin: %s\n", current_results[i]->plugin->plugin_metadata.plugin_display_name);
			
			default_ui_provider.ui_draw_entry(current_results[i]->name, current_results[i]->plugin->plugin_metadata.plugin_display_name, icon, i == 0);
		}

		EndDrawing();

		if (!IsWindowFocused()) {
			quit = true;
		}
	}

	EnableCursor();
	CloseWindow();

	return 0;
}

void search_buffer_backspace(bool ctrl) {
	if (search_buffer_len <= 0) return;
	
	if (ctrl) {
		bool first_non_space_char_found = false;
		
		for (int i = search_buffer_len - 1; i >= 0; i--) {
			if (!first_non_space_char_found && !isspace(search_buffer[i])) {
				first_non_space_char_found = true;
			}
				
			if (i == 0) {
				search_buffer[i] = 0;
				search_buffer_len = 0;
				break;
			} else if (search_buffer[i] == ' ' && first_non_space_char_found) {
				search_buffer[i + 1] = 0;
				search_buffer_len = i + 1;
				break;
			}
		}
	} else {
		search_buffer[--search_buffer_len] = 0;
		assert(search_buffer_len >= 0);
	}
}

