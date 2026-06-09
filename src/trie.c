#include "trie.h"
#include <stdlib.h>
#include <string.h>

static int char_index(char c);
static char index_char(int index);
static int completion_result_contains(CompletionResult *result, const char *word);
static void completion_result_add(CompletionResult *result, const char *word);
static void find_matches(Trie *obj, char *buffer, size_t depth, CompletionResult *result);

/**
 * @brief   Initializes a CompletionResult before collecting matches.
 * @param   result (CompletionResult *) Result object to initialize.
 */
void completion_result_init(CompletionResult *result) {
    if (result == NULL)
        return;

    result->count = 0;
}

/**
 * @brief   Creates a new Trie object by memory allocation.
 * @return  Pointer to the initialized Trie object, or NULL if allocation fails.
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
 * @param   obj (Trie *) A Trie object to be searched.
 * @param   word (char *) A string word to search for.
 * @return  1 if word is stored in the Trie, 0 otherwise.
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
 * @param   obj (Trie *) A Trie object to be searched.
 * @param   prefix (char *) A prefix string to search for.
 * @return  1 if prefix exists in the Trie, 0 otherwise.
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
 *          Adds up to MAX_MATCHES matches.
 * @param   obj (Trie *) A Trie object to be searched.
 * @param   prefix (const char *) A prefix string to search for.
 * @param   result (CompletionResult *) Stores matching words and their count.
 * @return  Total number of matches stored in result.
 */
int trie_add_matches(Trie *obj, const char *prefix, CompletionResult *result) {
    Trie *curr = obj;
    size_t depth = 0;

    if (obj == NULL || prefix == NULL || result == NULL)
        return 0;

    char buffer[MAX_MATCH_LENGTH];

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

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        trie_free(obj->children[i]);
    }
    free(obj);
}

/**
 * @brief Converts a supported command-name character into a Trie child index.
 * @param c (char) Character to convert.
 * @return Trie child index, or -1 if c is not supported.
 */
static int char_index(char c) {
    if (c >= 'a' && c <= 'z')
        return c - 'a';
    if (c >= 'A' && c <= 'Z')
        return LOWERCASE_COUNT + (c - 'A');
    if (c >= '0' && c <= '9')
        return LOWERCASE_COUNT + UPPERCASE_COUNT + (c - '0');
    if (c == '_')
        return LOWERCASE_COUNT + UPPERCASE_COUNT + DIGIT_COUNT;
    if (c == '-')
        return LOWERCASE_COUNT + UPPERCASE_COUNT + DIGIT_COUNT + 1;
    if (c == '.')
        return LOWERCASE_COUNT + UPPERCASE_COUNT + DIGIT_COUNT + 2;
    return -1;
}

/**
 * @brief Converts a Trie child index back into its command-name character.
 * @param index (int) Trie child index to convert.
 * @return Character represented by index.
 */
static char index_char(int index) {
    if (index < LOWERCASE_COUNT)
        return (char)('a' + index);
    index -= LOWERCASE_COUNT;

    if (index < UPPERCASE_COUNT)
        return (char)('A' + index);
    index -= UPPERCASE_COUNT;

    if (index < DIGIT_COUNT)
        return (char)('0' + index);
    index -= DIGIT_COUNT;

    if (index == 0)
        return '_';
    if (index == 1)
        return '-';
    return '.';
}

/**
 * @brief Checks whether a completion result already contains a word.
 * @param result (CompletionResult *) Result object to search.
 * @param word (const char *) Word to check.
 * @return 1 if word is already stored in result, 0 otherwise.
 */
static int completion_result_contains(CompletionResult *result, const char *word) {
    for (int i = 0; i < result->count; i++) {
        if (strcmp(result->matches[i], word) == 0)
            return 1;
    }
    return 0;
}

/**
 * @brief Adds one word to a completion result if there is room and no duplicate.
 * @param result (CompletionResult *) Result object to update.
 * @param word (const char *) Word to add.
 */
static void completion_result_add(CompletionResult *result, const char *word) {
    if (result->count >= MAX_MATCHES || completion_result_contains(result, word))
        return;

    strncpy(result->matches[result->count], word, MAX_MATCH_LENGTH - 1);
    result->matches[result->count][MAX_MATCH_LENGTH - 1] = '\0';
    result->count++;
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

    if (obj->is_word)
        completion_result_add(result, buffer);

    for (int i = 0; i < ALPHABET_SIZE && result->count < MAX_MATCHES; i++) {
        if (obj->children[i] == NULL)
            continue;
        if (depth + 1 >= MAX_MATCH_LENGTH)
            return;

        buffer[depth] = index_char(i);
        buffer[depth + 1] = '\0';
        find_matches(obj->children[i], buffer, depth + 1, result);
    }
}
