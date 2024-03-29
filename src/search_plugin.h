#pragma once

#include <stdio.h>

#include "./common/trie.h"

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN (2048)
#endif

typedef struct SearchPluginResult {
		char name[MAX_SMALL_STRING_LEN];
		void *user_ptr;
		float score; // 0 -> 1
		struct SearchPlugin *plugin; // Internal use
		size_t results_count; // Length of results determined by first element
		char icon_path[MAX_PATH_LEN];
} SearchPluginResult;

typedef struct SearchPluginMetadata {
	const char *plugin_display_name;
	bool init_status;
} SearchPluginMetadata;

typedef SearchPluginMetadata (*search_plugin_init_t)(void);
typedef SearchPluginResult* (*search_plugin_query_t)(const char *query);
typedef void (*search_plugin_execute_t)(SearchPluginResult *result_to_execute);

typedef struct SearchPlugin {
	char plugin_file_name[MAX_SMALL_STRING_LEN];
	SearchPluginMetadata plugin_metadata;
	
	search_plugin_init_t search_plugin_init;
	search_plugin_query_t search_plugin_query;
	search_plugin_execute_t search_plugin_execute;
} SearchPlugin;

