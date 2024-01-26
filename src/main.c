#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "raylib.h"

#define _GNU_SOURCE
#include <dlfcn.h>

#define SV_IMPLEMENTATION
#include "sv/sv.h"

#include "ui_provider.h"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 500

#define SEARCH_BUFFER_MAX_LEN 32
char search_buffer[SEARCH_BUFFER_MAX_LEN] = { 0 };
int search_buffer_len = 0;

#define ARRAY_LEN(xs) (sizeof(xs)/sizeof(xs[0]))

static UIProvider default_ui_provider = { 0 };

bool load_ui_functions(const char *ui_so_path, UIProvider *ui_provider) {
		void *ui_so_handle = dlopen(ui_so_path, RTLD_NOW | RTLD_GLOBAL);
		if (!ui_so_handle) {
			fprintf(stderr, "Error loading ui theme: `%s`.\n", ui_so_path);
			fprintf(stderr, "\t\tdlerror: `%s`.\n", dlerror());
			return false;
		}

		printf("Loading functions from UI .so file: `%s`.\n", ui_so_path);

		ui_provider->ui_init = dlsym(ui_so_handle, "ui_init");
		if (!ui_provider->ui_init) {
			fprintf(stderr, "\tYour UI theme does not have an `ui_init` function.\n");
			fprintf(stderr, "\t\tdlerror: `%s`.\n", dlerror());

			return false;
		}

		ui_provider->ui_get_background_color = dlsym(ui_so_handle, "ui_get_background_color");
		if (!ui_provider->ui_get_background_color) {
			fprintf(stderr, "\tYour UI theme does not have an `ui_get_background_color` function.\n");
			fprintf(stderr, "\t\tdlerror: `%s`.\n", dlerror());

			return false;
		}
		
		ui_provider->ui_draw_user_input_field = dlsym(ui_so_handle, "ui_draw_user_input_field");
		if (!ui_provider->ui_draw_user_input_field) {
			fprintf(stderr, "\tYour UI theme does not have an `ui_draw_user_input_field` function.\n");
			fprintf(stderr, "\t\tdlerror: `%s`.\n", dlerror());

			return false;
		}
		
		return true;
}

void requery_all_plugins() {
}

static void search_buffer_backspace(bool ctrl);

int main(void) {
	assert(load_ui_functions("./plugins/default_ui.so", &default_ui_provider) != false);
	
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lighthouse Launcher");
	SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

	
	SetWindowMaxSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	bool quit = false;
	int selected_entry_index = 0;
	
	SetTargetFPS(60);
	DisableCursor();
	
	default_ui_provider.ui_init();
	
	while (!WindowShouldClose() && !quit) {
		if (IsKeyReleased(KEY_ESCAPE)) {
			CloseWindow();	
		}

		BeginDrawing();
		
		ClearBackground(default_ui_provider.ui_get_background_color());

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

		if (IsKeyPressed(KEY_UP)) {
			selected_entry_index += 1;
		}
		if (IsKeyPressed(KEY_DOWN)) {
			selected_entry_index -= 1;
		}
		
		if (requery_plugins) {
			requery_all_plugins();
		}

		default_ui_provider.ui_draw_user_input_field(search_buffer);

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

