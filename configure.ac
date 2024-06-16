AC_INIT([cano], [0.1.0], [], [], [https://github.com/CobbCoding1/Cano])
AM_INIT_AUTOMAKE([-Wall -Wextra foreign])

AC_PROG_CC
AC_PROG_INSTALL

PKG_CHECK_MODULES([NCURSES], ncurses, [], AC_MSG_ERROR([Failed to find ncurses]))
# AC_CHECK_LIB([m], [floor], [], AC_MSG_ERROR([Failed to find 'floor' in libm]))
# AC_CHECK_LIB([m], [log10], [], AC_MSG_ERROR([Failed to find 'log10' in libm]))
AC_CHECK_HEADERS([locale.h], [], AC_MSG_ERROR([Failed to find locale.h]))

AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

AC_FUNC_MALLOC
AC_FUNC_REALLOC

# AC_CHECK_FUNCS([setlocale])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
