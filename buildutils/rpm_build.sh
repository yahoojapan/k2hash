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

#
# Autobuid for rpm package
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [-buildnum <build number>] [-product <product name>] [-y]"
	echo "        -buildnum                     specify build number for packaging(default 1)"
	echo "        -product                      specify product name(use PACKAGE_NAME in Makefile s default)"
	echo "        -y                            runs no interactive mode."
	echo "        -h                            print help"
	echo ""
}
PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
MYSCRIPTDIR=`cd ${MYSCRIPTDIR}; pwd`
SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`
RPM_TOPDIR=${SRCTOP}/rpmbuild

#
# Check options
#
IS_INTERACTIVE=1
BUILD_NUMBER=1
while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break

	elif [ "X$1" = "X-h" -o "X$1" = "X-help" ]; then
		func_usage $PRGNAME
		exit 0

	elif [ "X$1" = "X-buildnum" ]; then
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : -buildnum option needs parameter." 1>&2
			exit 1
		fi
		BUILD_NUMBER=$1

	elif [ "X$1" = "X-product" ]; then
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : -product option needs parameter." 1>&2
			exit 1
		fi
		PACKAGE_NAME=$1

	elif [ "X$1" = "X-y" ]; then
		IS_INTERACTIVE=0

	else
		echo "[ERROR] ${PRGNAME} : unknown option $1." 1>&2
		exit 1
	fi
	shift
done

#
# Package name
#
if [ "X${PACKAGE_NAME}" = "X" ]; then
	PACKAGE_NAME=`grep PACKAGE_NAME ${SRCTOP}/Makefile 2>/dev/null | awk '{print $3}' 2>/dev/null`
	if [ "X${PACKAGE_NAME}" = "X" ]; then
		echo "[ERROR] ${PRGNAME} : no product name" 1>&2
		exit 1
	fi
fi

#
# Welcome message and confirming for interactive mode
#
if [ ${IS_INTERACTIVE} -eq 1 ]; then
	echo "---------------------------------------------------------------"
	echo " Do you change these file and commit to github?"
	echo " - ChangeLog              modify / add changes like dch tool format"
	echo " - Git TAG                stamp git tag for release"
	echo "---------------------------------------------------------------"
	while true; do
		echo -n "Confirm: [y/n] "
		read CONFIRM

		if [ "X${CONFIRM}" = "XY" -o "X${CONFIRM}" = "Xy" ]; then
			break;
		elif [ "X${CONFIRM}" = "XN" -o "X${CONFIRM}" = "Xn" ]; then
			echo "Bye..."
			exit 1
		fi
	done
	echo ""
fi

#
# before building
#
cd ${SRCTOP}

#
# package version
#
PACKAGE_VERSION=`${MYSCRIPTDIR}/make_variables.sh -pkg_version`

#
# copy spec file
#
cp ${SRCTOP}/buildutils/*.spec ${SRCTOP}/
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : could not find and copy spec files." 1>&2
	exit 1
fi

#
# create rpm top directory and etc
#
mkdir -p ${RPM_TOPDIR}/{BUILD,BUILDROOT,RPM,SOURCES,SPECS,SRPMS}
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : could not make ${RPM_TOPDIR}/* directories." 1>&2
	exit 1
fi

#
# copy source tar.gz from git by archive
#
git archive HEAD --prefix=${PACKAGE_NAME}-${PACKAGE_VERSION}/ --output=${RPM_TOPDIR}/SOURCES/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : could not make source tar ball(${RPM_TOPDIR}/SOURCES/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz) from github repository." 1>&2
	exit 1
fi

#
# rpm build
#
rpmbuild -vv -ba --define "_topdir ${RPM_TOPDIR}" --define "_prefix /usr" --define "_mandir /usr/share/man" --define "_defaultdocdir /usr/share/doc" --define "package_revision ${BUILD_NUMBER}" *.spec
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : failed to build rpm packages by rpmbuild." 1>&2
	exit 1
fi

#
# copy RPM files to package directory for uploading
#
cp ${SRCTOP}/rpmbuild/RPMS/*/*.rpm ${SRCTOP}/
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : failed to copy rpm files to ${SRCTOP} directory." 1>&2
	exit 1
fi

cp ${SRCTOP}/rpmbuild/SRPMS/*.rpm ${SRCTOP}/
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : failed to copy source rpm files to ${SRCTOP} directory." 1>&2
	exit 1
fi

#
# finish
#
echo ""
echo "You can find ${PACKAGE_NAME} ${PACKAGE_VERSION}-${BUILD_NUMBER} version rpm package in ${SRCTOP} directory."
echo ""
exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
