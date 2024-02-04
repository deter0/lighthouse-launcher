#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

// Get home dir
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <dirent.h>
#include <errno.h>

#define _GNU_SOURCE
#include <dlfcn.h>

typedef const char* String;

String find_icon_helper(String icon_name, String icon_size, String scale, String theme);
String lookup_icon(String icon_name, String icon_size, String scale, String theme_name);

String find_icon(String icon_name, String icon_size, String scale) {
  String file_name = find_icon_helper(icon_name, icon_size, scale, "Adwaita");
  if (file_name != NULL) {
    return file_name;
  }

  file_name = find_icon_helper(icon_name, icon_size, scale, "hicolor");
  if (file_name != NULL) {
    return file_name;
  }

  return NULL;
}

String find_icon_helper(String icon_name, String icon_size, String scale, String theme) {
    String file_name = lookup_icon(icon_name, icon_size, scale, theme);
    if (file_name != NULL) {
      return file_name;
    }

    // TODO(deter0): Search theme parents

    return NULL;
}

static const char *paths[] = {
  "~/.icons",
  "/usr/share/icons",
  "usr/share/pixmaps"
};
static const char *home_dir;


static const char *icon_extensions[] = {
  "png",
  "svg"
};

String lookup_icon(String icon_name, String icon_size, String scale, String theme_name) {
  if (!home_dir) {
    if ((home_dir = getenv("HOME")) == NULL) {
      home_dir = getpwuid(getuid())->pw_dir;
    }
  }

  DIR *theme_dir;
	struct dirent *theme_dir_entry;

  char theme_dir_full_path[1024] = { 0 };
  bool theme_dir_found = false;
  for (size_t i = 0; i < sizeof(paths)/sizeof(*paths); i++) {
    if (*paths[i] == '~') {
      snprintf(theme_dir_full_path, 1024, "%s%s/%s", home_dir, paths[i] + 1, theme_name);
    } else {
      snprintf(theme_dir_full_path, 1024, "%s/%s", paths[i], theme_name);
    }
    printf("Searching for theme path: %s\n", theme_dir_full_path);
    
    if ((theme_dir = opendir(theme_dir_full_path)) != NULL) {
      theme_dir_found = true;
      break;
    }
  }

  if (!theme_dir_found) {
    fprintf(stderr, "Error: could not find icon theme: `%s`\n", theme_name);
    return NULL;
  } else {
    printf("Icon theme directory => `%s`.\n", theme_dir_full_path);
  }

	while ((theme_dir_entry = readdir(theme_dir)) != NULL) {
    if (strcmp(theme_dir_entry->d_name, ".") == 0 || strcmp(theme_dir_entry->d_name, "..") == 0) {
			continue;
		}

    const char *size_info = theme_dir_entry->d_name;
    char subdir_full_path[2048] = { 0 };
		snprintf(subdir_full_path, 2048, "%s/%s", theme_dir_full_path, theme_dir_entry->d_name);

		if (theme_dir_entry->d_type == DT_DIR) {
      printf("\tSize => `%s`.\n", size_info);
      // Search size base of size directory
      for (size_t i = 0; i < sizeof(icon_extensions)/sizeof(*icon_extensions); i++) {
        char icon_file_name[2048] = {0};
        // subdir_full_path/iconname.extension
        snprintf(icon_file_name, 2048, "%s/%s.%s", subdir_full_path, icon_name, icon_extensions[i]);
        
        if (access(icon_file_name, F_OK) == 0) {
          printf("Found icon: %s.\n", icon_file_name);
        }
      }

      // Search categories
      DIR *size_categories_dir;
      struct dirent *size_categories_dir_entry;

      if ((size_categories_dir = opendir(subdir_full_path)) == NULL) {
        fprintf(stderr, "Error (icon finder): could not open dir: %s:%s.\n",
                        subdir_full_path, strerror(errno));
        return NULL; // TODO(deter0): Should we continue?
      }

      while ((size_categories_dir_entry = readdir(size_categories_dir)) != NULL) {
        if (*(size_categories_dir_entry->d_name) == '.') continue;

        if (size_categories_dir_entry->d_type == DT_DIR) {
          char image_files_dir_path[2048] = { 0 };
          snprintf(image_files_dir_path, 2048, "%s/%s", subdir_full_path, size_categories_dir_entry->d_name);

          printf("\t\tSubdir => `%s`\n", size_categories_dir_entry->d_name);
          
          DIR *image_files_dir;
          struct dirent *image_files_dir_entry;

          if ((image_files_dir = opendir(image_files_dir_path)) == NULL) {
            fprintf(stderr, "Error (icon finder): could not open dir: %s:%s.\n",
                        subdir_full_path, strerror(errno));
            continue;
          }

          bool icon_found = false;
          while (!icon_found && (image_files_dir_entry = readdir(image_files_dir)) != NULL) {
            if (*(image_files_dir_entry->d_name) == '.') continue;

            for (size_t i = 0; i < sizeof(icon_extensions)/sizeof(*icon_extensions); i++) {
              char icon_name_with_extension[128] = { 0 };
              snprintf(icon_name_with_extension, 128, "%s.%s", icon_name, icon_extensions[i]);

              if (strcmp(image_files_dir_entry->d_name, icon_name_with_extension) == 0) {
                char full_icon_path[2048] = { 0 };
                snprintf(full_icon_path, 2048, "%s/%s", image_files_dir_path, image_files_dir_entry->d_name);
                
                printf("\t\t\tFound icon => `%s`!\n", full_icon_path);

                icon_found = true;
                break;
              }
            }
          }

          closedir(image_files_dir);
        }
      }

      closedir(size_categories_dir);
		}
	}

	closedir(theme_dir);

	return NULL;
}


#ifdef ICON_FINDER_TEST

int main() {
  find_icon("firefox", "*", "*");
  return 0;
}

#endif // ICON_FINDER_TEST

