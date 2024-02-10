#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <hashmap/hashmap.h>
#include "raylib.h"


typedef struct {
  struct hashmap_s texture_hashmap;
  
  Texture2D *texture_storage;
  size_t texture_storage_count;
  size_t texture_storage_alloc;
} ImageCache;

ImageCache create_image_cache();
Texture2D *image_cache_put(ImageCache *cache, const char *image_path, int width, int height);
Texture2D *image_cache_get(ImageCache *cache, const char *image_path);
Texture2D *image_cache_get_if_not_put(ImageCache *cache, const char *image_path, int width, int height);

