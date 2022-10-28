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

#==============================================================
# Autobuild for debian package
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
MYSCRIPTDIR=$(cd "${MYSCRIPTDIR}" || exit 1; pwd)
SRCTOP=$(cd "${MYSCRIPTDIR}/.." || exit 1; pwd)

#
# Variables
#
BUILDDEBDIR="${SRCTOP}/debian_build"
NO_INTERACTIVE=0
BUILD_NUMBER=0
PACKAGE_NAME=""
PKG_CLASS_NAME=""
OS_VERSION_NAME=""
IS_ROOTDIR=0
DEBUILD_OPT=""
DH_MAKE_AUTORUN_OPTION=""

PRGNAME_NOEXT=$(echo "${PRGNAME}" | sed -e 's/[\.].*$//g' | tr -d '\n')
EXTRA_COPY_FILES_CONF="${MYSCRIPTDIR}/${PRGNAME_NOEXT}_copy.conf"

#
# Utility functions
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [--help(-h)] [--buildnum(-b) <build number>] [--product(-p) <product name>] [--class(-c) <class name>] [--disttype(-dt) <os/version>] [--rootdir(-r)] [--yes(-y)] [-- <additional debuild options>]"
	echo "        --help(-h)                     print help."
	echo "        --buildnum(-b) <build number>  specify build number for packaging(default 1)"
	echo "        --product(-p) <product name>   specify product name(use PACKAGE_NAME in Makefile s default)"
	echo "        --class(-c) <class name>       specify package class name(optional)"
	echo "        --disttype(-dt) <os/version>   specify \"OS/version name\", ex: ubuntu/jammy, debian/buster"
	echo "        --rootdir(-r)                  layout \"debian\" directory for packaging under source top directory"
	echo "        --yes(-y)                      runs no interactive mode."
	echo ""
}

get_default_package_class()
{
	if dh_make -h 2>/dev/null | grep -q '\--multi'; then
		printf 'multi'
	else
		printf 'library'
	fi
}

#
# Check options
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

	elif [ "$1" = "-c" ] || [ "$1" = "-C" ] || [ "$1" = "--class" ] || [ "$1" = "--CLASS" ]; then
		if [ -n "${PKG_CLASS_NAME}" ]; then
			echo "[ERROR] Already --class(-c) option is specified(${PKG_CLASS_NAME})." 1>&2
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] --class(-c) option need parameter." 1>&2
			exit 1
		fi
		PKG_CLASS_NAME="$1"

	elif [ "$1" = "-dt" ] || [ "$1" = "-DT" ] || [ "$1" = "--disttype" ] || [ "$1" = "--DISTTYPE" ]; then
		if [ -n "${OS_VERSION_NAME}" ]; then
			echo "[ERROR] Already --disttype(-dt) option is specified(${OS_VERSION_NAME})." 1>&2
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] --disttype(-dt) option need parameter." 1>&2
			exit 1
		fi
		OS_VERSION_NAME="$1"

	elif [ "$1" = "-r" ] || [ "$1" = "-R" ] || [ "$1" = "--rootdir" ] || [ "$1" = "--ROOTDIR" ]; then
		if [ "${IS_ROOTDIR}" -ne 0 ]; then
			echo "[ERROR] Already --rootdir(-r) option is specified." 1>&2
			exit 1
		fi
		IS_ROOTDIR=1

	elif [ "$1" = "-y" ] || [ "$1" = "-Y" ] || [ "$1" = "--yes" ] || [ "$1" = "--YES" ]; then
		if [ "${NO_INTERACTIVE}" -ne 0 ]; then
			echo "[ERROR] Already --yes(-y) option is specified." 1>&2
			exit 1
		fi
		NO_INTERACTIVE=1
		DH_MAKE_AUTORUN_OPTION="-y"

	elif [ "$1" = "--" ]; then
		if [ -n "${DEBUILD_OPT}" ]; then
			echo "[ERROR] Already --(additional debuild options) option is specified(${DEBUILD_OPT})." 1>&2
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] --(additional debuild options) option need parameter." 1>&2
			exit 1
		fi
		DEBUILD_OPT="$*"
		break

	else
		echo "[ERROR] Unknown option $1." 1>&2
		exit 1
	fi
	shift
done

#
# Check parameters
#
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
PACKAGE_DEV_NAME="${PACKAGE_NAME}-dev"
LIB_BASENAME="lib${PACKAGE_NAME}"

if [ "${BUILD_NUMBER}" -eq 0 ]; then
	BUILD_NUMBER=1
fi
if [ -z "${PKG_CLASS_NAME}" ]; then
	PKG_CLASS_NAME="$(get_default_package_class)"
fi
if [ -z "${OS_VERSION_NAME}" ]; then
	echo "[ERROR] --disttype(-dt) option is not specified." 1>&2
	exit 1
else
	if echo "${OS_VERSION_NAME}" | grep -q -i '^ubuntu/'; then
		OS_VERSION_NAME=$(echo "${OS_VERSION_NAME}" | sed -e 's#^ubuntu/##gi' | tr -d '\n')
	elif echo "${OS_VERSION_NAME}" | grep -q -i '^debian/'; then
		OS_VERSION_NAME=$(echo "${OS_VERSION_NAME}" | sed -e 's#^debian/##gi' | tr -d '\n')
	else
		echo "[ERROR] --disttype(-dt) option parameter must be ubuntu or debian(ex: \"ubuntu/jammy\")." 1>&2
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
# Create debian package directory
#
if [ -f "${BUILDDEBDIR}" ] || [ -d "${BUILDDEBDIR}" ]; then
	echo "[WANING] ${BUILDDEBDIR} directory(or file) exists, remove and remake it." 1>&2
	rm -rf "${BUILDDEBDIR}"
fi
if ! mkdir "${BUILDDEBDIR}"; then
	echo "[ERROR] Failed to create ${BUILDDEBDIR} directory." 1>&2
	exit 1
fi

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
# Get package version and major version
#
if ! PACKAGE_VERSION=$("${MYSCRIPTDIR}/make_variables.sh" --pkg_version); then
	echo "[ERROR] Failed to get package version(make_variables.sh)." 1>&2
	exit 1
fi
if ! PACKAGE_MAJOR_VER=$("${MYSCRIPTDIR}/make_variables.sh" --major_number); then
	echo "[ERROR] Failed to get package major version(make_variables.sh)." 1>&2
	exit 1
fi

#---------------------------------------------------------------
# Prepare base source codes
#---------------------------------------------------------------
#
# Create dist file(tar.gz)
#
echo ""
echo "[INFO] Create dist file(tar.gz)"
if ({ make dist || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] Failed to create dist package(make dist)." 1>&2
	exit 1
fi

#
# Change current directory to package top
#
cd "${BUILDDEBDIR}" || exit 1

#
# Copy dist file and expand source files
#
echo ""
echo "[INFO] Copy dist file(tar.gz) and expand it"
if ! cp "${SRCTOP}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz" "${BUILDDEBDIR}/."; then
	echo "[ERROR] Failed to copy ${SRCTOP}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz file to ${BUILDDEBDIR}" 1>&2
	exit 1
fi
if ({ tar xvfz "${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] Failed to expand source code from ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz" 1>&2
	exit 1
fi

#---------------------------------------------------------------
# Initialize files
#---------------------------------------------------------------
#
# Change current directory to source code
#
EXPANDDIR="${BUILDDEBDIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}"
cd "${EXPANDDIR}" || exit 1

#
# Initialize debian directory
#
echo ""
echo "[INFO] Initialize files in debian directory"
if [ -z "${LOGNAME}" ] && [ -z "${USER}" ]; then
	# [NOTE]
	# if run in docker container, Neither LOGNAME nor USER may be set in the environment variables.
	# dh_make needs one of these environments.
	#
	export USER="root"
	export LOGNAME="root"
fi
if ({ /bin/sh -c "dh_make -f ${BUILDDEBDIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz --createorig --${PKG_CLASS_NAME} ${DH_MAKE_AUTORUN_OPTION}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] Failed to run dh_make with ${BUILDDEBDIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz for initializing debian directory." 1>&2
	exit 1
fi

#
# Remove unnecessary template files
#
rm -rf "${EXPANDDIR}"/debian/*.ex "${EXPANDDIR}"/debian/*.EX "${EXPANDDIR}"/debian/"${PACKAGE_NAME}"-doc.* "${EXPANDDIR}"/debian/README.* "${EXPANDDIR}"/debian/docs "${EXPANDDIR}"/debian/*.install

#
# Copy copyright / control files
#
if ! cp "${MYSCRIPTDIR}/copyright" "${EXPANDDIR}/debian/copyright"; then
	echo "[ERROR] Failed to copy ${MYSCRIPTDIR}/copyright to ${EXPANDDIR}/debian directory." 1>&2
	exit 1
fi
if ! cp "${MYSCRIPTDIR}/control" "${EXPANDDIR}/debian/control"; then
	echo "[ERROR] Failed to copy ${MYSCRIPTDIR}/control to ${EXPANDDIR}/debian directory." 1>&2
	exit 1
fi

#
# Copy rules file with replacing placefolder
#
# [NOTE]
# Replaces "# [PLACEFOLDER CONFIGURE OPTION]" keyword(one comment line) in the rules
# file created from rules.in.
# If configure options are required, they are replaced as follows:
# -------------------------------------------------
# override_dh_auto_configure:
#     dh_auto_configure -- <configure options>
# -------------------------------------------------
#
if [ -z "${CONFIGUREOPT}" ]; then
	if ! sed -e "s/^# \[PLACEFOLDER CONFIGURE OPTION\].*$//g" "${MYSCRIPTDIR}/rules" > "${EXPANDDIR}/debian/rules"; then
		echo "[ERROR] Failed to copy(with cutting) ${MYSCRIPTDIR}/rules to ${EXPANDDIR}/debian directory." 1>&2
		exit 1
	fi
else
	if ! sed -e "s/^# \[PLACEFOLDER CONFIGURE OPTION\].*$/override_dh_auto_configure:\n\tdh_auto_configure -- ${CONFIGUREOPT}/g" "${MYSCRIPTDIR}/rules" > "${EXPANDDIR}/debian/rules"; then
		echo "[ERROR] Failed to copy(with converting) ${MYSCRIPTDIR}/rules to ${EXPANDDIR}/debian directory." 1>&2
		exit 1
	fi
fi

#
# Create links file for library
#
FOUND_LIB_LINES=$(find ./ -name Makefile.am -exec grep "${LIB_BASENAME}" "{}" \; 2>/dev/null)
if [ -n "${FOUND_LIB_LINES}" ]; then
	if ! LIBRARY_LIBTOOL_VERSION=$("${MYSCRIPTDIR}/make_variables.sh" --lib_version_for_link 2>/dev/null); then
		echo "[ERROR] Failed to get libtool version." 1>&2
		exit 1
	fi

	echo "usr/lib/x86_64-linux-gnu/${LIB_BASENAME}.so.${LIBRARY_LIBTOOL_VERSION} usr/lib/x86_64-linux-gnu/${LIB_BASENAME}.so"						>> "${EXPANDDIR}/debian/lib${PACKAGE_DEV_NAME}.links"	|| exit 1
	echo "usr/lib/x86_64-linux-gnu/${LIB_BASENAME}.so.${LIBRARY_LIBTOOL_VERSION} usr/lib/x86_64-linux-gnu/${LIB_BASENAME}.so.${PACKAGE_MAJOR_VER}"	>> "${EXPANDDIR}/debian/lib${PACKAGE_NAME}.links"		|| exit 1
fi

#
# Copy ChangeLog with converting build number
#
# [NOTE]
# The ChangeLog will include 'unstable', which will be replaced with
# the Version Code Name(ex. jammy).
#
if ! sed -e "s/[\(]${PACKAGE_VERSION}[\)]/\(${PACKAGE_VERSION}-${BUILD_NUMBER}\)/g" -e "s/[\)] unstable; /\) ${OS_VERSION_NAME}; /g" ChangeLog > "${EXPANDDIR}/debian/changelog"; then
	echo "[ERROR] Failed to create changelog file with converting." 1>&2
	exit 1
fi

#
# Check compat file
#
if [ ! -f "${EXPANDDIR}/debian/compat" ]; then
	echo "9" > "${EXPANDDIR}/debian/compat"
fi

#
# Copy preinst/postinst/prerm/postrm files (copy the file only if it exists)
#
PKG_INSTALL_SCRIPT_FILE_EXTS="
	preinst
	postinst
	prerm
	postrm
"
PKG_INSTALL_SCRIPT_PREFIXES="
	${PACKAGE_NAME}
	${PACKAGE_DEV_NAME}
	lib${PACKAGE_NAME}
	lib${PACKAGE_DEV_NAME}
"
for _inst_script_prefix in ${PKG_INSTALL_SCRIPT_PREFIXES}; do
	for _inst_script_ext in ${PKG_INSTALL_SCRIPT_FILE_EXTS}; do
		if [ -f "${MYSCRIPTDIR}/${_inst_script_prefix}.${_inst_script_ext}" ]; then
			if ! cp -p "${MYSCRIPTDIR}/${_inst_script_prefix}.${_inst_script_ext}" "${EXPANDDIR}/debian/${_inst_script_prefix}.${_inst_script_ext}"; then
				echo "[ERROR] Failed to copy ${_inst_script_prefix}.${_inst_script_ext} file." 1>&2
				exit 1
			fi
			if ! chmod +x "${EXPANDDIR}/debian/${_inst_script_prefix}.${_inst_script_ext}"; then
				echo "[ERROR] Failed to set attribute to ${_inst_script_prefix}.${_inst_script_ext} file." 1>&2
				exit 1
			fi
		fi
	done
done

#
# Copy extra files
#
# [NOTE]
# If you have files to copy under "<package build to pdirectory>/debian" directory
# (includes in your package), you can prepare "buildutils/debian_build_copy.conf"
# file and lists target files int it.
# The file names in this configuration file list with relative paths from the source
# top directory.
#	ex)	src/myfile
#		lib/mylib
#
if [ -f "${EXTRA_COPY_FILES_CONF}" ]; then
	EXTRA_COPY_FILES=$(sed -e 's/#.*$//g' -e 's/^[[:space:]]*//g' -e 's/[[:space:]]*$//g' -e '/^$/d' "${EXTRA_COPY_FILES_CONF}")
	for _extra_file in ${EXTRA_COPY_FILES}; do
		if [ ! -f "${SRCTOP}/${_extra_file}" ]; then
			echo "[ERROR] Not found ${SRCTOP}/${_extra_file} file for extra copy." 1>&2
			exit 1
		fi
		if ! cp -p "${SRCTOP}/${_extra_file}" "${EXPANDDIR}/debian/"; then
			echo "[ERROR] Failed to copy ${SRCTOP}/${_extra_file} file to ${EXPANDDIR}/debian." 1>&2
			exit 1
		fi
	done
fi

#
# Copy "package.install" file
#
FOUND_PACKAGE_INSTALL_FILES=$(find "${MYSCRIPTDIR}" -name \*"${PACKAGE_NAME}".install 2>/dev/null; find "${MYSCRIPTDIR}" -name \*"${PACKAGE_DEV_NAME}".install 2>/dev/null)

for _package_install_file in ${FOUND_PACKAGE_INSTALL_FILES}; do
	if ! cp -p "${_package_install_file}" "${EXPANDDIR}/debian/"; then
		echo "[ERROR] Failed to copy ${_package_install_file} file to ${EXPANDDIR}/debian." 1>&2
		exit 1
	fi
done

#
# Change debian directory to source top directory (--rootdir(-r) option is specified)
#
if [ "${IS_ROOTDIR}" -eq 1 ]; then
	if [ -f "${SRCTOP}/debian" ] || [ -d "${SRCTOP}/debian" ]; then
		echo "[WANING] ${SRCTOP}/debian directory(or file) exists, remove and remake it." 1>&2
		rm -rf "${SRCTOP}/debian"
	fi

	if ! cp -rp "${EXPANDDIR}/debian" "${SRCTOP}/."; then
		echo "[ERROR] Failed to copy ${EXPANDDIR}/debian directory to ${SRCTOP}." 1>&2
		exit 1
	fi

	#
	# Change package build directory and current directory
	#
	BUILDDEBDIR="${SRCTOP}"
	cd "${BUILDDEBDIR}" || exit 1
fi

#---------------------------------------------------------------
# Create packages
#---------------------------------------------------------------
#
# Run debuild
#
# [NOTE]
# If you don't give the -d option, it will fail on ubuntu16.04.
# This option can be removed if ubuntu16.04 is unsupported.
#
echo "[INFO] Create debian packages"
if ({ debuild -us -uc -d || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "[ERROR] Failed to run \"debuild -us -uc\"." 1>&2
	exit 1
fi

#
# Check and show debian package
#
# [NOTE] Check following files:
#	${PACKAGE_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb
#	${PACKAGE_DEV_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb
#	lib${PACKAGE_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb
#	lib${PACKAGE_DEV_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb
#
FOUND_DEB_PACKAGES=$(find "${BUILDDEBDIR}" -name \*"${PACKAGE_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}"\*.deb 2>/dev/null; find "${BUILDDEBDIR}" -name \*"${PACKAGE_DEV_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}"\*.deb 2>/dev/null)

if [ -z "${FOUND_DEB_PACKAGES}" ]; then
	echo "[ERROR] No debian package in ${BUILDDEBDIR}." 1>&2
	exit 1
fi
for _one_pkg in ${FOUND_DEB_PACKAGES}; do
	echo ""
	echo "[INFO] ${_one_pkg} package information"
	if ({ dpkg -c "${_one_pkg}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] Failed to print ${_one_pkg} package insformation by \"dpkg -c\"." 1>&2
		exit 1
	fi
	echo "    ---------------------------"

	if ({ dpkg -I "${_one_pkg}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "[ERROR] Failed to print ${_one_pkg} package insformation by \"dpkg -I\"." 1>&2
		exit 1
	fi
done

#---------------------------------------------------------------
# Finish
#---------------------------------------------------------------
echo ""
echo "[SUCCEED] You can find debian package for ${PACKAGE_NAME} : ${PACKAGE_VERSION}-${BUILD_NUMBER} version in ${BUILDDEBDIR} directory."
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
