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

#
# Common variables
#
PRGNAME=$(basename "${0}")
MYSCRIPTDIR=$(dirname "${0}")
SRCTOP=$(cd "${MYSCRIPTDIR}/.." || exit 1; pwd)

#
# Variables
#
RELEASE_VERSION_FILE="${SRCTOP}/RELEASE_VERSION"
GIT_DIR="${SRCTOP}/.git"
DEFAULT_CHANGELOG_FILE="${SRCTOP}/ChangeLog"
CHANGELOG_FILE=""
NOT_USE_GIT=0
NO_CHECK_VER_DIFF=0

#
# Utility functions
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [--help(-h)] [--not_use_git(-ng)] [--no_check_ver_diff(-nd)] [--file(-f) <changelog file>]"
	echo "        --help(-h)                   print help"
	echo "        --not_use_git(-ng)           specify for not checking git release tag by git tag command"
	echo "        --no_check_ver_diff(-nd)     specify for not comparing git release tag and changelog"
	echo "        --file(-f) <changelog file>  specify changelog file name in \"source top\" directory"
	echo ""
}

#
# Check options
#
while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		break

	elif echo "$1" | grep -q -i -e "^-h$" -e "^--help$"; then
		func_usage "${PRGNAME}"
		exit 0

	elif echo "$1" | grep -q -i -e "^-ng$" -e "^--not_use_git$"; then
		if [ "${NOT_USE_GIT}" -ne 0 ]; then
			echo "[ERROR] already --not_use_git(-ng) option is specified." 1>&2
			exit 1
		fi
		NOT_USE_GIT=1

	elif echo "$1" | grep -q -i -e "^-nd$" -e "^--no_check_ver_diff$"; then
		if [ "${NO_CHECK_VER_DIFF}" -ne 0 ]; then
			echo "[ERROR] already --no_check_ver_diff(-nd) option is specified." 1>&2
			exit 1
		fi
		NO_CHECK_VER_DIFF=1

	elif echo "$1" | grep -q -i -e "^-f$" -e "^--file$"; then
		if [ -n "${CHANGELOG_FILE}" ]; then
			echo "[ERROR] already --file(-f) option is specified(${CHANGELOG_FILE})." 1>&2
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] Must set changelog file path after -file(-f) option." 1>&2
			exit 1
		fi
		if [ ! -f "${SRCTOP}/$1" ]; then
			echo "[ERROR] Not found changelog($1) file " 1>&2
			exit 1
		fi
		CHANGELOG_FILE="${SRCTOP}/$1"

	else
		echo "[ERROR] Unknown option $1" 1>&2
		exit 1
	fi
	shift
done

if [ -z "${CHANGELOG_FILE}" ]; then
	if [ ! -f "${DEFAULT_CHANGELOG_FILE}" ]; then
		echo "[ERROR] Not found changelog(${DEFAULT_CHANGELOG_FILE}) file " 1>&2
		exit 1
	fi
	CHANGELOG_FILE="${DEFAULT_CHANGELOG_FILE}"
fi

#
# Version number from git tag command
#
# get version number from git release tag formatted following:
#	"v10", "v 10", "ver10", "ver-10", "version10", "version,10"
#	"v10.0.0", "v 10.0", "ver 10.0.0a", "v10.0.0-1", etc
#
# and the last build number is cut.(ex, "v10.0.1-1" -> "10.0.1")
#
if [ -d "${GIT_DIR}" ]; then
	if [ "${NOT_USE_GIT}" -eq 0 ]; then
		GIT_RELEASE_VERSION=$(git tag | grep -i '^v\(er\(sion\)\{0,1\}\)\{0,1\}' | sed -e 's/^v\(er\(sion\)\{0,1\}\)\{0,1\}//gi' | grep -o '[0-9]\+\([\.]\([0-9]\)\+\)\+\(.\)*$' | sed -e 's/-\(.\)*$//' | sort -t . -n -k 1,1 -k 2,2 -k 3,3 -k 4,4 | uniq | tail -1 | tr -d '\n')

		if [ -z "${GIT_RELEASE_VERSION}" ]; then
			echo "[WARNING] Could not get latest release tag from git release tag" 1>&2
			GIT_RELEASE_VERSION=""
		fi
	else
		GIT_RELEASE_VERSION=""
	fi
else
	echo "[WARNING] ${GIT_DIR} directory is not existed." 1>&2
	GIT_RELEASE_VERSION=
fi

#
# Version number from ChangeLog
#
# get version number from ChangeLog file formatted like debian.
# and the last build number is cut.(ex, "10.0.1-1" -> "10.0.1")
#
if [ -f "${CHANGELOG_FILE}" ]; then
	CHANGELOG_RELEASE_VERSION=$(grep -o '^.*[(].*[)].*[;].*$' "${CHANGELOG_FILE}" | grep -o '[(].*[)]' | sed -e 's/[(|)]//g' | head -1)

	if [ -z "${CHANGELOG_RELEASE_VERSION}" ]; then
		echo "[WARNING] Could not get latest release tag from ChangeLog file ( ${CHANGELOG_FILE} )" 1>&2
		CHANGELOG_RELEASE_VERSION=""
	fi
else
	echo "[MESSAGE] not found ChangeLog file(${CHANGELOG_FILE})" 1>&2
	CHANGELOG_RELEASE_VERSION=""
fi

#
# Check version number between git release tag and ChangeLog file
#
# If version number from git release tag is later than one from ChangeLog,
# this script puts error and exits.
# The other case, this script continue to work and puts version number
# to RELEASE_VERSION file.
# If there are no version number from git release tag and ChangeLog, this
# script checks RELEASE_VERSION file existing.
#
IS_PUT_RELEASE_VERSION_FILE=1

if [ -n "${GIT_RELEASE_VERSION}" ] && [ -n "${CHANGELOG_RELEASE_VERSION}" ]; then
	if [ "${NO_CHECK_VER_DIFF}" -eq 0 ]; then
		#
		# Check latest version
		#
		GIT_VERS=$(echo "${GIT_RELEASE_VERSION}" | sed -e 's/\./ /g')
		CHANGELOG_VERS=$(echo "${CHANGELOG_RELEASE_VERSION}" | sed -e 's/\./ /g')

		GIT_VER_PART_CNT=0
		LATEST_VER_TYPE=""
		for _git_ver_part in ${GIT_VERS}; do
			_changelog_ver_part=""
			CHANGELOG_VER_PART_CNT=0
			for _ver_tmp in ${CHANGELOG_VERS}; do
				_changelog_ver_part=$(echo "${_ver_tmp}" | sed -e 's/^[[:space:]]*//g' | tr -d '\n')

				CHANGELOG_VER_PART_CNT=$((CHANGELOG_VER_PART_CNT + 1))
				if [ "${GIT_VER_PART_CNT}" -lt "${CHANGELOG_VER_PART_CNT}" ]; then
					break
				fi
			done

			if [ -n "${_changelog_ver_part}" ]; then
				if [ "${_git_ver_part}" -gt "${_changelog_ver_part}" ]; then
					LATEST_VER_TYPE="gitver"
					break
				elif [ "${_git_ver_part}" -lt "${_changelog_ver_part}" ]; then
					LATEST_VER_TYPE="changlogver"
					break
				fi
			else
				LATEST_VER_TYPE="gitver"
				break
			fi

			GIT_VER_PART_CNT=$((GIT_VER_PART_CNT + 1))
		done

		if [ -z "${LATEST_VER_TYPE}" ]; then
			GIT_VER_PART_CNT=0
			for _git_ver_part in ${GIT_VERS}; do
				GIT_VER_PART_CNT=$((GIT_VER_PART_CNT + 1))
			done

			CHANGELOG_VER_PART_CNT=0
			for _ver_tmp in ${CHANGELOG_VERS}; do
				CHANGELOG_VER_PART_CNT=$((CHANGELOG_VER_PART_CNT + 1))
			done

			if [ "${GIT_VER_PART_CNT}" -lt "${CHANGELOG_VER_PART_CNT}" ]; then
				LATEST_VER_TYPE="changlogver"
			fi
		fi

		if [ -n "${LATEST_VER_TYPE}" ] && [ "${LATEST_VER_TYPE}" = "gitver" ]; then
			echo "[ERROR] git release tag ( ${GIT_RELEASE_VERSION} ) is later than ChangeLog file ( ${CHANGELOG_FILE} ) version ( ${CHANGELOG_RELEASE_VERSION} )." 1>&2
			exit 1

		elif [ -n "${LATEST_VER_TYPE}" ] && [ "${LATEST_VER_TYPE}" = "changlogver" ]; then
			echo "[WARNING] ChangeLog file ( ${CHANGELOG_FILE} ) version ( ${CHANGELOG_RELEASE_VERSION} ) is later than git release tag ( ${GIT_RELEASE_VERSION} )." 1>&2
			echo "          Then RELEASE_VERSION file is put git release tag ( ${GIT_RELEASE_VERSION} )" 1>&2

			RELEASE_VERSION="${GIT_RELEASE_VERSION}"

		else
			#
			# LATEST_VER_TYPE is not set, this means same version.
			#
			RELEASE_VERSION="${GIT_RELEASE_VERSION}"
		fi
	else
		#
		# Not check version number, so only use it from git
		#
		RELEASE_VERSION="${GIT_RELEASE_VERSION}"
	fi

elif [ -n "${GIT_RELEASE_VERSION}" ]; then
	RELEASE_VERSION="${GIT_RELEASE_VERSION}"

elif [ -n "${CHANGELOG_RELEASE_VERSION}" ]; then
	RELEASE_VERSION="${CHANGELOG_RELEASE_VERSION}"

elif [ -f "${RELEASE_VERSION_FILE}" ]; then
	RELEASE_VERSION=$(cat "${RELEASE_VERSION_FILE}")
	IS_PUT_RELEASE_VERSION_FILE=0

else
	echo "[ERROR] There is no version number information."				1>&2
	echo "        The version number must be given by git release tag"	1>&2
	echo "        or ChangeLog file or RELEASE_VERSION file."			1>&2
	exit 1
fi

#
# Make RELEASE_VERSION file
#
if [ "${IS_PUT_RELEASE_VERSION_FILE}" -eq 1 ]; then
	echo "[MESSAGE] Put version number \"${RELEASE_VERSION}\" to RELEASE_VERSION file" 1>&2
	printf '%s' "${RELEASE_VERSION}" > "${RELEASE_VERSION_FILE}"
fi


#
# Finish
#
echo "[SUCCEED] Version number is ${RELEASE_VERSION}" 1>&2
exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
