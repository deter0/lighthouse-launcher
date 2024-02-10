#pragma once

#define MAX_PATH_LEN (2048)
typedef const char* String;

// #define ICON_FINDER_DEBUG

// 0 for scalable
bool find_icon(const char *icon_name, int icon_size, const char *user_theme, char *icon_path, size_t icon_path_max);

bool find_icon_helper(String icon_name, int icon_size, String theme, char *icon_path, size_t icon_path_max);
bool lookup_icon(String icon_name, int icon_size, String theme_name, char *icon_path, size_t icon_path_max);


