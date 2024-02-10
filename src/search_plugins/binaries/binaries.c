#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../../search_plugin.h"

void search_plugin_execute(SearchPluginResult *result_to_execute) {
  (void)result_to_execute;
  
  assert(0 && "Not Implemented");
}

SearchPluginMetadata search_plugin_init(void) {
  return (SearchPluginMetadata){
    .plugin_display_name = "Binaries",
    .init_status = 1
  };
}

SearchPluginResult test = {
  .name = "Example plugin",
  .user_ptr = NULL,
  .score = 1.f,
  .plugin = NULL,
  .results_count = 1
};

void* memdup(void* mem, size_t size) { 
   void* out = malloc(size);

   if (out != NULL)
       memcpy(out, mem, size);

   return out;
}

SearchPluginResult *search_plugin_query(const char *query) {
  (void)query;
  return memdup(&test, sizeof(test));
}
