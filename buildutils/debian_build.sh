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
# Autobuid for debian package
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [-buildnum <build number>] [-nodebuild] [-rootdir] [-product <product name>] [-y] [additional debuild options]"
	echo "        -buildnum                     specify build number for packaging(default 1)"
	echo "        -nodebuild                    stops before do debuild command."
	echo "        -rootdir                      layout \"debian\" directory for packaging under source top directory"
	echo "        -product                      specify product name(use PACKAGE_NAME in Makefile s default)"
	echo "        -y                            runs no interacitive mode."
	echo "        additional debuild options    this script run debuild with \"-uc -us\", can specify additional options."
	echo "        -h                            print help"
	echo ""
}
PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
MYSCRIPTDIR=`cd ${MYSCRIPTDIR}; pwd`
SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`
BUILDDEBDIR=${SRCTOP}/debian_build

#
# Check options
#
IS_DEBUILD=1
IS_INTERACTIVE=1
IS_ROOTDIR=0
DH_MAKE_AUTORUN_OPTION=""
BUILD_NUMBER=1
DEBUILD_OPT=""
while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break

	elif [ "X$1" = "X-h" -o "X$1" = "X-help" ]; then
		func_usage $PRGNAME
		exit 0

	elif [ "X$1" = "X-buildnum" ]; then
		shift
		if [ $# -eq 0 ]; then
			echo "ERROR: -buildnum option needs parameter." 1>&2
			exit 1
		fi
		BUILD_NUMBER=$1

	elif [ "X$1" = "X-nodebuild" ]; then
		IS_DEBUILD=0
		BUILD_NUMBER=

	elif [ "X$1" = "X-rootdir" ]; then
		IS_ROOTDIR=1

	elif [ "X$1" = "X-product" ]; then
		shift
		if [ $# -eq 0 ]; then
			echo "ERROR: -product option needs parameter." 1>&2
			exit 1
		fi
		PACKAGE_NAME=$1

	elif [ "X$1" = "X-y" ]; then
		IS_INTERACTIVE=0
		DH_MAKE_AUTORUN_OPTION="-y"

	else
		if [ "X${DEBUILD_OPT}" != "X" ]; then
			DEBUILD_OPT="${DEBUILD_OPT} $1"
		else
			DEBUILD_OPT="$1"
		fi
	fi
	shift
done

#
# Package name
#
if [ "X${PACKAGE_NAME}" = "X" ]; then
	PACKAGE_NAME=`grep PACKAGE_NAME ${SRCTOP}/Makefile 2>/dev/null | awk '{print $3}' 2>/dev/null`
	if [ "X${PACKAGE_NAME}" = "X" ]; then
		echo "ERROR: no product name" 1>&2
		exit 1
	fi
fi
PACKAGE_DEV_NAME="${PACKAGE_NAME}-dev"
LIB_BASENAME="lib${PACKAGE_NAME}"

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
# Make dist package by make dist
#
${SRCTOP}/autogen.sh	|| exit 1
${SRCTOP}/configure		|| exit 1
PACKAGE_VERSION=`${MYSCRIPTDIR}/make_variables.sh -pkg_version`
PACKAGE_MAJOR_VER=`${MYSCRIPTDIR}/make_variables.sh -major_number`

echo "===== make dist: start =============================="
make dist || exit 1
echo "===== make dist: end   =============================="

#
# Create debian package directory and change current
#
echo "===== prepare working directory: start ============="

if [ -f ${BUILDDEBDIR} ]; then
	echo "ERROR: debian file exists, could not make debian directory." 1>&2
	exit 1
fi
if [ -d ${BUILDDEBDIR} ]; then
	echo "WANING: debian directory exists, remove and remake it." 1>&2
	rm -rf ${BUILDDEBDIR} || exit 1
fi
mkdir ${BUILDDEBDIR}	|| exit 1
cd ${BUILDDEBDIR}		|| exit 1

#
# copy dist package and expand source files
#
cp ${SRCTOP}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz .	|| exit 1
tar xvfz ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz			|| exit 1

#
# change current directory
#
EXPANDDIR=${BUILDDEBDIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}
cd ${EXPANDDIR} || exit 1

#
# initialize debian directory
#
dh_make -f ${BUILDDEBDIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz --createorig -m ${DH_MAKE_AUTORUN_OPTION} || exit 1

#
# remove unnecessary template files
#
rm -rf ${EXPANDDIR}/debian/*.ex ${EXPANDDIR}/debian/*.EX ${EXPANDDIR}/debian/${PACKAGE_NAME}-doc.* ${EXPANDDIR}/debian/README.* ${EXPANDDIR}/debian/docs

#
# adding some lines into rules file
#
echo ""																				>> ${EXPANDDIR}/debian/rules || exit 1
echo "# for ${PACKAGE_NAME} and ${PACKAGE_DEV_NAME} packages"						>> ${EXPANDDIR}/debian/rules || exit 1
echo "override_dh_auto_install:"													>> ${EXPANDDIR}/debian/rules || exit 1
echo "	dh_auto_install --destdir=debian/${PACKAGE_NAME}"							>> ${EXPANDDIR}/debian/rules || exit 1
echo "	dh_auto_install --destdir=debian/${PACKAGE_DEV_NAME}"						>> ${EXPANDDIR}/debian/rules || exit 1
echo "	if [ -d debian/${PACKAGE_NAME}/usr/include ]; then                          rm -rf debian/${PACKAGE_NAME}/usr/include;                                                fi"	>> ${EXPANDDIR}/debian/rules || exit 1
echo "	if [ -d debian/${PACKAGE_NAME}/usr/lib/x86_64-linux-gnu/pkgconfig ]; then   rm -rf debian/${PACKAGE_NAME}/usr/lib/x86_64-linux-gnu/pkgconfig;                         fi"	>> ${EXPANDDIR}/debian/rules || exit 1
echo "	if [ -d debian/${PACKAGE_DEV_NAME}/usr/bin ]; then                          rm -rf debian/${PACKAGE_DEV_NAME}/usr/bin;                                                fi"	>> ${EXPANDDIR}/debian/rules || exit 1
echo "	if [ -d debian/${PACKAGE_DEV_NAME}/usr/share/man ]; then                    rm -rf debian/${PACKAGE_DEV_NAME}/usr/share/man;                                          fi"	>> ${EXPANDDIR}/debian/rules || exit 1
echo "	ls debian/${PACKAGE_NAME}/usr/lib/x86_64-linux-gnu/*.a ]; >/dev/null 2>&1;    if [ $? -eq 0 ]; then rm -rf debian/${PACKAGE_NAME}/usr/lib/x86_64-linux-gnu/*.a;       fi"	>> ${EXPANDDIR}/debian/rules || exit 1
echo "	ls debian/${PACKAGE_NAME}/usr/lib/x86_64-linux-gnu/*.la ]; >/dev/null 2>&1;   if [ $? -eq 0 ]; then rm -rf debian/${PACKAGE_NAME}/usr/lib/x86_64-linux-gnu/*.la;      fi"	>> ${EXPANDDIR}/debian/rules || exit 1
echo "	ls debian/${PACKAGE_DEV_NAME}/usr/lib/x86_64-linux-gnu/*.a >/dev/null 2>&1;   if [ $? -eq 0 ]; then rm -rf debian/${PACKAGE_DEV_NAME}/usr/lib/x86_64-linux-gnu/*.a;   fi"	>> ${EXPANDDIR}/debian/rules || exit 1
echo "	ls debian/${PACKAGE_DEV_NAME}/usr/lib/x86_64-linux-gnu/*.la >/dev/null 2>&1;  if [ $? -eq 0 ]; then rm -rf debian/${PACKAGE_DEV_NAME}/usr/lib/x86_64-linux-gnu/*.la;  fi"	>> ${EXPANDDIR}/debian/rules || exit 1
echo "	ls debian/${PACKAGE_DEV_NAME}/usr/lib/x86_64-linux-gnu/*.so* >/dev/null 2>&1; if [ $? -eq 0 ]; then rm -rf debian/${PACKAGE_DEV_NAME}/usr/lib/x86_64-linux-gnu/*.so*; fi"	>> ${EXPANDDIR}/debian/rules || exit 1

#
# create links file for library
#
FOUND_LIB_LINES=`find ./ -name Makefile.am -exec grep ${LIB_BASENAME} {} \; 2>/dev/null`
if [ "X${FOUND_LIB_LINES}" != "X" ]; then
	LIBRARY_LIBTOOL_VERSION=`${MYSCRIPTDIR}/make_variables.sh -lib_version_for_link 2>/dev/null` || exit 1
	echo "usr/lib/x86_64-linux-gnu/${LIB_BASENAME}.so.${LIBRARY_LIBTOOL_VERSION} usr/lib/x86_64-linux-gnu/${LIB_BASENAME}.so"						>  ${EXPANDDIR}/debian/${PACKAGE_NAME}.links || exit 1
	echo "usr/lib/x86_64-linux-gnu/${LIB_BASENAME}.so.${LIBRARY_LIBTOOL_VERSION} usr/lib/x86_64-linux-gnu/${LIB_BASENAME}.so.${PACKAGE_MAJOR_VER}"	>> ${EXPANDDIR}/debian/${PACKAGE_NAME}.links || exit 1
fi

#
# copy copyright
#
cp ${MYSCRIPTDIR}/copyright ${EXPANDDIR}/debian/copyright || exit 1

#
# copy control file
#
cp ${MYSCRIPTDIR}/control ${EXPANDDIR}/debian/control || exit 1

#
# copy changelog with converting build number
#
CHLOG_ORG_MENT=`cat ChangeLog | grep "^ --" | head -1`
CHLOG_NEW_MENT=`cat ${EXPANDDIR}/debian/changelog | grep "^ --" | head -1`
if [ "X${BUILD_NUMBER}" = "X" ]; then
	cat ChangeLog | sed "s/${CHLOG_ORG_MENT}/${CHLOG_NEW_MENT}/g" > ${EXPANDDIR}/debian/changelog || exit 1
else
	cat ChangeLog | sed "s/${PACKAGE_VERSION}/${PACKAGE_VERSION}-${BUILD_NUMBER}/g" | sed "s/${CHLOG_ORG_MENT}/${CHLOG_NEW_MENT}/g" > ${EXPANDDIR}/debian/changelog || exit 1
fi

#
# preinst/postinst/prerm/postrm
#
if [ -f ${MYSCRIPTDIR}/${PACKAGE_NAME}.preinst ]; then
	cp -p ${MYSCRIPTDIR}/${PACKAGE_NAME}.preinst ${EXPANDDIR}/debian/${PACKAGE_NAME}.preinst || exit 1
	chmod +x ${EXPANDDIR}/debian/${PACKAGE_NAME}.preinst || exit 1
fi
if [ -f ${MYSCRIPTDIR}/${PACKAGE_NAME}.postinst ]; then
	cp -p ${MYSCRIPTDIR}/${PACKAGE_NAME}.postinst ${EXPANDDIR}/debian/${PACKAGE_NAME}.postinst || exit 1
	chmod +x ${EXPANDDIR}/debian/${PACKAGE_NAME}.postinst || exit 1
fi
if [ -f ${MYSCRIPTDIR}/${PACKAGE_NAME}.prerm ]; then
	cp -p ${MYSCRIPTDIR}/${PACKAGE_NAME}.prerm ${EXPANDDIR}/debian/${PACKAGE_NAME}.prerm || exit 1
	chmod +x ${EXPANDDIR}/debian/${PACKAGE_NAME}.prerm || exit 1
fi
if [ -f ${MYSCRIPTDIR}/${PACKAGE_NAME}.postrm ]; then
	cp -p ${MYSCRIPTDIR}/${PACKAGE_NAME}.postrm ${EXPANDDIR}/debian/${PACKAGE_NAME}.postrm || exit 1
	chmod +x ${EXPANDDIR}/debian/${PACKAGE_NAME}.postrm || exit 1
fi
if [ -f ${MYSCRIPTDIR}/${PACKAGE_DEV_NAME}.preinst ]; then
	cp -p ${MYSCRIPTDIR}/${PACKAGE_DEV_NAME}.preinst ${EXPANDDIR}/debian/${PACKAGE_DEV_NAME}.preinst || exit 1
	chmod +x ${EXPANDDIR}/debian/${PACKAGE_DEV_NAME}.preinst || exit 1
fi
if [ -f ${MYSCRIPTDIR}/${PACKAGE_DEV_NAME}.postinst ]; then
	cp -p ${MYSCRIPTDIR}/${PACKAGE_DEV_NAME}.postinst ${EXPANDDIR}/debian/${PACKAGE_DEV_NAME}.postinst || exit 1
	chmod +x ${EXPANDDIR}/debian/${PACKAGE_DEV_NAME}.postinst || exit 1
fi
if [ -f ${MYSCRIPTDIR}/${PACKAGE_DEV_NAME}.prerm ]; then
	cp -p ${MYSCRIPTDIR}/${PACKAGE_DEV_NAME}.prerm ${EXPANDDIR}/debian/${PACKAGE_DEV_NAME}.prerm || exit 1
	chmod +x ${EXPANDDIR}/debian/${PACKAGE_DEV_NAME}.prerm || exit 1
fi
if [ -f ${MYSCRIPTDIR}/${PACKAGE_DEV_NAME}.postrm ]; then
	cp -p ${MYSCRIPTDIR}/${PACKAGE_DEV_NAME}.postrm ${EXPANDDIR}/debian/${PACKAGE_DEV_NAME}.postrm || exit 1
	chmod +x ${EXPANDDIR}/debian/${PACKAGE_DEV_NAME}.postrm || exit 1
fi

echo "===== prepare working directory: end ==============="

#
# change debian directory to source top directory
#
if [ ${IS_ROOTDIR} -eq 1 ]; then
	if [ -f ${SRCTOP}/debian ]; then
		echo "ERROR: ${SRCTOP}/debian file exists, could not make debian directory." 1>&2
		exit 1
	fi
	if [ -d ${SRCTOP}/debian ]; then
		echo "${SRCTOP}/debian directory exists, remove and remake it..." 1>&2
		rm -rf ${SRCTOP}/debian || exit 1
	fi
	cp -rp ${EXPANDDIR}/debian ${SRCTOP}/. || exit 1

	#
	# change current directory
	#
	cd ${SRCTOP}

	#
	# base directory is changed
	#
	BUILDDEBDIR=${SRCTOP}
fi

#
# Check stop before debuild(for manually)
#
if [ ${IS_DEBUILD} -ne 1 ]; then
	#
	# Not run debuild (this means just stop preparing the file)
	#
	echo "MESSGAE: ${PRGNAME} exits immediately before debuild is executed,"
	echo "         that is, it prepares only files and directories."
	echo "         By running \"debuild -uc -us(-tc -b)\", you can create"
	echo "         the debian package manually and find the created package"
	echo "         in \"${BUILDDEBDIR}/..\" directory."
	echo ""

	exit 0
fi

#
# Run debuild
#
echo "===== build package: start ========================="
debuild -us -uc || exit 1
echo "===== build package: end ==========================="

#
# Check and show debian package
#
ls ${BUILDDEBDIR}/${PACKAGE_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb >/dev/null 2>&1
if [ $? -eq 0 ]; then
	echo "===== show ${BUILDDEBDIR}/${PACKAGE_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb package: start ====="
	dpkg -c ${BUILDDEBDIR}/${PACKAGE_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb
	echo ""
	dpkg -I ${BUILDDEBDIR}/${PACKAGE_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb
	echo "===== show ${BUILDDEBDIR}/${PACKAGE_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb package: end ====="
fi

ls ${BUILDDEBDIR}/${PACKAGE_DEV_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb >/dev/null 2>&1
if [ $? -eq 0 ]; then
	echo "===== show ${BUILDDEBDIR}/${PACKAGE_DEV_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb package: start ====="
	dpkg -c ${BUILDDEBDIR}/${PACKAGE_DEV_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb
	echo ""
	dpkg -I ${BUILDDEBDIR}/${PACKAGE_DEV_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb
	echo "===== show ${BUILDDEBDIR}/${PACKAGE_DEV_NAME}_${PACKAGE_VERSION}-${BUILD_NUMBER}*.deb package: end ====="
fi

#
# finish
#
echo ""
echo "You can find ${PACKAGE_NAME} ${PACKAGE_VERSION}-${BUILD_NUMBER} version debian package in ${BUILDDEBDIR} directory."
echo ""
exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
