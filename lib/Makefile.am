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

## AUTOMAKE_OPTIONS =

pkginclude_HEADERS = k2hash.h k2harchive.h k2hashfunc.h k2hcommand.h k2hcommon.h k2hdbg.h k2hfind.h k2hlock.h k2hmmapinfo.h k2hpagefile.h k2hpage.h k2hpagemem.h k2hshm.h k2hshmdirect.h k2hstructure.h k2hsubkeys.h k2hattrs.h k2htransfunc.h k2htrans.h k2hutil.h k2hdaccess.h k2hstream.h k2hfilemonitor.h k2hshmupdater.h k2hqueue.h k2hattrop.h k2hattrbuiltin.h k2hattropsman.h k2hattrplugin.h k2hattrfunc.h
pkgincludedir = $(includedir)/k2hash

EXTRA_DIST = 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libk2hash.pc
DISTCLEANFILES = $(pkgconfig_DATA)

lib_LTLIBRARIES = libk2hash.la
libk2hash_la_SOURCES = k2hash.cc k2hdbg.cc k2hmmapinfo.cc k2hpage.cc k2hpagefile.cc k2hpagemem.cc k2hshm.cc k2hshmdirect.cc k2hshminit.cc k2hshmdump.cc k2hshmcomp.cc k2hashfunc.cc k2hutil.cc k2hfind.cc k2hsubkeys.cc k2hattrs.cc k2hlock.cc k2hcommand.cc k2htrans.cc k2htransfunc.cc k2harchive.cc k2hdaccess.cc k2hfilemonitor.cc k2hshmupdater.cc k2hashversion.cc k2hshmque.cc k2hqueue.cc k2hattrop.cc k2hattrbuiltin.cc k2hattropsman.cc k2hattrplugin.cc
libk2hash_la_LDFLAGS = -version-info $(LIB_VERSION_INFO)
libk2hash_la_LIBADD = $(fullock_LIBS) -lrt -lpthread -lssl -lcrypto

AM_CFLAGS = -DK2HASH
AM_CPPFLAGS = -DK2HASH

ACLOCAL_AMFLAGS = -I m4

### version(commit hash)
.PHONY: k2hashversion

k2hashversion.cc: k2hashversion
	@../make_rev.sh k2hashversion.cc k2h_commit_hash

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
