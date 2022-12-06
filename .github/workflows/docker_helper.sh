#!/bin/sh
#
# Utility helper tools for Github Actions by AntPickax
#
# Copyright 2021 Yahoo Japan Corporation.
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
# CREATE:   Tue, Aug 3 2021
# REVISION:	1.0
#

#----------------------------------------------------------
# Docker Image Helper for container on Github Actions
#----------------------------------------------------------

#==========================================================
# Common setting
#==========================================================
#
# Instead of pipefail(for shells not support "set -o pipefail")
#
PIPEFAILURE_FILE="/tmp/.pipefailure.$(od -An -tu4 -N4 /dev/random | tr -d ' \n')"

#
# For shellcheck
#
if locale -a | grep -q -i '^[[:space:]]*C.utf8[[:space:]]*$'; then
	LANG=$(locale -a | grep -i '^[[:space:]]*C.utf8[[:space:]]*$' | sed -e 's/^[[:space:]]*//g' -e 's/[[:space:]]*$//g' | tr -d '\n')
	LC_ALL="${LANG}"
	export LANG
	export LC_ALL
elif locale -a | grep -q -i '^[[:space:]]*en_US.utf8[[:space:]]*$'; then
	LANG=$(locale -a | grep -i '^[[:space:]]*en_US.utf8[[:space:]]*$' | sed -e 's/^[[:space:]]*//g' -e 's/[[:space:]]*$//g' | tr -d '\n')
	LC_ALL="${LANG}"
	export LANG
	export LC_ALL
fi

#==========================================================
# Common Variables
#==========================================================
PRGNAME=$(basename "$0")
SCRIPTDIR=$(dirname "$0")
SCRIPTDIR=$(cd "${SCRIPTDIR}" || exit 1; pwd)
SRCTOP=$(cd "${SCRIPTDIR}/../.." || exit 1; pwd)

#
# Directories / Files
#
DOCKER_TEMPL_FILE="Dockerfile.templ"
DOCKER_FILE="Dockerfile"

#==========================================================
# Utility functions and variables for messaging
#==========================================================
#
# Utilities for message
#
if [ -t 1 ] || { [ -n "${CI}" ] && [ "${CI}" = "true" ]; }; then
	CBLD=$(printf '\033[1m')
	CREV=$(printf '\033[7m')
	CRED=$(printf '\033[31m')
	CYEL=$(printf '\033[33m')
	CGRN=$(printf '\033[32m')
	CDEF=$(printf '\033[0m')
else
	CBLD=""
	CREV=""
	CRED=""
	CYEL=""
	CGRN=""
	CDEF=""
fi
if [ -n "${CI}" ] && [ "${CI}" = "true" ]; then
	GHAGRP_START="::group::"
	GHAGRP_END="::endgroup::"
else
	GHAGRP_START=""
	GHAGRP_END=""
fi

PRNGROUPEND()
{
	if [ -n "${IN_GHAGROUP_AREA}" ] && [ "${IN_GHAGROUP_AREA}" -eq 1 ]; then
		if [ -n "${GHAGRP_END}" ]; then
			echo "${GHAGRP_END}"
		fi
	fi
	IN_GHAGROUP_AREA=0
}
PRNTITLE()
{
	PRNGROUPEND
	echo "${GHAGRP_START}${CBLD}${CGRN}${CREV}[TITLE]${CDEF} ${CGRN}$*${CDEF}"
	IN_GHAGROUP_AREA=1
}
PRNINFO()
{
	echo "${CBLD}${CREV}[INFO]${CDEF} $*"
}
PRNWARN()
{
	echo "${CBLD}${CYEL}${CREV}[WARNING]${CDEF} ${CYEL}$*${CDEF}"
}
PRNERR()
{
	echo "${CBLD}${CRED}${CREV}[ERROR]${CDEF} ${CRED}$*${CDEF}"
	PRNGROUPEND
}
PRNSUCCESS()
{
	echo "${CBLD}${CGRN}${CREV}[SUCCEED]${CDEF} ${CGRN}$*${CDEF}"
	PRNGROUPEND
}
PRNFAILURE()
{
	echo "${CBLD}${CRED}${CREV}[FAILURE]${CDEF} ${CRED}$*${CDEF}"
	PRNGROUPEND
}
RUNCMD()
{
	PRNINFO "Run \"$*\""
	if ! /bin/sh -c "$*"; then
		PRNERR "Failed to run \"$*\""
		return 1
	fi
	return 0
}

#
# Print Usage
#
func_usage()
{
	echo ""
	echo "Usage: $1 [options...]"
	echo ""
	echo "  Option:"
	echo "    --help(-h)                                print help"
	echo "    --imagetype-vars-file(-f) <file path>     specify the file path to imagetype variable(deafult. \"imagetypevars.sh\")"
	echo "    --imageinfo(-i)           <image info>    specify infomration about "base docker image", "base docker dev image", "os type tag" and "default flag" (ex. \"alpine:latest,alpine:latest,alpine,default\")"
	echo "    --organization(-o)        <organization>  specify organaization name on DockerHub(default. \"antpickax\")"
	echo "    --imagenames(-n)          <image name>    specify build image names, separate multiple names with commas(ex. \"target,target-dev\")"
	echo "    --imageversion(-v)        <version>       the value of this option is set automatically and is usually not specified.(ex. \"1.0.0\")"
	echo "    --giturl(-g)              <url>           specify git domain url. if not specify, using GITHUB_SERVER_URL environment value. if both are not specified, using \"https://github.com\""
	echo "    --push(-p)                                specify this when force pushing the image to Docker Hub, normally the images is pushed only when it is tagged(determined from GITHUB_REF/GITHUB_EVENT_NAME)"
	echo "    --notpush(-np)                            specify this when force never pushing the image to Docker Hub."
	echo ""
	echo "  Environments:"
	echo "    ENV_IMAGEVAR_FILE         the file path to imagetype variable           ( same as option '--imagetype-vars-file(-f)' )"
	echo "    ENV_DOCKER_IMAGE_INFO     image infomration                             ( same as option '--imageinfo(-i)' )"
	echo "    ENV_DOCKER_HUB_ORG        organaization name on DockerHub               ( same as option '--organization(-o)' )"
	echo "    ENV_IMAGE_NAMES           build image names                             ( same as option '--imagenames(-n)' )"
	echo "    ENV_IMAGE_VERSION         the value of this option is set automatically ( same as option '--imageversion(-v)' )"
	echo "    ENV_GIT_URL               git domain url.                               ( same as option '--giturl(-g)' or environment 'GITHUB_SERVER_URL' )"
	echo "    ENV_FORCE_PUSH            force the release package to push: true/false ( same as option '--push(-p)' and '--notpush(-np)' )"
	echo ""
	echo "    This program uses folowing environment variable internally."
	echo "      GITHUB_SERVER_URL"
	echo "      GITHUB_REPOSITORY"
	echo "      GITHUB_REF"
	echo "      GITHUB_EVENT_NAME"
	echo "      GITHUB_EVENT_PATH"
	echo ""
	echo "  Node: \"--imageinfo(-i)\" option"
	echo "    Specify the \"baseimage\" in the following format: \"<base image tag>,<base dev image tag>,<OS tag name>(,<default tag flag>)\""
	echo "      <base image tag>:     specify the Docker image name(ex. \"alpine:latest\")"
	echo "      <base dev image tag>: specify the Docker image name(ex. \"alpine:latest\")"
	echo "      <OS tag name>:        OS tag attached to the created Docker image"
	echo "      <default tag flag>:   If you want to use the created Docker image as the default image, specify \"default\"."
	echo ""
	echo "  Note:"
	echo "    Specifying the above options will create the image shown in the example below:"
	echo "      antpickax/image:1.0.0-alpine  (imagetag is \"alpine\")"
	echo "      antpickax/image:1.0.0         (imagetag is not specified)"
	echo ""
}

#==============================================================
# Customizable execution function
#==============================================================
#
# Get version from repository package
#
# [NOTE]
# Set "REPOSITORY_PACKAGE_VERSION" environment
#
get_repository_package_version()
{
	REPOSITORY_PACKAGE_VERSION=""
	return 0
}

#
# Print custom variables
#
print_custom_variabels()
{
	return 0
}

#
# Preprocessing
#
run_preprocess()
{
	return 0
}

#
# Custom Dockerfile conversion
#
# $1	: Dockerfile path
#
custom_dockerfile_conversion()
{
	if [ -z "$1" ] || [ ! -f "$1" ]; then
		PRNERR "Dockerfile path($1) is empty or not existed."
		return 1
	fi
	return 0
}

#==========================================================
# Parse options and check environments
#==========================================================
PRNTITLE "Parse options and check environments"

OPT_IMAGEVAR_FILE=""
OPT_DOCKER_IMAGE_INFO=""
OPT_DOCKER_HUB_ORG=""
OPT_IMAGE_NAMES=""
OPT_IMAGE_VERSION=""
OPT_GIT_URL=""
OPT_FORCE_PUSH=""

while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		break

	elif [ "$1" = "-h" ] || [ "$1" = "-H" ] || [ "$1" = "--help" ] || [ "$1" = "--HELP" ]; then
		func_usage "${PRGNAME}"
		exit 0

	elif [ "$1" = "-f" ] || [ "$1" = "-F" ] || [ "$1" = "--imagetype-vars-file" ] || [ "$1" = "--IMAGETYPE-VARS-FILE" ]; then
		if [ -n "${OPT_IMAGEVAR_FILE}" ]; then
			PRNERR "already set \"--imagetype-vars-file(-f)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--imagetype-vars-file(-f)\" option is specified without parameter."
			exit 1
		fi
		if [ ! -f "$1" ]; then
			if [ ! -f "${SCRIPTDIR}/$1" ]; then
				PRNERR "Could not file : $1."
				exit 1
			fi
			OPT_IMAGEVAR_FILE="${SCRIPTDIR}/$1"
		else
			OPT_IMAGEVAR_FILE="$1"
		fi

	elif [ "$1" = "-i" ] || [ "$1" = "-I" ] || [ "$1" = "--imageinfo" ] || [ "$1" = "--IMAGEINFO" ]; then
		if [ -n "${OPT_DOCKER_IMAGE_INFO}" ]; then
			PRNERR "already set \"--imageinfo(-i)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--imageinfo(-i)\" option is specified without parameter."
			exit 1
		fi
		OPT_DOCKER_IMAGE_INFO="$1"

	elif [ "$1" = "-o" ] || [ "$1" = "-O" ] || [ "$1" = "--organization" ] || [ "$1" = "--ORGANIZATION" ]; then
		if [ -n "${OPT_DOCKER_HUB_ORG}" ]; then
			PRNERR "already set \"--organization(-o)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--organization(-o)\" option is specified without parameter."
			exit 1
		fi
		OPT_DOCKER_HUB_ORG="$1"

	elif [ "$1" = "-n" ] || [ "$1" = "-N" ] || [ "$1" = "--imagenames" ] || [ "$1" = "--IMAGENAMES" ]; then
		if [ -n "${OPT_IMAGE_NAMES}" ]; then
			PRNERR "already set \"--imagenames(-n)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--imagenames(-n)\" option is specified without parameter."
			exit 1
		fi
		OPT_IMAGE_NAMES="$1"

	elif [ "$1" = "-v" ] || [ "$1" = "-V" ] || [ "$1" = "--imageversion" ] || [ "$1" = "--IMAGEVERSION" ]; then
		if [ -n "${OPT_IMAGE_VERSION}" ]; then
			PRNERR "already set \"--imageversion(-v)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--imageversion(-v)\" option is specified without parameter."
			exit 1
		fi
		OPT_IMAGE_VERSION="$1"

	elif [ "$1" = "-g" ] || [ "$1" = "-G" ] || [ "$1" = "--giturl" ] || [ "$1" = "--GITURL" ]; then
		if [ -n "${OPT_GIT_URL}" ]; then
			PRNERR "already set \"--giturl(-g)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--giturl(-g)\" option is specified without parameter."
			exit 1
		fi
		OPT_GIT_URL="$1"

	elif [ "$1" = "-p" ] || [ "$1" = "-P" ] || [ "$1" = "--push" ] || [ "$1" = "--PUSH" ]; then
		if [ -n "${OPT_FORCE_PUSH}" ]; then
			PRNERR "already set \"--push(-p)\" or \"--notpush(-np)\" option."
			exit 1
		fi
		OPT_FORCE_PUSH="true"

	elif [ "$1" = "-np" ] || [ "$1" = "-NP" ] || [ "$1" = "--notpush" ] || [ "$1" = "--NOTPUSH" ]; then
		if [ -n "${OPT_FORCE_PUSH}" ]; then
			PRNERR "already set \"--push(-p)\" or \"--notpush(-np)\" option."
			exit 1
		fi
		OPT_FORCE_PUSH="false"

	else
		PRNERR "Unknown \"$1\" option."
		exit 1
	fi
	shift
done

#----------------------------------------------------------
# Check required options
#----------------------------------------------------------
if [ -z "${OPT_IMAGE_NAMES}" ]; then
	if [ -z "${ENV_IMAGE_NAMES}" ]; then
		PRNERR "The \"--imagenames(-n)\" option or \"ENV_IMAGE_NAMES\" environment is required."
		exit 1
	fi
	CI_IMAGE_NAMES="${ENV_IMAGE_NAMES}"
else
	CI_IMAGE_NAMES="${OPT_IMAGE_NAMES}"
fi

if [ -z "${OPT_DOCKER_IMAGE_INFO}" ]; then
	if [ -z "${ENV_DOCKER_IMAGE_INFO}" ]; then
		PRNERR "The \"--imageinfo(-i)\" option or \"ENV_DOCKER_IMAGE_INFO\" environment is required."
		exit 1
	fi
	CI_DOCKER_IMAGE_INFO="${ENV_DOCKER_IMAGE_INFO}"
else
	CI_DOCKER_IMAGE_INFO="${OPT_DOCKER_IMAGE_INFO}"
fi

#----------------------------------------------------------
# Variables from image inforamtion
#----------------------------------------------------------
#
# Parse image information
#
CI_DOCKER_IMAGE_INFO_TMP="$(echo    "${CI_DOCKER_IMAGE_INFO}"     | sed -e 's#,# #g' | tr -d '\n')"
CI_DOCKER_IMAGE_BASE="$(echo        "${CI_DOCKER_IMAGE_INFO_TMP}" | awk '{print $1}' | tr -d '\n')"
CI_DOCKER_IMAGE_DEV_BASE="$(echo    "${CI_DOCKER_IMAGE_INFO_TMP}" | awk '{print $2}' | tr -d '\n')"
CI_DOCKER_IMAGE_OSTYPE="$(echo      "${CI_DOCKER_IMAGE_INFO_TMP}" | awk '{print $3}' | tr -d '\n')"
CI_DOCKER_IMAGE_DEFAULT_TAG="$(echo "${CI_DOCKER_IMAGE_INFO_TMP}" | awk '{print $4}' | tr -d '\n')"

#
# Check CI_DOCKER_IMAGE_{DEV_}_BASE
#
if [ -z "${CI_DOCKER_IMAGE_BASE}" ] || [ -z "${CI_DOCKER_IMAGE_DEV_BASE}" ]; then
	PRNERR "The \"--imageinfo(-i)\" option value does not have base image name."
	exit 1
fi

#
# Check CI_DOCKER_IMAGE_OSTYPE{_TAG}
#
if [ -z "${CI_DOCKER_IMAGE_OSTYPE}" ]; then
	#
	# Instead of OS type from base image name
	#
	CI_DOCKER_IMAGE_OSTYPE="$(echo "${CI_DOCKER_IMAGE_BASE}" | sed -e 's#^.*/##g' -e 's#:.*$##g' | tr -d '\n')"
	if [ -z "${CI_DOCKER_IMAGE_OSTYPE}" ]; then
		PRNERR "The \"--imageinfo(-i)\" option value does not have image os type."
		exit 1
	fi
	PRNWARN "The \"--imageinfo(-i)\" option value does not have image os type, but get it from base image name."
fi
CI_DOCKER_IMAGE_OSTYPE_TAG="-${CI_DOCKER_IMAGE_OSTYPE}"

#
# Check CI_DEFAULT_IMAGE_TAGGING
#
if [ -n "${CI_DOCKER_IMAGE_DEFAULT_TAG}" ] && [ "${CI_DOCKER_IMAGE_DEFAULT_TAG}" = "default" ]; then
	CI_DEFAULT_IMAGE_TAGGING=1
else
	CI_DEFAULT_IMAGE_TAGGING=0
fi

PRNSUCCESS "Parsed options and checked environments"

#==========================================================
# Load variables from custom file
#==========================================================
PRNTITLE "Load variables from custom file"

#
# The file for customization
#
if [ -n "${OPT_IMAGEVAR_FILE}" ]; then
	CI_IMAGEVAR_FILE="${OPT_IMAGEVAR_FILE}"
elif [ -n "${ENV_IMAGEVAR_FILE}" ]; then
	if [ ! -f "${ENV_IMAGEVAR_FILE}" ]; then
		PRNERR "The \"ENV_IMAGEVAR_FILE\" environment value(${ENV_IMAGEVAR_FILE}) is not existed file path."
		exit 1
	fi
	CI_IMAGEVAR_FILE="${ENV_IMAGEVAR_FILE}"
elif [ -f "${SCRIPTDIR}/imagetypevars.sh" ]; then
	CI_IMAGEVAR_FILE="${SCRIPTDIR}/imagetypevars.sh"
else
	CI_IMAGEVAR_FILE=""
fi

#
# Load variables from custom file
#
if [ -n "${CI_IMAGEVAR_FILE}" ] && [ -f "${CI_IMAGEVAR_FILE}" ]; then
	PRNINFO "Load ${CI_IMAGEVAR_FILE} for local variables."
	. "${CI_IMAGEVAR_FILE}"
fi

PRNSUCCESS "Loaded variables from custom file"

#==========================================================
# Check options for default value
#==========================================================
PRNTITLE "Check options for default value"

#
# Information about github url
#
if [ -n "${OPT_GIT_URL}" ]; then
	CI_GIT_URL="${OPT_GIT_URL}"
elif [ -n "${ENV_GIT_URL}" ]; then
	CI_GIT_URL="${ENV_GIT_URL}"
elif [ -n "${GITHUB_SERVER_URL}" ]; then
	CI_GIT_URL="${GITHUB_SERVER_URL}"
else
	CI_GIT_URL="https://github.com"
fi

#
# Infomration about Docker hub
#
if [ -n "${OPT_DOCKER_HUB_ORG}" ]; then
	CI_DOCKER_HUB_ORG="${OPT_DOCKER_HUB_ORG}"
elif [ -n "${ENV_DOCKER_HUB_ORG}" ]; then
	CI_DOCKER_HUB_ORG="${ENV_DOCKER_HUB_ORG}"
else
	CI_DOCKER_HUB_ORG="antpickax"
fi

#
# Set "REPOSITORY_PACKAGE_VERSION" variable
#
if ! get_repository_package_version; then
	PRNERR "Failed to run get_repository_package_version function."
	exit 1
fi

#
# GITHUB_REF Environments
#
if [ -n "${GITHUB_REF}" ] && echo "${GITHUB_REF}" | grep -q 'refs/tags/'; then
	TAGGED_VERSION="$(echo "${GITHUB_REF}" | sed -e 's#refs/tags/v##g' -e 's#refs/tags/##g' | tr -d '\n')"
else
	TAGGED_VERSION=""
fi

#
# image version
#
if [ -n "${OPT_IMAGE_VERSION}" ]; then
	CI_IMAGE_VERSION="${OPT_IMAGE_VERSION}"
elif [ -n "${ENV_IMAGE_VERSION}" ]; then
	CI_IMAGE_VERSION="${ENV_IMAGE_VERSION}"
else
	#
	# Default image version from github tag or package.json
	#
	if [ -n "${TAGGED_VERSION}" ]; then
		CI_IMAGE_VERSION=${TAGGED_VERSION}
	else
		if [ -n "${REPOSITORY_PACKAGE_VERSION}" ]; then
			CI_IMAGE_VERSION="${REPOSITORY_PACKAGE_VERSION}"
		else
			#
			# Not found image version
			#
			PRNWARN "Not found image version thus use default image version(0.0.0)."
			CI_IMAGE_VERSION="0.0.0"
		fi
	fi
fi

if [ -n "${OPT_FORCE_PUSH}" ]; then
	CI_FORCE_PUSH="${OPT_FORCE_PUSH}"
elif [ -n "${ENV_FORCE_PUSH}" ]; then
	if echo "${ENV_FORCE_PUSH}" | grep -q -i '^true$'; then
		CI_FORCE_PUSH="true"
	elif echo "${ENV_FORCE_PUSH}" | grep -q -i '^false$'; then
		CI_FORCE_PUSH="false"
	else
		PRNERR "\"ENV_FORCE_PUSH\" value is wrong."
		exit 1
	fi
else
	CI_FORCE_PUSH=""
fi

#----------------------------------------------------------
# Base Repository/SHA1 for creating Docker images
#----------------------------------------------------------
if [ -n "${GITHUB_EVENT_NAME}" ] && [ "${GITHUB_EVENT_NAME}" = "pull_request" ]; then
	# [NOTE]
	# In the PR case, GITHUB_REPOSITORY and GITHUB_REF cannot be used
	# because they are the information on the merging side.
	# Then the Organization/Repository/branch(or SHA1) of the PR source
	# is fetched from the file(json) indicated by the GITHUB_EVENT_PATH
	# environment variable.
	#	ex.
	#		{
	#		  "pull_request": {
	#		    "head": {
	#		      "repo": {
	#		        "full_name": "org/repo",
	#		      },
	#		      "sha": "776........",
	#		    },
	#		  },
	#		}
	#
	if [ ! -f "${GITHUB_EVENT_PATH}" ]; then
		PRNERR "\"GITHUB_EVENT_PATH\" environment is empty or not file path."
		exit 1
	fi

	# [NOTE]
	# we use "jq" for parsing json, it is installed on default github actions runner.
	#
	PR_GITHUB_REPOSITORY="$(jq -r '.pull_request.head.repo.full_name' "${GITHUB_EVENT_PATH}" | tr -d '\n')"
	CI_DOCKER_GIT_ORGANIZATION="$(echo "${PR_GITHUB_REPOSITORY}" | sed 's#/# #g' | awk '{print $1}' | tr -d '\n')"
	CI_DOCKER_GIT_REPOSITORY="$(echo "${PR_GITHUB_REPOSITORY}" | sed 's#/# #g' | awk '{print $2}' | tr -d '\n')"
	CI_DOCKER_GIT_BRANCH="$(jq -r '.pull_request.head.sha' "${GITHUB_EVENT_PATH}" | tr -d '\n')"
else
	CI_DOCKER_GIT_ORGANIZATION="$(echo "${GITHUB_REPOSITORY}" | sed 's#/# #g' | awk '{print $1}' | tr -d '\n')"
	CI_DOCKER_GIT_REPOSITORY="$(echo "${GITHUB_REPOSITORY}" | sed 's#/# #g' | awk '{print $2}' | tr -d '\n')"
	CI_DOCKER_GIT_BRANCH="$(echo "${GITHUB_REF}" | sed 's#^refs/.*/##g' | tr -d '\n')"
fi
if [ "${CI_DOCKER_GIT_ORGANIZATION}" = "" ] || [ "${CI_DOCKER_GIT_REPOSITORY}" = "" ] || [ "${CI_DOCKER_GIT_BRANCH}" = "" ]; then
	PRNERR "Not found Organization/Repository/branch(or SHA1)."
	exit 1
fi

#----------------------------------------------------------
# Push mode
#----------------------------------------------------------
if [ -n "${CI_FORCE_PUSH}" ] && [ "${CI_FORCE_PUSH}" = "true" ]; then
	#
	# FORCE PUSH
	#
	if [ -n "${GITHUB_EVENT_NAME}" ] && [ "${GITHUB_EVENT_NAME}" = "schedule" ]; then
		PRNWARN "specified \"--push(-p)\" option or \"ENV_FORCE_PUSH=true\" environment, but not push images because this process is kicked by scheduler."
		CI_DO_PUSH=0
	else
		CI_DO_PUSH=1
	fi
elif [ -n "${CI_FORCE_PUSH}" ] && [ "${CI_FORCE_PUSH}" = "false" ]; then
	#
	# FORCE NOT PUSH
	#
	CI_DO_PUSH=0
else
	if [ -n "${GITHUB_EVENT_NAME}" ] && [ "${GITHUB_EVENT_NAME}" = "schedule" ]; then
		CI_DO_PUSH=0
	else
		if [ -z "${TAGGED_VERSION}" ]; then
			CI_DO_PUSH=0
		else
			CI_DO_PUSH=1
		fi
	fi
fi

PRNSUCCESS "Check options for default value"

#==========================================================
# Print information
#==========================================================
PRNTITLE "Print all local variables"

echo "  PRGNAME                     = ${PRGNAME}"
echo "  SCRIPTDIR                   = ${SCRIPTDIR}"
echo "  SRCTOP                      = ${SRCTOP}"
echo ""
echo "  DOCKER_TEMPL_FILE           = ${DOCKER_TEMPL_FILE}"
echo "  DOCKER_FILE                 = ${DOCKER_FILE}"
echo ""
echo "  REPOSITORY_PACKAGE_VERSION  = ${REPOSITORY_PACKAGE_VERSION}"
echo ""
echo "  CI_IMAGEVAR_FILE            = ${CI_IMAGEVAR_FILE}"
echo "  CI_DOCKER_IMAGE_INFO        = ${CI_DOCKER_IMAGE_INFO}"
echo "  CI_DOCKER_HUB_ORG           = ${CI_DOCKER_HUB_ORG}"
echo "  CI_IMAGE_NAMES              = ${CI_IMAGE_NAMES}"
echo "  CI_IMAGE_VERSION            = ${CI_IMAGE_VERSION}"
echo "  CI_DOCKER_IMAGE_BASE        = ${CI_DOCKER_IMAGE_BASE}"
echo "  CI_DOCKER_IMAGE_DEV_BASE    = ${CI_DOCKER_IMAGE_DEV_BASE}"
echo "  CI_DOCKER_IMAGE_OSTYPE      = ${CI_DOCKER_IMAGE_OSTYPE}"
echo "  CI_GIT_URL                  = ${CI_GIT_URL}"
echo "  CI_DOCKER_GIT_ORGANIZATION  = ${CI_DOCKER_GIT_ORGANIZATION}"
echo "  CI_DOCKER_GIT_REPOSITORY    = ${CI_DOCKER_GIT_REPOSITORY}"
echo "  CI_DOCKER_GIT_BRANCH        = ${CI_DOCKER_GIT_BRANCH}"
echo "  CI_DEFAULT_IMAGE_TAGGING    = ${CI_DEFAULT_IMAGE_TAGGING}"
echo "  CI_FORCE_PUSH               = ${CI_FORCE_PUSH}"
echo "  CI_DO_PUSH                  = ${CI_DO_PUSH}"
echo ""
echo "  DOCKERFILE_TEMPL_SUBDIR     = ${DOCKERFILE_TEMPL_SUBDIR}"
echo "  PKGMGR_NAME                 = ${PKGMGR_NAME}"
echo "  PKGMGR_UPDATE_OPT           = ${PKGMGR_UPDATE_OPT}"
echo "  PKGMGR_INSTALL_OPT          = ${PKGMGR_INSTALL_OPT}"
echo "  PKG_INSTALL_LIST_BUILDER    = ${PKG_INSTALL_LIST_BUILDER}"
echo "  PKG_INSTALL_LIST_BIN        = ${PKG_INSTALL_LIST_BIN}"
echo "  BUILDER_ENVIRONMENT         = ${BUILDER_ENVIRONMENT}"
echo "  UPDATE_LIBPATH              = ${UPDATE_LIBPATH}"
echo "  RUNNER_INSTALL_PACKAGES     = ${RUNNER_INSTALL_PACKAGES}"

#
# Printing additional custom variables
#
print_custom_variabels

PRNSUCCESS "Printed all local variables"

#==========================================================
# Initialize Runner for creating Dockerfile
#==========================================================
# [NOTE]
# Github Actions Runner uses Ubuntu to create Docker images.
# Therefore, the below code is written here assuming that
# Ubuntu is used.
#
PRNTITLE "Initialize Runner for creating Dockerfile"

#
# Update pacakges
#
PRNINFO "Update local packages and caches"

export DEBIAN_FRONTEND="noninteractive"
if ({ RUNCMD sudo apt-get update -y -q || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	PRNERR "Failed to update packages"
	exit 1
fi

#
# Check and install curl
#
if ! CURLCMD=$(command -v curl); then
	PRNINFO "Install curl command"
	if ({ RUNCMD sudo apt-get install -y -q curl || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNERR "Failed to install curl command"
		exit 1
	fi
	if ! CURLCMD=$(command -v curl); then
		PRNERR "Not found curl command"
		exit 1
	fi
else
	PRNINFO "Already curl is insatlled."
fi

#
# Setup packagecloud.io repository
#
PRNINFO "Setup packagecloud.io repository."

if ({ RUNCMD "${CURLCMD} -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh | sudo bash" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	PRNERR "Failed to download script or setup packagecloud.io reposiory"
	exit 1
fi

#
# Install packages
#
PRNINFO "Install packages for Github Actions Runner."
if [ -n "${RUNNER_INSTALL_PACKAGES}" ]; then
	if ({ RUNCMD "sudo apt-get install -y -q ${RUNNER_INSTALL_PACKAGES}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNERR "Failed to install packages"
		exit 1
	fi
fi

PRNSUCCESS "Initialized Runner for creating Dockerfile"

#==========================================================
# Create Dockerfile from template
#==========================================================
PRNTITLE "Create Dockerfile from template"

cd "${SRCTOP}" || exit 1

#
# Preprocessing
#
if ! run_preprocess; then
	PRNERR "Failed to run run_preprocess function."
	exit 1
fi

#
# Create each Dockerfile
#
PRNINFO "Create Dockerfile from ${DOCKER_TEMPL_FILE}"

if [ -n "${PKG_INSTALL_LIST_BUILDER}" ]; then
	PKG_INSTALL_BUILDER_COMMAND="${PKGMGR_NAME} ${PKGMGR_INSTALL_OPT} ${PKG_INSTALL_LIST_BUILDER}"
else
	#
	# Set no-operation command
	#
	PKG_INSTALL_BUILDER_COMMAND=":"
fi

if [ -n "${PKG_INSTALL_LIST_BIN}" ]; then
	PKG_INSTALL_BIN_COMMAND="${PKGMGR_NAME} ${PKGMGR_INSTALL_OPT} ${PKG_INSTALL_LIST_BIN}"
else
	#
	# Set no-operation command
	#
	PKG_INSTALL_BIN_COMMAND=":"
fi

if [ -z "${UPDATE_LIBPATH}" ]; then
	#
	# Set no-operation command
	#
	UPDATE_LIBPATH=":"
fi

#
# Create dockerfile from template(Common conversion)
#
if ! sed -e "s#%%GIT_DOMAIN_URL%%#${CI_GIT_URL}#g"							\
		-e "s#%%DOCKER_IMAGE_BASE%%#${CI_DOCKER_IMAGE_BASE}#g"				\
		-e "s#%%DOCKER_IMAGE_DEV_BASE%%#${CI_DOCKER_IMAGE_DEV_BASE}#g"		\
		-e "s#%%DOCKER_GIT_ORGANIZATION%%#${CI_DOCKER_GIT_ORGANIZATION}#g"	\
		-e "s#%%DOCKER_GIT_REPOSITORY%%#${CI_DOCKER_GIT_REPOSITORY}#g"		\
		-e "s#%%DOCKER_GIT_BRANCH%%#${CI_DOCKER_GIT_BRANCH}#g"				\
		-e "s#%%PKG_UPDATE%%#${PKGMGR_NAME} ${PKGMGR_UPDATE_OPT}#g"			\
		-e "s#%%PKG_INSTALL_BUILDER%%#${PKG_INSTALL_BUILDER_COMMAND}#g"		\
		-e "s#%%PKG_INSTALL_BIN%%#${PKG_INSTALL_BIN_COMMAND}#g"				\
		-e "s#%%UPDATE_LIBPATH%%#${UPDATE_LIBPATH}#g"						\
		-e "s#%%BUILD_ENV%%#${BUILDER_ENVIRONMENT}#g"						\
		"${SRCTOP}/${DOCKERFILE_TEMPL_SUBDIR}/${DOCKER_TEMPL_FILE}"			> "${SRCTOP}/${DOCKER_FILE}"; then

	PRNERR "Failed to creating ${DOCKER_FILE} from ${DOCKER_TEMPL_FILE} (Common conversion)."
	exit 1
fi

#
# Custom dockerfile conversion
#
if ! custom_dockerfile_conversion "${SRCTOP}/${DOCKER_FILE}"; then
	PRNERR "Failed to custom dockerfile(${SRCTOP}/${DOCKER_FILE}) conversion."
	exit 1
fi

PRNINFO "Dockerfile : ${SRCTOP}/${DOCKER_FILE}"
echo ""
sed -e 's/^/    /g' "${SRCTOP}/${DOCKER_FILE}"
echo ""

PRNSUCCESS "Create Dockerfile from template"

#==========================================================
# Build docker images
#==========================================================
PRNTITLE "Build Docker Images"

CI_IMAGE_NAMES="$(echo "${CI_IMAGE_NAMES}" | sed -e 's/,/ /g' | tr '\n' ' ')"

for ONE_IMAGE_NAME in ${CI_IMAGE_NAMES}; do
	PRNINFO "Build docker image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}, ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG} from ${ONE_IMAGE_NAME}"

	if [ "${CI_DEFAULT_IMAGE_TAGGING}" -eq 1 ]; then
		if ({ RUNCMD docker image build -f "${SRCTOP}/${DOCKER_FILE}" --target "${ONE_IMAGE_NAME}" -t "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}" -t "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG}" -t "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}" -t "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest" . || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to build image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}, ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG} from ${ONE_IMAGE_NAME}"
			exit 1
		fi
	else
		if ({ RUNCMD docker image build -f "${SRCTOP}/${DOCKER_FILE}" --target "${ONE_IMAGE_NAME}" -t "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}" -t "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG}" . || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to build image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}, ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG} from ${ONE_IMAGE_NAME}"
			exit 1
		fi
	fi
	PRNINFO "Success to build image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}, ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG}"
	echo ""
done

PRNSUCCESS "Built Docker Images"

#==========================================================
# Push Docker Images
#==========================================================
PRNTITLE "Push Docker Images"

if [ "${CI_DO_PUSH}" -eq 1 ]; then

	for ONE_IMAGE_NAME in ${CI_IMAGE_NAMES}; do
		#
		# Push images
		#
		PRNINFO "Push docker image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}"
		if ({ RUNCMD docker push "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to push image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}"
			exit 1
		fi

		PRNINFO "Push docker image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG}"
		if ({ RUNCMD docker push "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to push image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG}"
			exit 1
		fi

		#
		# Push image as default tag
		#
		if [ "${CI_DEFAULT_IMAGE_TAGGING}" -eq 1 ]; then
			PRNINFO "Push docker image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}"
			if ({ RUNCMD docker push "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to push image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}"
				exit 1
			fi

			PRNINFO "Push docker image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest"
			if ({ RUNCMD docker push "${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to push image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest"
				exit 1
			fi
			PRNINFO "Success to build image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}, ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG}, ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}, ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest"
		else
			PRNINFO "Success to build image : ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${CI_IMAGE_VERSION}${CI_DOCKER_IMAGE_OSTYPE_TAG}, ${CI_DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${CI_DOCKER_IMAGE_OSTYPE_TAG}"
		fi
		echo ""
	done

	PRNSUCCESS "Pushed docker images"
else
	PRNSUCCESS "Do not push docker images"
fi

#---------------------------------------------------------------------
# Finish
#---------------------------------------------------------------------
exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
