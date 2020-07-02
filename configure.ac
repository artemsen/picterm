AC_INIT([picterm], [1.1], [https://github.com/artemsen/picterm])
AC_LANG([C++])
AM_SILENT_RULES([yes])
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall])

# Compiler flags
AX_CXX_COMPILE_STDCXX([11], [noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Wextra], [CXXFLAGS])

# Build dependencies
AC_PROG_CXX
AC_PROG_INSTALL

# Runtime dependencies
AC_CHECK_LIB([X11], [XOpenDisplay], [], [AC_MSG_ERROR([X11 lib is required])])
AC_CHECK_LIB([png], [png_read_image])
AC_CHECK_LIB([jpeg], [jpeg_finish_decompress])
AC_CHECK_LIB([gif], [DGifOpen])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
