#pragma once

#include "../../common/trie.h"

#define MAX_GROUPS_PER_FILE 32
#define MAX_ENTRIES_PER_GROUP 128
#define MAX_GROUP_NAME_LEN 128
#define MAX_ENTRY_NAME_LEN 128

typedef struct {
  char name[MAX_SMALL_STRING_LEN];
  char generic_name[MAX_SMALL_STRING_LEN];
  char categories[MAX_SMALL_STRING_LEN];
  char icon[MAX_SMALL_STRING_LEN];
  char type[MAX_SMALL_STRING_LEN];
  char exec[MAX_SMALL_STRING_LEN];
} LighthouseDesktopEntry;

typedef enum ValueType { DF_VALUE_NONE, DF_VALUE_INT, DF_VALUE_FLOAT, DF_VALUE_STRING_L, DF_VALUE_STRING_S, DF_VALUE_BOOL }  ValueType;

typedef struct {
  char entry_key[MAX_ENTRY_NAME_LEN];
  
  enum ValueType value_type;
  int32_t value_int;
  float value_float;
  char *value_string_large;
  char value_string_small[256];
  bool value_bool;
} DesktopFileEntry;

typedef struct {
  char group_name[MAX_GROUP_NAME_LEN];
  
  size_t entries_count;
  DesktopFileEntry entries[MAX_ENTRIES_PER_GROUP];
} DesktopFileGroup;

typedef struct {
  DesktopFileGroup groups[MAX_GROUPS_PER_FILE];
  size_t groups_count;
} DesktopFile;

LighthouseDesktopEntry get_desktop_file_info(const char *file_path);
LighthouseDesktopEntry get_parse_desktop_file_contents_info(const char *file_contents_c_str);

