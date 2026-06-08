#include "path.h"
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


static int is_inpath(const char *path);

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

/**
 * @brief Searches PATH for a given command argument.
 *        Stores the full executable path in pathbuf if found.
 *        Returns 1 if the command is found, 0 otherwise.
 * @param pathbuf (char *) Buffer used to store the full executable path.
 * @param pathbuf_size (size_t) Size of the path buffer.
 * @param arg (const char *) Command name to search for.
 */
int parse_path(char *pathbuf, size_t pathbuf_size, const char *arg) {
  const char *path = getenv("PATH");
  if (path == NULL || arg == NULL) return 0;

  char *path_copy = strdup(path);
  char *saveptr = NULL;
  char *dir = strtok_r(path_copy, ":", &saveptr);

  // Look for the path with the command argument
  int found = 0;
  while (dir != NULL) {
    snprintf(pathbuf, pathbuf_size, "%s/%s", dir, arg);
    if (is_inpath(pathbuf) == 1) {
      found = 1;
      break;
    }
    dir = strtok_r(NULL, ":", &saveptr);
  }

  free(path_copy);
  return found;
}

/**
 * @brief Checks whether a given file path exists
 *        and has at least one execution permission.
 *        Returns 1 if it does, 0 otherwise.
 * @param path (const char *) Path to be checked.
 */
static int is_inpath(const char *path) {
  struct stat st;
  // stat(path, &st) fills st with the information about the given path
  if (stat(path, &st) == 0) {
    // Execution permission bits
    // S_IXUSR: owner
    // S_IXGRP: group
    // S_IXOTH: others
    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
      return 1;
  }
  return 0;
}