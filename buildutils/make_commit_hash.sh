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
# This script gets commit hash(sha1) from github.com or .git directory
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
RELEASEVER_FILE="${SRCTOP}/RELEASE_VERSION"
COMMITHASH_FILE="${SRCTOP}/COMMIT_HASH"
DEFAULTENDPOINT="https://api.github.com/repos/"
GITDIR=0
ORGNAME=""
REPONAME=""
ENDPOINT=""
TAGNAME=""
ISSHORT=0

#
# Utility functions
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [--help(-h)] [--organization(-o) <org>] [--reponame(-r) <repo>] [--endpoint(-ep) <endpoint>] [--tag(-t) <tag>] [--short(-s)]"
	echo "        --help(-h)                 print help"
	echo "        --organization(-o) <org>   specify organazation name if .git directory is not existed"
	echo "        --reponame(-r) <repo>      specify repository name if .git directory is not existed"
	echo "        --endpoint(-ep) <endpoint> if not github.com, specify endpoint for api"
	echo "        --tag(-t) <tag>            if not find release version tag, use this"
	echo "        --short(-s)                get short commit hash(sha1)"
	echo ""
}

#
# Parse parameters
#
while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		break

	elif echo "$1" | grep -q -i -e "^-h$" -e "^--help$"; then
		func_usage "${PRGNAME}"
		exit 0

	elif echo "$1" | grep -q -i -e "^-o$" -e "^--organization$"; then
		if [ -n "${ORGNAME}" ]; then
			echo "[ERROR] already --organization(-o) option is specified(${ORGNAME})." 1>&2
			echo "unknown"
			exit 1
		fi
		shift
		if [ $# -eq 0 ] || [ -z "$1" ]; then
			echo "[ERROR] --organization(-o) option needs parameter" 1>&2
			echo "unknown"
			exit 1
		fi
		ORGNAME="$1"

	elif echo "$1" | grep -q -i -e "^-r$" -e "^--reponame$"; then
		if [ -n "${REPONAME}" ]; then
			echo "[ERROR] already --reponame(-r) option is specified(${REPONAME})." 1>&2
			echo "unknown"
			exit 1
		fi
		shift
		if [ $# -eq 0 ] || [ -z "$1" ]; then
			echo "[ERROR] --reponame(-r) option needs parameter" 1>&2
			echo "unknown"
			exit 1
		fi
		REPONAME="$1"

	elif echo "$1" | grep -q -i -e "^-ep$" -e "^--endpoint$"; then
		if [ -n "${ENDPOINT}" ]; then
			echo "[ERROR] already --endpoint(-ep) option is specified(${ENDPOINT})." 1>&2
			echo "unknown"
			exit 1
		fi
		shift
		if [ $# -eq 0 ] || [ -z "$1" ]; then
			echo "[ERROR] --endpoint(-ep) option needs parameter" 1>&2
			echo "unknown"
			exit 1
		fi
		ENDPOINT="$1"

	elif echo "$1" | grep -q -i -e "^-t$" -e "^--tag$"; then
		if [ -n "${TAGNAME}" ]; then
			echo "[ERROR] already --tag(-t) option is specified(${TAGNAME})." 1>&2
			echo "unknown"
			exit 1
		fi
		shift
		if [ $# -eq 0 ] || [ -z "$1" ]; then
			echo "[ERROR] --tag(-t) option needs parameter" 1>&2
			echo "unknown"
			exit 1
		fi
		TAGNAME="$1"

	elif echo "$1" | grep -q -i -e "^-s$" -e "^--short$"; then
		if [ "${ISSHORT}" -eq 1 ]; then
			echo "[ERROR] already --short(-s) option is specified." 1>&2
			echo "unknown"
			exit 1
		fi
		ISSHORT=1

	else
		echo "[ERROR] unknown option $1" 1>&2
		echo "unknown"
		exit 1
	fi
	shift
done

if [ -z "${ENDPOINT}" ]; then
	ENDPOINT="${DEFAULTENDPOINT}"
fi

#
# Check .git directory
#
if [ -d "${SRCTOP}/.git" ]; then
	GITDIR=1
fi

#
# Main
#
if [ "${GITDIR}" -eq 1 ]; then
	#
	# Can use git command
	#
	if [ -f "${COMMITHASH_FILE}" ]; then
		rm -f "${COMMITHASH_FILE}"
	fi

	if [ "${ISSHORT}" -eq 1 ]; then
		SHORTOPT="--short"
	else
		SHORTOPT=""
	fi
	SHA1RESULT=$(git rev-parse "${SHORTOPT}" HEAD)

elif [ -f "${COMMITHASH_FILE}" ]; then
	#
	# Found COMMIT_HASH file, load hash value from it
	#
	if [ "${ISSHORT}" -eq 1 ]; then
		SHA1RESULT=$(cut -c 1-7 < "${COMMITHASH_FILE}" 2>/dev/null)
	else
		SHA1RESULT=$(cat "${COMMITHASH_FILE}")
	fi

else
	#
	# Try to access github api
	#
	if [ -z "${ORGNAME}" ] || [ -z "${REPONAME}" ]; then
		echo "[ERROR] must specify --organization(-o) and --reponame(-r) options, because .git directory is not existed." 1>&2
		echo "unknown"
		exit 1
	fi

	if [ -z "${TAGNAME}" ]; then
		if [ -f "${RELEASEVER_FILE}" ]; then
			#
			# release tag from RELEASE_VERSION file
			#
			TAGNAME=$(cat "${RELEASEVER_FILE}")
		else
			#
			# release tag from source top directory name(expects source files from tar.gz file on github)
			#
			SRCTOP_NAME=$(basename "${SRCTOP}")
			TAGNAME=$(echo "${SRCTOP_NAME}" | sed -e 's/-/ /g' | awk '{print $NF}')
		fi
	fi

	#
	# first, access github api with adding "v" to tagname
	#
	SHA1RESULT=$(curl "${ENDPOINT}${ORGNAME}/${REPONAME}/git/refs/tags/v${TAGNAME}" 2>/dev/null | grep '"sha":' 2>/dev/null | awk '{print $2}' 2>/dev/null | sed -e 's/[",]//g' 2>/dev/null)
	if [ -z "${SHA1RESULT}" ]; then
		#
		# retry without "v"
		#
		SHA1RESULT=$(curl "${ENDPOINT}${ORGNAME}/${REPONAME}/git/refs/tags/${TAGNAME}" 2>/dev/null | grep '"sha":' 2>/dev/null | awk '{print $2}' 2>/dev/null | sed -e 's/[",]//g' 2>/dev/null)
		if [ -z "${SHA1RESULT}" ]; then
			echo "[ERROR] Could not get commit hash for ${ORGNAME}/${REPONAME}:${TAGNAME}" 1>&2
			echo "unknown"
			exit 1
		fi
	fi

	#
	# put commit hash file(as long)
	#
	echo "${SHA1RESULT}" > "${COMMITHASH_FILE}"

	if [ "${ISSHORT}" -eq 1 ]; then
		SHA1RESULT=$(echo "${SHA1RESULT}" 2>/dev/null | cut -c 1-7 2>/dev/null)
	fi
fi

#
# Finally confirmation
#
if [ -z "${SHA1RESULT}" ]; then
	echo "[ERROR] Could not get commit hash" 1>&2
	echo "unknown"
	exit 1
fi

echo "${SHA1RESULT}"
exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
