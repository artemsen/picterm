AC_INIT([picterm], [0.1], [https://github.com/artemsen/picterm])
AC_LANG([C++])
AM_SILENT_RULES([yes])
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall])

AC_PROG_CXX
AC_PROG_INSTALL

AC_CHECK_LIB([X11], [XOpenDisplay], [], [AC_MSG_ERROR([X11 lib is required])])
AC_CHECK_LIB([png], [png_read_image])
AC_CHECK_LIB([jpeg], [jpeg_finish_decompress])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
