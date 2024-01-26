#pragma once

#include <stdio.h>

#include "raylib.h"

typedef bool (*ui_init_t)(void);
typedef Color (*ui_get_background_color_t)(void);
typedef void (*ui_draw_user_input_field_t)(const char *input_text);
typedef const char* (*ui_test_t)(void);

typedef struct {
  ui_init_t ui_init;
  ui_get_background_color_t ui_get_background_color;
  ui_draw_user_input_field_t ui_draw_user_input_field;
  ui_test_t ui_test;
} UIProvider;



