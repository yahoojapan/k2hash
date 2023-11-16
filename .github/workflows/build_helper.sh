#!/bin/sh
#
# Utility helper tools for Github Actions by AntPickax
#
# Copyright 2020 Yahoo Japan Corporation.
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
# CREATE:   Mon, Oct 31 2022
# REVISION:	1.3
#

#==============================================================
# Build helper for C/C++ etc on Github Actions
#==============================================================
#
# Instead of pipefail(for shells not support "set -o pipefail")
#
PIPEFAILURE_FILE="/tmp/.pipefailure.$(od -An -tu4 -N4 /dev/random | tr -d ' \n')"

#
# For shellcheck
#
if command -v locale >/dev/null 2>&1; then
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
fi

#==============================================================
# Common variables
#==============================================================
PRGNAME=$(basename "$0")
SCRIPTDIR=$(dirname "$0")
SCRIPTDIR=$(cd "${SCRIPTDIR}" || exit 1; pwd)
SRCTOP=$(cd "${SCRIPTDIR}"/../.. || exit 1; pwd)

#
# Message variables
#
IN_GHAGROUP_AREA=0

#
# Variables with default values
#
CI_OSTYPE=""

CI_OSTYPE_VARS_FILE="${SCRIPTDIR}/ostypevars.sh"
CI_BUILD_NUMBER=1
CI_DEVELOPER_FULLNAME=""
CI_DEVELOPER_EMAIL=""
CI_FORCE_PUBLISH=""
CI_USE_PACKAGECLOUD_REPO=1
CI_PACKAGECLOUD_TOKEN=""
CI_PACKAGECLOUD_OWNER="antpickax"
CI_PACKAGECLOUD_PUBLISH_REPO="stable"
CI_PACKAGECLOUD_DOWNLOAD_REPO="stable"

CI_IN_SCHEDULE_PROCESS=0
CI_PUBLISH_TAG_NAME=""
CI_DO_PUBLISH=0

#==============================================================
# Utility functions and variables for messaging
#==============================================================
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

#----------------------------------------------------------
# Helper for container on Github Actions
#----------------------------------------------------------
func_usage()
{
	echo ""
	echo "Usage: $1 [options...]"
	echo ""
	echo "  Required option:"
	echo "    --help(-h)                                             print help"
	echo "    --ostype(-os)                             <os:version> specify os and version as like \"ubuntu:jammy\""
	echo ""
	echo "  Option:"
	echo "    --ostype-vars-file(-f)                    <file path>  specify the file that describes the package list to be installed before build(default is ostypevars.sh)"
	echo "    --force-publish(-p)                                    force the release package to be uploaded. normally the package is uploaded only when it is tagged(determined from GITHUB_REF/GITHUB_EVENT_NAME)."
	echo "    --not-publish(-np)                                     do not force publish the release package."
	echo "    --build-number(-num)                      <number>     build number for packaging(default 1)"
	echo "    --developer-fullname(-devname)            <string>     specify developer name for debian and ubuntu packaging(default is null, it is specified in configure.ac)"
	echo "    --developer-email(-devmail)               <string>     specify developer e-mail for debian and ubuntu packaging(default is null, it is specified in configure.ac)"
	echo ""
	echo "  Option for packagecloud.io:"
	echo "    --use-packagecloudio-repo(-usepc)                      use packagecloud.io repository(default), exclusive -notpc option"
	echo "    --not-use-packagecloudio-repo(-notpc)                  not use packagecloud.io repository, exclusive -usepc option"
	echo "    --packagecloudio-token(-pctoken)          <token>      packagecloud.io token for uploading(specify when uploading)"
	echo "    --packagecloudio-owner(-pcowner)          <owner>      owner name of uploading destination to packagecloud.io, this is part of the repository path(default is antpickax)"
	echo "    --packagecloudio-publish-repo(-pcprepo)   <repository> repository name of uploading destination to packagecloud.io, this is part of the repository path(default is current)"
	echo "    --packagecloudio-download-repo(-pcdlrepo) <repository> repository name of installing packages in packagecloud.io, this is part of the repository path(default is stable)"
	echo ""
	echo "  Environments:"
	echo "    ENV_OSTYPE_VARS_FILE                      the file that describes the package list                  ( same as option '--ostype-vars-file(-f)' )"
	echo "    ENV_BUILD_NUMBER                          build number                                              ( same as option '--build-number(-num)' )"
	echo "    ENV_DEVELOPER_FULLNAME                    developer name                                            ( same as option '--developer-fullname(-devname)' )"
	echo "    ENV_DEVELOPER_EMAIL                       developer e-mail                                          ( same as option '--developer-email(-devmail)' )"
	echo "    ENV_FORCE_PUBLISH                         force the release package to be uploaded: true/false      ( same as option '--force-publish(-p)' and '--not-publish(-np)' )"
	echo "    ENV_USE_PACKAGECLOUD_REPO                 use packagecloud.io repository: true/false                ( same as option '--use-packagecloudio-repo(-usepc)' and '--not-use-packagecloudio-repo(-notpc)' )"
	echo "    ENV_PACKAGECLOUD_TOKEN                    packagecloud.io token                                     ( same as option '--packagecloudio-token(-pctoken)' )"
	echo "    ENV_PACKAGECLOUD_OWNER                    owner name for uploading to packagecloud.io               ( same as option '--packagecloudio-owner(-pcowner)' )"
	echo "    ENV_PACKAGECLOUD_PUBLISH_REPO             repository name of uploading to packagecloud.io           ( same as option '--packagecloudio-publish-repo(-pcprepo)' )"
	echo "    ENV_PACKAGECLOUD_DOWNLOAD_REPO            repository name of installing packages in packagecloud.io ( same as option '--packagecloudio-download-repo(-pcdlrepo)' )"
	echo "    GITHUB_REF                                use internally for release tag"
	echo "    GITHUB_EVENT_NAME                         use internally for checking schedule processing"
	echo ""
	echo "  Note:"
	echo "    Environment variables and options have the same parameter items."
	echo "    If both are specified, the option takes precedence."
	echo "    Environment variables are set from Github Actions Secrets, etc."
	echo "    GITHUB_REF and GITHUB_EVENT_NAME environments are used internally."
	echo ""
}

#==============================================================
# Default execution functions and variables
#==============================================================
#
# Execution flag ( default is for C/C++ )
#
RUN_PRE_CONFIG=1
RUN_CONFIG=1
RUN_PRE_CLEANUP=0
RUN_CLEANUP=1
RUN_POST_CLEANUP=0
RUN_CPPCHECK=1
RUN_SHELLCHECK=1
RUN_CHECK_OTHER=0
RUN_PRE_BUILD=0
RUN_BUILD=1
RUN_POST_BUILD=0
RUN_PRE_TEST=0
RUN_TEST=1
RUN_POST_TEST=0
RUN_PRE_PACKAGE=0
RUN_PACKAGE=1
RUN_POST_PACKAGE=0
RUN_PUBLISH_PACKAGE=1

#
# Before configuration
#
run_pre_configuration()
{
	if ! /bin/sh -c "./autogen.sh ${AUTOGEN_EXT_OPT}"; then
		PRNERR "Failed to run \"autogen.sh\" before configration."
		return 1
	fi
	return 0
}

#
# Configuration
#
run_configuration()
{
	if ! /bin/sh -c "./configure --prefix=/usr ${CONFIGURE_EXT_OPT}"; then
		PRNERR "Failed to run \"configure\"."
		return 1
	fi
	return 0
}

#
# Before Cleanup
#
run_pre_cleanup()
{
	PRNWARN "Not implement process before cleanup."
	return 0
}

#
# Cleanup
#
run_cleanup()
{
	if ! /bin/sh -c "make clean"; then
		PRNWARN "Failed to run \"make clean\", but continue because this command is failed if there is no Makefile."
	fi
	return 0
}

#
# After Cleanup
#
run_post_cleanup()
{
	PRNWARN "Not implement process after cleanup."
	return 0
}

#
# Check before building : CppCheck
#
run_cppcheck()
{
	if ! /bin/sh -c "make cppcheck"; then
		PRNERR "Failed to run \"make cppcheck\"."
		return 1
	fi
	return 0
}

#
# Check before building : ShellCheck
#
run_shellcheck()
{
	if ! /bin/sh -c "make shellcheck"; then
		PRNERR "Failed to run \"make shellcheck\"."
		return 1
	fi
	return 0
}

#
# Check before building : Other
#
run_othercheck()
{
	PRNWARN "Not implement other check before building."
	return 0
}

#
# Before Build
#
run_pre_build()
{
	PRNWARN "Not implement process before building."
	return 0
}

#
# Build
#
run_build()
{
	if ! /bin/sh -c "make ${BUILD_MAKE_EXT_OPT}"; then
		PRNERR "Failed to run \"make\"."
		return 1
	fi
	return 0
}

#
# After Build
#
run_post_build()
{
	PRNWARN "Not implement process after building."
	return 0
}

#
# Before Test
#
run_pre_test()
{
	PRNWARN "Not implement process before testing."
	return 0
}

#
# Test
#
run_test()
{
	if ! /bin/sh -c "make ${MAKE_TEST_OPT}"; then
		PRNERR "Failed to run \"make ${MAKE_TEST_OPT} ${MAKE_TEST_EXT_OPT}\"."
		return 1
	fi
	return 0
}

#
# After Test
#
run_post_test()
{
	PRNWARN "Not implement process after testing."
	return 0
}

#
# Before Create Package
#
run_pre_create_package()
{
	PRNWARN "Not implement process before packaging."
	return 0
}

#
# Create Package
#
run_create_package()
{
	if ! CONFIGUREOPT="${CONFIGURE_EXT_OPT}" /bin/sh -c "${CREATE_PACKAGE_TOOL} --buildnum ${CI_BUILD_NUMBER} ${CREATE_PACKAGE_TOOL_OPT} ${CREATE_PACKAGE_TOOL_OPT_AUTO}"; then
		PRNERR "Failed to create debian type packages"
		return 1
	fi
	return 0
}

#
# After Create Package
#
run_post_create_package()
{
	PRNWARN "Not implement process after packaging."
	return 0
}

#
# Publish Package
#
run_publish_package()
{
	if [ "${CI_DO_PUBLISH}" -eq 1 ]; then
		if [ -z "${CI_PACKAGECLOUD_TOKEN}" ]; then
			PRNERR "Token for uploading to packagecloud.io is not specified."
			return 1
		fi

		# [NOTE]
		# The Ruby environment of some OS uses RVM (Ruby Version Manager) and requires a Bash shell environment.
		#
		if [ "${IS_OS_DEBIAN}" -eq 1 ] && echo "${CI_OSTYPE}" | sed -e 's#:##g' | grep -q -i -e 'debian10' -e 'debianbuster'; then
			#
			# Case for Debian 10(buster)
			#
			{
				#
				# Create bash script for run package_cloud command, because using RVM(Ruby Version Manager).
				#
				echo '#!/bin/bash'
				echo ''
				echo 'source /etc/profile.d/rvm.sh'
				echo ''
				echo 'if ! '"PACKAGECLOUD_TOKEN=${CI_PACKAGECLOUD_TOKEN} package_cloud push ${CI_PACKAGECLOUD_OWNER}/${CI_PACKAGECLOUD_PUBLISH_REPO}/${DIST_TAG} ${SRCTOP}/${PKG_OUTPUT_DIR}/*.${PKG_EXT}; then"
				echo '	exit 1'
				echo 'fi'
				echo ''
				echo 'exit 0'
			} > /tmp/run_package_cloud.sh
			chmod +x /tmp/run_package_cloud.sh

			#
			# Run bash script
			#
			if ({ RUNCMD /tmp/run_package_cloud.sh || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to publish *.${PKG_EXT} packages to ${CI_PACKAGECLOUD_OWNER}/${CI_PACKAGECLOUD_PUBLISH_REPO}/${DIST_TAG}"
				rm -f /tmp/run_package_cloud.sh
				return 1
			fi
			rm -f /tmp/run_package_cloud.sh
		else
			#
			# Case for other than Debian 10(buster)
			#
			if ! PACKAGECLOUD_TOKEN="${CI_PACKAGECLOUD_TOKEN}" /bin/sh -c "package_cloud push ${CI_PACKAGECLOUD_OWNER}/${CI_PACKAGECLOUD_PUBLISH_REPO}/${DIST_TAG} ${SRCTOP}/${PKG_OUTPUT_DIR}/*.${PKG_EXT}"; then
				PRNERR "Failed to publish *.${PKG_EXT} packages to ${CI_PACKAGECLOUD_OWNER}/${CI_PACKAGECLOUD_PUBLISH_REPO}/${DIST_TAG}"
				return 1
			fi
		fi
	else
		PRNINFO "Not need to publish packages"
	fi
	return 0
}

#==============================================================
# Check options and environments
#==============================================================
PRNTITLE "Start to check options and environments"

#
# Parse options
#
OPT_OSTYPE=""
OPT_OSTYPE_VARS_FILE=""
OPT_BUILD_NUMBER=
OPT_DEVELOPER_FULLNAME=""
OPT_DEVELOPER_EMAIL=""
OPT_FORCE_PUBLISH=""
OPT_USE_PACKAGECLOUD_REPO=
OPT_PACKAGECLOUD_TOKEN=""
OPT_PACKAGECLOUD_OWNER=""
OPT_PACKAGECLOUD_PUBLISH_REPO=""
OPT_PACKAGECLOUD_DOWNLOAD_REPO=""

while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		break

	elif [ "$1" = "-h" ] || [ "$1" = "-H" ] || [ "$1" = "--help" ] || [ "$1" = "--HELP" ]; then
		func_usage "${PRGNAME}"
		exit 0

	elif [ "$1" = "-os" ] || [ "$1" = "-OS" ] || [ "$1" = "--ostype" ] || [ "$1" = "--OSTYPE" ]; then
		if [ -n "${OPT_OSTYPE}" ]; then
			PRNERR "already set \"--ostype(-os)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--ostype(-os)\" option is specified without parameter."
			exit 1
		fi
		OPT_OSTYPE="$1"

	elif [ "$1" = "-f" ] || [ "$1" = "-F" ] || [ "$1" = "--ostype-vars-file" ] || [ "$1" = "--OSTYPE-VARS-FILE" ]; then
		if [ -n "${OPT_OSTYPE_VARS_FILE}" ]; then
			PRNERR "already set \"--ostype-vars-file(-f)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--ostype-vars-file(-f)\" option is specified without parameter."
			exit 1
		fi
		if [ ! -f "$1" ]; then
			PRNERR "$1 file is not existed, it is specified \"--ostype-vars-file(-f)\" option."
			exit 1
		fi
		OPT_OSTYPE_VARS_FILE="$1"

	elif [ "$1" = "-p" ] || [ "$1" = "-P" ] || [ "$1" = "--force-publish" ] || [ "$1" = "--FORCE-PUBLISH" ]; then
		if [ -n "${OPT_FORCE_PUBLISH}" ]; then
			PRNERR "already set \"--force-publish(-p)\" or \"--not-publish(-np)\" option."
			exit 1
		fi
		OPT_FORCE_PUBLISH="true"

	elif [ "$1" = "-np" ] || [ "$1" = "-NP" ] || [ "$1" = "--not-publish" ] || [ "$1" = "--NOT-PUBLISH" ]; then
		if [ -n "${OPT_FORCE_PUBLISH}" ]; then
			PRNERR "already set \"--force-publish(-p)\" or \"--not-publish(-np)\" option."
			exit 1
		fi
		OPT_FORCE_PUBLISH="false"

	elif [ "$1" = "-num" ] || [ "$1" = "-NUM" ] || [ "$1" = "--build-number" ] || [ "$1" = "--BUILD-NUMBER" ]; then
		if [ -n "${OPT_BUILD_NUMBER}" ]; then
			PRNERR "already set \"--build-number(-num)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--build-number(-num)\" option is specified without parameter."
			exit 1
		fi
		if echo "$1" | grep -q '[^0-9]'; then
			PRNERR "\"--build-number(-num)\" option specify with positive NUMBER parameter."
			exit 1
		fi
		OPT_BUILD_NUMBER="$1"

	elif [ "$1" = "-devname" ] || [ "$1" = "-DEVNAME" ] || [ "$1" = "--developer-fullname" ] || [ "$1" = "--DEVELOPER-FULLNAME" ]; then
		if [ -n "${OPT_DEVELOPER_EMAIL}" ]; then
			PRNERR "already set \"--developer-fullname(-devname)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--developer-fullname(-devname)\" option is specified without parameter."
			exit 1
		fi
		OPT_DEVELOPER_EMAIL="$1"

	elif [ "$1" = "-devmail" ] || [ "$1" = "-DEVMAIL" ] || [ "$1" = "--developer-email" ] || [ "$1" = "--DEVELOPER-EMAIL" ]; then
		if [ -n "${OPT_DEVELOPER_FULLNAME}" ]; then
			PRNERR "already set \"--developer-email(-devmail)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--developer-email(-devmail)\" option is specified without parameter."
			exit 1
		fi
		OPT_DEVELOPER_FULLNAME="$1"

	elif [ "$1" = "-usepc" ] || [ "$1" = "-USEPC" ] || [ "$1" = "--use-packagecloudio-repo" ] || [ "$1" = "--USE-PACKAGECLOUDIO-REPO" ]; then
		if [ -n "${OPT_USE_PACKAGECLOUD_REPO}" ]; then
			PRNERR "already set \"--use-packagecloudio-repo(-usepc)\" or \"--not-use-packagecloudio-repo(-notpc)\" option."
			exit 1
		fi
		OPT_USE_PACKAGECLOUD_REPO=1

	elif [ "$1" = "-notpc" ] || [ "$1" = "-NOTPC" ] || [ "$1" = "--not-use-packagecloudio-repo" ] || [ "$1" = "--NOT-USE-PACKAGECLOUDIO-REPO" ]; then
		if [ -n "${OPT_USE_PACKAGECLOUD_REPO}" ]; then
			PRNERR "already set \"--use-packagecloudio-repo(-usepc)\" or \"--not-use-packagecloudio-repo(-notpc)\" option."
			exit 1
		fi
		OPT_USE_PACKAGECLOUD_REPO=0

	elif [ "$1" = "-pctoken" ] || [ "$1" = "-PCTOKEN" ] || [ "$1" = "--packagecloudio-token" ] || [ "$1" = "--PACKAGECLOUDIO-TOKEN" ]; then
		if [ -n "${OPT_PACKAGECLOUD_TOKEN}" ]; then
			PRNERR "already set \"--packagecloudio-token(-pctoken)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--packagecloudio-token(-pctoken)\" option is specified without parameter."
			exit 1
		fi
		OPT_PACKAGECLOUD_TOKEN="$1"

	elif [ "$1" = "-pcowner" ] || [ "$1" = "-PCOWNER" ] || [ "$1" = "--packagecloudio-owner" ] || [ "$1" = "--PACKAGECLOUDIO-OWNER" ]; then
		if [ -n "${OPT_PACKAGECLOUD_OWNER}" ]; then
			PRNERR "already set \"--packagecloudio-owner(-pcowner)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--packagecloudio-owner(-pcowner)\" option is specified without parameter."
			exit 1
		fi
		OPT_PACKAGECLOUD_OWNER="$1"

	elif [ "$1" = "-pcprepo" ] || [ "$1" = "-PCPREPO" ] || [ "$1" = "--packagecloudio-publish-repo" ] || [ "$1" = "--PACKAGECLOUDIO-PUBLICH-REPO" ]; then
		if [ -n "${OPT_PACKAGECLOUD_PUBLISH_REPO}" ]; then
			PRNERR "already set \"--packagecloudio-publish-repo(-pcprepo)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--packagecloudio-publish-repo(-pcprepo)\" option is specified without parameter."
			exit 1
		fi
		OPT_PACKAGECLOUD_PUBLISH_REPO="$1"

	elif [ "$1" = "-pcdlrepo" ] || [ "$1" = "-PCDLREPO" ] || [ "$1" = "--packagecloudio-download-repo" ] || [ "$1" = "--PACKAGECLOUDIO-DOWNLOAD-REPO" ]; then
		if [ -n "${OPT_PACKAGECLOUD_DOWNLOAD_REPO}" ]; then
			PRNERR "already set \"--packagecloudio-download-repo(-pcdlrepo)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			PRNERR "\"--packagecloudio-download-repo(-pcdlrepo)\" option is specified without parameter."
			exit 1
		fi
		OPT_PACKAGECLOUD_DOWNLOAD_REPO="$1"
	fi
	shift
done

#
# [Required option] check OS and version
#
if [ -z "${OPT_OSTYPE}" ]; then
	PRNERR "\"--ostype(-os)\" option is not specified."
	exit 1
else
	CI_OSTYPE="${OPT_OSTYPE}"
fi

#
# Check other options and enviroments
#
if [ -n "${OPT_OSTYPE_VARS_FILE}" ]; then
	CI_OSTYPE_VARS_FILE="${OPT_OSTYPE_VARS_FILE}"
elif [ -n "${ENV_OSTYPE_VARS_FILE}" ]; then
	CI_OSTYPE_VARS_FILE="${ENV_OSTYPE_VARS_FILE}"
fi

if [ -n "${OPT_BUILD_NUMBER}" ]; then
	CI_BUILD_NUMBER="${OPT_BUILD_NUMBER}"
elif [ -n "${ENV_BUILD_NUMBER}" ]; then
	CI_BUILD_NUMBER="${ENV_BUILD_NUMBER}"
fi

if [ -n "${OPT_DEVELOPER_FULLNAME}" ]; then
	CI_DEVELOPER_FULLNAME="${OPT_DEVELOPER_FULLNAME}"
elif [ -n "${ENV_DEVELOPER_FULLNAME}" ]; then
	CI_DEVELOPER_FULLNAME="${ENV_DEVELOPER_FULLNAME}"
fi

if [ -n "${OPT_DEVELOPER_EMAIL}" ]; then
	CI_DEVELOPER_EMAIL="${OPT_DEVELOPER_EMAIL}"
elif [ -n "${ENV_DEVELOPER_EMAIL}" ]; then
	CI_DEVELOPER_EMAIL="${ENV_DEVELOPER_EMAIL}"
fi

if [ -n "${OPT_FORCE_PUBLISH}" ]; then
	if echo "${OPT_FORCE_PUBLISH}" | grep -q -i '^true$'; then
		CI_FORCE_PUBLISH="true"
	elif echo "${OPT_FORCE_PUBLISH}" | grep -q -i '^false$'; then
		CI_FORCE_PUBLISH="false"
	else
		PRNERR "\"OPT_FORCE_PUBLISH\" value is wrong."
		exit 1
	fi
elif [ -n "${ENV_FORCE_PUBLISH}" ]; then
	if echo "${ENV_FORCE_PUBLISH}" | grep -q -i '^true$'; then
		CI_FORCE_PUBLISH="true"
	elif echo "${ENV_FORCE_PUBLISH}" | grep -q -i '^false$'; then
		CI_FORCE_PUBLISH="false"
	else
		PRNERR "\"ENV_FORCE_PUBLISH\" value is wrong."
		exit 1
	fi
fi

if [ -n "${OPT_USE_PACKAGECLOUD_REPO}" ]; then
	if [ "${OPT_USE_PACKAGECLOUD_REPO}" -eq 1 ]; then
		CI_USE_PACKAGECLOUD_REPO=1
	elif [ "${OPT_USE_PACKAGECLOUD_REPO}" -eq 0 ]; then
		CI_USE_PACKAGECLOUD_REPO=0
	else
		PRNERR "\"OPT_USE_PACKAGECLOUD_REPO\" value is wrong."
		exit 1
	fi
elif [ -n "${ENV_USE_PACKAGECLOUD_REPO}" ]; then
	if echo "${ENV_USE_PACKAGECLOUD_REPO}" | grep -q -i '^true$'; then
		CI_USE_PACKAGECLOUD_REPO=1
	elif echo "${ENV_USE_PACKAGECLOUD_REPO}" | grep -q -i '^false$'; then
		CI_USE_PACKAGECLOUD_REPO=0
	else
		PRNERR "\"ENV_USE_PACKAGECLOUD_REPO\" value is wrong."
		exit 1
	fi
fi

if [ -n "${OPT_PACKAGECLOUD_TOKEN}" ]; then
	CI_PACKAGECLOUD_TOKEN="${OPT_PACKAGECLOUD_TOKEN}"
elif [ -n "${ENV_PACKAGECLOUD_TOKEN}" ]; then
	CI_PACKAGECLOUD_TOKEN="${ENV_PACKAGECLOUD_TOKEN}"
fi

if [ -n "${OPT_PACKAGECLOUD_OWNER}" ]; then
	CI_PACKAGECLOUD_OWNER="${OPT_PACKAGECLOUD_OWNER}"
elif [ -n "${ENV_PACKAGECLOUD_OWNER}" ]; then
	CI_PACKAGECLOUD_OWNER="${ENV_PACKAGECLOUD_OWNER}"
fi

if [ -n "${OPT_PACKAGECLOUD_PUBLISH_REPO}" ]; then
	CI_PACKAGECLOUD_PUBLISH_REPO="${OPT_PACKAGECLOUD_PUBLISH_REPO}"
elif [ -n "${ENV_PACKAGECLOUD_PUBLISH_REPO}" ]; then
	CI_PACKAGECLOUD_PUBLISH_REPO="${ENV_PACKAGECLOUD_PUBLISH_REPO}"
fi

if [ -n "${OPT_PACKAGECLOUD_DOWNLOAD_REPO}" ]; then
	CI_PACKAGECLOUD_DOWNLOAD_REPO="${OPT_PACKAGECLOUD_DOWNLOAD_REPO}"
elif [ -n "${ENV_PACKAGECLOUD_DOWNLOAD_REPO}" ]; then
	CI_PACKAGECLOUD_DOWNLOAD_REPO="${ENV_PACKAGECLOUD_DOWNLOAD_REPO}"
fi

#
# Set environments for debian package
#
if [ -n "${CI_DEVELOPER_FULLNAME}" ]; then
	export DEBEMAIL="${CI_DEVELOPER_FULLNAME}"
fi
if [ -n "${CI_DEVELOPER_EMAIL}" ]; then
	export DEBFULLNAME="${CI_DEVELOPER_EMAIL}"
fi

# [NOTE] for ubuntu/debian
# When start to update, it may come across an unexpected interactive interface.
# (May occur with time zone updates)
# Set environment variables to avoid this.
#
export DEBIAN_FRONTEND=noninteractive

PRNSUCCESS "Start to check options and environments"

#==============================================================
# Set Variables
#==============================================================
#
# Default command parameters for each phase
#
AUTOGEN_EXT_OPT_RPM=""
AUTOGEN_EXT_OPT_DEBIAN=""
AUTOGEN_EXT_OPT_ALPINE=""
AUTOGEN_EXT_OPT_OTHER=""

CONFIGURE_EXT_OPT_RPM=""
CONFIGURE_EXT_OPT_DEBIAN=""
CONFIGURE_EXT_OPT_ALPINE=""
CONFIGURE_EXT_OPT_OTHER=""

BUILD_MAKE_EXT_OPT_RPM=""
BUILD_MAKE_EXT_OPT_DEBIAN=""
BUILD_MAKE_EXT_OPT_ALPINE=""
BUILD_MAKE_EXT_OPT_OTHER=""

MAKE_TEST_OPT_RPM="check"
MAKE_TEST_OPT_DEBIAN="check"
MAKE_TEST_OPT_ALPINE="check"
MAKE_TEST_OPT_OTHER="check"

CREATE_PACKAGE_TOOL_RPM="buildutils/rpm_build.sh"
CREATE_PACKAGE_TOOL_DEBIAN="buildutils/debian_build.sh"
CREATE_PACKAGE_TOOL_ALPINE="buildutils/alpine_build.sh"
CREATE_PACKAGE_TOOL_OTHER=""

CREATE_PACKAGE_TOOL_OPT_AUTO="-y"
CREATE_PACKAGE_TOOL_OPT_RPM=""
CREATE_PACKAGE_TOOL_OPT_DEBIAN=""
CREATE_PACKAGE_TOOL_OPT_ALPINE=""
CREATE_PACKAGE_TOOL_OPT_OTHER=""

#
# Load variables from file
#
PRNTITLE "Load local variables with an external file"

#
# Load external variable file
#
if [ -f "${CI_OSTYPE_VARS_FILE}" ]; then
	PRNINFO "Load ${CI_OSTYPE_VARS_FILE} file for local variables by os:type(${CI_OSTYPE})"
	. "${CI_OSTYPE_VARS_FILE}"
else
	PRNWARN "${CI_OSTYPE_VARS_FILE} file is not existed."
fi

#
# Check loading variables
#
if [ -z "${DIST_TAG}" ]; then
	PRNERR "Distro/Version is not set, please check ${CI_OSTYPE_VARS_FILE} and check \"DIST_TAG\" varibale."
	exit 1
fi

#
# Set command parameters
#
if [ "${IS_OS_UBUNTU}" -eq 1 ]; then
	AUTOGEN_EXT_OPT="${AUTOGEN_EXT_OPT_DEBIAN}"
	CONFIGURE_EXT_OPT="${CONFIGURE_EXT_OPT_DEBIAN}"
	BUILD_MAKE_EXT_OPT="${BUILD_MAKE_EXT_OPT_DEBIAN}"
	MAKE_TEST_OPT="${MAKE_TEST_OPT_DEBIAN}"
	CREATE_PACKAGE_TOOL="${CREATE_PACKAGE_TOOL_DEBIAN}"
	CREATE_PACKAGE_TOOL_OPT="${CREATE_PACKAGE_TOOL_OPT_DEBIAN}"

elif [ "${IS_OS_DEBIAN}" -eq 1 ]; then
	AUTOGEN_EXT_OPT="${AUTOGEN_EXT_OPT_DEBIAN}"
	CONFIGURE_EXT_OPT="${CONFIGURE_EXT_OPT_DEBIAN}"
	BUILD_MAKE_EXT_OPT="${BUILD_MAKE_EXT_OPT_DEBIAN}"
	MAKE_TEST_OPT="${MAKE_TEST_OPT_DEBIAN}"
	CREATE_PACKAGE_TOOL="${CREATE_PACKAGE_TOOL_DEBIAN}"
	CREATE_PACKAGE_TOOL_OPT="${CREATE_PACKAGE_TOOL_OPT_DEBIAN}"

elif [ "${IS_OS_CENTOS}" -eq 1 ]; then
	AUTOGEN_EXT_OPT="${AUTOGEN_EXT_OPT_RPM}"
	CONFIGURE_EXT_OPT="${CONFIGURE_EXT_OPT_RPM}"
	BUILD_MAKE_EXT_OPT="${BUILD_MAKE_EXT_OPT_DEBIAN}"
	MAKE_TEST_OPT="${MAKE_TEST_OPT_DEBIAN}"
	CREATE_PACKAGE_TOOL="${CREATE_PACKAGE_TOOL_RPM}"
	CREATE_PACKAGE_TOOL_OPT="${CREATE_PACKAGE_TOOL_OPT_RPM}"

elif [ "${IS_OS_FEDORA}" -eq 1 ]; then
	AUTOGEN_EXT_OPT="${AUTOGEN_EXT_OPT_RPM}"
	CONFIGURE_EXT_OPT="${CONFIGURE_EXT_OPT_RPM}"
	BUILD_MAKE_EXT_OPT="${BUILD_MAKE_EXT_OPT_RPM}"
	MAKE_TEST_OPT="${MAKE_TEST_OPT_RPM}"
	CREATE_PACKAGE_TOOL="${CREATE_PACKAGE_TOOL_RPM}"
	CREATE_PACKAGE_TOOL_OPT="${CREATE_PACKAGE_TOOL_OPT_RPM}"

elif [ "${IS_OS_ROCKY}" -eq 1 ]; then
	AUTOGEN_EXT_OPT="${AUTOGEN_EXT_OPT_RPM}"
	CONFIGURE_EXT_OPT="${CONFIGURE_EXT_OPT_RPM}"
	BUILD_MAKE_EXT_OPT="${BUILD_MAKE_EXT_OPT_RPM}"
	MAKE_TEST_OPT="${MAKE_TEST_OPT_RPM}"
	CREATE_PACKAGE_TOOL="${CREATE_PACKAGE_TOOL_RPM}"
	CREATE_PACKAGE_TOOL_OPT="${CREATE_PACKAGE_TOOL_OPT_RPM}"

elif [ "${IS_OS_ALPINE}" -eq 1 ]; then
	AUTOGEN_EXT_OPT="${AUTOGEN_EXT_OPT_ALPINE}"
	CONFIGURE_EXT_OPT="${CONFIGURE_EXT_OPT_ALPINE}"
	BUILD_MAKE_EXT_OPT="${BUILD_MAKE_EXT_OPT_ALPINE}"
	MAKE_TEST_OPT="${MAKE_TEST_OPT_ALPINE}"
	CREATE_PACKAGE_TOOL="${CREATE_PACKAGE_TOOL_ALPINE}"
	CREATE_PACKAGE_TOOL_OPT="${CREATE_PACKAGE_TOOL_OPT_ALPINE}"

else
	AUTOGEN_EXT_OPT="${AUTOGEN_EXT_OPT_OTHER}"
	CONFIGURE_EXT_OPT="${CONFIGURE_EXT_OPT_OTHER}"
	BUILD_MAKE_EXT_OPT="${BUILD_MAKE_EXT_OPT_OTHER}"
	MAKE_TEST_OPT="${MAKE_TEST_OPT_OTHER}"
	CREATE_PACKAGE_TOOL="${CREATE_PACKAGE_TOOL_OTHER}"
	CREATE_PACKAGE_TOOL_OPT="${CREATE_PACKAGE_TOOL_OPT_OTHER}"
fi

PRNSUCCESS "Load local variables with an external file"

#----------------------------------------------------------
# Check github actions environments
#----------------------------------------------------------
PRNTITLE "Check github actions environments"

#
# GITHUB_EVENT_NAME Environment
#
if [ -n "${GITHUB_EVENT_NAME}" ] && [ "${GITHUB_EVENT_NAME}" = "schedule" ]; then
	CI_IN_SCHEDULE_PROCESS=1
else
	CI_IN_SCHEDULE_PROCESS=0
fi

#
# GITHUB_REF Environments
#
if [ -n "${GITHUB_REF}" ] && echo "${GITHUB_REF}" | grep -q 'refs/tags/'; then
	CI_PUBLISH_TAG_NAME=$(echo "${GITHUB_REF}" | sed -e 's#refs/tags/##g' | tr -d '\n')
fi

PRNSUCCESS "Check github actions environments"

#----------------------------------------------------------
# Check whether to publish
#----------------------------------------------------------
PRNTITLE "Check whether to publish"

#
# Check whether to publish
#
if [ -z "${CI_FORCE_PUBLISH}" ]; then
	if [ -n "${CI_PUBLISH_TAG_NAME}" ] && [ "${CI_IN_SCHEDULE_PROCESS}" -ne 1 ]; then
		CI_DO_PUBLISH=1
	else
		CI_DO_PUBLISH=0
	fi
elif [ "${CI_FORCE_PUBLISH}" = "true" ]; then
	#
	# Force publishing
	#
	if [ -n "${CI_PUBLISH_TAG_NAME}" ] && [ "${CI_IN_SCHEDULE_PROCESS}" -ne 1 ]; then
		PRNINFO "specified \"--force-publish(-p)\" option or set \"ENV_FORCE_PUBLISH=true\" environment, then forcibly publish"
		CI_DO_PUBLISH=1
	else
		PRNWARN "specified \"--force-publish(-p)\" option or set \"ENV_FORCE_PUBLISH=true\" environment, but Ci was launched by schedule or did not have tag name. Thus it do not run publishing."
		CI_DO_PUBLISH=0
	fi
else
	#
	# FORCE NOT PUBLISH
	#
	PRNINFO "specified \"--not-publish(-np)\" option or set \"ENV_FORCE_PUBLISH=false\" environment, then it do not run publishing."
	CI_DO_PUBLISH=0
fi

PRNSUCCESS "Check whether to publish"

#----------------------------------------------------------
# Show execution environment variables
#----------------------------------------------------------
PRNTITLE "Show execution environment variables"

#
# Information
#
echo "  PRGNAME                       = ${PRGNAME}"
echo "  SCRIPTDIR                     = ${SCRIPTDIR}"
echo "  SRCTOP                        = ${SRCTOP}"
echo ""
echo "  CI_IN_SCHEDULE_PROCESS        = ${CI_IN_SCHEDULE_PROCESS}"
echo "  CI_OSTYPE_VARS_FILE           = ${CI_OSTYPE_VARS_FILE}"
echo "  CI_OSTYPE                     = ${CI_OSTYPE}"
echo "  CI_BUILD_NUMBER               = ${CI_BUILD_NUMBER}"
echo "  CI_DO_PUBLISH                 = ${CI_DO_PUBLISH}"
echo "  CI_PUBLISH_TAG_NAME           = ${CI_PUBLISH_TAG_NAME}"
echo "  CI_USE_PACKAGECLOUD_REPO      = ${CI_USE_PACKAGECLOUD_REPO}"
echo "  CI_PACKAGECLOUD_TOKEN         = **********"
echo "  CI_PACKAGECLOUD_OWNER         = ${CI_PACKAGECLOUD_OWNER}"
echo "  CI_PACKAGECLOUD_PUBLISH_REPO  = ${CI_PACKAGECLOUD_PUBLISH_REPO}"
echo "  CI_PACKAGECLOUD_DOWNLOAD_REPO = ${CI_PACKAGECLOUD_DOWNLOAD_REPO}"
echo ""
echo "  DIST_TAG                      = ${DIST_TAG}"
echo "  IS_OS_UBUNTU                  = ${IS_OS_UBUNTU}"
echo "  IS_OS_DEBIAN                  = ${IS_OS_DEBIAN}"
echo "  IS_OS_CENTOS                  = ${IS_OS_CENTOS}"
echo "  IS_OS_FEDORA                  = ${IS_OS_FEDORA}"
echo "  IS_OS_ROCKY                   = ${IS_OS_ROCKY}"
echo "  IS_OS_ALPINE                  = ${IS_OS_ALPINE}"
echo "  INSTALL_PKG_LIST              = ${INSTALL_PKG_LIST}"
echo "  INSTALLER_BIN                 = ${INSTALLER_BIN}"
echo "  UPDATE_CMD                    = ${UPDATE_CMD}"
echo "  UPDATE_CMD_ARG                = ${UPDATE_CMD_ARG}"
echo "  INSTALL_CMD                   = ${INSTALL_CMD}"
echo "  INSTALL_CMD_ARG               = ${INSTALL_CMD_ARG}"
echo "  INSTALL_AUTO_ARG              = ${INSTALL_AUTO_ARG}"
echo "  INSTALL_QUIET_ARG             = ${INSTALL_QUIET_ARG}"
echo "  PKG_OUTPUT_DIR                = ${PKG_OUTPUT_DIR}"
echo "  PKG_EXT                       = ${PKG_EXT}"
echo "  DEBEMAIL                      = ${DEBEMAIL}"
echo "  DEBFULLNAME                   = ${DEBFULLNAME}"
echo ""
echo "  AUTOGEN_EXT_OPT               = ${AUTOGEN_EXT_OPT}"
echo "  CONFIGURE_EXT_OPT             = ${CONFIGURE_EXT_OPT}"
echo "  BUILD_MAKE_EXT_OPT            = ${BUILD_MAKE_EXT_OPT}"
echo "  MAKE_TEST_OPT                 = ${MAKE_TEST_OPT}"
echo "  CREATE_PACKAGE_TOOL           = ${CREATE_PACKAGE_TOOL}"
echo "  CREATE_PACKAGE_TOOL_OPT       = ${CREATE_PACKAGE_TOOL_OPT}"
echo ""

PRNSUCCESS "Show execution environment variables"

#==============================================================
# Install all packages
#==============================================================
PRNTITLE "Update repository and Install curl"

#
# Update local packages
#
PRNINFO "Update local packages"
if ({ RUNCMD "${INSTALLER_BIN}" "${UPDATE_CMD}" "${UPDATE_CMD_ARG}" "${INSTALL_AUTO_ARG}" "${INSTALL_QUIET_ARG}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	PRNERR "Failed to update local packages"
	exit 1
fi

#
# Check and install curl
#
if ! CURLCMD=$(command -v curl); then
	PRNINFO "Install curl command"
	if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" "${INSTALL_QUIET_ARG}" curl || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
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
PRNSUCCESS "Update repository and Install curl"

#--------------------------------------------------------------
# Set package repository for packagecloud.io
#--------------------------------------------------------------
PRNTITLE "Set package repository for packagecloud.io"

if [ "${CI_USE_PACKAGECLOUD_REPO}" -eq 1 ]; then
	#
	# Setup packagecloud.io repository
	#
	if [ "${IS_OS_CENTOS}" -eq 1 ] || [ "${IS_OS_FEDORA}" -eq 1 ] || [ "${IS_OS_ROCKY}" -eq 1 ]; then
		PC_REPO_ADD_SH="script.rpm.sh"
		PC_REPO_ADD_SH_RUN="bash"
	elif [ "${IS_OS_UBUNTU}" -eq 1 ] || [ "${IS_OS_DEBIAN}" -eq 1 ]; then
		PC_REPO_ADD_SH="script.deb.sh"
		PC_REPO_ADD_SH_RUN="bash"
	elif [ "${IS_OS_ALPINE}" -eq 1 ]; then
		PC_REPO_ADD_SH="script.alpine.sh"
		PC_REPO_ADD_SH_RUN="sh"
	else
		PC_REPO_ADD_SH=""
		PC_REPO_ADD_SH_RUN=""
	fi
	if [ -n "${PC_REPO_ADD_SH}" ]; then
		PRNINFO "Download script and setup packagecloud.io reposiory"
		if ({ RUNCMD "${CURLCMD} -s https://packagecloud.io/install/repositories/${CI_PACKAGECLOUD_OWNER}/${CI_PACKAGECLOUD_DOWNLOAD_REPO}/${PC_REPO_ADD_SH} | ${PC_REPO_ADD_SH_RUN}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to download script or setup packagecloud.io reposiory"
			exit 1
		fi
	else
		PRNWARN "OS is not debian/ubuntu nor centos/fedora/rocky nor alpine, then we do not know which download script use. Thus skip to setup packagecloud.io repository."
	fi
else
	PRNINFO "Not set packagecloud.io repository."
fi
PRNSUCCESS "Set package repository for packagecloud.io"

#--------------------------------------------------------------
# Install packages
#--------------------------------------------------------------
PRNTITLE "Install packages for building/packaging"

if [ -n "${INSTALL_PKG_LIST}" ]; then
	PRNINFO "Install packages"
	if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" "${INSTALL_QUIET_ARG}" "${INSTALL_PKG_LIST}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNERR "Failed to install packages"
		exit 1
	fi
else
	PRNINFO "Specified no packages for installing. "
fi

PRNSUCCESS "Install packages for building/packaging"

#--------------------------------------------------------------
# Install published tools for uploading packages to packagecloud.io
#--------------------------------------------------------------
PRNTITLE "Install published tools for uploading packages to packagecloud.io"

if [ "${CI_DO_PUBLISH}" -eq 1 ]; then
	PRNINFO "Install published tools for uploading packages to packagecloud.io"
	GEM_BIN="gem"
	GEM_INSTALL_CMD="install"

	if [ "${IS_OS_CENTOS}" -eq 1 ] && echo "${CI_OSTYPE}" | sed -e 's#:##g' | grep -q -i -e 'centos7' -e 'centos6'; then
		#
		# Case for CentOS
		#
		PRNWARN "OS is CentOS 7(6), so install ruby by special means(SCL)."

		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" "${INSTALL_QUIET_ARG}" centos-release-scl || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install SCL packages"
			exit 1
		fi
		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" "${INSTALL_QUIET_ARG}" rh-ruby27 rh-ruby27-ruby-devel rh-ruby27-rubygem-rake || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install ruby packages"
			exit 1
		fi
		. /opt/rh/rh-ruby27/enable

		if ({ RUNCMD "${GEM_BIN}" "${GEM_INSTALL_CMD}" package_cloud || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install packagecloud.io upload tools"
			exit 1
		fi

	elif [ "${IS_OS_ALPINE}" -eq 1 ]; then
		#
		# Case for Alpine
		#
		if ({ RUNCMD "${GEM_BIN}" "${GEM_INSTALL_CMD}" package_cloud || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install packagecloud.io upload tools"
			exit 1
		fi

	elif [ "${IS_OS_ROCKY}" -eq 1 ] && echo "${CI_OSTYPE}" | sed -e 's#:##g' | grep -q -i -e 'rockylinux8' -e 'rocky8'; then
		#
		# Case for Rocky Linux 8 (default ruby 2.5)
		#

		#
		# Switch ruby module
		#
		if ({ RUNCMD "${INSTALLER_BIN}" module "${INSTALL_AUTO_ARG}" reset ruby || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to reset ruby module"
			exit 1
		fi
		if ({ RUNCMD "${INSTALLER_BIN}" module "${INSTALL_AUTO_ARG}" install ruby:2.7 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install ruby 2.7 module"
			exit 1
		fi
		if ({ RUNCMD "${INSTALLER_BIN}" module "${INSTALL_AUTO_ARG}" enable ruby:2.7 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to enable ruby 2.7 module"
			exit 1
		fi
		if ({ RUNCMD "${INSTALLER_BIN}" module "${INSTALL_AUTO_ARG}" update ruby:2.7 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to update ruby 2.7 module"
			exit 1
		fi

		#
		# Install package_cloud tool
		#
		if ({ RUNCMD "${GEM_BIN}" "${GEM_INSTALL_CMD}" package_cloud || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install packagecloud.io upload tools"
			exit 1
		fi

	elif [ "${IS_OS_DEBIAN}" -eq 1 ] && echo "${CI_OSTYPE}" | sed -e 's#:##g' | grep -q -i -e 'debian10' -e 'debianbuster'; then
		#
		# Case for Debian 10/buster (default ruby 2.5)
		#

		#
		# Set RVM(Ruby Version Manager) and install Ruby 2.7 and package_cloud
		#
		# [NOTE]
		# Install Ruby2.7 using RVM tools.
		# Installation and running RVM tools must be done in Bash.
		# This set of installations will create a Bash script and run it.
		#
		# The script does the following:
		# First, we need to install the GPG key before installing RVM.
		# This is done with one of the following commands:
		#
		#	sudo gpg --keyserver hkp://keyserver.ubuntu.com --recv-keys 409B6B1796C275462A1703113804BB82D39DC0E3 7D2BAF1CF37B13E2069D6956105BD0E739499BDB
		#		or
		#	command curl -sSL https://rvm.io/mpapis.asc | sudo gpg --import -
		#	command curl -sSL https://rvm.io/pkuczynski.asc | sudo gpg --import -
		#
		# After that, install RVM installation, RVM environment settings, Ruby2.7 installation, and package_cloud tools.
		#
		# [NOTE]
		# The RVM installation requires running from a bash shell.
		# So create a Bash script and run it.
		{
			echo '#!/bin/bash'
			echo ''
			echo 'if ! curl -sSL https://rvm.io/mpapis.asc | gpg --import - 2>&1; then'
			echo '	echo "Failed to run [ curl -sSL https://rvm.io/mpapis.asc | gpg --import - ] command."'
			echo '	exit 1'
			echo 'fi'
			echo 'if ! curl -sSL https://rvm.io/pkuczynski.asc | gpg --import - 2>&1; then'
			echo '	echo "Failed to run [ curl -sSL https://rvm.io/pkuczynski.asc | gpg --import - ] command."'
			echo '	exit 1'
			echo 'fi'
			echo ''
			echo 'if ! curl -sSL https://get.rvm.io | bash -s stable --ruby 2>&1; then'
			echo '	echo "Failed to install RVM tool."'
			echo '	exit 1'
			echo 'fi'
			echo ''
			echo 'if [ ! -f /etc/profile.d/rvm.sh ]; then'
			echo '	echo "Not found /etc/profile.d/rvm.sh file."'
			echo '	exit 1'
			echo 'fi'
			echo 'source /etc/profile.d/rvm.sh'
			echo ''
			echo 'if ! rvm get stable --autolibs=enable 2>&1; then'
			echo '	echo "Failed to get/update RVM stable."'
			echo '	exit 1'
			echo 'fi'
			echo 'if ! usermod -a -G rvm root 2>&1; then'
			echo '	echo "Failed to add rvm user to root group."'
			echo '	exit 1'
			echo 'fi'
			echo 'if ! rvm install ruby-2.7 2>&1; then'
			echo '	echo "Failed to install ruby 2.7."'
			echo '	exit 1'
			echo 'fi'
			echo 'if ! rvm --default use ruby-2.7 2>&1; then'
			echo '	echo "Failed to set ruby 2.7 as default."'
			echo '	exit 1'
			echo 'fi'
			echo ''
			echo 'if ! '"${GEM_BIN} ${GEM_INSTALL_CMD} package_cloud 2>&1; then"
			echo '	echo "Failed to install packagecloud.io upload tools"'
			echo '	exit 1'
			echo 'fi'
			echo ''
			echo 'exit 0'
		} > /tmp/rvm_setup.sh
		chmod +x /tmp/rvm_setup.sh

		#
		# Run bash script
		#
		if ({ RUNCMD /tmp/rvm_setup.sh || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to set up RVM."
			rm -f /tmp/rvm_setup.sh
			exit 1
		fi
		rm -f /tmp/rvm_setup.sh

	else
		#
		# Case for other than CentOS / Alpine / Debian 10 / Rocky Linux 8
		#
		if ({ RUNCMD "${GEM_BIN}" "${GEM_INSTALL_CMD}" rake package_cloud || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install packagecloud.io upload tools"
			exit 1
		fi
	fi
else
	PRNINFO "Skip to install published tools for uploading packages to packagecloud.io, because this CI process does not upload any packages."
fi
PRNSUCCESS "Install published tools for uploading packages to packagecloud.io"

#--------------------------------------------------------------
# Install cppcheck
#--------------------------------------------------------------
PRNTITLE "Install cppcheck"

IS_SET_ANOTHER_REPOSITORIES=0
if [ "${RUN_CPPCHECK}" -eq 1 ]; then
	PRNINFO "Install cppcheck package."

	if [ "${IS_OS_CENTOS}" -eq 1 ]; then
		#
		# CentOS
		#
		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" epel-release || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install epel repository"
			exit 1
		fi
		if ({ RUNCMD yum-config-manager --disable epel || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to disable epel repository"
			exit 1
		fi
		if ({ RUNCMD "${INSTALLER_BIN}" --enablerepo=epel "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" cppcheck || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install cppcheck from epel repository"
			exit 1
		fi
		IS_SET_ANOTHER_REPOSITORIES=1

	elif [ "${IS_OS_FEDORA}" -eq 1 ]; then
		#
		# Fedora
		#
		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" cppcheck || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install cppcheck"
			exit 1
		fi

	elif [ "${IS_OS_ROCKY}" -eq 1 ]; then
		if echo "${CI_OSTYPE}" | sed -e 's#:##g' | grep -q -i 'rockylinux8'; then
			#
			# Rocky 8
			#
			if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to install epel repository"
				exit 1
			fi
			if ({ RUNCMD "${INSTALLER_BIN}" config-manager --enable epel || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to enable epel repository"
				exit 1
			fi
			if ({ RUNCMD "${INSTALLER_BIN}" config-manager --set-enabled powertools || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to enable powertools"
				exit 1
			fi
		else
			#
			# Rocky 9 or later
			#
			if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to install epel repository"
				exit 1
			fi
			if ({ RUNCMD "${INSTALLER_BIN}" config-manager --enable epel || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to enable epel repository"
				exit 1
			fi
		fi
		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" cppcheck || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install cppcheck"
			exit 1
		fi
		IS_SET_ANOTHER_REPOSITORIES=1

	elif [ "${IS_OS_UBUNTU}" -eq 1 ] || [ "${IS_OS_DEBIAN}" -eq 1 ]; then
		#
		# Ubuntu or Debian
		#
		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" cppcheck || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install cppcheck"
			exit 1
		fi

	elif [ "${IS_OS_ALPINE}" -eq 1 ]; then
		#
		# Alpine
		#
		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" cppcheck || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install cppcheck"
			exit 1
		fi

	else
		PRNINFO "Skip to install cppcheck package, because unknown to install it."
	fi
else
	PRNINFO "Skip to install cppcheck package, because cppcheck process does not need."
fi
PRNSUCCESS "Install cppcheck"

#--------------------------------------------------------------
# Install shellcheck
#--------------------------------------------------------------
PRNTITLE "Install shellcheck"

if [ "${RUN_SHELLCHECK}" -eq 1 ]; then
	PRNINFO "Install shellcheck package."

	if [ "${IS_OS_CENTOS}" -eq 1 ]; then
		#
		# CentOS
		#
		if [ "${IS_SET_ANOTHER_REPOSITORIES}" -eq 0 ]; then
			if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" epel-release || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to install epel repository"
				exit 1
			fi
			if ({ RUNCMD yum-config-manager --disable epel || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
				PRNERR "Failed to disable epel repository"
				exit 1
			fi
			IS_SET_ANOTHER_REPOSITORIES=1
		fi
		if ({ RUNCMD "${INSTALLER_BIN}" --enablerepo=epel "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" ShellCheck || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install ShellCheck from epel repository"
			exit 1
		fi

	elif [ "${IS_OS_FEDORA}" -eq 1 ]; then
		#
		# Fedora
		#
		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" ShellCheck || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install cppcheck"
			exit 1
		fi

	elif [ "${IS_OS_ROCKY}" -eq 1 ]; then
		#
		# Rocky
		#
		if ! LATEST_SHELLCHECK_DOWNLOAD_URL=$("${CURLCMD}" -s -S https://api.github.com/repos/koalaman/shellcheck/releases/latest | tr '{' '\n' | tr '}' '\n' | tr '[' '\n' | tr ']' '\n' | tr ',' '\n' | grep '"browser_download_url"' | grep 'linux.x86_64' | sed -e 's|"||g' -e 's|^.*browser_download_url:[[:space:]]*||g' -e 's|^[[:space:]]*||g' -e 's|[[:space:]]*$||g' | tr -d '\n'); then
			PRNERR "Failed to get shellcheck download url path"
			exit 1
		fi
		if ({ RUNCMD "${CURLCMD}" -s -S -L -o /tmp/shellcheck.tar.xz "${LATEST_SHELLCHECK_DOWNLOAD_URL}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to download latest shellcheck tar.xz"
			exit 1
		fi
		if ({ RUNCMD tar -C /usr/bin/ -xf /tmp/shellcheck.tar.xz --no-anchored 'shellcheck' --strip=1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to extract latest shellcheck binary"
			exit 1
		fi
		rm -f /tmp/shellcheck.tar.xz

	elif [ "${IS_OS_UBUNTU}" -eq 1 ] || [ "${IS_OS_DEBIAN}" -eq 1 ]; then
		#
		# Ubuntu or Debian
		#
		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" shellcheck || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install cppcheck"
			exit 1
		fi

	elif [ "${IS_OS_ALPINE}" -eq 1 ]; then
		#
		# Alpine
		#
		if ({ RUNCMD "${INSTALLER_BIN}" "${INSTALL_CMD}" "${INSTALL_CMD_ARG}" "${INSTALL_AUTO_ARG}" shellcheck || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
			PRNERR "Failed to install cppcheck"
			exit 1
		fi

	else
		PRNINFO "Skip to install shellcheck package, because unknown to install it."
	fi
else
	PRNINFO "Skip to install shellcheck package, because shellcheck process does not need."
fi
PRNSUCCESS "Install shellcheck"

#==============================================================
# Processing
#==============================================================
#
# Change current directory
#
PRNTITLE "Change current directory"

if ! RUNCMD cd "${SRCTOP}"; then
	PRNERR "Failed to chnage current directory to ${SRCTOP}"
	exit 1
fi
PRNSUCCESS "Changed current directory"

#--------------------------------------------------------------
# Configuration
#--------------------------------------------------------------
#
# Before configuration
#
if [ "${RUN_PRE_CONFIG}" -eq 1 ]; then
	PRNTITLE "Before configration"
	if ({ run_pre_configuration 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Before configration\"."
		exit 1
	fi
	PRNSUCCESS "Before configration."
fi

#
# Configuration
#
if [ "${RUN_CONFIG}" -eq 1 ]; then
	PRNTITLE "Configration"
	if ({ run_configuration 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Configration\"."
		exit 1
	fi
	PRNSUCCESS "Configuration."
fi

#--------------------------------------------------------------
# Cleanup
#--------------------------------------------------------------
#
# Before Cleanup
#
if [ "${RUN_PRE_CLEANUP}" -eq 1 ]; then
	PRNTITLE "Before Cleanup"
	if ({ run_pre_cleanup 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Before Cleanup\"."
		exit 1
	fi
	PRNSUCCESS "Before Cleanup."
fi

#
# Cleanup
#
if [ "${RUN_CLEANUP}" -eq 1 ]; then
	PRNTITLE "Cleanup"
	if ({ run_cleanup 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Cleanup\"."
		exit 1
	fi
	PRNSUCCESS "Cleanup."
fi

#
# After Cleanup
#
if [ "${RUN_POST_CLEANUP}" -eq 1 ]; then
	PRNTITLE "After Cleanup"
	if ({ run_post_cleanup 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"After Cleanup\"."
		exit 1
	fi
	PRNSUCCESS "After Cleanup."
fi

#--------------------------------------------------------------
# Check before building
#--------------------------------------------------------------
#
# CppCheck
#
if [ "${RUN_CPPCHECK}" -eq 1 ]; then
	PRNTITLE "Check before building : CppCheck"
	if ({ run_cppcheck 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Check before building : CppCheck\"."
		exit 1
	fi
	PRNSUCCESS "Check before building : CppCheck."
fi

#
# ShellCheck
#
if [ "${RUN_SHELLCHECK}" -eq 1 ]; then
	PRNTITLE "Check before building : ShellCheck"
	if ({ run_shellcheck 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Check before building : ShellCheck\"."
		exit 1
	fi
	PRNSUCCESS "Check before building : ShellCheck."
fi

#
# Other check
#
if [ "${RUN_CHECK_OTHER}" -eq 1 ]; then
	PRNTITLE "Check before building : Other"
	if ({ run_othercheck 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Check before building : Other\"."
		exit 1
	fi
	PRNSUCCESS "Check before building : Other."
fi

#--------------------------------------------------------------
# Build
#--------------------------------------------------------------
#
# Before Build
#
if [ "${RUN_PRE_BUILD}" -eq 1 ]; then
	PRNTITLE "Before Build"
	if ({ run_pre_build 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Before Build\"."
		exit 1
	fi
	PRNSUCCESS "Before Build."
fi

#
# Build
#
if [ "${RUN_BUILD}" -eq 1 ]; then
	PRNTITLE "Build"
	if ({ run_build 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Build\"."
		exit 1
	fi
	PRNSUCCESS "Build."
fi

#
# After Build
#
if [ "${RUN_POST_BUILD}" -eq 1 ]; then
	PRNTITLE "After Build"
	if ({ run_post_build 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"After Build\"."
		exit 1
	fi
	PRNSUCCESS "After Build."
fi

#--------------------------------------------------------------
# Test
#--------------------------------------------------------------
#
# Before Test
#
if [ "${RUN_PRE_TEST}" -eq 1 ]; then
	PRNTITLE "Before Test"
	if ({ run_pre_test 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Before Test\"."
		exit 1
	fi
	PRNSUCCESS "Before Test."
fi

#
# Test
#
if [ "${RUN_TEST}" -eq 1 ]; then
	PRNTITLE "Test"
	if ({ run_test 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Test\"."
		exit 1
	fi
	PRNSUCCESS "Test."
fi

#
# After Test
#
if [ "${RUN_POST_TEST}" -eq 1 ]; then
	PRNTITLE "After Test"
	if ({ run_post_test 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"After Test\"."
		exit 1
	fi
	PRNSUCCESS "After Test."
fi

#--------------------------------------------------------------
# Package
#--------------------------------------------------------------
#
# Before Create Package
#
if [ "${RUN_PRE_PACKAGE}" -eq 1 ]; then
	PRNTITLE "Before Create Package"
	if ({ run_pre_create_package 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Before Create Package\"."
		exit 1
	fi
	PRNSUCCESS "Before Create Package."
fi

#
# Create Package
#
if [ "${RUN_PACKAGE}" -eq 1 ]; then
	PRNTITLE "Create Package"
	if ({ run_create_package 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Create Package\"."
		exit 1
	fi
	PRNSUCCESS "Create Package."
fi

#
# After Create Package
#
if [ "${RUN_POST_PACKAGE}" -eq 1 ]; then
	PRNTITLE "After Create Package"
	if ({ run_post_create_package 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"After Create Package\"."
		exit 1
	fi
	PRNSUCCESS "After Create Package."
fi

#--------------------------------------------------------------
# Publish Package
#--------------------------------------------------------------
#
# Publish Package
#
if [ "${RUN_PUBLISH_PACKAGE}" -eq 1 ]; then
	PRNTITLE "Publish Package"
	if ({ run_publish_package 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		PRNFAILURE "Failed \"Publish Package\"."
		exit 1
	fi
	PRNSUCCESS "Publish Package."
fi

#----------------------------------------------------------
# Finish
#----------------------------------------------------------
PRNSUCCESS "Finished all processing without error."

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
