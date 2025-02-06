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
# CREATE:   Fri, Jan 13 2023
# REVISION: 1.2
#

#==============================================================
# Autobuild for apk package
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

#
# Common variables
#
PRGNAME=$(basename "${0}")
MYSCRIPTDIR=$(dirname "${0}")
SRCTOP=$(cd "${MYSCRIPTDIR}/.." || exit 1; pwd)
SRCTOP_DIRNAME=$(basename "${SRCTOP}")

#
# Variables
#
APK_TOPDIR="${SRCTOP}/apk_build"
APKBUILD_TEMPLATE_FILE="${SRCTOP}/buildutils/APKBUILD.templ"
APKBUILD_FILE="${APK_TOPDIR}/APKBUILD"
APKBUILD_CONFIG_DIR="${HOME}/.abuild"
APK_KEYS_DIR="/etc/apk/keys"
MAKE_VARIABLES_TOOL="${SRCTOP}/buildutils/make_variables.sh"

PRGNAME_NOEXT=$(echo "${PRGNAME}" | sed -e 's/[\.].*$//g' | tr -d '\n')
EXTRA_COPY_FILES_CONF="${MYSCRIPTDIR}/${PRGNAME_NOEXT}_copy.conf"

#---------------------------------------------------------------
# Utility functions
#---------------------------------------------------------------
func_usage()
{
	echo ""
	echo "Usage:  $1 [--help(-h)] [--buildnum(-b) <build number>] [--product(-p) <product name>] [--yes(-y)]"
	echo "        --help(-h)                     print help."
	echo "        --buildnum(-b) <build number>  specify build number for packaging(default 1)"
	echo "        --product(-p) <product name>   specify product name(use PACKAGE_NAME in Makefile s default), this value is using a part of package directory path"
	echo "        --yes(-y)                      runs no interactive mode."
	echo ""
	echo "Environment:"
	echo "        CONFIGUREOPT                   specify options when running configure."
	echo "        GITHUB_REF_TYPE                Github Actions CI sets \"branch\" or \"tag\"."
	echo "        DEBEMAIL                       Use this value to create an RSA key."
	echo "        DEBFULLNAME                    Use this value to create an RSA key."
	echo ""
}

#---------------------------------------------------------------
# Parse parameters
#---------------------------------------------------------------
NO_INTERACTIVE=0
BUILD_NUMBER=
PACKAGE_NAME=""

while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		break

	elif echo "$1" | grep -q -i -e "^-h$" -e "^--help$"; then
		func_usage "${PRGNAME}"
		exit 0

	elif echo "$1" | grep -q -i -e "^-b$" -e "^--buildnum$"; then
		if [ -n "${BUILD_NUMBER}" ]; then
			echo "[ERROR] Already --buildnum(-b) option is specified(${BUILD_NUMBER})." 1>&2
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] --buildnum(-b) option need parameter." 1>&2
			exit 1
		fi
		if echo "$1" | grep -q "[^0-9]"; then
			echo "[ERROR] --buildnum(-b) option parameter must be number." 1>&2
			exit 1
		fi
		BUILD_NUMBER="$1"

	elif echo "$1" | grep -q -i -e "^-p$" -e "^--product$"; then
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

	elif echo "$1" | grep -q -i -e "^-y$" -e "^--yes$"; then
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
# Check input parameters
#
# [NOTE]
# APK package revision starts "0", it is default value.
#
if [ -z "${BUILD_NUMBER}" ]; then
	BUILD_NUMBER=0
elif [ "${BUILD_NUMBER}" -gt 0 ]; then
	BUILD_NUMBER=$((BUILD_NUMBER - 1))
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

#---------------------------------------------------------------
# Create a directory for building APK packages.
#---------------------------------------------------------------
# [NOTE]
# This directory must be created to store RSA keys.
#
if [ -d "${APK_TOPDIR}" ] || [ -f "${APK_TOPDIR}" ]; then
	rm -rf "${APK_TOPDIR}"
fi
if ! mkdir -p "${APK_TOPDIR}"; then
	echo "[ERROR] Could not make ${APK_TOPDIR} directory." 1>&2
	exit 1
fi

#---------------------------------------------------------------
# Check Environments and variables
#---------------------------------------------------------------
# [NOTE]
# This script sets variables in the APKBUILD file from environment
# and variables.
# Use the following environment variables and variables.
#
#	DEBEMAIL						: Used for creating RSA keys
#	DEBFULLNAME						: Used for creating RSA keys
#	CONFIGUREOPT					: Parameters passed to configure.
#									  This variable is automatically passed when invoking this
#									  script.
#	GITHUB_REF_TYPE					: This environment variable is set by GithubActions.
#									  For release tagging, this value is set to "tag".
#									  Unset and otherwise("branch") indicate that the call was
#									  not made by the release process.
#	BUILD_NUMBER					: Parameter or default value("0") for this script.
#	ABUILD_OPT						: abuild common option for running as root user.
#

if [ -z "${DEBEMAIL}" ]; then
	echo "[WARNING] DEBEMAIL environment is not set, so set default \"${USER}@$(hostname)\"" 1>&2
	DEBEMAIL="${USER}@$(hostname)"
fi

if [ -z "${DEBFULLNAME}" ]; then
	echo "[WARNING] DEBFULLNAME environment is not set, so set default \"${USER}\"." 1>&2
	DEBFULLNAME="${USER}"
fi

if [ -z "${GITHUB_REF_TYPE}" ] || [ "${GITHUB_REF_TYPE}" != "tag" ]; then
	SOURCE_ARCHIVE_URL=""
	SOURCE_ARCHIVE_SEPARATOR=""
else
	SOURCE_ARCHIVE_URL="https://\$_git_domain/\$_organization_name/\$_repository_name/archive/refs/tags/v\$pkgver.tar.gz"
	SOURCE_ARCHIVE_SEPARATOR="::"
fi

#
# Check running as root user
#
RUN_USER_ID=$(id -u)

if [ -n "${RUN_USER_ID}" ] && [ "${RUN_USER_ID}" -eq 0 ]; then
	SUDO_CMD=""
else
	SUDO_CMD="sudo"
fi

# [NOTE]
# The abuild tool drains errors when run as root.
# To avoid this, the "-F" option is required.
# If you need verbose message, you can add "-v" option here.
#
if [ -n "${RUN_USER_ID}" ] && [ "${RUN_USER_ID}" -eq 0 ]; then
	ABUILD_OPT="-F"
else
	ABUILD_OPT=""
fi

#---------------------------------------------------------------
# Print information
#---------------------------------------------------------------
#
# Information
#
echo "[INFO] Information for building APK packages."
echo ""
echo "    APK_TOPDIR                = ${APK_TOPDIR}"
echo "    PACKAGE_NAME              = ${PACKAGE_NAME}"
echo "    NO_INTERACTIVE            = ${NO_INTERACTIVE}"
echo ""
echo "    GITHUB_REF_TYPE           = ${GITHUB_REF_TYPE}"
echo "    CONFIGUREOPT              = ${CONFIGUREOPT}"
echo "    SOURCE_ARCHIVE_URL        = ${SOURCE_ARCHIVE_URL}"
echo "    BUILD_NUMBER              = ${BUILD_NUMBER}"
echo "    ABUILD_OPT                = ${ABUILD_OPT}"
echo ""

#
# Welcome message and confirming for interactive mode
#
if [ "${NO_INTERACTIVE}" -eq 0 ]; then
	echo " Do you create APK packages with above variables?"
	IS_CONFIRMED=0
	while [ "${IS_CONFIRMED}" -eq 0 ]; do
		printf ' [INPUT] Confirm (y/n) : '
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

#---------------------------------------------------------------
# Create RSA key for signing
#---------------------------------------------------------------
echo "[TITLE] Create RSA key for signing"

#
# Determining the RSA key location directory
#
# [NOTE]
# This directory path depends on the apk-tools version.
# It is different for 2.14.4 and later and previous versions.
#
APKTOOLS_ALL_VER=$(apk list apk-tools | awk '{print $1}' | tail -1)
APKTOOLS_MAJOR_VER=$(echo "${APKTOOLS_ALL_VER}" | sed -e 's#^[[:space:]]*apk-tools-##g' -e 's#-# #g' -e 's#\.# #g' | awk '{print $1}')
APKTOOLS_MINOR_VER=$(echo "${APKTOOLS_ALL_VER}" | sed -e 's#^[[:space:]]*apk-tools-##g' -e 's#-# #g' -e 's#\.# #g' | awk '{print $2}')
APKTOOLS_PATCH_VER=$(echo "${APKTOOLS_ALL_VER}" | sed -e 's#^[[:space:]]*apk-tools-##g' -e 's#-# #g' -e 's#\.# #g' | awk '{print $3}')
if [ -z "${APKTOOLS_MAJOR_VER}" ] || [ -z "${APKTOOLS_MINOR_VER}" ] || [ -z "${APKTOOLS_PATCH_VER}" ]; then
	echo "[ERROR] Could not get aok-tools package version." 1>&2
	exit 1
fi
APKTOOLS_MIX_VER=$((APKTOOLS_MAJOR_VER * 1000 * 1000 + APKTOOLS_MINOR_VER * 1000 + APKTOOLS_PATCH_VER))
if [ "${APKTOOLS_MIX_VER}" -lt 2014004 ]; then
	RSA_KEYS_DIR="${APK_TOPDIR}"
else
	RSA_KEYS_DIR="${APK_KEYS_DIR}"
fi

#
# Check "${HOME}/.abuild" directory
#
if [ -d "${APKBUILD_CONFIG_DIR}" ] || [ -f "${APKBUILD_CONFIG_DIR}" ]; then
	rm -rf "${APKBUILD_CONFIG_DIR}"
fi

#
# Create temporary RSA key
#
# [NOTE]
# The keys that are created are the following files:
#	${APKBUILD_CONFIG_DIR}/${DEBEMAIL}-<unixtime(hex)>.rsa
#	${APKBUILD_CONFIG_DIR}/${DEBEMAIL}-<unixtime(hex)>.rsa.pub
#
if ({ PACKAGER="${DEBFULLNAME} <${DEBEMAIL}>" abuild-keygen -a -n 2>&1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's#^#    #') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] Failed to create temporary RSA key for building APK packages." 1>&2
	exit 1
fi

#
# Check and Copy RSA keys
#
if ! find "${APKBUILD_CONFIG_DIR}" -name "${DEBEMAIL}"-\*\.rsa | grep -q "${DEBEMAIL}"; then
	echo "[ERROR] Not found ${APKBUILD_CONFIG_DIR}/${DEBEMAIL}-<unixtime(hex)>.rsa files." 1>&2
	exit 1
fi
if ! find "${APKBUILD_CONFIG_DIR}" -name "${DEBEMAIL}"-\*\.rsa\.pub | grep -q "${DEBEMAIL}"; then
	echo "[ERROR] Not found ${APKBUILD_CONFIG_DIR}/${DEBEMAIL}-<unixtime(hex)>.rsa.pub files." 1>&2
	exit 1
fi
if ! /bin/sh -c "${SUDO_CMD} cp -p ${APKBUILD_CONFIG_DIR}/${DEBEMAIL}-*.rsa ${RSA_KEYS_DIR} >/dev/null 2>&1"; then
	echo "[ERROR] Failed to copy RSA private key(${APKBUILD_CONFIG_DIR}/${DEBEMAIL}-<unixtime(hex)>.rsa) to ${RSA_KEYS_DIR} directory." 1>&2
	exit 1
fi
if ! /bin/sh -c "${SUDO_CMD} cp -p ${APKBUILD_CONFIG_DIR}/${DEBEMAIL}-*.rsa.pub ${RSA_KEYS_DIR} >/dev/null 2>&1"; then
	echo "[ERROR] Failed to copy RSA public key(${APKBUILD_CONFIG_DIR}/${DEBEMAIL}-<unixtime(hex)>.rsa.pub) to ${RSA_KEYS_DIR} directory." 1>&2
	exit 1
fi

#
# Remove abuild configuration directory(with abuild.conf)
#
# [NOTE]
# When building the package, the RSA key is specified by an environment
# variable, so the presence of this configuration file is misleading.
#
rm -rf "${APKBUILD_CONFIG_DIR}"

#
# Set file name/key contents to variables
#
APK_PACKAGE_PRIV_KEYNAME="$(find "${RSA_KEYS_DIR}" -name "${DEBEMAIL}"-\*\.rsa 2>/dev/null | head -1 | sed -e "s#${RSA_KEYS_DIR}/##g" | tr -d '\n')"
APK_PACKAGE_PUB_KEYNAME="$(find "${RSA_KEYS_DIR}" -name "${DEBEMAIL}"-\*\.rsa\.pub 2>/dev/null | head -1 | sed -e "s#${RSA_KEYS_DIR}/##g" | tr -d '\n')"

echo "[SUCCEED] Created RSA keys"
echo "    RSA private key : ${RSA_KEYS_DIR}/${APK_PACKAGE_PRIV_KEYNAME}"
echo "    RSA public key  : ${RSA_KEYS_DIR}/${APK_PACKAGE_PUB_KEYNAME}"
echo ""

#---------------------------------------------------------------
# Create source archive ( Not release processing )
#---------------------------------------------------------------
echo "[TITLE] Source archive"

if [ -z "${GITHUB_REF_TYPE}" ] || [ "${GITHUB_REF_TYPE}" != "tag" ]; then
	SOURCE_ARCHIVE_VERSION=$("${MAKE_VARIABLES_TOOL}" --pkg_version)

	#
	# Create source archive by make dist
	#
	if ({ make dist || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's#^#    #') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] Failed to create source archive." 1>&2
		exit 1
	fi
	if [ ! -f "${SRCTOP}/${PACKAGE_NAME}-${SOURCE_ARCHIVE_VERSION}".tar.gz ]; then
		echo "[ERROR] Not found created source archive(${PACKAGE_NAME}-${SOURCE_ARCHIVE_VERSION}.tar.gz)." 1>&2
		exit 1
	fi

	#
	# Copy source archive file
	#
	if ! cp "${SRCTOP}/${PACKAGE_NAME}-${SOURCE_ARCHIVE_VERSION}".tar.gz "${APK_TOPDIR}"; then
		echo "[ERROR] Copy source archive(${PACKAGE_NAME}-${SOURCE_ARCHIVE_VERSION}.tar.gz) to ${APK_TOPDIR} directory." 1>&2
		exit 1
	fi

	echo "[SUCCEED] Created source archive"
	echo "    Source archive file : ${SRCTOP}/${PACKAGE_NAME}-${SOURCE_ARCHIVE_VERSION}.tar.gz"
	echo ""
else
	echo "[SUCCEED] Source archive"
	echo "    Source archive URL : ${SOURCE_ARCHIVE_URL}"
	echo ""
fi

#---------------------------------------------------------------
# Copy extra files
#---------------------------------------------------------------
# [NOTE]
# If you have files to copy under "<package build to pdirectory>/apk_build" directory
# (includes in your package), you can prepare "buildutils/alpine_build_copy.conf"
# file and lists target files int it.
# The file names in this configuration file list with relative paths from the source
# top directory.
#	ex)	src/myfile
#		lib/mylib
#
if [ -f "${EXTRA_COPY_FILES_CONF}" ]; then
	echo "[TITLE] Copy extra files"

	EXTRA_COPY_FILES=$(sed -e 's/#.*$//g' -e 's/^[[:space:]]*//g' -e 's/[[:space:]]*$//g' -e '/^$/d' "${EXTRA_COPY_FILES_CONF}")
	for _extra_file in ${EXTRA_COPY_FILES}; do
		if [ ! -f "${SRCTOP}/${_extra_file}" ]; then
			echo "[ERROR] Not found ${SRCTOP}/${_extra_file} file for extra copy." 1>&2
			exit 1
		fi
		if ! cp -p "${SRCTOP}/${_extra_file}" "${APK_TOPDIR}"; then
			echo "[ERROR] Failed to copy ${SRCTOP}/${_extra_file} file to ${APK_TOPDIR}." 1>&2
			exit 1
		fi
	done

	echo "[SUCCEED] Copied extra files"
fi

#---------------------------------------------------------------
# Create APKBUILD file
#---------------------------------------------------------------
echo "[TITLE] Create APKBUILD file from template"

#
# Create APKBUILD file from template
#
if ! sed -e "s#%%BUILD_NUMBER%%#${BUILD_NUMBER}#g"											\
		-e "s#%%CONFIGUREOPT%%#${CONFIGUREOPT}#g"											\
		-e "s#%%SOURCE_ARCHIVE_URL%%#${SOURCE_ARCHIVE_SEPARATOR}${SOURCE_ARCHIVE_URL}#g"	\
		"${APKBUILD_TEMPLATE_FILE}" > "${APKBUILD_FILE}"									; then

	echo "[ERROR] Failed to create APKBUILD file in ${APK_TOPDIR} directory." 1>&2
	exit 1
fi

#
# Add checksum to APKBUILD file
#
cd "${APK_TOPDIR}" || exit 1

if ! /bin/sh -c "abuild ${ABUILD_OPT} checksum"; then
	echo "[ERROR] Failed to add checksum to APKBUILD file(${APKBUILD_FILE})." 1>&2
	exit 1
fi

#
# Get Arch name for a part of package directory path
#
ARCH_NAME=$(grep '^arch=' "${APKBUILD_FILE}" | sed -e 's#^arch=##g' -e 's#"##g' | tail -1 | tr -d '\n')

#
# Print APKBUILD file
#
echo "[SUCCEED] Created APKBUILD file(${APKBUILD_FILE})"
echo ""
sed -e 's#^#    #g' "${APKBUILD_FILE}"
echo ""

#---------------------------------------------------------------
# Create packages
#---------------------------------------------------------------
echo "[TITLE] Build APK packages."

#
# build APK packages
#
if ({ /bin/sh -c "PACKAGER_PRIVKEY=${RSA_KEYS_DIR}/${APK_PACKAGE_PRIV_KEYNAME} abuild ${ABUILD_OPT} -r -P $(pwd)" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's#^#    #') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] Failed to create APK packages." 1>&2
	exit 1
fi
echo "[SUCCEED] Built APK packages"

#
# Show APK packages
#
for _one_pkg in "${APK_TOPDIR}"/"${SRCTOP_DIRNAME}"/"${ARCH_NAME}"/*.apk; do
	echo ""
	echo "[INFO] Dump APK package : ${_one_pkg}"

	if ({ tar tvfz "${_one_pkg}" 2>/dev/null || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's#^#    #') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] Failed to extract ${_one_pkg} APK package." 1>&2
		exit 1
	fi
done

#
# Copy APK packages
#
if ! cp -p "${APK_TOPDIR}"/"${SRCTOP_DIRNAME}"/"${ARCH_NAME}"/*.apk "${APK_TOPDIR}"; then
	echo "[ERROR] Failed to copy ${APK_TOPDIR}/${SRCTOP_DIRNAME}/${ARCH_NAME}/*.apk package to ${APK_TOPDIR} directory." 1>&2
	exit 1
fi

#---------------------------------------------------------------
# Finish
#---------------------------------------------------------------
echo ""
echo "[SUCCEED] You can find APK package in ${APK_TOPDIR} directory"
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
