#
# K2HASH
#
# Copyright 2013 Yahoo! JAPAN corporation.
#
# K2HASH is key-valuew store base libraries.
# K2HASH is made for the purpose of the construction of
# original KVS system and the offer of the library.
# The characteristic is this KVS library which Key can
# layer. And can support multi-processing and multi-thread,
# and is provided safely as available KVS.
#
# For the full copyright and license information, please view
# the LICENSE file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Fri Dec 2 2013
# REVISION:
#

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT(k2hash, m4_esyscmd([tr -d '\n' < $(pwd)/RELEASE_VERSION]))
AM_INIT_AUTOMAKE()
LT_INIT()

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(dl, dlopen)
#AC_CHECK_LIB([k2hash], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h strings.h sys/time.h termios.h unistd.h netdb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T
AC_TYPE_UID_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_FORK
AC_CHECK_FUNCS([fdatasync ftruncate gettimeofday memset munmap realpath strcasecmp strncasecmp clock_gettime gethostname strdup])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile
                 docs/Makefile
                 docs/examples/Makefile
                 lib/Makefile
                 lib/libk2hash.pc
                 tests/Makefile])

# CFLAGS/CXXFLAGS
CFLAGS="-Wall $CFLAGS"
CXXFLAGS="-Wall $CXXFLAGS"
AC_ARG_ENABLE(expanding-debug,
	[  --enable-expanding-debug       Turn on depanging area debugging],
	[case "${enableval}" in
	        yes)	CFLAGS="-DDEBUG_FOR_EXPANDING $CFLAGS"
	        	CXXFLAGS="-DDEBUG_FOR_EXPANDING $CXXFLAGS"
	        ;;
	esac]
)

AC_ARG_ENABLE(std-fnv-function,
	[  --enable-std-fnv-function      Use STD::FNV-1A hash function],
	[case "${enableval}" in
	        yes)	CFLAGS="-DUSE_STD_FNV_HASH_FUNCTION $CFLAGS"
	        	CXXFLAGS="-DUSE_STD_FNV_HASH_FUNCTION $CXXFLAGS"
	        ;;
	esac]
)

# chek for LIBFULLOCK
if test "x${CHECK_LIBFULLOCK}" != "xno"; then
	PKG_CHECK_MODULES([fullock], [libfullock >= 1.0.21], [have_fullock=yes], [have_fullock=no])
	if test "x${have_fullock}" = "xno"; then
		AC_MSG_ERROR([not found libfullock-dev package.])
	fi
fi

# for MAP_ANONYMOUS
AC_MSG_CHECKING(for MAP_ANONYMOUS symbol)
AC_CACHE_VAL(ac_cv_map_anonymous,
	AC_TRY_COMPILE([
		# include <sys/mman.h>],
		[int val = MAP_ANONYMOUS; ],
		ac_cv_map_anonymous=yes,
		ac_cv_map_anonymous=no
	)
)
AC_MSG_RESULT($ac_cv_map_anonymous)
if test $ac_cv_map_anonymous = no; then
	AC_DEFINE(MAP_ANONYMOUS, MAP_ANON, [Define to MAP_ANON if you do not have MAP_ANONYMOUS symbol.])
fi

# LIB_VERSION_INFO for libtool library version info
LIB_VERSION_INFO=`$(pwd)/make_valiables.sh -lib_version_info`
AC_SUBST([LIB_VERSION_INFO])

AC_OUTPUT

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
