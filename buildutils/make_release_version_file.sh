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
# Make RELEASE_VERSION file in source top directory
#
# RELEASE_VERSION file is used from configure.ac and other files for building.
# RELEASE_VERSION has version(release) number in it, it is created from Git
# release tag or latest version in ChangeLog file.
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [-not_use_git] [-no_check_ver_diff] [-f changelog file path]"
	echo "        -not_use_git          specify for not checking git release tag by git tag command"
	echo "        -no_check_ver_diff    specify for not comparing git release tag and changelog"
	echo "        -f changelog          specify changelog file name in source top directory"
	echo "        -h                    print help"
	echo ""
}
PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`
RELEASE_VERSION_FILE="${SRCTOP}/RELEASE_VERSION"
GIT_DIR="${SRCTOP}/.git"


#
# Check options
#
NOGIT="no"
NOCHECKVERDIFF="no"
CHANGELOGFILE="${SRCTOP}/ChangeLog"
while [ $# -ne 0 ]; do
	if [ "X$1" = "X-h" -o "X$1" = "X-help" ]; then
		func_usage $PRGNAME
		exit 0

	elif [ "X$1" = "X-not_use_git" ]; then
		NOGIT="yes"

	elif [ "X$1" = "X-no_check_ver_diff" ]; then
		NOCHECKVERDIFF="yes"

	elif [ "X$1" = "X-f" ]; then
		shift
		if [ $# -eq 0 ]; then
			echo "ERROR: Must set changelog file name after -f option." 1>&2
			exit 1
		fi
		if [ ! -f ${SRCTOP}/$1 ]; then
			echo "ERROR: Not found changelog($1) file " 1>&2
			exit 1
		fi
		CHANGELOGFILE=${SRCTOP}/$1

	else
		echo "ERROR: Unkown option $1" 1>&2
		exit 1
	fi
	shift
done

#
# Version number from git tag command
#
# get version number from git release tag formatted following:
#	"v10", "v 10", "ver10", "ver-10", "version10", "version,10"
#	"v10.0.0", "v 10.0", "ver 10.0.0a", "v10.0.0-1", etc
#
# and the last build number is cut.(ex, "v10.0.1-1" -> "10.0.1")
#
if [ -d ${GIT_DIR} ]; then
	if [ "X${NOGIT}" = "Xno" ]; then
		GIT_RELEASE_VERSION=`git tag | grep '^[v|V]\([e|E][r|R]\([s|S][i|I][o|O][n|N]\)\{0,1\}\)\{0,1\}' | sed 's/^[v|V]\([e|E][r|R]\([s|S][i|I][o|O][n|N]\)\{0,1\}\)\{0,1\}//' | grep -o '[0-9]\+\([\.]\([0-9]\)\+\)\+\(.\)*$' | sed 's/-\(.\)*$//' | sort -t . -n -k 1,1 -k 2,2 -k 3,3 -k 4,4 | uniq | tail -1 | tr -d '\n'`

		if [ "X${GIT_RELEASE_VERSION}" = "X" ]; then
			echo "WARNING: Could not get latest release tag from git release tag" 1>&2
			GIT_RELEASE_VERSION=
		fi
	else
		GIT_RELEASE_VERSION=
	fi
else
	echo "WARNING: ${GIT_DIR} directory is not existed." 1>&2
	GIT_RELEASE_VERSION=
fi

#
# Version number from ChangeLog
#
# get version number from ChangeLog file formatted like debian.
# and the last build number is cut.(ex, "10.0.1-1" -> "10.0.1")
#
if [ -f ${CHANGELOGFILE} ]; then
	CH_RELEASE_VERSION=`grep -o '^.*[(].*[)].*[;].*$' ${CHANGELOGFILE} | grep -o '[(].*[)]' | head -1 | sed 's/[(|)]//g'`

	if [ "X${CH_RELEASE_VERSION}" = "X" ]; then
		echo "WARNING: Could not get latest release tag from ChangeLog file ( ${CHANGELOGFILE} )" 1>&2
		CH_RELEASE_VERSION=
	fi
else
	echo "MESSAGE: not found ChangeLog file ( ${CHANGELOGFILE} )" 1>&2
	CH_RELEASE_VERSION=
fi

#
# Check version number between git release tag and ChangeLog file
#
# If version number from git release tag is later than one from ChangeLog,
# this script puts error and exits.
# The other case, this script continue to work and puts version number
# to RELEASE_VERION file.
# If there are no version number from git release tag and ChangeLog, this
# script checks RELEASE_VERION file existing.
#
IS_PUT_RELEASE_VERSION_FILE=yes

if [ "X${GIT_RELEASE_VERSION}" != "X" -a "X${CH_RELEASE_VERSION}" != "X" ]; then
	if [ "X${NOCHECKVERDIFF}" = "Xno" ]; then
		#
		# Check latest version
		#
		GIT_VERS=`echo ${GIT_RELEASE_VERSION} | sed 's/\./ /g'`
		CH_VERS=`echo ${CH_RELEASE_VERSION} | sed 's/\./ /g'`

		GIT_VER_PART_CNT=0
		LATEST_VER_TYPE=
		for git_ver_part in ${GIT_VERS}; do
			ch_ver_part=

			CH_VER_PART_CNT=0
			for ver_tmp in ${CH_VERS}; do
				ch_ver_part=`echo ${ver_tmp}`
				CH_VER_PART_CNT=`expr ${CH_VER_PART_CNT} + 1`

				if [ ${GIT_VER_PART_CNT} -lt ${CH_VER_PART_CNT} ]; then
					break
				fi
			done

			if [ "X${ch_ver_part}" != "X" ]; then
				if [ ${git_ver_part} -gt ${ch_ver_part} ]; then
					LATEST_VER_TYPE=gitver
					break
				elif [ ${git_ver_part} -lt ${ch_ver_part} ]; then
					LATEST_VER_TYPE=chver
					break
				fi
			else
				LATEST_VER_TYPE=gitver
				break
			fi

			GIT_VER_PART_CNT=`expr ${GIT_VER_PART_CNT} + 1`
		done

		if [ "X${LATEST_VER_TYPE}" = "X" ]; then
			GIT_VER_PART_CNT=0
			for git_ver_part in ${GIT_VERS}; do
				GIT_VER_PART_CNT=`expr ${GIT_VER_PART_CNT} + 1`
			done

			CH_VER_PART_CNT=0
			for ver_tmp in ${CH_VERS}; do
				CH_VER_PART_CNT=`expr ${CH_VER_PART_CNT} + 1`
			done

			if [ ${GIT_VER_PART_CNT} -lt ${CH_VER_PART_CNT} ]; then
				LATEST_VER_TYPE=chver
			fi
		fi

		if [ "X${LATEST_VER_TYPE}" = "Xgitver" ]; then
			echo "ERROR: git release tag ( ${GIT_RELEASE_VERSION} ) is later than ChangeLog file ( ${CHANGELOGFILE} ) version ( ${CH_RELEASE_VERSION} )." 1>&2
			exit 1

		elif [ "X${LATEST_VER_TYPE}" = "Xchver" ]; then
			echo "WARNING: ChangeLog file ( ${CHANGELOGFILE} ) version ( ${CH_RELEASE_VERSION} ) is later than git release tag ( ${GIT_RELEASE_VERSION} )." 1>&2
			echo "         Then RELEASE_VERSION file is put git release tag ( ${GIT_RELEASE_VERSION} )" 1>&2

			RELEASE_VERSION=${GIT_RELEASE_VERSION}

		else
			# LATEST_VER_TYPE is not set, this means same version.

			RELEASE_VERSION=${GIT_RELEASE_VERSION}
		fi

	else
		#
		# Not check version number, so only use it from git
		#
		RELEASE_VERSION=${GIT_RELEASE_VERSION}
	fi

elif [ "X${GIT_RELEASE_VERSION}" != "X" ]; then
	RELEASE_VERSION=${GIT_RELEASE_VERSION}

elif [ "X${CH_RELEASE_VERSION}" != "X" ]; then
	RELEASE_VERSION=${CH_RELEASE_VERSION}

elif [ -f ${RELEASE_VERSION_FILE} ]; then
	RELEASE_VERSION=`cat ${RELEASE_VERSION_FILE}`
	IS_PUT_RELEASE_VERSION_FILE=no

else
	echo "ERROR: There is no version number information." 1>&2
	echo "       The version number must be given by git release tag" 1>&2
	echo "       or ChangeLog file or RELEASE_VERSION file." 1>&2
	exit 1
fi

#
# Make RELEASE_VERSION file
#
if [ "X${IS_PUT_RELEASE_VERSION_FILE}" = "Xyes" ]; then
	echo "MESSAGE: Put version number ${RELEASE_VERSION} to RELEASE_VERSION file" 1>&2
	echo -n ${RELEASE_VERSION} > ${RELEASE_VERSION_FILE}
fi


#
# finish
#
echo "SUCCEED: Version number is ${RELEASE_VERSION}" 1>&2
exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
