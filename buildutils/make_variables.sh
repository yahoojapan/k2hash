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

	elif echo "$1" | grep -q -i -e "^-h$" -e "^--help$"; then
		func_usage "${PRGNAME}"
		exit 0

	elif echo "$1" | grep -q -i -e "^-pv$" -e "^--pkg_version$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="PKG"
		EXCLUSIVE_OPT=1

	elif echo "$1" | grep -q -i -e "^-lvi$" -e "^--lib_version_info$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="LIB"
		EXCLUSIVE_OPT=1

	elif echo "$1" | grep -q -i -e "^-lvl$" -e "^--lib_version_for_link$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="LINK"
		EXCLUSIVE_OPT=1

	elif echo "$1" | grep -q -i -e "^-mn$" -e "^--major_number$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="MAJOR"
		EXCLUSIVE_OPT=1

	elif echo "$1" | grep -q -i -e "^-dd$" -e "^--debhelper_dep$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="DEBHELPER"
		DEB_WITH_SYSTEMD=0
		EXCLUSIVE_OPT=1

	elif echo "$1" | grep -q -i -e "^-dds$" -e "^--debhelper_dep_with_systemd$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --pkg_version(-pv), --lib_version_info(-lvi), --lib_version_for_link(-lvl), --major_number(-mn), --debhelper_dep(-dd), --debhelper_dep_with_systemd(-dds), --rpmpkg_group(-rg) ) is specified." 1>&2
			printf '0'
			exit 1
		fi
		PRGMODE="DEBHELPER"
		DEB_WITH_SYSTEMD=1
		EXCLUSIVE_OPT=1

	elif echo "$1" | grep -q -i -e "^-rg$" -e "^--rpmpkg_group$"; then
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
	OS_ID_STRING=$(grep '^ID[[:space:]]*=[[:space:]]*' /etc/os-release | sed -e 's|^ID[[:space:]]*=[[:space:]]*||g' -e 's|^[[:space:]]*||g' -e 's|[[:space:]]*$||g' -e 's|"||g')

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
		if echo "${OS_ID_STRING}" | grep -q -i "debian"; then
			RESULT="debhelper (>= 9.20160709)${DEB_WITH_SYSTEMD_STRING}, autotools-dev"

		elif echo "${OS_ID_STRING}" | grep -q -i "ubuntu"; then
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
	if grep -q -i '^ID[[:space:]]*=[[:space:]]*["]*fedora["]*[[:space:]]*$' /etc/os-release; then
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
