#!/bin/sh
#
# K2HASH
#
# Utility tools for building configure/packages by AntPickax
#
# Copyright 2018 Yahoo Japan Corporation.
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
		echo "ERROR: Unknown option $1"
		echo "Usage: ${AUTOGEN_NAME} [-noupdate_version_file] [-no_aclocal_force] [-no_check_ver_diff]"
		exit 1
	fi
	shift
done

#
# update RELEASE_VERSION file
#
if [ "X${NOUPDATE}" = "Xno" ]; then
	echo "--- run make_release_version_file.sh"
	${SRCTOP}/buildutils/make_release_version_file.sh ${PARAMETERS}
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
