#pragma once

#include <stdio.h>

#include "./common/trie.h"

typedef struct {
		char name[MAX_SMALL_STRING_LEN];
		void *user_ptr;
		float score; // 0 -> 1
		size_t results_count; // Length of results determined by first element
} SearchPluginResult;

typedef bool (*search_plugin_init_t)(void);
typedef SearchPluginResult* (*search_plugin_query_t)(const char *query); // Length determined by first element

typedef struct {
	char plugin_name[MAX_SMALL_STRING_LEN];
	search_plugin_init_t search_plugin_init;
	search_plugin_query_t search_plugin_query;
} SearchPlugin;

