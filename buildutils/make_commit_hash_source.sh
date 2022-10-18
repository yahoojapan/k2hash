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

#
# Common variables
#
PRGNAME=$(basename "${0}")
MYSCRIPTDIR=$(dirname "${0}")
#SRCTOP=$(cd "${MYSCRIPTDIR}/.." || exit 1; pwd)

#
# Variables
#
COMMITHASH_TOOL="${MYSCRIPTDIR}/make_commit_hash.sh"
DIRECT_COMMIT_HASH=""
FILEPATH=""
VALUENAME=""

#
# Utility functions
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [--help(-h)] [--direct commit_hash(-dch) <commit hash>] <file path> <value name>"
	echo "        --help(-h)                                print help."
	echo "        --direct commit_hash(-dch) <commit_hash>  the commit_hash value directly when .git directory does not exist."
	echo "        file path                                 specify output file path"
	echo "        value name                                specify variable name in output C source file"
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

	elif [ "$1" = "-dch" ] || [ "$1" = "-DCH" ] || [ "$1" = "--direct commit_hash" ] || [ "$1" = "--DIRECT COMMIT_HASH" ]; then
		if [ -n "${DIRECT_COMMIT_HASH}" ]; then
			echo "[ERROR] Already --direct_commit_hash(-dch) option is specified(${DIRECT_COMMIT_HASH})." 1>&2
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] --direct_commit_hash(-dch) option needs parameter." 1>&2
			exit 1
		fi
		DIRECT_COMMIT_HASH="$1"

	else
		if [ -z "${FILEPATH}" ]; then
			FILEPATH="$1"
		elif [ -z "${VALUENAME}" ]; then
			VALUENAME="$1"
		else
			echo "[ERROR] unknown option $1" 1>&2
			exit 1
		fi
	fi
	shift
done
if [ -z "${FILEPATH}" ] || [ -z "${VALUENAME}" ]; then
	echo "[ERROR] must specify file path and value name" 1>&2
	exit 1
fi

#
# Get commit hash value
#
if ! COMMITHASH=$("${COMMITHASH_TOOL}" --short); then
	if [ -n "${DIRECT_COMMIT_HASH}" ]; then
		echo "[INFO] git commit hash code is not found, but commit hash was specified by option so use it." 1>&2
		COMMITHASH="${DIRECT_COMMIT_HASH}"
	else
		echo "[WARNING] git commit hash code is not found, so set to \"unknown\"." 1>&2
		COMMITHASH="unknown"
	fi
fi

#
# Gerenate codes
#
GENARATED_CODES="char ${VALUENAME}[] = \"${COMMITHASH}\";"

#
# Put codes to file
#
if [ -f "${FILEPATH}" ]; then
	#
	# The file exists, then we need to check it whichever codes is same.
	#
	EXISTED_CODES=$(cat "${FILEPATH}")

	if [ -n "${EXISTED_CODES}" ] && [ "${EXISTED_CODES}" = "${GENARATED_CODES}" ]; then
		echo "[SUCCESS] ${FILEPATH} already has current git commit hash value." 1>&2
		exit 0
	fi
fi

if ! echo "${GENARATED_CODES}" > "${FILEPATH}"; then
	echo "[ERROR] Could not put git commit hash value to ${FILEPATH}" 1>&2
	exit 1
fi

echo "[SUCCESS] ${FILEPATH} is updated with current git commit hash value(${COMMITHASH}) in ${VALUENAME} variable." 1>&2
exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
