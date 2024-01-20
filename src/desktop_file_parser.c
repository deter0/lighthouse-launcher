#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>

#include "sv/sv.h"

#include "slurp.h"

#include "desktop_file_parser.h"

void delete_desktop_file(DesktopFile *file) {
  for (size_t i = 0; i < file->groups_count; i++) {
    DesktopFileGroup *group = &file->groups[i];

    for (size_t j = 0; j < group->entries_count; j++) {
      DesktopFileEntry *entry = &group->entries[j];
      if (entry->value_string_large || entry->value_type == DF_VALUE_STRING_L) {
        free(entry->value_string_large);
      }
    }
  }
}

static DesktopFile parse_desktop_file(const char *file_contents_c_str) {
  String_View file_contents = sv_from_cstr(file_contents_c_str);
  String_View line = sv_chop_by_delim(&file_contents, '\n');
  
  DesktopFile desktop_file = { 0 };

  // Files can have nameless groups at the start 
  // E.g.:
  //  foo=bar
  //  [group_a]
  DesktopFileGroup *current_group = &desktop_file.groups[desktop_file.groups_count++];
  
  while (!sv_eq(line, SV(""))) {
    sv_trim_left(line);
    
    if (sv_starts_with(line, SV("#"))) {
      goto next_line;
    }
    
    if (sv_starts_with(line, SV("["))) {
      sv_chop_right(&line, 1);
      sv_chop_left(&line, 1);

      current_group = &desktop_file.groups[desktop_file.groups_count++];
      size_t group_name_len = line.count > 127 ? 127 : line.count;
      memcpy(current_group->group_name, line.data, group_name_len);

      if (group_name_len != line.count) {
        fprintf(stderr, "[WARN] Group name too long. Trimmed from '" SV_Fmt "' to '%s'.\n", SV_Arg(line), current_group->group_name);
      }
    } else {
      DesktopFileEntry *group_entry = &current_group->entries[current_group->entries_count++];
      
      String_View entry_key = sv_chop_by_delim(&line, '=');
      
      size_t entry_key_len = entry_key.count > 127 ? 127 : entry_key.count;
      memcpy(group_entry->entry_key, entry_key.data, entry_key_len);
      
      if (entry_key_len != entry_key.count) {
        fprintf(stderr, "[WARN] Key too long. Trimmed from '" SV_Fmt "' to '%s'.\n", SV_Arg(entry_key), group_entry->entry_key);
      }
      
      if (sv_eq(line, SV("true")) || sv_eq(line, SV("false"))) {
        group_entry->value_type = DF_VALUE_BOOL;
        group_entry->value_bool = sv_eq(line, SV("true"));
      } else {
        // TODO(deter): Implement ints & floats

        if (line.count > MAX_SMALL_STRING_LEN) {
          group_entry->value_type = DF_VALUE_STRING_L;

          char *value_buffer = malloc(line.count + 1);
          assert(value_buffer != NULL);
          memcpy(value_buffer, line.data, line.count);
          value_buffer[line.count] = 0;

          group_entry->value_string_large = value_buffer;
        } else {
          group_entry->value_type = DF_VALUE_STRING_S;

          memcpy(group_entry->value_string_small, line.data, line.count);
        }
      }
      
      // printf("Key: %s\n", group_entry->entry_key);
      // printf("Key len: %zu\n", strlen(group_entry->entry_key));
      
      //printf("'" SV_Fmt "' = '" SV_Fmt "'\n", SV_Arg(entry_key), SV_Arg(line));
    }
    
    next_line:
    line = sv_chop_by_delim(&file_contents, '\n');
  }

  return desktop_file;
}

LighthouseDesktopEntry get_desktop_file_contents_info(const char *file_contents_c_str) {
  DesktopFile parsed_file = parse_desktop_file(file_contents_c_str);
  LighthouseDesktopEntry info = { 0 };

  for (size_t i = 0; i < parsed_file.groups_count; i++) {
    DesktopFileGroup *group = &parsed_file.groups[i];
    if (strcmp(group->group_name, "Desktop Entry") == 0) {
      for (size_t j = 0; j < group->entries_count; j++) {
        DesktopFileEntry *entry = &group->entries[j];

        if (strcmp(entry->entry_key, "Name") == 0) {
          assert(entry->value_type == DF_VALUE_STRING_S); // Only supporting small strings for now
          memcpy(info.name, entry->value_string_small, MAX_SMALL_STRING_LEN);
        } else if (strcmp(entry->entry_key, "GenericName") == 0) {
          assert(entry->value_type == DF_VALUE_STRING_S); // Only supporting small strings for now
          memcpy(info.generic_name, entry->value_string_small, MAX_SMALL_STRING_LEN);
        } else if (strcmp(entry->entry_key, "Categories") == 0) {
          assert(entry->value_type == DF_VALUE_STRING_S); // Only supporting small strings for now
          memcpy(info.categories, entry->value_string_small, MAX_SMALL_STRING_LEN);
        } else if (strcmp(entry->entry_key, "Icon") == 0) {
          assert(entry->value_type == DF_VALUE_STRING_S); // Only supporting small strings for now
          memcpy(info.icon, entry->value_string_small, MAX_SMALL_STRING_LEN);
        } else if (strcmp(entry->entry_key, "Type") == 0) {
          assert(entry->value_type == DF_VALUE_STRING_S); // Only supporting small strings for now
          memcpy(info.type, entry->value_string_small, MAX_SMALL_STRING_LEN);
        }
      }
    }
  }

  delete_desktop_file(&parsed_file);
  
  return info;
}

LighthouseDesktopEntry get_desktop_file_info(const char *file_path) {
  char *file_content = slurp_file(file_path);
  
  LighthouseDesktopEntry desktop_file_parsed = get_desktop_file_contents_info(file_content);
  free(file_content);

  return desktop_file_parsed;
}


