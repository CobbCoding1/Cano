AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = cano
cano_SOURCES = src/colors.h \
			   src/defs.h \
			   src/lex.h \
			   src/main.h \
			   src/view.h \
			   src/commands.h \
			   src/cgetopt.h \
			   src/frontend.h \
			   src/keys.h     \
			   src/tools.h  \
			   src/buffer.h \
			   src/commands.c \
			   src/lex.c \
			   src/main.c \
			   src/view.c \
			   src/frontend.c \
			   src/cgetopt.c  \
			   src/keys.c   \
			   src/tools.c  \
			   src/buffer.c

cano_CFLAGS = @NCURSES_CFLAGS@
cano_LDADD = @NCURSES_LIBS@ -lm -lpthread

man1_MANS = docs/cano.1
