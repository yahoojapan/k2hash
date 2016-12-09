#!/bin/sh
#
# Make RELEASE_VERSION file - make_release_version_file.sh
#
# Copyright 2016 Yahoo! JAPAN corporation.
#
# Templates for customizing screwdriver CPP and autotools.
# This template files are provided by yjcore team.
#
# This script makes RELEASE_VERSION from github release tag
# or ChangeLog file, and check version.
#
# For the full copyright and license information, please view
# the LICENSE file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Thu, Jun 2 2016
# REVISION:
#

#####################################################################
#
# Usage: make_release_version_file.sh [-not_use_git] [-no_check_ver_diff] [-f changelog file path]
#
PROGRAM_NAME=`basename $0`
SRCTOP=`dirname $0`

#####################################################################
#
# Parameter
#
NOGIT="no"
NOCHECKVERDIFF="no"
CHANGELOGFILE="ChangeLog"
while [ $# -ne 0 ]; do
	if [ "X$1" = "X-not_use_git" ]; then
		NOGIT="yes"

	elif [ "X$1" = "X-no_check_ver_diff" ]; then
		NOCHECKVERDIFF="yes"

	elif [ "X$1" = "X-f" ]; then
		shift
		if [ $# -eq 0 ]; then
			echo "ERROR: Must set changelog file name after -f option."
			echo "Usage: ${PROGRAM_NAME} [-not_use_git] [-no_check_ver_diff] [-f changefile path]"
			exit 1
		fi
		if [ ! -f ${SRCTOP}/$1 ]; then
			echo "ERROR: Not found changelog($1) file "
			exit 1
		fi
		CHANGELOGFILE=$1

	elif [ "X$1" = "X-h" -o "X$1" = "X--help" ]; then
		echo "Usage: ${PROGRAM_NAME} [-not_use_git] [-no_check_ver_diff] [-f changefile path]"
		exit 1
	else
		echo "ERROR: Unkown option $1"
		echo "Usage: ${PROGRAM_NAME} [-not_use_git] [-no_check_ver_diff] [-f changefile path]"
		exit 1
	fi

	shift
done

#####################################################################
#
# Version number from Github
#
# get version number from git release tag formatted following:
#	"v10", "v 10", "ver10", "ver-10", "version10", "version,10"
#	"v10.0.0", "v 10.0", "ver 10.0.0a", "v10.0.0-1", etc
#
# and the last build number is cut.(ex, "v10.0.1-1" -> "10.0.1")
#
if [ "X${NOGIT}" = "Xno" ]; then
	GIT_RELEASE_VERSION=`git tag | grep '^[v|V]\([e|E][r|R]\([s|S][i|I][o|O][n|N]\)\{0,1\}\)\{0,1\}' | sed 's/^[v|V]\([e|E][r|R]\([s|S][i|I][o|O][n|N]\)\{0,1\}\)\{0,1\}//' | grep -o '[0-9]\+\([\.]\([0-9]\)\+\)\+\(.\)*$' | sed 's/-\(.\)*$//' | sort -t . -n -k 1,1 -k 2,2 -k 3,3 -k 4,4 | uniq | tail -1 | tr -d '\n'`

	if [ "X${GIT_RELEASE_VERSION}" = "X" ]; then
		echo "WARNING: Could not get latest release tag from github release tag"
		GIT_RELEASE_VERSION=
	fi
else
	GIT_RELEASE_VERSION=
fi

#####################################################################
#
# Version number from ChangeLog
#
# get version number from ChangeLog file formatted like debian.
# and the last build number is cut.(ex, "10.0.1-1" -> "10.0.1")
#
if [ -f ${SRCTOP}/${CHANGELOGFILE} ]; then
	CH_RELEASE_VERSION=`grep -o '^.*[(].*[)].*[;].*$' ${SRCTOP}/${CHANGELOGFILE} | grep -o '[(].*[)]' | head -1 | sed 's/[(|)]//g'`

	if [ "X${CH_RELEASE_VERSION}" = "X" ]; then
		echo "WARNING: Could not get latest release tag from ChangeLog file ( ${SRCTOP}/${CHANGELOGFILE} )"
		CH_RELEASE_VERSION=
	fi
else
	echo "MESSAGE: not found ChangeLog file ( ${SRCTOP}/${CHANGELOGFILE} )"
	CH_RELEASE_VERSION=
fi

#####################################################################
#
# Check version number between github release tag and ChangeLog file
#
# If version number from Github is later than one from ChangeLog,
# this script puts error and exits.
# The other case, this script continue to work and puts version number
# to RELEASE_VERION file.
# If there are no version number from Github and ChangeLog, this script
# checks RELEASE_VERION file existing.
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
			echo "ERROR: Github release tag ( ${GIT_RELEASE_VERSION} ) is later than ChangeLog file ( ${CHANGELOGFILE} ) version ( ${CH_RELEASE_VERSION} )."
			exit 1

		elif [ "X${LATEST_VER_TYPE}" = "Xchver" ]; then
			echo "WARNING: ChangeLog file ( ${CHANGELOGFILE} ) version ( ${CH_RELEASE_VERSION} ) is later than Github release tag ( ${GIT_RELEASE_VERSION} )."
			echo "         Then RELEASE_VERSION file is put Github release tag ( ${GIT_RELEASE_VERSION} )"

			RELEASE_VERSION=${GIT_RELEASE_VERSION}

		else
			# LATEST_VER_TYPE is not set, this means same version.

			RELEASE_VERSION=${GIT_RELEASE_VERSION}
		fi

	else
		#
		# Not check version number, so only use it from Github
		#
		RELEASE_VERSION=${GIT_RELEASE_VERSION}
	fi

elif [ "X${GIT_RELEASE_VERSION}" != "X" ]; then
	RELEASE_VERSION=${GIT_RELEASE_VERSION}

elif [ "X${CH_RELEASE_VERSION}" != "X" ]; then
	RELEASE_VERSION=${CH_RELEASE_VERSION}

elif [ -f ${SRCTOP}/RELEASE_VERSION ]; then
	RELEASE_VERSION=`cat ${SRCTOP}/RELEASE_VERSION`
	IS_PUT_RELEASE_VERSION_FILE=no

else
	echo "ERROR: There is no version number information."
	echo "       The version number must be given by Github release tag"
	echo "       or ChangeLog file or RELEASE_VERSION file."
	exit 1
fi

#####################################################################
#
# Make RELEASE_VERSION file
#
if [ "X${IS_PUT_RELEASE_VERSION_FILE}" = "Xyes" ]; then
	echo "MESSAGE: Put version number ${RELEASE_VERSION} to RELEASE_VERSION file"

	echo -n ${RELEASE_VERSION} > ${SRCTOP}/RELEASE_VERSION
fi


#####################################################################
#
# finish
#
echo "SUCCEED: ${PROGRAM_NAME}"
echo "         Result - Version number is ${RELEASE_VERSION}"
echo ""

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
