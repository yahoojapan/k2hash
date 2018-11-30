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
func_usage()
{
	echo ""
	echo "Usage:  $1 [-pkg_version | -lib_version_info | -lib_version_for_link | -major_number | -debhelper_dep | -rpmpkg_group]"
	echo "	-pkg_version            returns package version."
	echo "	-lib_version_info       returns library libtools revision"
	echo "	-lib_version_for_link   return library version for symbolic link"
	echo "	-major_number           return major version number"
	echo "	-debhelper_dep          return debhelper dependency string"
	echo "	-rpmpkg_group           return group string for rpm package"
	echo "	-h(help)                print help."
	echo ""
}
PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`
RELEASE_VERSION_FILE="${SRCTOP}/RELEASE_VERSION"

#
# Check options
#
PRGMODE=""
while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break;

	elif [ "X$1" = "X-h" -o "X$1" = "X-help" ]; then
		func_usage $PRGNAME
		exit 0

	elif [ "X$1" = "X-pkg_version" ]; then
		PRGMODE="PKG"

	elif [ "X$1" = "X-lib_version_info" ]; then
		PRGMODE="LIB"

	elif [ "X$1" = "X-lib_version_for_link" ]; then
		PRGMODE="LINK"

	elif [ "X$1" = "X-major_number" ]; then
		PRGMODE="MAJOR"

	elif [ "X$1" = "X-debhelper_dep" ]; then
		PRGMODE="DEBHELPER"

	elif [ "X$1" = "X-rpmpkg_group" ]; then
		PRGMODE="RPMGROUP"

	else
		echo "ERROR: unknown option $1" 1>&2
		echo -n "0"
		exit 1
	fi
	shift
done
if [ "X${PRGMODE}" = "X" ]; then
	echo "ERROR: option is not specified." 1>&2
	echo -n "0"
	exit 1
fi

#
# Make result
#
if [ ${PRGMODE} = "PKG" ]; then
	RESULT=`cat ${RELEASE_VERSION_FILE}`

elif [ ${PRGMODE} = "LIB" -o ${PRGMODE} = "LINK" ]; then
	MAJOR_VERSION=`cat ${RELEASE_VERSION_FILE} | sed 's/["|\.]/ /g' | awk '{print $1}'`
	MID_VERSION=`cat ${RELEASE_VERSION_FILE} | sed 's/["|\.]/ /g' | awk '{print $2}'`
	LAST_VERSION=`cat ${RELEASE_VERSION_FILE} | sed 's/["|\.]/ /g' | awk '{print $3}'`

	# check version number
	expr "${MAJOR_VERSION}" + 1 >/dev/null 2>&1
	if [ $? -ge 2 ]; then
		echo "ERROR: wrong version number in RELEASE_VERSION file" 1>&2
		echo -n "0"
		exit 1
	fi
	expr "${MID_VERSION}" + 1 >/dev/null 2>&1
	if [ $? -ge 2 ]; then
		echo "ERROR: wrong version number in RELEASE_VERSION file" 1>&2
		echo -n "0"
		exit 1
	fi
	expr "${LAST_VERSION}" + 1 >/dev/null 2>&1
	if [ $? -ge 2 ]; then
		echo "ERROR: wrong version number in RELEASE_VERSION file" 1>&2
		echo -n "0"
		exit 1
	fi

	# make library revision number
	if [ ${MID_VERSION} -gt 0 ]; then
		REV_VERSION=`expr ${MID_VERSION} \* 100`
		REV_VERSION=`expr ${LAST_VERSION} + ${REV_VERSION}`
	else
		REV_VERSION=${LAST_VERSION}
	fi

	if [ ${PRGMODE} = "LIB" ]; then
		RESULT="${MAJOR_VERSION}:${REV_VERSION}:0"
	else
		RESULT="${MAJOR_VERSION}.0.${REV_VERSION}"
	fi

elif [ ${PRGMODE} = "MAJOR" ]; then
	RESULT=`cat ${RELEASE_VERSION_FILE} | sed 's/["|\.]/ /g' | awk '{print $1}'`

elif [ ${PRGMODE} = "DEBHELPER" ]; then
	# [NOTE]
	# This option returns debhelper dependency string in control file for debian package.
	# That string is depended debhelper package version and os etc.
	# (if not ubuntu/debian os, returns default string)
	#
	apt-cache --version >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		IS_OS_UBUNTU=0
		if [ -f /etc/lsb-release ]; then
			grep [Uu]buntu /etc/lsb-release >/dev/null 2>&1
			if [ $? -eq 0 ]; then
				IS_OS_UBUNTU=1
			fi
		fi

		DEBHELPER_MAJOR_VER=`apt-cache show debhelper 2>/dev/null | grep Version 2>/dev/null | awk '{print $2}' 2>/dev/null | sed 's/\..*/ /g' 2>/dev/null`
		expr "${DEBHELPER_MAJOR_VER}" + 1 >/dev/null 2>&1
		if [ $? -ne 0 ]; then
			DEBHELPER_MAJOR_VER=0
		else
			DEBHELPER_MAJOR_VER=`expr "${DEBHELPER_MAJOR_VER}" + 0`
		fi
		if [ ${DEBHELPER_MAJOR_VER} -lt 10 ]; then
			RESULT="debhelper (>= 9), autotools-dev"
		else
			if [ ${IS_OS_UBUNTU} -eq 1 ]; then
				RESULT="debhelper (>= 10)"
			else
				RESULT="debhelper (>= 10), autotools-dev"
			fi
		fi
	else
		# Not debian/ubuntu, set default
		RESULT="debhelper (>= 10), autotools-dev"
	fi

elif [ ${PRGMODE} = "RPMGROUP" ]; then
	# [NOTE]
	# Fedora rpm does not need "Group" key in spec file.
	# If not fedora, returns "NEEDRPMGROUP", and you must replace this string in configure.ac
	#
	if [ -f /etc/fedora-release ]; then
		RESULT=""
	else
		RESULT="NEEDRPMGROUP"
	fi
fi

#
# Output result
#
echo -n $RESULT

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
