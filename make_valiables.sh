#!/bin/sh
#
# Puts project variables for building package - make_valiables.sh
#
# Copyright 2016 Yahoo! JAPAN corporation.
#
# Templates for customizing screwdriver CPP and autotools.
# This template files are provided by yjcore team.
#
# This script file puts some formatted variables for building
# packages. For building package, the project needs common
# valiables for example package name, version, etc.
# This script puts those variables from project files.
# 
# For the full copyright and license information, please view
# the LICENSE file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Thu, Jun 2 2016
# REVISION:
#

#
# Usage make_valiables.sh [-pkg_version | -lib_version_info | -major_number]
#
PROGRAM_NAME=`basename $0`
PROGRAM_DIR=`dirname $0`
TOP_DIR=`cd ${PROGRAM_DIR}; pwd`

if [ $# -ne 1 ]; then
	echo "ERROR: ${PROGRAM_NAME} needs parameter."
	echo "Usage: ${PROGRAM_NAME} [-pkg_version | -lib_version_info | -lib_version_for_link | -major_number]"
	exit 1
fi

#
# Make result
#
if [ "X$1" = "X-h" -o "X$1" = "X--help" ]; then
	echo "Usage: ${PROGRAM_NAME} [-pkg_version | -lib_version_info | -major_number]"
	exit 0

elif [ "X$1" = "X-pkg_version" ]; then
	RESULT=`cat ${TOP_DIR}/RELEASE_VERSION`

elif [ "X$1" = "X-lib_version_info" -o "X$1" = "X-lib_version_for_link" ]; then
	MAJOR_VERSION=`cat ${TOP_DIR}/RELEASE_VERSION | sed 's/["|\.]/ /g' | awk '{print $1}'`
	MID_VERSION=`cat ${TOP_DIR}/RELEASE_VERSION | sed 's/["|\.]/ /g' | awk '{print $2}'`
	LAST_VERSION=`cat ${TOP_DIR}/RELEASE_VERSION | sed 's/["|\.]/ /g' | awk '{print $3}'`

	# check version number
	expr "${MAJOR_VERSION}" + 1 >/dev/null 2>&1
	if [ $? -ge 2 ]; then
		echo "ERROR: wrong version number in RELEASE_VERSION file"
		exit 1
	fi
	expr "${MID_VERSION}" + 1 >/dev/null 2>&1
	if [ $? -ge 2 ]; then
		echo "ERROR: wrong version number in RELEASE_VERSION file"
		exit 1
	fi
	expr "${LAST_VERSION}" + 1 >/dev/null 2>&1
	if [ $? -ge 2 ]; then
		echo "ERROR: wrong version number in RELEASE_VERSION file"
		exit 1
	fi

	# make library revision number
	if [ ${MID_VERSION} -gt 0 ]; then
		REV_VERSION=`expr ${MID_VERSION} \* 1000`
		REV_VERSION=`expr ${LAST_VERSION} + ${REV_VERSION}`
	else
		REV_VERSION=${LAST_VERSION}
	fi

	if [ "X$1" = "X-lib_version_info" ]; then
		RESULT="${MAJOR_VERSION}:${REV_VERSION}:0"
	else
		RESULT="${MAJOR_VERSION}.0.${REV_VERSION}"
	fi

elif [ "X$1" = "X-major_number" ]; then
	RESULT=`cat ${TOP_DIR}/RELEASE_VERSION | sed 's/["|\.]/ /g' | awk '{print $1}'`

else
	echo "ERROR: unkown parameter $1"
	echo "Usage: ${PROGRAM_NAME} [-pkg_version | -lib_version_info | -lib_version_for_link | -major_number]"
	exit 1
fi

#
# Output result
#
echo -n $RESULT

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
