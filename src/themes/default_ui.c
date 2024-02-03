#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "raylib.h"

static Font font = { 0 };

static const float text_size = 32.f;
static const float padding = 4.f;
static int text_rect_height = text_size + padding * 2;
static float icon_size = text_size;
static Texture2D missing_icon;

bool ui_init() {
  font = LoadFontEx("./src/res/Prompt-Regular.ttf", 32, NULL, 0);
  Image image = LoadImage("./src/res/missing.png");
  ImageResize(&image, icon_size, icon_size);

  missing_icon = LoadTextureFromImage(image);
  
  return true;
}

void ui_draw_user_input_field(const char *input_text) {
  DrawRectangle(0, GetRenderHeight() - text_rect_height, GetRenderWidth(), text_rect_height, WHITE);

  Vector2 search_buffer_dims = MeasureTextEx(font, input_text, text_size, 0.f); // For cursor
  // Render at bottom
  Vector2 search_buffer_start = (Vector2){ padding*4, GetRenderHeight() - text_size - padding };

  // Draw Cursor
  DrawRectangle(search_buffer_start.x + search_buffer_dims.x,
                search_buffer_start.y, 1, text_size,
                (Color){0, 0, 0, (sin(GetTime()*20) + 1)/2*255 });

  if (strlen(input_text) == 0) {
    DrawTextEx(font, "Search...", search_buffer_start, text_size, 0.f, GRAY);
  } else {
    DrawTextEx(font, input_text, search_buffer_start, text_size, 0.f, BLACK);
  }
}

static int entry_index = 0;
void ui_draw_entry(const char *entry_name, const char *source, Texture2D *icon, bool highlight) {
  if (entry_name == NULL) { // Reset
    entry_index = 0;
  } else {
    entry_index++;

    int y = GetRenderHeight() - text_rect_height - text_rect_height * entry_index; 
    if (highlight) {
      DrawRectangleRounded((Rectangle){padding*2, y, GetRenderWidth()-4*padding, text_size}, .4f, 6, (Color){36, 36, 36, 255});
      DrawTextEx(font, entry_name, (Vector2){padding*4 + icon_size, (float)y}, text_size, 0.f, WHITE);
    } else {
      DrawTextEx(font, entry_name, (Vector2){padding*4 + icon_size, (float)y}, text_size, 0.f, WHITE);
    }

    if (!icon) {
      DrawTexture(missing_icon, padding*2, y, WHITE);
    }

    DrawTextEx(font, source, (Vector2){padding*4 + GetRenderWidth()/2 + icon_size, (float)y}, text_size, 0.f, GRAY);
  }
}

void ui_draw_entry_group(const char *entry_group_name) {
  if (entry_group_name == NULL) { // Reset
    entry_index = 0;
  } else {
    entry_index++;

    int y = GetRenderHeight() - text_rect_height - text_rect_height * entry_index; 
    DrawTextEx(font, entry_group_name, (Vector2){padding*4, (float)y}, text_size, 0.f, GRAY);
    DrawRectangle(padding*4, y + text_size, GetRenderWidth() - padding*8, 1, GRAY);
  }
}

// TODO(deter): In the future we can have dark/ light themes, maybe even follow desktop spec.
Color ui_get_background_color(void) {
  return BLACK;
}

