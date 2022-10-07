#
# K2HASH
#
# Utility tools for building configure/packages by AntPickax
#
# Copyright 2018 Yahoo Japan corporation.
#
# AntPickax provides utility tools for supporting autotools
# builds.
#
# These tools retrieve the necessary information from the
# repository and appropriately set the setting values of
# configure, Makefile, spec,etc file and so on.
# These tools were recreated to reduce the number of fixes and
# reduce the workload of developers when there is a change in
# the project configuration.
# 
# For the full copyright and license information, please view
# the license file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Fri, Apr 13 2018
# REVISION:
#

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ([2.63])
AC_INIT(k2hash, m4_esyscmd([tr -d '\n' < $(pwd)/RELEASE_VERSION]))
AM_INIT_AUTOMAKE()
LT_INIT()

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

#
# Checks for programs.
#
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

#
# Checks for libraries.
#
AC_CHECK_LIB(dl, dlopen)

#
# Checks for header files.
#
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h strings.h sys/time.h termios.h unistd.h netdb.h endian.h inttypes.h sys/endian.h])

#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T
AC_TYPE_INT32_T
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_CHECK_TYPES([ptrdiff_t])

#
# Checks for library functions.
#
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_FORK
AC_CHECK_FUNCS([fdatasync ftruncate gettimeofday memset munmap realpath strcasecmp strncasecmp clock_gettime gethostname strdup])

AC_CONFIG_MACRO_DIR([m4])

#
# Load customizable variables
#
AC_CHECK_FILE([configure.custom],
	[
		configure_custom_file="configure.custom"
		custom_git_domain="$(grep '^\s*GIT_DOMAIN\s*=' configure.custom | sed -e 's|^\s*GIT_DOMAIN\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_git_org="$(grep '^\s*GIT_ORG\s*=' configure.custom | sed -e 's|^\s*GIT_ORG\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_git_repo="$(grep '^\s*GIT_REPO\s*=' configure.custom | sed -e 's|^\s*GIT_REPO\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_git_endpoint="$(grep '^\s*GIT_EP_V3_REPO\s*=' configure.custom | sed -e 's|^\s*GIT_EP_V3_REPO\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_dev_email="$(grep '^\s*DEV_EMAIL\s*=' configure.custom | sed -e 's|^\s*DEV_EMAIL\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
		custom_dev_name="$(grep '^\s*DEB_NAME\s*=' configure.custom | sed -e 's|^\s*DEB_NAME\s*=\s*||g' -e 's|^\s*||g' -e 's|\s*$||g')"
	],
	[
		configure_custom_file=""
		custom_git_domain="github.com"
		custom_git_org="yahoojapan"
		custom_git_repo="k2hash"
		custom_git_endpoint="https://api.github.com/repos/"
		custom_dev_email="antpickax-support@mail.yahoo.co.jp"
		custom_dev_name="K2HASH_DEVELOPER"
	]
)

#
# Symbols for buildutils
#
AC_SUBST([CONFIGURECUSTOM], "${configure_custom_file}")
AC_SUBST([GIT_DOMAIN], "${custom_git_domain}")
AC_SUBST([GIT_ORG], "${custom_git_org}")
AC_SUBST([GIT_REPO], "${custom_git_repo}")
AC_SUBST([CURRENTREV], "$($(pwd)/buildutils/make_commit_hash.sh -o "${custom_git_org}" -r "${custom_git_repo}" -ep "${custom_git_endpoint}" -s)")
AC_SUBST([DEV_EMAIL], "$(echo ${DEBEMAIL:-"${custom_dev_email}"})")
AC_SUBST([DEV_NAME], "$(echo ${DEBFULLNAME:-"${custom_dev_name}"})")
#
AC_SUBST([RPMCHANGELOG], "`$(pwd)/buildutils/make_rpm_changelog.sh $(pwd)/ChangeLog`")
AC_SUBST([SHORTDESC], "`$(pwd)/buildutils/make_description.sh $(pwd)/docs/k2hash.1 --short`")
AC_SUBST([LONGDESC], "`$(pwd)/buildutils/make_description.sh $(pwd)/docs/k2hash.1 --long`")
AC_SUBST([ESCLONGDESC], "`$(pwd)/buildutils/make_description.sh $(pwd)/docs/k2hash.1 --esclong`")
AC_SUBST([DEBLONGDESC], "`$(pwd)/buildutils/make_description.sh $(pwd)/docs/k2hash.1 --deblong`")
AC_SUBST([PKG_VERSION], "`$(pwd)/buildutils/make_variables.sh -pkg_version`")
AC_SUBST([LIB_VERSION_INFO], "`$(pwd)/buildutils/make_variables.sh -lib_version_info`")
AC_SUBST([LIB_VERSION_LINK], "`$(pwd)/buildutils/make_variables.sh -lib_version_for_link`")
AC_SUBST([LIB_MAJOR_NUMBER], "`$(pwd)/buildutils/make_variables.sh -major_number`")
AC_SUBST([PKGLICENSE], "`grep License COPYING | sed 's/ License//g'`")
AC_SUBST([DEBCOPYING], "`tail -n +5 COPYING | sed 's/^$/./g' | sed 's/^/ /g'`")
AC_SUBST([DEBHELPER_DEP], "`$(pwd)/buildutils/make_variables.sh -debhelper_dep`")
AC_SUBST([RPMPKG_GROUP], ["`$(pwd)/buildutils/make_variables.sh -rpmpkg_group | sed 's#NEEDRPMGROUP#Group: Applications/Databases#g'`"])
AM_SUBST_NOTMAKE([CURRENTREV])
AM_SUBST_NOTMAKE([RPMCHANGELOG])
AM_SUBST_NOTMAKE([SHORTDESC])
AM_SUBST_NOTMAKE([LONGDESC])
AM_SUBST_NOTMAKE([ESCLONGDESC])
AM_SUBST_NOTMAKE([DEBLONGDESC])
AM_SUBST_NOTMAKE([DEBCOPYING])
AM_SUBST_NOTMAKE([DEBHELPER_DEP])
AM_SUBST_NOTMAKE([RPMPKG_GROUP])

#
# pkg-config for old version
#
AC_PATH_PROG(PKG_CONFIG, pkg-config, [AC_MSG_RESULT(no)])
AS_IF([test "$PKG_CONFIG" = "no"], [AC_MSG_ERROR(You have to install pkg-config to compile $PACKAGE_NAME v$PACKAGE_VERSION)])

#
# Option for crypt libraries
#
crypt_lib=openssl

AC_MSG_CHECKING([for crypt library option with gcrypt(GnuTLS)])
AC_ARG_WITH(gcrypt,
	[AS_HELP_STRING([--with-gcrypt], [build with gcrypt(GnuTLS)])],
	[case "${withval}" in
		yes)	crypt_lib=gcrypt;;
		*)		;;
	esac]
)
AS_IF([test ${crypt_lib} = gcrypt], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for crypt library option with nettle(GnuTLS)])
AC_ARG_WITH(nettle,
	[AS_HELP_STRING([--with-nettle], [build with nettle(GnuTLS)])],
	[case "${withval}" in
		yes)	crypt_lib=nettle;;
		*)		;;
	esac]
)
AS_IF([test ${crypt_lib} = nettle], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for crypt library option with NSS])
AC_ARG_WITH(nss,
	[AS_HELP_STRING([--with-nss], [build with NSS])],
	[case "${withval}" in
		yes)	crypt_lib=nss;;
		*)		;;
	esac]
)
AS_IF([test ${crypt_lib} = nss], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])

AC_MSG_CHECKING([for crypt library option with OpenSSL(default)])
AC_ARG_WITH(openssl,
	[AS_HELP_STRING([--with-openssl], [build with OpenSSL(default)])],
	[case "${withval}" in
		yes)	crypt_lib=openssl;;
		*)		;;
	esac]
)
AS_IF([test ${crypt_lib} = openssl], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])

#
# Checking crypt libraries
#
# [NOTE]
# Although gcrypt does not provide a pkg-config file, it provides
# libgcrypt-config and we check it with libgcrypt-config.
#
AC_MSG_CHECKING([for using crypt library type])
case "${crypt_lib}" in
	openssl)
		AC_MSG_RESULT(OpenSSL)
		PKG_CHECK_MODULES([CRYPT], [libcrypto >= 1.0.0 ])
		AC_SUBST([CRYPT_LIB_SOURCE], "k2hcryptopenssl.cc")
		AC_SUBST([CONFIGUREWITHOPT], "--with-openssl")
		AM_SUBST_NOTMAKE([CONFIGUREWITHOPT])
		;;
	nss)
		AC_MSG_RESULT(NSS)
		PKG_CHECK_MODULES([CRYPT], [nss >= 3.28.0 ])
		AC_SUBST([CRYPT_LIB_SOURCE], "k2hcryptnss.cc")
		AC_SUBST([CONFIGUREWITHOPT], "--with-nss")
		AM_SUBST_NOTMAKE([CONFIGUREWITHOPT])
		;;
	nettle)
		AC_MSG_RESULT(nettle)
		PKG_CHECK_MODULES([CRYPT], [nettle >= 3.2 ])
		AC_SUBST([CRYPT_LIB_SOURCE], "k2hcryptnettle.cc")
		AC_SUBST([CONFIGUREWITHOPT], "--with-nettle")
		AM_SUBST_NOTMAKE([CONFIGUREWITHOPT])
		;;
	gcrypt)
		AC_MSG_RESULT(gcrypt)
		AC_CHECK_PROG(gcrypt_config, [libgcrypt-config], [yes], [no])
		AS_IF([test ${gcrypt_config} = no], [AC_MSG_ERROR([libgcrypt-config is not found])])
		CRYPT_VERSION=`libgcrypt-config --version`
		CRYPT_CFLAGS=`libgcrypt-config --cflags`
		CRYPT_LIBS=`libgcrypt-config --libs`
		AC_MSG_CHECKING([for libgcrypt-config version])
		AC_MSG_RESULT($CRYPT_VERSION)
		AC_SUBST([CRYPT_LIB_SOURCE], "k2hcryptgcrypt.cc")
		AC_SUBST([CONFIGUREWITHOPT], "--with-gcrypt")
		AM_SUBST_NOTMAKE([CONFIGUREWITHOPT])
		;;
	*)
		AC_MSG_ERROR([unknown crypt library type.])
		;;
esac

AM_CONDITIONAL([CRYPT_LIB_OPENSSL],	[test "$crypt_lib" = openssl])
AM_CONDITIONAL([CRYPT_LIB_NSS],		[test "$crypt_lib" = nss])
AM_CONDITIONAL([CRYPT_LIB_NETTLE],	[test "$crypt_lib" = nettle])
AM_CONDITIONAL([CRYPT_LIB_GCRYPT],	[test "$crypt_lib" = gcrypt])

#
# Checking Libraries
#
check_depend_libs=1
AC_MSG_CHECKING([for checking dependent libraries])
AC_ARG_ENABLE(check-depend-libs,
	[AS_HELP_STRING([--disable-check-depend-libs], [disable checking dependent libraries])],
	[case "${enableval}" in
		yes)	check_depend_libs=1;;
		*)		check_depend_libs=0;;
	esac]
)
AS_IF([test ${check_depend_libs} = 1], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])
AS_IF([test ${check_depend_libs} = 1], [PKG_CHECK_MODULES([fullock], [libfullock >= 1.0.35], [], [AC_MSG_ERROR(not found libfullock package)])])

#
# CFLAGS/CXXFLAGS
#
CFLAGS="-Wall $CFLAGS $CRYPT_CFLAGS $fullock_CFLAGS"
CXXFLAGS="-Wall $CXXFLAGS $CRYPT_CFLAGS $fullock_CFLAGS"

#
# for STD::FNV-1A
#
std_fnv_function=0
AC_MSG_CHECKING([for using std fnv-1a hash function])
AC_ARG_ENABLE(std-fnv-function,
	[AS_HELP_STRING([--enable-std-fnv-function], [using STD::FNV-1A hash function])],
	[case "${enableval}" in
		yes)	std_fnv_function=1;;
		*)		std_fnv_function=0;;
	esac]
)
AS_IF([test ${std_fnv_function} = 1], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])
AS_IF([test ${std_fnv_function} = 1],
	[
		CFLAGS="-DUSE_STD_FNV_HASH_FUNCTION $CFLAGS"
	 	CXXFLAGS="-DUSE_STD_FNV_HASH_FUNCTION $CXXFLAGS"
	]
)

#
# for MAP_ANONYMOUS
#
AC_MSG_CHECKING(for MAP_ANONYMOUS symbol)
AC_CACHE_VAL(ac_cv_map_anonymous,
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([[#include <sys/mman.h>; int val = MAP_ANONYMOUS;]])],
		ac_cv_map_anonymous=yes,
		ac_cv_map_anonymous=no
	)
)
AC_MSG_RESULT($ac_cv_map_anonymous)
if test $ac_cv_map_anonymous = no; then
	AC_DEFINE(MAP_ANONYMOUS, MAP_ANON, [Define to MAP_ANON if you do not have MAP_ANONYMOUS symbol.])
fi

#
# Config files
#
AC_CONFIG_FILES([Makefile
				docs/Makefile
				docs/examples/Makefile
				lib/Makefile
				lib/libk2hash.pc
				tests/Makefile
				buildutils/Makefile
				buildutils/control
				buildutils/copyright
				buildutils/k2hash.spec
				buildutils/Dockerfile.templ])

AC_OUTPUT

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
