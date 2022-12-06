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
# Puts project version/revision/age/etc variables for building
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
EXCLUSIVE_OPT=0
PRGMODE=""
DEB_WITH_SYSTEMD=0

#
# Utility functions
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [--help(-h)] [--pkg_version(-pv) | --lib_version_info(-lvi) | --lib_version_for_link(-lvl) | --major_number(-mn) | --debhelper_dep(-dd) | --debhelper_dep_with_systemd(-dds) | --rpmpkg_group(-rg)]"
	echo "  --help(-h)                    print help"
	echo "	--pkg_version(-pv)            returns package version."
	echo "	--lib_version_info(-lvi)      returns library libtools revision"
	echo "	--lib_version_for_link(-lvl)  return library version for symbolic link"
	echo "	--major_number(-mn)           return major version number"
	echo "	--debhelper_dep(-dd)          return debhelper dependency string"
	echo "	--rpmpkg_group(-rg)           return group string for rpm package"
	echo ""
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

	elif [ "$1" = "-pv" ] || [ "$1" = "-PV" ] || [ "$1" = "--pkg_version" ] || [ "$1" = "--PKG_VERSION" ]; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="PKG"
		EXCLUSIVE_OPT=1

	elif [ "$1" = "-lvi" ] || [ "$1" = "-LVI" ] || [ "$1" = "--lib_version_info" ] || [ "$1" = "--LIB_VERSION_INFO" ]; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="LIB"
		EXCLUSIVE_OPT=1

	elif [ "$1" = "-lvl" ] || [ "$1" = "-LVL" ] || [ "$1" = "--lib_version_for_link" ] || [ "$1" = "--LIB_VERSION_FOR_LINK" ]; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="LINK"
		EXCLUSIVE_OPT=1

	elif [ "$1" = "-mn" ] || [ "$1" = "-MN" ] || [ "$1" = "--major_number" ] || [ "$1" = "--MAJOR_NUMBER" ]; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="MAJOR"
		EXCLUSIVE_OPT=1

	elif [ "$1" = "-dd" ] || [ "$1" = "-DD" ] || [ "$1" = "--debhelper_dep" ] || [ "$1" = "--DEBHELPER_DEP" ]; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="DEBHELPER"
		DEB_WITH_SYSTEMD=0
		EXCLUSIVE_OPT=1

	elif [ "$1" = "-dds" ] || [ "$1" = "-DDS" ] || [ "$1" = "--debhelper_dep_with_systemd" ] || [ "$1" = "--DEBHELPER_DEP_WITH_SYSTEMD" ]; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="DEBHELPER"
		DEB_WITH_SYSTEMD=1
		EXCLUSIVE_OPT=1

	elif [ "$1" = "-rg" ] || [ "$1" = "-RG" ] || [ "$1" = "--rpmpkg_group" ] || [ "$1" = "--RPMPKG_GROUP" ]; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="RPMGROUP"
		EXCLUSIVE_OPT=1

	else
		echo "[ERROR] unknown option $1" 1>&2
		printf '0'
		exit 1
	fi
	shift
done
if [ -z "${PRGMODE}" ]; then
	echo "[ERROR] option is not specified." 1>&2
	printf '0'
	exit 1
fi

#
# Make result
#
if [ "${PRGMODE}" = "PKG" ]; then
	RESULT=$(cat "${RELEASE_VERSION_FILE}")

elif [ "${PRGMODE}" = "LIB" ] || [ "${PRGMODE}" = "LINK" ]; then
	#
	# get version numbers
	#
	MAJOR_VERSION=$(sed -e 's/["|\.]/ /g' "${RELEASE_VERSION_FILE}" | awk '{print $1}')
	MID_VERSION=$(sed -e 's/["|\.]/ /g' "${RELEASE_VERSION_FILE}" | awk '{print $2}')
	LAST_VERSION=$(sed -e 's/["|\.]/ /g' "${RELEASE_VERSION_FILE}" | awk '{print $3}')

	#
	# check version number
	#
	if echo "${MAJOR_VERSION}" | grep -q "[^0-9]"; then
		echo "[ERROR] wrong version number in RELEASE_VERSION file" 1>&2
		printf '0'
		exit 1
	fi
	if echo "${MID_VERSION}" | grep -q "[^0-9]"; then
		echo "[ERROR] wrong version number in RELEASE_VERSION file" 1>&2
		printf '0'
		exit 1
	fi
	if echo "${LAST_VERSION}" | grep -q "[^0-9]"; then
		echo "[ERROR] wrong version number in RELEASE_VERSION file" 1>&2
		printf '0'
		exit 1
	fi

	#
	# make library revision number
	#
	if [ "${MID_VERSION}" -gt 0 ]; then
		REV_VERSION=$((MID_VERSION * 100))
		REV_VERSION=$((LAST_VERSION + REV_VERSION))
	else
		REV_VERSION="${LAST_VERSION}"
	fi

	if [ "${PRGMODE}" = "LIB" ]; then
		RESULT="${MAJOR_VERSION}:${REV_VERSION}:0"
	else
		RESULT="${MAJOR_VERSION}.0.${REV_VERSION}"
	fi

elif [ "${PRGMODE}" = "MAJOR" ]; then
	RESULT=$(sed -e 's/["|\.]/ /g' "${RELEASE_VERSION_FILE}" | awk '{print $1}')

elif [ "${PRGMODE}" = "DEBHELPER" ]; then
	# [NOTE]
	# This option returns debhelper dependency string in control file for debian package.
	# That string is depended debhelper package version and os etc.
	# (if not ubuntu/debian os, returns default string)
	#
	OS_ID_STRING=$(grep '^ID[[:space:]]*=[[:space:]]*' /etc/os-release | sed -e 's|^ID[[:space:]]*=[[:space:]]*||g' -e 's|^[[:space:]]*||g' -e 's|[[:space:]]*$||g')

	DEBHELPER_MAJOR_VER=$(apt-cache show debhelper 2>/dev/null | grep Version 2>/dev/null | awk '{print $2}' 2>/dev/null | sed -e 's/\..*/ /g' 2>/dev/null)

	if echo "${DEBHELPER_MAJOR_VER}" | grep -q "[^0-9]"; then
		DEBHELPER_MAJOR_VER=0
	fi

	if [ "${DEB_WITH_SYSTEMD}" -eq 1 ]; then
		DEB_WITH_SYSTEMD_STRING=" | dh-systemd"
	else
		DEB_WITH_SYSTEMD_STRING=""
	fi

	if [ -n "${OS_ID_STRING}" ]; then
		if [ "${OS_ID_STRING}" = "debian" ]; then
			RESULT="debhelper (>= 9.20160709)${DEB_WITH_SYSTEMD_STRING}, autotools-dev"

		elif [ "${OS_ID_STRING}" = "ubuntu" ]; then
			if [ "${DEBHELPER_MAJOR_VER}" -lt 10 ]; then
				RESULT="debhelper (>= 9.20160709)${DEB_WITH_SYSTEMD_STRING}, autotools-dev"
			else
				RESULT="debhelper (>= 9.20160709)${DEB_WITH_SYSTEMD_STRING}"
			fi
		else
			# Not debian/ubuntu, set default
			RESULT="debhelper (>= 9.20160709)${DEB_WITH_SYSTEMD_STRING}, autotools-dev"
		fi
	else
		# Unknown OS, set default
		RESULT="debhelper (>= 9.20160709)${DEB_WITH_SYSTEMD_STRING}, autotools-dev"
	fi

elif [ "${PRGMODE}" = "RPMGROUP" ]; then
	# [NOTE]
	# Fedora rpm does not need "Group" key in spec file.
	# If not fedora, returns "NEEDRPMGROUP", and you must replace this string in configure.ac
	#
	if grep -q '^ID[[:space:]]*=[[:space:]]*["]*fedora["]*[[:space:]]*$' /etc/os-release; then
		RESULT=""
	else
		RESULT="NEEDRPMGROUP"
	fi
fi

#
# Output result
#
printf '%s' "${RESULT}"

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
