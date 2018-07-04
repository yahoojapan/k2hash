#!/bin/sh
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
# This script puts git commit hash string to C header file.
# ex) char version[] = "....";
#
func_usage()
{
	echo ""
	echo "Usage:  $1 <file path> <value name>"
	echo "        file path     specify output file path"
	echo "        value name    specify variable name in output C source file"
	echo "        -h            print help"
	echo ""
}
PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`
COMMITHASH_TOOL="${MYSCRIPTDIR}/make_commit_hash.sh"

#
# Check options
#
FILEPATH=""
VALUENAME=""
while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break
	elif [ "X$1" = "X-h" -o "X$1" = "X-help" ]; then
		func_usage $PRGNAME
		exit 0
	else
		if [ "X${FILEPATH}" = "X" ]; then
			FILEPATH=$1
		elif [ "X${VALUENAME}" = "X" ]; then
			VALUENAME=$1
		else
			echo "ERROR: unknown option $1" 1>&2
			exit 1
		fi
	fi
	shift
done
if [ "X${FILEPATH}" = "X" -o "X${VALUENAME}" = "X" ]; then
	echo "ERROR: not specify file path and value name" 1>&2
	exit 1
fi

#
# Get commit hash value
#
COMMITHASH=`${COMMITHASH_TOOL} -short`
if [ $? -ne 0 ]; then
	echo "WARNING: git commit hash code is not found, so set to \"unknown\"." 1>&2
	COMMITHASH="unknown"
fi

#
# Make code line
#
NEWCODES="char ${VALUENAME}[] = \"${COMMITHASH}\";"

#
# Put codes to file
#
if [ -f ${FILEPATH} ]; then
	#
	# The file exists, then we need to check it whichever codes is same.
	#
	FILECODES=`cat ${FILEPATH}`

	if [ "X${FILECODES}" = "X${NEWCODES}" ]; then
		echo "SUCCESS: ${FILEPATH} already has current git commit hash value." 1>&2
		exit 0
	fi
fi

echo ${NEWCODES} > ${FILEPATH}
if [ $? -ne 0 ]; then
	echo "ERROR: Could not put git commit hash value to ${FILEPATH}" 1>&2
	exit 1
fi

echo "SUCCESS: ${FILEPATH} is updated with current git commit hash value(${COMMITHASH}) in ${VALUENAME} variable." 1>&2
exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
