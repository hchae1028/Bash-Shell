#include "trie.h"
#include <stdlib.h>
#include <string.h>

#define ALPHABET_SIZE 26

static int char_index(char c);
static void find_matches(Trie *obj, char *buffer, size_t depth, CompletionResult *result);

/**
 * @brief   Creates a new Trie object by memory allocation.
 *          Returns an intialized Trie object.
 */
Trie* trie_create() {
    Trie *node = calloc(1, sizeof(Trie));
    return node;
}

/**
 * @brief   Inserts a given word into a Trie object.
 * @param   obj (Trie *) A Trie object to be inserted into.
 * @param   word (char *) A string word to insert.
 */
void trie_insert(Trie *obj, const char *word) {
    Trie *curr = obj;
    for (int i = 0; word[i] != '\0'; i++) {
        int index = char_index(word[i]);
        if (index == -1)
            return;
        if (!curr->children[index]) {
            curr->children[index] = trie_create();
        }
        curr = curr->children[index];
    }
    curr->is_word = 1;
}

/**
 * @brief   Searches a given word in the Trie object.
 *          Returns 0 if not found, or is_word otherwise.
 * @param   obj (Trie *) A Trie object to be searched.
 * @param   word (char *) A string word to search for.
 */
int trie_search(Trie *obj, const char *word) {
    Trie *curr = obj;
    for (int i = 0; word[i] != '\0'; i++) {
        int index = char_index(word[i]);
        if (index == -1)
            return 0;
        if (!curr->children[index])
            return 0;
        curr = curr->children[index];
    }
    return curr->is_word;
}

/**
 * @brief   Determines if a given prefix of a word exists in the Trie object.
 *          Returns 0 if not found, or 1 otherwise.
 * @param   obj (Trie *) A Trie object to be searched.
 * @param   prefix (char *) A prefix string to search for.
 */
int trie_starts_with(Trie *obj, const char *prefix) {
    Trie *curr = obj;
    for (int i = 0; prefix[i] != '\0'; i++) {
        int index = char_index(prefix[i]);
        if (index == -1)
            return 0;
        if (!curr->children[index])
            return 0;
        curr = curr->children[index];
    }
    return 1;
}

/**
 * @brief   Collects words in the Trie that start with a given prefix.
 *          Stores up to MAX_MATCHES matches and returns the number found.
 * @param   obj (Trie *) A Trie object to be searched.
 * @param   prefix (const char *) A prefix string to search for.
 * @param   result (CompletionResult *) Stores matching words and their count.
 */
int trie_collect_matches(Trie *obj, const char *prefix, CompletionResult *result) {
    Trie *curr = obj;
    size_t depth = 0;

    if (obj == NULL || prefix == NULL || result == NULL)
        return 0;

    char buffer[MAX_MATCH_LENGTH];
    result->count = 0;

    for (size_t i = 0; prefix[i] != '\0'; i++) {
        int index = char_index(prefix[i]);
        if (index == -1 || curr->children[index] == NULL)
            return 0;
        if (depth + 1 >= MAX_MATCH_LENGTH)
            return 0;

        buffer[depth++] = prefix[i];
        curr = curr->children[index];
    }
    buffer[depth] = '\0';

    find_matches(curr, buffer, depth, result);
    return result->count;
}

/**
 * @brief   Frees a given Trie object from memory.
 * @param   obj (Trie *) A Trie object to be freed.
 */
void trie_free(Trie *obj) {
    if (!obj) return;

    for (int i = 0; i < 26; i++) {
        trie_free(obj->children[i]);
    }
    free(obj);
}

/**
 * @brief Converts a lowercase letter into a Trie child index.
 *        Returns -1 if the character is not in the range 'a' to 'z'.
 * @param c (char) Character to convert.
 */
static int char_index(char c) {
    if (c < 'a' || c > 'z')
        return -1;
    return c - 'a';
}

/**
 * @brief Recursively counts words below a Trie node.
 *        Stores each complete word it finds until MAX_MATCHES is reached.
 * @param obj (Trie *) Current Trie node to search from.
 * @param buffer (char *) Current word built during the recursive search.
 * @param depth (size_t) Current depth in the Trie.
 * @param result (CompletionResult *) Stores matching words and their count.
 */
static void find_matches(Trie *obj, char *buffer, size_t depth, CompletionResult *result) {
    if (obj == NULL || result->count == MAX_MATCHES)
        return;

    if (obj->is_word) {
        strncpy(result->matches[result->count], buffer, MAX_MATCH_LENGTH - 1);
        result->matches[result->count][MAX_MATCH_LENGTH - 1] = '\0';
        (result->count)++;
    }

    for (int i = 0; i < ALPHABET_SIZE && result->count < MAX_MATCHES; i++) {
        if (obj->children[i] == NULL)
            continue;
        if (depth + 1 >= MAX_MATCH_LENGTH)
            return;

        buffer[depth] = (char)('a' + i);
        buffer[depth + 1] = '\0';
        find_matches(obj->children[i], buffer, depth + 1, result);
    }
}
