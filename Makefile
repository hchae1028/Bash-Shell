CC      := gcc
CFLAGS  := -Wall -Werror -std=c99 -g
DEPFLAGS := -MMD -MP

SRCS    := src/main.c src/tokenizer.c src/shell.c src/builtins.c src/redirection.c src/executor.c src/trie.c src/path.c src/pipeline.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

.PHONY: all clean

all: shell

shell: $(OBJS)
	$(CC) $(OBJS) -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -f shell $(OBJS) $(DEPS)

-include $(DEPS)
