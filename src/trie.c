#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "fruits.h"

#include "./trie.h"

#define ARRAY_LEN(xs) (sizeof(xs)/sizeof(xs[0]))

static TrieNode trie_node_pool[TRIE_NODE_POOL_CAP] = {0};
static size_t trie_node_pool_count = 0;

TrieNode *trie_alloc_node(void) {
  assert(trie_node_pool_count < TRIE_NODE_POOL_CAP);
  return &trie_node_pool[trie_node_pool_count++];
}

void trie_push_text(TrieNode *root, const char *text) {
  if (text == NULL || *text == '\0') {
    return;
  }

  assert(root != NULL);
  size_t index = (size_t) *text;

  if (root->children[index] == NULL) {
    root->children[index] = trie_alloc_node();
    root->children[index]->parent = root;
  }

  root->children[index]->character = *text;
  
  if (*(text + 1) == 0) {
    root->children[index]->word = true;
  }
  trie_push_text(root->children[index], text + 1);
}

static void strrev(char* str) {
  // if the string is empty
  if (!str) {
    return;
  }
  // pointer to start and end at the string
  int i = 0;
  int j = strlen(str) - 1;
 
  // reversing string
  while (i < j) {
    char c = str[i];
    str[i] = str[j];
    str[j] = c;
    i++;
    j--;
  }
}

static void collect_word(TrieNode *node, char *word_out) {
  TrieNode *working_node = node;

  size_t word_size = strlen(word_out);
  
  working_node = node;
  while (working_node->parent != NULL) {
    assert(word_size < MAX_WORD_LEN);
    word_out[word_size++] = working_node->character;
    working_node = working_node->parent;
  }

  strrev(word_out);

  word_out[word_size + 1] = 0;
}

static void search_down_for_words(TrieNode *root, WordPool *word_pool) {
  if (root->word == true) {
    char word[MAX_WORD_LEN] = { 0 };
    collect_word(root, word);
    
    memcpy(word_pool->words + (word_pool->words_count++), word, MAX_WORD_LEN);
    if (word_pool->words_count >= MAX_WORDS_PER_TRIE_POOL) {
      printf("Max search reached.\n");
      return;
    }
  }

  for (size_t i = 0; i < ARRAY_LEN(root->children); i++) {
    if (root->children[i] != NULL) {
      search_down_for_words(root->children[i], word_pool);
    }
  }
}

void trie_search(TrieNode *root, const char *text, WordPool *search_results) {
  TrieNode *furthest_node = root;

  for (size_t i = 0; i < strlen(text); i++) {
    TrieNode *child = furthest_node->children[(size_t)text[i]];
    if (child == NULL) {
      break;
    } else {
      furthest_node = child;
    }
  }
  
  search_down_for_words(furthest_node, search_results);
}

static void dump_dot(TrieNode *root) {
  size_t index = root - trie_node_pool;

  for (size_t i = 0; i < ARRAY_LEN(root->children); i++) {
    if (root->children[i] != NULL) {
      size_t child_index = root->children[i] - trie_node_pool;
      printf("    Node_%zu [label=\"%c\"]\n", child_index, (char)i);
      printf("    Node_%zu -> Node_%zu [label=\"%s\"]\n", index, child_index, root->children[i]->word ? "WORD" : (char[2]){i, 0});
      dump_dot(root->children[i]);
    }
  }
}

#ifdef TRIE_TESTING

int main(void) {
  TrieNode *root = trie_alloc_node();

  // insert_text(root, "hello");
  // insert_text(root, "helium");
  for (size_t i = 0; i < fruits_count; ++i) {
    trie_push_text(root, fruits[i]);
  }

  assert(root->children['A']);
  assert(!root->children['z']);
  assert(root->children['A']->children['p']->children['p']->character == 'p');
  assert(root->children['A']->children['p']->children['p']->word == false);
  assert(root->children['A']->children['p']->children['p']->children['l']->children['e']->word == true);

  //printf("digraph Trie {\n");
  //dump_dot(root);
  //printf("}\n");

  char word[MAX_WORD_LEN];
  collect_word(root->children['A']->children['p']->children['p']->children['l']->children['e'], word);
  assert(strcmp(word, "Apple") == 0);
  assert(strlen(word) == 5);

  {
    WordPool search_results = { 0 };
    trie_search(root, "A", &search_results);

    assert(search_results.words_count == 3);
    assert(strcmp(search_results.words[0], "Apple") == 0);
    assert(strcmp(search_results.words[1], "Apricot") == 0);
    assert(strcmp(search_results.words[2], "Avocado") == 0);
    
    for (size_t i = 0; i < search_results.words_count; i++) {
      printf("Result: %s\n", search_results.words[i]);
    }
  }

  {
    WordPool search_results = { 0 };
    
    trie_search(root, "Apple", &search_results);
    assert(search_results.words_count == 1);

    printf("%s\n", search_results.words[0]);
    assert(strcmp(search_results.words[0], "Apple") == 0);
  }
  
  return 0;
}

#endif
