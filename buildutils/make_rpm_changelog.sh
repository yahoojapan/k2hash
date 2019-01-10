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
# Convert ChangeLog to use it in spec file for rpm
#
func_usage()
{
	echo ""
	echo "Usage:  $1 <ChangeLog file> [-h]"
	echo "	<ChangeLog file>    specify ChnageLog file path. if not specify, use ChangeLog file in top directory as default."
	echo "	-h(help)            print help."
	echo ""
}
PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`

#
# Check options
#
CHANGELOG_FILE="${SRCTOP}/ChangeLog"
ISSETCHANGELOG=0
while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break;

	elif [ "X$1" = "X-h" -o "X$1" = "X-help" ]; then
		func_usage $PRGNAME
		exit 0

	else
		if [ ${ISSETCHANGELOG} -ne 0 ]; then
			echo "ERROR: already ${CHANGELOG_FILE} file is specified." 1>&2
			echo "No changelog by ${CHANGELOG_FILE} with error."
			exit 1
		fi
		if [ ! -f $1 ]; then
			echo "ERROR: $1 file is not existed." 1>&2
			echo "No changelog by ${CHANGELOG_FILE} with error."
			exit 1
		fi
		CHANGELOG_FILE=$1
		ISSETCHANGELOG=1
	fi
	shift
done

#
# convert ChangeLog to spec file format for rpm
#
if [ "X${BUILD_NUMBER}" = "X" ]; then
	# default build number is 1
	BUILD_NUMBER_STR="-1"
else
	BUILD_NUMBER_STR="-${BUILD_NUMBER}"
fi

INONEVER=0
DETAILS=""
ALLLINES=""
SET_FIRST_VERSION=0
while read oneline; do
	oneline=`echo "${oneline}"`
	if [ "X${oneline}" = "X" ]; then
		continue
	fi

	if [ ${INONEVER} -eq 0 ]; then
		PKG_VERSION=`echo "${oneline}" | grep '^.*[(].*\..*[)].*[;].*$' | grep -o '[(].*[)]' | sed 's/[(|)]//g'`
		PKG_VERSION=`echo "${PKG_VERSION}"`
		if [ "X${PKG_VERSION}" != "X" ]; then
			INONEVER=1
			DETAILS=""
			if [ ${SET_FIRST_VERSION} -eq 0 ]; then
				PKG_VERSION="${PKG_VERSION}${BUILD_NUMBER_STR}"
				SET_FIRST_VERSION=1
			fi
		fi
	else
		TEST_CONTENTS=`echo "${oneline}" | grep '^[-][-].*[ ][ ].*$'`
		PKG_RF2822=`echo "${TEST_CONTENTS}" | grep -o '[ ][ ].*'`
		PKG_RF2822=`echo ${PKG_RF2822}`
		PKG_COMMITTER=`echo "${TEST_CONTENTS}" | grep -o '.*[ ][ ]' | sed 's/^[-][-][ ]//'`
		if [ "X${PKG_RF2822}" != "X" -a "X${PKG_COMMITTER}" != "X" ]; then
			INONEVER=0
			PKG_DATE=`echo ${PKG_RF2822} | sed 's/,/ /g' | awk '{print $1" "$3" "$2" "$4}'`
			PKG_LINE="* ${PKG_DATE} ${PKG_COMMITTER} ${PKG_VERSION}${DETAILS}"
			if [ "X${ALLLINES}" != "X" ]; then
				ALLLINES="${ALLLINES}\\n\\n${PKG_LINE}"
			else
				ALLLINES="${PKG_LINE}"
			fi
		else
			ONEDETAIL=`echo "$oneline" | grep '^[\*][ ].*' | sed 's/^[\*]//g'`
			ONEDETAIL=`echo ${ONEDETAIL}`
			if [ "X${ONEDETAIL}" != "X" ]; then
				DETAILS="${DETAILS}\\n- ${ONEDETAIL}"
			fi
		fi
	fi
done < ${CHANGELOG_FILE}

#
# print changelog
#
# NOTE: echo command on ubuntu is print '-e', we need to cut it.
#
echo -e "${ALLLINES}" | sed 's/^-e //g'

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
