#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>

#include <hashmap/hashmap.h>

#include "desktop_file_parser.h"
#include "slurp.h"

#include "icon_finder.h"

#define FILE_CACHE_INIT 128
static bool has_file_cache_init = false;
static struct hashmap_s file_cache_hm;

#define CACHE_ALL_FILES true

void file_cache_init() {
  assert(hashmap_create(FILE_CACHE_INIT, &file_cache_hm) == 0);
  has_file_cache_init = true;
}

void get_file_path(File *file, char *path) {
  snprintf(path, PATH_MAX, "%s/%s", file->containing_path, file->name);
}

void unload_file(File *file) {
  char file_path[PATH_MAX] = { 0 };
  get_file_path(file, file_path);

  if (hashmap_remove(&file_cache_hm, file_path, strlen(file_path)) != 0) {
    // File maybe didn't exist in cache?
  } else {
    printf("File removed from cache: `%s`\n", file_path);
  }
  
  if (file->invalid) {
    printf("Invalid file: %s\n", file->name);
    assert(!file->invalid);
  }
  if (file->is_folder) {
    for (size_t i = 0; i < file->children_count; i++) {
      unload_file(&file->children[i]);
    }
  }
  if (file->children) {
    free(file->children);
  }
  if (file->is_heap) {
    free(file);
  }
}

static int log_and_free_all(void* const context, struct hashmap_element_s* const e) {
  (void)context;

  putc('`', stdout);
  for (size_t i = 0; i < e->key_len; i++) {
    putc(((const char*)e->key)[i], stdout);
  }
  putc('`', stdout);
  
  printf(" = %p file from cache has been freed.\n", e->data);
  free(e->data);
  
  return -1;
}

void file_cache_deinit() {
  if (0!=hashmap_iterate_pairs(&file_cache_hm, log_and_free_all, stdout)) {
    fprintf(stderr, "failed to deallocate hashmap entries\n");
  }
  hashmap_destroy(&file_cache_hm);
  has_file_cache_init = false;
}

bool file_exists(const char *path) {
  return access(path, F_OK) == 0;
}

void cache_file(File *file) {
  char file_path[PATH_MAX] = { 0 };
  get_file_path(file, file_path);

  printf("`%s` file placed in cache.\n", file_path);
  if (hashmap_put(&file_cache_hm, (const char*)file_path, strlen(file_path), file) != 0) {
    fprintf(stderr, "Error caching file: `%s`.\n", file_path);
  }
}

File *create_empty_file(void) {
  File *next = calloc(1, sizeof(File));
  assert(next);

  next->is_heap = true;

  return next;
}

File *create_folder(const char *path, File *into_this) {
  if (!has_file_cache_init) {
    file_cache_init();
  }

  File *cached_file = hashmap_get(&file_cache_hm, path, strlen(path));
  if (cached_file != NULL) {
    printf("Cached file: `%s`\n", path);
    if (into_this) {
      printf("\tCopying.\n");
      memcpy(into_this, cached_file, sizeof(*cached_file));
      return into_this;
    }
    return cached_file;
  }

  File *folder = into_this != NULL ? into_this : create_empty_file();

  DIR *dir;
  struct dirent *dir_entry;

  if ((dir = opendir(path)) == NULL) {
    fprintf(stderr, "Error (icon finder): could not open dir: %s:%s.\n", path, strerror(errno));
    folder->invalid = true;
    return folder;
  }

  folder->children_alloc = 32;
  folder->children = calloc(folder->children_alloc, sizeof(*folder->children));
  assert(folder->children != NULL && "Buy more ram!\n");
  folder->is_folder = true;

  while ((dir_entry = readdir(dir)) != NULL) {
    if (*(dir_entry->d_name) == '.') continue; // We don't care about hidden files

    File *child = &folder->children[folder->children_count++];
    if (folder->children_count >= folder->children_alloc) {
      folder->children_alloc *= 2;
      folder->children = realloc(folder->children, sizeof(*folder->children)*folder->children_alloc);
      assert(folder->children && "Buy more & better ram!\n");
      memset(folder->children + folder->children_alloc/2, 0, sizeof(File)*folder->children_alloc/2); // TODO(deter0): Double check
    }

    memcpy(child->containing_path, path, PATH_MAX);
    memcpy(child->name, dir_entry->d_name, NAME_MAX);

    if (dir_entry->d_type == DT_DIR) {
      child->is_folder = true;
    }
  }

  {
    char *path_cpy = strdup(path);
    char *directory_parent = dirname(path_cpy);
    memcpy(folder->containing_path, directory_parent, strlen(directory_parent));
    free(path_cpy);
  }
  {
    char *path_cpy = strdup(path);
    char *directory_name = basename(path_cpy);
    memcpy(folder->name, directory_name, strlen(directory_name));
    free(path_cpy);
  }

  folder->is_folder_loaded = true;

  closedir(dir);

  return folder;
}

void load_folder(File *folder) {
  char folder_path[PATH_MAX] = { 0 };
  snprintf(folder_path, PATH_MAX, "%s/%s", folder->containing_path, folder->name);

  assert(create_folder(folder_path, folder) != NULL);
}

static bool get_icon_theme(const char *theme_name, char *path, size_t path_max_len) {
  static const char *icon_theme_paths[] = {
    "/usr/share/icons",
    "/usr/share/pixmaps",
  };

  for (size_t i = 0; i < sizeof(icon_theme_paths)/sizeof(*icon_theme_paths); i++) {
    const char *icon_theme_path = icon_theme_paths[i];

    File *icon_theme_container = create_folder(icon_theme_path, NULL);
    cache_file(icon_theme_container);
    
    if (icon_theme_container->is_folder_loaded) {
      for (size_t c_idx = 0; c_idx < icon_theme_container->children_count; c_idx++) {
        if (strcmp(icon_theme_container->children[c_idx].name, theme_name) == 0) {
          char theme_path[PATH_MAX] = { 0 };
          get_file_path(&icon_theme_container->children[c_idx], theme_path);
          
          memset(path, 0, path_max_len);
          memcpy(path, theme_path, path_max_len > PATH_MAX ? PATH_MAX : PATH_MAX);

          return true;
        }
      }
    } else {
      fprintf(stderr, "Error loading folder.\n");
    }

    // Do not unload for cache reasons
    // unload_file(icon_theme_container);
  }

  return false;
}

Icon find_icon(const char *icon_name, const char *user_theme, IconSize target_size) {
  Icon result = { 0 };

  char theme_path[PATH_MAX] = { 0 };
  bool found_icon_theme = get_icon_theme(user_theme, theme_path, sizeof(theme_path));

  if (found_icon_theme) {
    printf("Found theme: %s\n", theme_path);
  } else {
    found_icon_theme = get_icon_theme(user_theme, "hicolor", sizeof(theme_path));
    assert(!found_icon_theme && "Couldn't find user theme or hicolor theme.");
    
    printf("Theme (`%s`) not found.\n", user_theme);
  }

  char index_theme_file_path[PATH_MAX] = { 0 };
  snprintf(index_theme_file_path, PATH_MAX, "%s/%s", theme_path, "index.theme");
  
  if (!file_exists(index_theme_file_path)) {
    fprintf(stderr, "Could not find index.theme file: `%s`\n", index_theme_file_path);
  }
  
  char *index_file_contents = slurp_file(index_theme_file_path);
  DesktopFile index_file = parse_desktop_file(index_file_contents);

  delete_desktop_file(&index_file);
  free(index_file_contents);

  return result;
}

#ifdef ICON_FINDER_TEST

int main() {
  // const char *path = "/usr/share/icons";
  
  // File *src_folder = create_folder(path, NULL);
  // assert(!src_folder->invalid);
  // assert(src_folder->children_count > 0);

  // printf("Path: `%s`\n", path);
  // printf("Containg folder: `%s`\n", src_folder->containing_path);
  // char pre_cache_name[NAME_MAX] = { 0 };
  // memcpy(pre_cache_name, src_folder->name, NAME_MAX);
  
  // printf("Name of folder: `%s`\n", src_folder->name);
  // printf("Folder loaded: %s\n", src_folder->is_folder_loaded ? "true" : "false");

  // for (size_t i = 0; i < src_folder->children_count; i++) {
  //   File *child = &src_folder->children[i];
  //   printf("\tFolder Child: `%s`, Child is folder: %s\n", child->name,
  //                                                         child->is_folder ? "true" : "false");
  //   if (child->is_folder) {
  //     load_folder(child);
  //     assert(child->is_folder_loaded);
  //     for (size_t j = 0; j < child->children_count; j++) {
  //       printf("\t\tGrandchild: `%s`\n", child->children[j].name);
  //     }
  //   }
  // }

  // create_folder(path, src_folder);
  // assert(src_folder->is_folder_loaded);
  // assert(strcmp(src_folder->name, pre_cache_name) == 0); 
  // unload_file(src_folder);

  find_icon("firefox", "Adwaita", (IconSize){ .scalable = true });
  
  file_cache_deinit();
}

#endif // ICON_FINDER_TEST

