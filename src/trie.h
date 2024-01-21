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
  void *user_ptr;
  struct TrieNode *children[256];
  TrieNode *parent;
};

typedef struct {
  char word[MAX_WORD_LEN];
  void *user_ptr;
} TrieWord;

typedef struct {
  TrieWord words[MAX_WORDS_PER_TRIE_POOL];
  size_t words_count;
} WordPool;

TrieNode *trie_alloc_node(void);
void trie_push_text(TrieNode *root, const char *text, void *user_ptr);
void trie_search(TrieNode *root, const char *text, WordPool *search_results);

