#ifndef TRIE_H
#define TRIE_H

#include <stddef.h>

#define MAX_MATCHES 64
#define MAX_MATCH_LENGTH 256

/**
 * @brief   A Trie (prefix tree) structure to store strings.
 * @note    Shell builtins are stored to implement the autocomplete feature in this program.
 */
typedef struct Trie {
    struct Trie *children[65];  /* Supported command-name characters */
    int is_word;                /* 1 to indicate whether it is the end of a word
                                 * 0 otherwise */
} Trie;

/**
 * @brief   Stores autocomplete matches found for a prefix.
 */
typedef struct {
    int count;                                      /* Number of matches */
    char matches[MAX_MATCHES][MAX_MATCH_LENGTH];    /* List of matching strings */
} CompletionResult;

/**
 * @brief   Initializes a CompletionResult before collecting matches.
 * @param   result (CompletionResult *) Result object to initialize.
 */
void completion_result_init(CompletionResult *result);

/**
 * @brief   Create a new Trie object.
 *          Returns a pointer to the intialized Trie object.
 */
Trie* trie_create();

/**
 * @brief   Inserts a given word into a Trie object.
 * @param   obj (Trie *) A Trie object to be inserted into.
 * @param   word (const char *) A string word to insert.
 */
void trie_insert(Trie *obj, const char *word);

/**
 * @brief   Searches a given word in the Trie object.
 *          Returns 0 if not found, or is_word otherwise.
 * @param   obj (Trie *) A Trie object to be searched.
 * @param   word (const char *) A string word to search for.
 */
int trie_search(Trie *obj, const char *word);

/**
 * @brief   Determines if a given prefix of a word exists in the Trie object.
 *          Returns 0 if not found, or 1 otherwise.
 * @param   obj (Trie *) A Trie object to be searched.
 * @param   prefix (const char *) A prefix string to search for.
 */
int trie_starts_with(Trie *obj, const char *prefix);

/**
 * @brief   Collects words in the Trie that start with a given prefix.
 *          Adds up to MAX_MATCHES matches and returns the total number stored.
 * @param   obj (Trie *) A Trie object to be searched.
 * @param   prefix (const char *) A prefix string to search for.
 * @param   result (CompletionResult *) Stores matching words and their count.
 */
int trie_add_matches(Trie *obj, const char *prefix, CompletionResult *result);

/**
 * @brief   Frees a given Trie object from memory.
 * @param   obj (Trie *) A Trie object to be freed.
 */
void trie_free(Trie *obj);

#endif
