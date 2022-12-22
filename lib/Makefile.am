#
# K2HASH
#
# Copyright 2013 Yahoo Japan Corporation.
#
# K2HASH is key-valuew store base libraries.
# K2HASH is made for the purpose of the construction of
# original KVS system and the offer of the library.
# The characteristic is this KVS library which Key can
# layer. And can support multi-processing and multi-thread,
# and is provided safely as available KVS.
#
# For the full copyright and license information, please view
# the license file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Fri Dec 2 2013
# REVISION:
#

pkginclude_HEADERS	=	k2hash.h \
						k2harchive.h \
						k2hashfunc.h \
						k2hcommand.h \
						k2hcommon.h \
						k2hdbg.h \
						k2hfind.h \
						k2hlock.h \
						k2hmmapinfo.h \
						k2hpagefile.h \
						k2hpage.h \
						k2hpagemem.h \
						k2hshm.h \
						k2hshmdirect.h \
						k2hstructure.h \
						k2hsubkeys.h \
						k2hattrs.h \
						k2htransfunc.h \
						k2htrans.h \
						k2hutil.h \
						k2hdaccess.h \
						k2hstream.h \
						k2hfilemonitor.h \
						k2hshmupdater.h \
						k2hqueue.h \
						k2hattrop.h \
						k2hattrbuiltin.h \
						k2hcryptcommon.h \
						k2hattropsman.h \
						k2hattrplugin.h \
						k2hattrfunc.h

pkgincludedir		= $(includedir)/k2hash
pkgconfigdir		= $(libdir)/pkgconfig
pkgconfig_DATA		= libk2hash.pc

DISTCLEANFILES		= $(pkgconfig_DATA)

lib_LTLIBRARIES		= libk2hash.la

libk2hash_la_SOURCES=	k2hash.cc \
						k2hdbg.cc \
						k2hmmapinfo.cc \
						k2hpage.cc \
						k2hpagefile.cc \
						k2hpagemem.cc \
						k2hshm.cc \
						k2hshmdirect.cc \
						k2hshminit.cc \
						k2hshmdump.cc \
						k2hshmcomp.cc \
						k2hashfunc.cc \
						k2hutil.cc \
						k2hfind.cc \
						k2hsubkeys.cc \
						k2hattrs.cc \
						k2hlock.cc \
						k2hcommand.cc \
						k2htrans.cc \
						k2htransfunc.cc \
						k2harchive.cc \
						k2hdaccess.cc \
						k2hfilemonitor.cc \
						k2hshmupdater.cc \
						k2hashversion.cc \
						k2hshmque.cc \
						k2hqueue.cc \
						k2hattrop.cc \
						k2hattrbuiltin.cc \
						k2hcryptcommon.cc \
						k2hattropsman.cc \
						k2hattrplugin.cc

### Source code for crypt
#
# [NOTE]
# The crypt type related source code has different source code
# linked depending on the option passed to configure. And only
# object built from selected crypt source code is linked.
# The distributed source codes(tgz) by 'make dist' must have
# only selected source which is selected crypt type. Because
# certain crypt libraries have problems that can not distribute
# the relevant source code due to licensing issues.
# Then source codes for crypt are declared with 'nodist', and
# they are defined in 'EXTRA_DIST' macro.
# By defining 'nodist' and 'EXTRA_DIST', we realize the
# distribution of selective source code.
#
EXTRA_DIST			= $(CRYPT_LIB_SOURCE)

if CRYPT_LIB_OPENSSL
nodist_libk2hash_la_SOURCES	= k2hcryptopenssl.cc
endif
if CRYPT_LIB_NSS
nodist_libk2hash_la_SOURCES	= k2hcryptnss.cc
endif
if CRYPT_LIB_NETTLE
nodist_libk2hash_la_SOURCES	= k2hcryptnettle.cc
endif
if CRYPT_LIB_GCRYPT
nodist_libk2hash_la_SOURCES	= k2hcryptgcrypt.cc
endif

libk2hash_la_LDFLAGS= -version-info $(LIB_VERSION_INFO)
libk2hash_la_LIBADD	= $(fullock_LIBS) $(CRYPT_LIBS) -lrt -lpthread

# [NOTE]
# "-Waddress-of-packed-member" optsion was introduced by default
# from GCC 9.
# Knowing that packed structure is CPU architecture dependent,
# this program ignores this warning.
#
AM_CFLAGS			= -DK2HASH -Wno-address-of-packed-member
AM_CPPFLAGS			= -DK2HASH -Wno-address-of-packed-member

ACLOCAL_AMFLAGS		= -I m4

### version(commit hash)
.PHONY:	k2hashversion

k2hashversion.cc:	k2hashversion
	@../buildutils/make_commit_hash_source.sh -dch @CURRENTREV@ k2hashversion.cc k2h_commit_hash

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
