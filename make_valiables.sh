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
	echo "Usage: ${PROGRAM_NAME} [-pkg_version | -lib_version_info | -major_number]"
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

elif [ "X$1" = "X-lib_version_info" ]; then
	RESULT=`cat ${TOP_DIR}/RELEASE_VERSION | sed 's/["|\.]/ /g' | awk '{print $1":"$3":0"}'`

elif [ "X$1" = "X-major_number" ]; then
	RESULT=`cat ${TOP_DIR}/RELEASE_VERSION | sed 's/["|\.]/ /g' | awk '{print $1}'`

else
	echo "ERROR: unkown parameter $1"
	echo "Usage: ${PROGRAM_NAME} [-pkg_version | -lib_version_info | -major_number]"
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
