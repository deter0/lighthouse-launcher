#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const char* str;

str get_icon(str icon_name, str icon_size, str scale) {
  str file_name = find_icon_helper(icon_name, icon_size, scale, "user selected  theme");
  if (file_name != NULL) {
    return file_name;
  }

  file_name = find_icon_helper(icon_name, icon_size, scale, "hicolor");
  if (file_name != NULL) {
    return file_name;
  }

  return lookup_fallback_icon(icon_name);
}

str lookup_icon_helper(str icon_name, str icon_size, str scale, str theme) {
    str file_name = lookup_icon(icon_name, icon_size, scale, theme);
    if (file_name != NULL) {
      return file_name;
    }

    // Do not search parents
}

str lookup_icon(str icon_name, str icon_size, str scale, str theme) {
  
}

