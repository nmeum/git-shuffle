CFLAGS ?= -O0 -g -Werror
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -Wpedantic -Wall -Wextra \
	      -Wmissing-prototypes -Wpointer-arith \
	      -Wstrict-prototypes -Wshadow -Wformat

CFLAGS += $(shell pkg-config --cflags libgit2)
LDLIBS += $(shell pkg-config --libs libgit2)

git-shuffle: git-shuffle.c
