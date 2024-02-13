// This code is messy.. am I gonna do anything about that?
// No.. shhh... it gets the job the done

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>

// Get home dir
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <dirent.h>

#define _GNU_SOURCE
#include <dlfcn.h>

#include "icon_finder.h"

#ifdef ICON_FINDER_DEBUG
#define ICON_FINDER_LOG printf
#else
#define ICON_FINDER_LOG(...) {}
#endif // ICON_FINDER_DEBUG

bool find_icon(String icon_name, int icon_size, const char *user_theme, char *icon_path, size_t icon_path_max) {
  bool status;
  
  if (user_theme != NULL) {
    status = find_icon_helper(icon_name, icon_size, user_theme, icon_path, icon_path_max);
    if (status != false) return true;
  }

  return find_icon_helper(icon_name, icon_size, "hicolor", icon_path, icon_path_max);
}

// This is kind of silly
bool find_icon_helper(String icon_name, int icon_size, String theme, char *icon_path, size_t icon_path_max) {
  // TODO(deter0): Search theme parents
  return lookup_icon(icon_name, icon_size, theme, icon_path, icon_path_max);
}

static const char *home_dir;
static const char *paths[] = {
  "/usr/share/icons",
  "/usr/share/pixmaps",
  "~/.icons",
  "~/.local/share/icons",
};

// We support:
static const char *icon_extensions[] = {
  "png",
  "svg"
};
// Optimization
static const char *precheck[] = {
  "apps"
};

bool lookup_icon_in_directory(String icon_name, String directory, char *icon_path, size_t icon_path_size) {
  DIR *dir;
  struct dirent *dir_entry;

  if ((dir = opendir(directory)) == NULL) {
    fprintf(stderr, "Error (icon finder): could not open dir: %s:%s.\n", directory, strerror(errno));
    return false;
  }
  
  while ((dir_entry = readdir(dir)) != NULL) {
    if (*(dir_entry->d_name) == '.') continue;
  
    for (size_t i = 0; i < sizeof(icon_extensions)/sizeof(*icon_extensions); i++) {
      char icon_name_with_extension[MAX_PATH_LEN/2] = { 0 };
      snprintf(icon_name_with_extension, MAX_PATH_LEN/2, "%s.%s", icon_name, icon_extensions[i]);

      if (strcmp(dir_entry->d_name, icon_name_with_extension) == 0) {
        snprintf(icon_path, icon_path_size, "%s/%s", directory, dir_entry->d_name);
        closedir(dir);
        return true;
      }
    }
  }

  closedir(dir);
  return false;
}

typedef struct {
  char icon_path[MAX_PATH_LEN];
  char size_description[MAX_PATH_LEN/4];
} IconWithSizeInfo;

typedef struct {
  IconWithSizeInfo *icons;
  size_t count;
  size_t alloc;
} IconWithSizeInfoPool;

bool lookup_icon(String icon_name, int icon_size, String theme_name, char *icon_path, size_t icon_path_max) {
  if (!home_dir) {
    if ((home_dir = getenv("HOME")) == NULL) {
      home_dir = getpwuid(getuid())->pw_dir;
    }
  }

  IconWithSizeInfoPool pool = { 0 };
  pool.alloc = 16;
  pool.icons = malloc(sizeof(IconWithSizeInfo) * pool.alloc);
  assert(pool.icons != NULL);

  DIR *theme_dir;
	struct dirent *theme_dir_entry;

  char theme_dir_full_path[MAX_PATH_LEN] = { 0 };
  bool theme_dir_found = false;
  for (size_t i = 0; i < sizeof(paths)/sizeof(*paths); i++) {
    if (*paths[i] == '~') {
      snprintf(theme_dir_full_path, MAX_PATH_LEN, "%s%s/%s", home_dir, paths[i] + 1, theme_name);
    } else {
      snprintf(theme_dir_full_path, MAX_PATH_LEN, "%s/%s", paths[i], theme_name);
    }
    ICON_FINDER_LOG("Searching for theme path: %s\n", theme_dir_full_path);
    
    if ((theme_dir = opendir(theme_dir_full_path)) != NULL) {
      theme_dir_found = true;
      break;
    }
  }

  if (!theme_dir_found) {
    fprintf(stderr, "Error: could not find icon theme: `%s`\n", theme_name);
    return NULL;
  } else {
    ICON_FINDER_LOG("Icon theme directory => `%s`.\n", theme_dir_full_path);
  }

	while ((theme_dir_entry = readdir(theme_dir)) != NULL) {
    if (strcmp(theme_dir_entry->d_name, ".") == 0 || strcmp(theme_dir_entry->d_name, "..") == 0) {
			continue;
		}

    const char *size_info = theme_dir_entry->d_name;
    char subdir_full_path[MAX_PATH_LEN] = { 0 };
		snprintf(subdir_full_path, MAX_PATH_LEN, "%s/%s", theme_dir_full_path, theme_dir_entry->d_name);

		if (theme_dir_entry->d_type == DT_DIR) {
      ICON_FINDER_LOG("\tSize => `%s`.\n", size_info);
      // Search size base of size directory
      for (size_t i = 0; i < sizeof(icon_extensions)/sizeof(*icon_extensions); i++) {
        char icon_file_name[MAX_PATH_LEN] = {0};
        snprintf(icon_file_name, MAX_PATH_LEN, "%s/%s.%s", subdir_full_path, icon_name, icon_extensions[i]);
        
        if (access(icon_file_name, F_OK) == 0) {
          ICON_FINDER_LOG("Found icon: %s.\n", icon_file_name);
        }
      }

      // Search categories
      DIR *size_categories_dir;
      struct dirent *size_categories_dir_entry;

      if ((size_categories_dir = opendir(subdir_full_path)) == NULL) {
        fprintf(stderr, "Error (icon finder): could not open dir: %s:%s.\n",
                        subdir_full_path, strerror(errno));
        return false; // TODO(deter0): Should we continue?
      }
      
      printf("Directory: %s\n", subdir_full_path);

      // For cache
      bool found_in_precheck = false;
      
      for (size_t i = 0; i < sizeof(precheck)/sizeof(*precheck); i++) {
        const char *dir_precheck = precheck[i];

        char precheck_path[MAX_PATH_LEN] = { 0 };
        snprintf(precheck_path, MAX_PATH_LEN, "%s/%s", subdir_full_path, dir_precheck);
      
        if (access(precheck_path, F_OK) == 0) {
          char icon_path[MAX_PATH_LEN];
          if (lookup_icon_in_directory(icon_name, precheck_path, icon_path, 2048)) {
            ICON_FINDER_LOG("\t\t\tFound icon in precheck => `%s`.", icon_path);

            IconWithSizeInfo *icon = &pool.icons[pool.count++];
            if (pool.count >= pool.alloc) {
              pool.alloc *= 2;
              pool.icons = realloc(pool.icons, sizeof(IconWithSizeInfo)*pool.alloc);
            }

            memcpy(icon->icon_path, icon_path, MAX_PATH_LEN);
            memcpy(icon->size_description, size_info, MAX_PATH_LEN/4);

            found_in_precheck = true;
          }
        }
      }
      
      if (found_in_precheck) { goto clean; }

      while ((size_categories_dir_entry = readdir(size_categories_dir)) != NULL) {
        printf("Sub file: %s\n", size_categories_dir_entry->d_name);
        if (*(size_categories_dir_entry->d_name) == '.') continue;
        if (size_categories_dir_entry->d_type == DT_DIR) {
          char image_files_dir_path[MAX_PATH_LEN] = { 0 };
          snprintf(image_files_dir_path, MAX_PATH_LEN, "%s/%s", subdir_full_path, size_categories_dir_entry->d_name);

          ICON_FINDER_LOG("\t\tSubdir => `%s`\n", size_categories_dir_entry->d_name);

          char icon_path[MAX_PATH_LEN];
          if (lookup_icon_in_directory(icon_name, image_files_dir_path, icon_path, 2048) == true) {
            ICON_FINDER_LOG("\t\t\tFound icon => `%s`.", icon_path);

            IconWithSizeInfo *icon = &pool.icons[pool.count++];
            if (pool.count >= pool.alloc) {
              pool.alloc *= 2;
              pool.icons = realloc(pool.icons, sizeof(IconWithSizeInfo)*pool.alloc);
            }

            memcpy(icon->icon_path, icon_path, MAX_PATH_LEN);
            memcpy(icon->size_description, size_info, MAX_PATH_LEN/4);
            break;
          }
        }
      }
      goto clean;
      clean:
        closedir(size_categories_dir);
        continue;
		}
	}

	closedir(theme_dir);

  IconWithSizeInfo *to_return = NULL;

  int max_width = 0;
  const char *max_icon_path = NULL;

  for (size_t i = 0; i < pool.count; i++) {
    int width = 0;
    int height = 0;
    if (sscanf(pool.icons[i].size_description, "%dx%d", &width, &height) == 2) {
      ICON_FINDER_LOG("%d - %d\n", width, height);
    }
    assert(width == height && "??? how is your icon theme not square");

    if (width == icon_size) {
      to_return = &pool.icons[i];
      break;
    }

    // printf("%s.\n", pool.icons[i].size_description);
    if (icon_size == 0 && strcmp(pool.icons[i].size_description, "scalable") == 0) {
      to_return = &pool.icons[i];
      break;
    }
    
    ICON_FINDER_LOG("%s => %s\n", pool.icons[i].icon_path, pool.icons[i].size_description);
  }

  if (to_return != NULL) {
    memcpy(icon_path, to_return->icon_path, icon_path_max);
  }

  free(pool.icons);
	return to_return != NULL;
}


#ifdef ICON_FINDER_TEST

int main() {
  const char *user_theme = "Adwaita";
  
  char buffer[MAX_PATH_LEN] = { 0 };
  
  assert(find_icon("firefox", 32, user_theme, buffer, sizeof(buffer)) == true);
  // assert(strcmp(buffer, "/usr/share/icons/hicolor/32x32/apps/firefox.png") == 0);
  // assert(find_icon("firefox", 0, user_theme, buffer, sizeof(buffer)) == true);
  // assert(strcmp(buffer, "/usr/share/icons/hicolor/scalable/apps/firefox.svg") == 0);

  // assert(find_icon("gimp", 0, user_theme, buffer, sizeof(buffer)) == false);
  // assert(find_icon("gimp", 32, user_theme, buffer, sizeof(buffer)) == true);
  // printf("%s\n", buffer);
  // assert(strcmp(buffer, "/usr/share/icons/hicolor/32x32/apps/gimp.png") == 0);


  // assert(find_icon("fallafel", 0, NULL, buffer, sizeof(buffer)) == false);
  
  return 0;
}

#endif // ICON_FINDER_TEST

