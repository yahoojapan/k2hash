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

#
# Common variables
#
PRGNAME=$(basename "${0}")
MYSCRIPTDIR=$(dirname "${0}")
SRCTOP=$(cd "${MYSCRIPTDIR}/.." || exit 1; pwd)

#
# Variables
#
DEFAULT_CHANGELOG_FILE="${SRCTOP}/ChangeLog"
CHANGELOG_FILE=""

#
# Utility functions
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [--help(-h)] <ChangeLog file>"
	echo "        --help(-h)          print help."
	echo "	      <ChangeLog file>    specify ChnageLog file path. if not specify, use ChangeLog file in top directory as default."
	echo ""
	echo "Environment:"
	echo "        BUILD_NUMBER        if specify a "build number" other than the default of 1, set it in this environment variable."
	echo ""
}

#
# Parse parameters
#
while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		break

	elif [ "$1" = "-h" ] || [ "$1" = "-H" ] || [ "$1" = "--help" ] || [ "$1" = "--HELP" ]; then
		func_usage "${PRGNAME}"
		exit 0

	else
		if [ -n "${CHANGELOG_FILE}" ]; then
			echo "[ERROR] already ChangeLog(${CHANGELOG_FILE}) file is specified." 1>&2
			exit 1
		fi
		if [ ! -f "$1" ]; then
			echo "[ERROR] $1 file is not existed." 1>&2
			exit 1
		fi
		CHANGELOG_FILE="$1"
	fi
	shift
done
if [ -z "${CHANGELOG_FILE}" ]; then
	if [ -f "${DEFAULT_CHANGELOG_FILE}" ]; then
		echo "[ERROR] ${DEFAULT_CHANGELOG_FILE} file is not existed." 1>&2
		exit 1
	fi
	CHANGELOG_FILE="${DEFAULT_CHANGELOG_FILE}"
fi

#
# Check Build number
#
# If the environment variable BUILD_NUMBER is specified, set its value to the
# build number. If not specified(default), it is 1.
#
if [ -z "${BUILD_NUMBER}" ]; then
	BUILD_NUMBER_STR="-1"
else
	if echo "${BUILD_NUMBER}" | grep -q "[^0-9]"; then
		echo "[ERROR] The BUILD_NUMBER environment has not number charactor(${BUILD_NUMBER})." 1>&2
		exit 1
	fi
	BUILD_NUMBER_STR="-${BUILD_NUMBER}"
fi

#
# Convert ChangeLog for RPM spec format
#
CONTENT_LINES_IN_VERSION=0
SET_FIRST_VERSION=0
OUTPUT_FIRST_LINE=0
CONTENT_LINES=""
while IFS= read -r ONE_LINE; do
	if [ -z "${ONE_LINE}" ]; then
		continue
	fi

	if ! echo "${ONE_LINE}" | grep -q "^[[:space:]]"; then
		#
		# new version line
		#
		if [ "${CONTENT_LINES_IN_VERSION}" -ne 0 ]; then
			echo "[ERROR] Something is wrong with the format. The committer and date do not exist on the line immediately before \"${ONE_LINE}\"." 1>&2
			exit 1
		fi
		PKG_VERSION=$(echo "${ONE_LINE}" | grep '^.*[(].*\..*[)].*[;].*$' | grep -o '[(].*[)]' | sed -e 's/[(|)]//g' -e 's/^[[:space:]]+//g' -e 's/[[:space:]]+$//g' | tr -d '\n')

		if [ -z "${PKG_VERSION}" ]; then
			echo "[ERROR] Something is wrong with the format. The package version does not exist in \"${ONE_LINE}\"." 1>&2
			exit 1
		fi

		if [ "${SET_FIRST_VERSION}" -eq 0 ]; then
			PKG_VERSION="${PKG_VERSION}${BUILD_NUMBER_STR}"
			SET_FIRST_VERSION=1
		fi
		CONTENT_LINES_IN_VERSION=1

	else
		#
		# in version lines
		#
		if [ "${CONTENT_LINES_IN_VERSION}" -eq 0 ]; then
			echo "[ERROR] Something is wrong with the format. The package name and version number do not exist in the line immediately before \"${ONE_LINE}\"." 1>&2
			exit 1
		fi

		if echo "${ONE_LINE}" | grep -q "^[[:space:]][-][-][[:space:]]"; then
			#
			# commiter/date line(last line)
			#
			PKG_COMMITTER=$(echo "${ONE_LINE}" | sed -e 's/^[[:space:]][-][-][[:space:]]//g' -e 's/[[:space:]][[:space:]].*$//g' -e 's/^[[:space:]]+//g' -e 's/[[:space:]]+$//g' | tr -d '\n')
			PKG_RFC2822=$(echo "${ONE_LINE}" | sed -e 's/^[[:space:]][-][-][[:space:]].*[[:space:]][[:space:]]//g' -e 's/^[[:space:]]+//g' -e 's/[[:space:]]+$//g' | tr -d '\n')

			if [ -z "${PKG_COMMITTER}" ] || [ -z "${PKG_RFC2822}" ]; then
				echo "[ERROR] Something is wrong with the format. The committer and date do not exist in \"${ONE_LINE}\"." 1>&2
				exit 1
			fi
			PKG_DATE=$(echo "${PKG_RFC2822}" | sed 's/,/ /g' | awk '{print $1" "$3" "$2" "$4}' | tr -d '\n')

			if [ "${OUTPUT_FIRST_LINE}" -eq 0 ]; then
				OUTPUT_FIRST_LINE=1
			else
				printf '\n'
			fi
			printf '* %s %s %s\n%s\n' "${PKG_DATE}" "${PKG_COMMITTER}" "${PKG_VERSION}" "${CONTENT_LINES}"

			CONTENT_LINES_IN_VERSION=0
			CONTENT_LINES=""

		elif echo "${ONE_LINE}" | grep -q "^[[:space:]][[:space:]]\*[[:space:]]"; then
			#
			# content line
			#
			ONE_CONTENT=$(echo "${ONE_LINE}" | sed -e 's/^[[:space:]][[:space:]]\*[[:space:]]//g' -e 's/^[[:space:]]+//g' -e 's/[[:space:]]+$//g' | tr -d '\n')

			if [ -z "${ONE_CONTENT}" ]; then
				echo "[ERROR] Something is wrong with the format. The content does not exist in \"${ONE_LINE}\"." 1>&2
				exit 1
			fi

			if [ -z "${CONTENT_LINES}" ]; then
				CONTENT_LINES=$(printf '%s- %s' '' "${ONE_CONTENT}")
			else
				CONTENT_LINES=$(printf '%s\n- %s' "${CONTENT_LINES}" "${ONE_CONTENT}")
			fi

		else
			echo "[ERROR] Something is wrong with the format in \"${ONE_LINE}\"." 1>&2
			exit 1
		fi
	fi
done < "${CHANGELOG_FILE}"

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
