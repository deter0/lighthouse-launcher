#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define MAX_WORDS_PER_TRIE_POOL 128
#define MAX_WORD_LEN 64

#define TRIE_NODE_POOL_CAP 1024

typedef struct TrieNode TrieNode;
struct TrieNode {
  bool word;
  char character;
  struct TrieNode *children[256];
  TrieNode *parent;
};

typedef struct {
  char words[MAX_WORD_LEN][MAX_WORDS_PER_TRIE_POOL];
  size_t words_count;
} WordPool;

TrieNode *trie_alloc_node(void);
void trie_push_text(TrieNode *root, const char *text);
void trie_search(TrieNode *root, const char *text, WordPool *search_results);

