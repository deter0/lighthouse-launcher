#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
  int size;
  int scale;
  bool scalable;
} IconSize;

typedef struct {
  bool exists;

  char path[PATH_MAX];
  IconSize size;
} Icon;

typedef struct File File;
struct File {
  bool invalid;
  bool is_heap;
  
  char name[NAME_MAX];
  char containing_path[PATH_MAX];

  bool is_folder;
  bool is_folder_loaded;
  File *children;
  size_t children_count;
  size_t children_alloc;

  struct File *parent;
};

