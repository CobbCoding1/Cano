AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = cano
cano_SOURCES = src/colors.h \
			   src/config.h \
			   src/defs.h \
			   src/lex.h \
			   src/main.h \
			   src/view.h \
			   src/commands.h \
			   src/commands.c \
			   src/lex.c \
			   src/main.c \
			   src/view.c
cano_CPPFLAGS = @NCURSES_CFLAGS@
cano_LDFLAGS = @NCURSES_LIBS@ -lm -lpthread
