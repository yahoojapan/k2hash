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
# CREATE:   Thu, Nov 22 2018
# REVISION:
#

#==============================================================
# Autobuild for rpm package
#==============================================================
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

#
# Common variables
#
PRGNAME=$(basename "${0}")
MYSCRIPTDIR=$(dirname "${0}")
SRCTOP=$(cd "${MYSCRIPTDIR}/.." || exit 1; pwd)

#
# Variables
#
RPM_TOPDIR="${SRCTOP}/rpmbuild"
NO_INTERACTIVE=0
BUILD_NUMBER=0
PACKAGE_NAME=""

#
# Utility functions
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [--help(-h)] [--buildnum(-b) <build number>] [--product(-p) <product name>] [--yes(-y)]"
	echo "        --help(-h)                     print help."
	echo "        --buildnum(-b) <build number>  specify build number for packaging(default 1)"
	echo "        --product(-p) <product name>   specify product name(use PACKAGE_NAME in Makefile s default)"
	echo "        --yes(-y)                      runs no interactive mode."
	echo ""
	echo "Environment:"
	echo "        CONFIGUREOPT                   specify options when running configure."
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

	elif [ "$1" = "-b" ] || [ "$1" = "-B" ] || [ "$1" = "--buildnum" ] || [ "$1" = "--BUILDNUM" ]; then
		if [ "${BUILD_NUMBER}" -ne 0 ]; then
			echo "[ERROR] Already --buildnum(-b) option is specified(${BUILD_NUMBER})." 1>&2
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] --buildnum(-b) option need parameter." 1>&2
			exit 1
		fi
		if echo "$1" | grep -q "[^0-9]"; then
			echo "[ERROR] --buildnum(-b) option parameter must be number(and not equal zero)." 1>&2
			exit 1
		fi
		if [ "$1" -eq 0 ]; then
			echo "[ERROR] --buildnum(-b) option parameter must be number(and not equal zero)." 1>&2
			exit 1
		fi
		BUILD_NUMBER="$1"

	elif [ "$1" = "-p" ] || [ "$1" = "-P" ] || [ "$1" = "--product" ] || [ "$1" = "--PRODUCT" ]; then
		if [ -n "${PACKAGE_NAME}" ]; then
			echo "[ERROR] Already --product(-p) option is specified(${PACKAGE_NAME})." 1>&2
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] --product(-p) option need parameter." 1>&2
			exit 1
		fi
		PACKAGE_NAME="$1"

	elif [ "$1" = "-y" ] || [ "$1" = "-Y" ] || [ "$1" = "--yes" ] || [ "$1" = "--YES" ]; then
		if [ "${NO_INTERACTIVE}" -ne 0 ]; then
			echo "[ERROR] Already --yes(-y) option is specified." 1>&2
			exit 1
		fi
		NO_INTERACTIVE=1

	else
		echo "[ERROR] Unknown option $1." 1>&2
		exit 1
	fi
	shift
done

#
# Check parameters
#
if [ "${BUILD_NUMBER}" -eq 0 ]; then
	BUILD_NUMBER=1
fi
if [ -z "${PACKAGE_NAME}" ]; then
	#
	# Get default package name from Makefile or ChangeLog
	#
	if [ -f "${SRCTOP}/Makefile" ]; then
		PACKAGE_NAME=$(grep '^PACKAGE_NAME' "${SRCTOP}"/Makefile 2>/dev/null | awk '{print $3}' | tr -d '\n')
	fi
	if [ -z "${PACKAGE_NAME}" ] && [ -f "${SRCTOP}/ChangeLog" ] ; then
		PACKAGE_NAME=$(grep -v '^$' "${SRCTOP}"/ChangeLog | grep -v '^[[:space:]]' | head -1 | awk '{print $1}' | tr -d '\n')
	fi
	if [ -z "${PACKAGE_NAME}" ]; then
		echo "[ERROR] Could not get package name from Makefile or ChangeLog, please use --product(-p) option." 1>&2
		exit 1
	fi
fi

#
# Welcome message and confirming for interactive mode
#
if [ "${NO_INTERACTIVE}" -eq 0 ]; then
	echo "---------------------------------------------------------------"
	echo " Do you change these file and commit to github?"
	echo " - ChangeLog     modify / add changes like dch tool format"
	echo " - Git TAG       stamp git tag for release"
	echo "---------------------------------------------------------------"
	IS_CONFIRMED=0
	while [ "${IS_CONFIRMED}" -eq 0 ]; do
		printf '[INPUT] Confirm (y/n) : '
		read -r CONFIRM

		if [ "${CONFIRM}" = "y" ] || [ "${CONFIRM}" = "Y" ] || [ "${CONFIRM}" = "yes" ] || [ "${CONFIRM}" = "YES" ]; then
			IS_CONFIRMED=1
		elif [ "${CONFIRM}" = "n" ] || [ "${CONFIRM}" = "N" ] || [ "${CONFIRM}" = "no" ] || [ "${CONFIRM}" = "NO" ]; then
			echo "Interrupt this processing, bye..."
			exit 0
		fi
	done
	echo ""
fi

#===============================================================
# Main processing
#===============================================================
cd "${SRCTOP}" || exit 1

#
# create rpm top directory and etc
#
_SUB_RPM_DIRS="BUILD BUILDROOT RPM SOURCES SPECS SRPMS"
for _SUB_RPM_DIR in ${_SUB_RPM_DIRS}; do
	if ! mkdir -p "${RPM_TOPDIR}/${_SUB_RPM_DIR}"; then
		echo "[ERROR] Could not make ${RPM_TOPDIR}/${_SUB_RPM_DIR} directory." 1>&2
		exit 1
	fi
done

#---------------------------------------------------------------
# Prepare base source codes
#---------------------------------------------------------------
#
# Run configure for package version
#
echo "[INFO] Run autogen"
if ({ "${SRCTOP}"/autogen.sh || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] Failed to run autogen.sh." 1>&2
	exit 1
fi
echo ""
echo "[INFO] Run configure"
if ({ /bin/sh -c "${SRCTOP}/configure ${CONFIGUREOPT}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] Failed to run configure." 1>&2
	exit 1
fi

#
# Get package version
#
if ! PACKAGE_VERSION=$("${MYSCRIPTDIR}/make_variables.sh" --pkg_version); then
	echo "[ERROR] Failed to get package version(make_variables.sh)." 1>&2
	exit 1
fi

#
# create archive file(tar.gz)
#
echo ""
echo "[INFO] Create base source code file(tar.gz)"
if [ -d "${SRCTOP}/.git" ] && [ "$(git status -s 2>&1 | wc -l)" -eq 0 ]; then
	#
	# No untracked files
	#
	RUN_AUTOGEN_FLAG=""

	#
	# make source package(tar.gz) by git archive
	#
	if ({ git archive HEAD --prefix="${PACKAGE_NAME}-${PACKAGE_VERSION}/" --output="${RPM_TOPDIR}/SOURCES/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] Could not make source tar ball(${RPM_TOPDIR}/SOURCES/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz) from github repository." 1>&2
		exit 1
	fi

else
	#
	# Found untracked files or Not have .git directory
	#
	RUN_AUTOGEN_FLAG='--define "not_run_autogen 1"'

	#
	# make dist package(tar.gz) and copy it
	#
	if ({ make dist || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] Failed to create dist package(make dist)." 1>&2
		exit 1
	fi
	if ! cp -p "${SRCTOP}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz" "${RPM_TOPDIR}/SOURCES/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz"; then
		echo "[ERROR] Failed to copy ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz to ${RPM_TOPDIR}/SOURCES/." 1>&2
		exit 1
	fi
fi

#---------------------------------------------------------------
# Initialize files
#---------------------------------------------------------------
#
# Copy spec file to root
#
if ! cp "${SRCTOP}"/buildutils/*.spec "${SRCTOP}"/; then
	echo "[ERROR] Failed to copy spec files to sour top directory." 1>&2
	exit 1
fi

#---------------------------------------------------------------
# Create packages
#---------------------------------------------------------------
#
# build RPM packages
#
echo "[INFO] Create RPM packages"
if ({ /bin/sh -c "rpmbuild -vv -ba ${RUN_AUTOGEN_FLAG} --define \"_topdir ${RPM_TOPDIR}\" --define \"_prefix /usr\" --define \"_mandir /usr/share/man\" --define \"_defaultdocdir /usr/share/doc\" --define \"package_revision ${BUILD_NUMBER}\" *.spec" 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] Failed to build rpm packages by rpmbuild." 1>&2
	exit 1
fi

#
# Copy RPM files to package directory for uploading
#
if ! cp "${SRCTOP}"/rpmbuild/RPMS/*/*.rpm "${SRCTOP}"/; then
	echo "[ERROR] Failed to copy rpm package files to ${SRCTOP} directory." 1>&2
	exit 1
fi
if ! cp "${SRCTOP}"/rpmbuild/SRPMS/*.rpm "${SRCTOP}"/; then
	echo "[ERROR] Failed to copy source rpm package files to ${SRCTOP} directory." 1>&2
	exit 1
fi

#---------------------------------------------------------------
# Finish
#---------------------------------------------------------------
echo ""
echo "[SUCCEED] You can find RPM package ${PACKAGE_NAME} : ${PACKAGE_VERSION}-${BUILD_NUMBER} version in ${SRCTOP} directory."
echo ""

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
