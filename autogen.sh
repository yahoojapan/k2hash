#!/bin/sh
#
# Automatically generate configuration - autogen.sh
#
# Copyright 2016 Yahoo! JAPAN corporation.
#
# Templates for customizing screwdriver CPP and autotools.
# This template files are provided by yjcore team.
#
# This script file is for initializing autoconf/autotools
# files. And make version number information file from
# git tag(release), this file is used by other build tools.
#
# For the full copyright and license information, please view
# the LICENSE file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Thu, Jun 2 2016
# REVISION:
#

#
# Usage: autogen.sh [-noupdate_version_file] [-no_aclocal_force] [-no_check_ver_diff]
#
AUTOGEN_NAME=`basename $0`
AUTOGEN_DIR=`dirname $0`
SRCTOP=`cd ${AUTOGEN_DIR}; pwd`

echo "** run autogen.sh"

#
# Parameter
#
NOUPDATE="no"
FORCEPARAM="--force"
PARAMETERS=""
while [ $# -ne 0 ]; do
	if [ "X$1" = "X-noupdate_version_file" ]; then
		NOUPDATE="yes"
		FORCEPARAM=""	# do not need force
	elif [ "X$1" = "X-no_aclocal_force" ]; then
		FORCEPARAM=""
	elif [ "X$1" = "X-no_check_ver_diff" ]; then
		PARAMETERS="${PARAMETERS} $1"
	elif [ "X$1" = "X-h" -o "X$1" = "X--help" ]; then
		echo "Usage: ${AUTOGEN_NAME} [-noupdate_version_file] [-no_aclocal_force] [-no_check_ver_diff]"
		exit 1
	else
		echo "ERROR: Unkown option $1"
		echo "Usage: ${AUTOGEN_NAME} [-noupdate_version_file] [-no_aclocal_force] [-no_check_ver_diff]"
		exit 1
	fi
	shift
done

#
# update RELEASE_VERSION file
#
if [ "X${NOUPDATE}" = "Xno" ]; then
	${SRCTOP}/make_release_version_file.sh ${PARAMETERS}
	if [ $? -ne 0 ]; then
		echo "ERROR: update RELEASE_VERSION file"
		exit 1
	fi
fi

#
# Check files
#
if [ ! -f ${SRCTOP}/NEWS ]; then
	touch ${SRCTOP}/NEWS
fi
if [ ! -f ${SRCTOP}/README ]; then
	touch ${SRCTOP}/README
fi
if [ ! -f ${SRCTOP}/AUTHORS ]; then
	touch ${SRCTOP}/AUTHORS
fi
if [ ! -f ${SRCTOP}/ChangeLog ]; then
	touch ${SRCTOP}/ChangeLog
fi

#
# Auto scan
#
if [ ! -f configure.scan -o "X${FORCEPARAM}" != "X" ]; then
	echo "--- run autoscan"
	autoscan
	if [ $? -ne 0 ]; then
		echo "ERROR: something error occurred in autoscan"
		exit 1
	fi
fi

#
# Copy libtools
#
libtoolize --force --copy
if [ $? -ne 0 ]; then
	echo "ERROR: something error occurred in libtoolize"
	exit 1
fi

#
# Build configure and Makefile
#
echo "--- run aclocal ${FORCEPARAM}"
aclocal ${FORCEPARAM}
if [ $? -ne 0 ]; then
	echo "ERROR: something error occurred in aclocal ${FORCEPARAM}"
	exit 1
fi

echo "--- run autoheader"
autoheader
if [ $? -ne 0 ]; then
	echo "ERROR: something error occurred in autoheader"
	exit 1
fi

echo "--- run automake -c --add-missing"
automake -c --add-missing
if [ $? -ne 0 ]; then
	echo "ERROR: something error occurred in automake -c --add-missing"
	exit 1
fi

echo "--- run autoconf"
autoconf
if [ $? -ne 0 ]; then
	echo "ERROR: something error occurred in autoconf"
	exit 1
fi

echo "** SUCCEED: autogen"
exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
