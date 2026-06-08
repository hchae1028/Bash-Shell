#include "expansion.h"
#include <stdlib.h>
#include <ctype.h>

int expand_env_var(const char *command, char *expanded, size_t expanded_size) {
    size_t i = 0, j = 0;   // Read and write indices
    int is_sq = 0;
    int is_dq = 0;

    while (command[i] != '\0') {
        if (command[i] == '\'' && !is_dq) {
            is_sq = !is_sq;
            expanded[j++] = command[i++];
        }
        else if (command[i] == '\"' && !is_sq) {
            is_dq = !is_dq;
            expanded[j++] = command[i++];
        }
        else if (command[i] == '$' && !is_sq && (i == 0 || command[i - 1] != '\\')) {
            char name[32];
            size_t name_len = 0;

            i++;    // Skip $
            while (command[i] != '\0' && (isalnum((unsigned char)command[i]) || command[i] == '_')) {
                if (name_len + 1 >= sizeof(name))
                    return 0;
                name[name_len++] = command[i++];
            }

            if (name_len == 0) {
                expanded[j++] = '$';
                continue;
            }

            name[name_len] = '\0';
            char *env_expanded = getenv(name);
            if (env_expanded == NULL)
                env_expanded = "";
            
            for (size_t k = 0; env_expanded[k] != '\0'; k++) {
                if (j + 1 >= expanded_size)
                    return 0;
                expanded[j++] = env_expanded[k];
            }
        }
        else
            expanded[j++] = command[i++];
    }
    expanded[j] = '\0';
    return 1;
}
