#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <hashmap/hashmap.h>

#include "raylib.h"
#include "image_cache.h"

#define TEXTURE_HM_INIT_SIZE 128

ImageCache create_image_cache() {
  ImageCache cache = { 0 };
  assert(hashmap_create(TEXTURE_HM_INIT_SIZE, &cache.texture_hashmap) == 0);

  cache.texture_storage_alloc = TEXTURE_HM_INIT_SIZE;
  cache.texture_storage = malloc(sizeof(*cache.texture_storage) * cache.texture_storage_alloc);
  assert(cache.texture_storage);

  return cache;
}

Texture2D *image_cache_put(ImageCache *cache, const char *image_path, int width, int height) {
  Texture2D texture;

  // is SVG?
  if (strlen(image_path) > 5 && strcmp((image_path + strlen(image_path) - 4), ".svg") == 0) {
    Image p_image = LoadImageSvg(image_path, width, height);
    texture = LoadTextureFromImage(p_image);
    UnloadImage(p_image);
  } else {
    texture = LoadTextureEx(image_path, width, height);  
  }

  Texture2D *stored = &cache->texture_storage[cache->texture_storage_count++];
  if (cache->texture_storage_count >= cache->texture_storage_alloc) {
    cache->texture_storage_alloc *= 2;
    cache->texture_storage = realloc(cache->texture_storage,
                                     sizeof(*cache->texture_storage) * cache->texture_storage_alloc);
  }
  memcpy(stored, &texture, sizeof(texture));
  
  assert(hashmap_put(&cache->texture_hashmap, image_path, strlen(image_path), stored) == 0);
  
  return stored;
}

Texture2D *image_cache_get(ImageCache *cache, const char *image_path) {
  return (Texture2D*)hashmap_get(&cache->texture_hashmap, image_path, strlen(image_path));
}

Texture2D *image_cache_get_if_not_put(ImageCache *cache, const char *image_path, int width, int height) {
  Texture2D *image = image_cache_get(cache, image_path);
  if (image) return image;

  return image_cache_put(cache, image_path, width, height);
}

