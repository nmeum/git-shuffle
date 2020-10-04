PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man
DOCDIR ?= $(PREFIX)/share/doc/git-shuffle

CFLAGS ?= -O0 -g -Werror
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -Wpedantic -Wall -Wextra -Wconversion \
	      -Wmissing-prototypes -Wpointer-arith \
	      -Wstrict-prototypes -Wshadow -Wformat

CFLAGS += $(shell pkg-config --cflags libgit2)
LDLIBS += $(shell pkg-config --libs libgit2)

git-shuffle: git-shuffle.c
install: git-shuffle git-shuffle.1 README.md
	install -Dm755 git-shuffle "$(DESTDIR)$(BINDIR)/git-shuffle"
	install -Dm644 git-shuffle.1 "$(DESTDIR)$(MANDIR)/man1/git-shuffle.1"
	install -Dm644 README.md "$(DESTDIR)$(DOCDIR)/README.md"
