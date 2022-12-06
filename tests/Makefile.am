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

bin_PROGRAMS = k2hlinetool k2hreplace k2hcompress k2htouch k2himport k2hbench

noinst_PROGRAMS = k2hinittest k2hrwtest k2hmemtest k2hexttest k2hstreamtest

k2hinittest_SOURCES = k2hinittest.cc
k2hinittest_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2hrwtest_SOURCES = k2hrwtest.cc
k2hrwtest_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2hmemtest_SOURCES = k2hmemtest.cc
k2hmemtest_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2hlinetool_SOURCES = k2hlinetool.cc
k2hlinetool_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2hexttest_SOURCES = k2hexttest.cc
k2hexttest_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2hstreamtest_SOURCES = k2hstreamtest.cc
k2hstreamtest_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2hreplace_SOURCES = k2hreplace.cc
k2hreplace_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2hcompress_SOURCES = k2hcompress.cc
k2hcompress_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2htouch_SOURCES = k2htouch.cc
k2htouch_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2himport_SOURCES = k2himport.cc
k2himport_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash

k2hbench_SOURCES = k2hbench.cc
k2hbench_LDADD = $(fullock_LIBS) -L../lib/.libs -lk2hash -lpthread

#
# [NOTE]
# If you need *.so shared libraries, use lib_LTLIBRARIES macro.
#
noinst_LTLIBRARIES = libk2htesthash.la libk2htesttrans.la libk2htestattr.la

libk2htesthash_la_SOURCES = k2htesthashfunc.cc
libk2htesthash_la_LDFLAGS = -version-info 1:0:0

libk2htesttrans_la_SOURCES = k2htesttransfunc.cc
libk2htesttrans_la_LDFLAGS = -version-info 1:1:0

libk2htestattr_la_SOURCES = k2htestattrfunc.cc
libk2htestattr_la_LDFLAGS = -version-info 1:0:0

ACLOCAL_AMFLAGS = -I m4

TESTS = test.sh

EXTRA_DIST = \
test.sh \
test_tool.sh \
test_linetool.sh \
test_encrypt_keys \
test_linetool.cmd \
test_linetool_dsave.cmd \
test_linetool.log

AM_CFLAGS = -I$(top_srcdir)/lib

AM_CPPFLAGS = -I$(top_srcdir)/lib

dist_bin_SCRIPTS = k2hedit

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
