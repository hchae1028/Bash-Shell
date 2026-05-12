#include "path.h"
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Inserts all executable names in PATH into a Trie.
 * @param root (Trie *) Trie object used for PATH executable autocompletion.
 */
void build_path_trie(Trie *root) {
    const char *path = getenv("PATH");
    if (path == NULL)
        return;

    char *path_copy = strdup(path);
    char *saveptr = NULL;
    char *dir = strtok_r(path_copy, ":", &saveptr);

    char full_path[1024];
    while (dir != NULL) {
        DIR *dp = opendir(dir);
        if (dp != NULL) {
            struct dirent *entry;
            while ((entry = readdir(dp)) != NULL) {
                if ((strcmp(entry->d_name, ".") == 0) || strcmp(entry->d_name, "..") == 0)
                    continue;
                
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, entry->d_name);
                if (access(full_path, X_OK) == 0)
                    trie_insert(root, entry->d_name);
            }
            closedir(dp);
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }
    free(path_copy);
}