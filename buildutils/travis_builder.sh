#!/bin/sh
#
# Utility helper tools for Travis CI by AntPickax
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
# CREATE:   Tue, Nov 20 2018
# REVISION:
#

#
# Helper for docker on Travis CI
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [-<ostype>] [-pctoken <package cloud token>] [-pcuser <user>] [-pcrepo <repository name>] [-pcdlrepo <repository name>] [-pcdist <any path(os/version)>] <package name>..."
	echo "        -<ostype>          specify OS type for script mode(ubuntu/debian/fedora/el)"
	echo "        -pctoken           specify packagecloud.io token for uploading(optional)"
	echo "        -pcuser            specify publish user name(optional)"
	echo "        -pcrepo            specify publish repository name(optional)"
	echo "        -pcdlrepo          specify download repository name(optional)"
	echo "        -pcdist            specify publish distribute(optional) - example: \"ubuntu/trusty\""
	echo "        <package name>...  specify package names needed before building"
	echo "        -h                 print help"
	echo "Environments"
	echo "        BUILD_NUMBER       specify build number for packaging(default 1)"
	echo "        TRAVIS_TAG         if the current build is for a git tag, this variable is set to the tag’s name"
	echo "        FORCE_BUILD_PKG    if this env is 'true', force packaging anytime"
	echo "        USE_PC_REPO        if this env is 'true', use packagecloud.io repository"
	echo "        CONFIGUREOPT       specify extra configure option."
	echo "        NO_DEBUILD         if this env is 'true'(on pull request), do not run debuild."
	echo ""
}

#
# Utility functions
#
prn_cmd()
{
	echo ""
	echo "$ $@"
}

run_cmd()
{
	echo ""
	echo "$ $@"
	$@
	if [ $? -ne 0 ]; then
		echo "[ERROR] ${PRGNAME} : \"$@\"" 1>&2
		exit 1
	fi
}

PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
MYSCRIPTDIR=`cd ${MYSCRIPTDIR}; pwd`
SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`

#
# Check options
#
INSTALLER_BIN=""
GEM_BIN="gem"
PKGTYPE_RPM=0
PKGDIR=""
PKGEXT=""
PCUSER=""
PCREPO=""
PCDLREPO=""
PCDIST=""
PCPUBLISH_PATH=""
INSTALL_PACKAGES=""
IS_FEDORA=0
IS_CENTOS=0
while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break

	elif [ "X$1" = "X-h" -o "X$1" = "X-H" -o "X$1" = "X--help" -o "X$1" = "X--HELP" ]; then
		func_usage $PRGNAME
		exit 0

	elif [ "X$1" = "X-debian" -o "X$1" = "X-DEBIAN" ]; then
		if [ "X${INSTALLER_BIN}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set script mode(${INSTALLER_BIN}), could not over write mode for debian." 1>&2
			exit 1
		fi
		INSTALLER_BIN="apt-get"
		PKGTYPE_RPM=0
		PKGDIR="debian_build"
		PKGEXT="deb"

	elif [ "X$1" = "X-ubuntu" -o "X$1" = "X-UBUNTU" ]; then
		if [ "X${INSTALLER_BIN}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set script mode(${INSTALLER_BIN}), could not over write mode for ubuntu." 1>&2
			exit 1
		fi
		INSTALLER_BIN="apt-get"
		PKGTYPE_RPM=0
		PKGDIR="debian_build"
		PKGEXT="deb"

	elif [ "X$1" = "X-fedora" -o "X$1" = "X-FEDORA" ]; then
		if [ "X${INSTALLER_BIN}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set script mode(${INSTALLER_BIN}), could not over write mode for fedora." 1>&2
			exit 1
		fi
		INSTALLER_BIN="yum"
		PKGTYPE_RPM=1
		PKGDIR="."
		PKGEXT="rpm"
		IS_FEDORA=1

	elif [ "X$1" = "X-centos" -o "X$1" = "X-CENTOS" ]; then
		if [ "X${INSTALLER_BIN}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set script mode(${INSTALLER_BIN}), could not over write mode for EL." 1>&2
			exit 1
		fi
		INSTALLER_BIN="yum"
		PKGTYPE_RPM=1
		PKGDIR="."
		PKGEXT="rpm"
		IS_CENTOS=1

	elif [ "X$1" = "X-pctoken" -o "X$1" = "X-PCTOKEN" ]; then
		if [ "X${PCTOKEN}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set packagecloud.io token." 1>&2
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : -pctoken option is specified without parameter." 1>&2
			exit 1
		fi
		PCTOKEN=$1

	elif [ "X$1" = "X-pcuser" -o "X$1" = "X-PCUSER" ]; then
		if [ "X${PCUSER}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set packagecloud.io user name." 1>&2
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : -pcuser option is specified without parameter." 1>&2
			exit 1
		fi
		PCUSER=$1

	elif [ "X$1" = "X-pcrepo" -o "X$1" = "X-PCREPO" ]; then
		if [ "X${PCREPO}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set packagecloud.io repository name." 1>&2
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : -pcrepo option is specified without parameter." 1>&2
			exit 1
		fi
		PCREPO=$1

	elif [ "X$1" = "X-pcdlrepo" -o "X$1" = "X-PCDLREPO" ]; then
		if [ "X${PCDLREPO}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set packagecloud.io download repository name." 1>&2
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : -pcdlrepo option is specified without parameter." 1>&2
			exit 1
		fi
		PCDLREPO=$1

	elif [ "X$1" = "X-pcdist" -o "X$1" = "X-PCDIST" ]; then
		if [ "X${PCDIST}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set packagecloud.io distribute." 1>&2
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : -pcdist option is specified without parameter." 1>&2
			exit 1
		fi
		PCDIST=$1

	else
		if [ "X${INSTALL_PACKAGES}" = "X" ]; then
			INSTALL_PACKAGES=$1
		else
			INSTALL_PACKAGES="${INSTALL_PACKAGES} $1"
		fi
	fi
	shift
done

if [ "X${INSTALLER_BIN}" = "X" ]; then
	echo "[ERROR] ${PRGNAME} : -deb or -rpm option must be specified." 1>&2
	exit 1
fi
if [ "X${PCTOKEN}" = "X" ]; then
	if [ "X${PCUSER}" != "X" -o "X${PCREPO}" != "X" -o "X${PCDIST}" = "X" ]; then
		echo "[ERROR] ${PRGNAME} : -pctoken is not specified, but other packagecloud.io options are specified." 1>&2
		exit 1
	fi
else
	if [ "X${PCUSER}" = "X" -o "X${PCREPO}" = "X" ]; then
		echo "[ERROR] ${PRGNAME} : -pctoken is specified, but -pcuser or -pcrepo options are not specified." 1>&2
		exit 1
	fi
	PCPUBLISH_PATH="${PCUSER}/${PCREPO}"

	if [ "X${PCDIST}" != "X" ]; then
		PCPUBLISH_PATH="${PCPUBLISH_PATH}/${PCDIST}"
	fi
fi

#
# Check environment for packaging
#
IS_PACKAGING=0
if [ "X${FORCE_BUILD_PKG}" = "Xtrue" -o "X${FORCE_BUILD_PKG}" = "XTRUE" ]; then
	#
	# force packaging
	#
	IS_PACKAGING=1

elif [ "X${TRAVIS_TAG}" != "X" ]; then
	#
	# source codes is tagged now.
	#
	IS_PACKAGING=1
fi
if [ ${IS_PACKAGING} -eq 1 -a "X${PCTOKEN}" = "X" ]; then
	echo "[ERROR] ${PRGNAME} : need to build packages, but packagecloud.io token is not specified." 1>&2
	exit 1
fi

if [ "X${BUILD_NUMBER}" = "X" ]; then
	#
	# default build number
	#
	BUILD_NUMBER=1
else
	#
	# check number
	#
	expr ${BUILD_NUMBER} + 0 >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "[ERROR] ${PRGNAME} : BUILD_NUMBER environment(build number) must be 1 or over." 1>&2
		exit 1
	fi
fi

#
# Set package repository on packagecloud.io
#
if [ "X${USE_PC_REPO}" = "Xtrue" -o "X${USE_PC_REPO}" = "XTRUE" ]; then
	#
	# Check curl
	#
	curl --version >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		run_cmd ${INSTALLER_BIN} update -y -qq
		run_cmd ${INSTALLER_BIN} install -y -qq curl
	fi

	#
	# Download repository name
	#
	PCDLREPO_NAME=${PCREPO}
	if [ "X${PCDLREPO}" != "X" ]; then
		PCDLREPO_NAME=${PCDLREPO}
	fi

	#
	# Download and set pckagecloud.io repository
	#
	if [ ${IS_CENTOS} -eq 1 -o ${IS_FEDORA} -eq 1 ]; then
		PC_REPO_ADD_SH="script.rpm.sh"
	else
		PC_REPO_ADD_SH="script.deb.sh"
	fi
	prn_cmd "curl -s https://packagecloud.io/install/repositories/${PCUSER}/${PCDLREPO_NAME}/${PC_REPO_ADD_SH} | bash"
	curl -s https://packagecloud.io/install/repositories/${PCUSER}/${PCDLREPO_NAME}/${PC_REPO_ADD_SH} | bash
	if [ $? -ne 0 ]; then
		echo "[ERROR] ${PRGNAME} : could not add packagecloud.io repository." 1>&2
		exit 1
	fi
fi

#
# Install packages
#
run_cmd ${INSTALLER_BIN} update -y -qq

if [ "X${INSTALL_PACKAGES}" != "X" ]; then
	run_cmd ${INSTALLER_BIN} install -y -qq ${INSTALL_PACKAGES}
fi

#
# For packagecloud.io CLI tool(package_cloud)
#
if [ ${IS_CENTOS} -ne 1 ]; then
	run_cmd ${GEM_BIN} install rake rubocop package_cloud
else
	#
	# Using RHSCL because centos has older ruby
	#
	run_cmd yum install -y -qq centos-release-scl
	run_cmd yum install -y -qq rh-ruby23 rh-ruby23-ruby-devel rh-ruby23-rubygem-rake
	source /opt/rh/rh-ruby23/enable
	run_cmd ${GEM_BIN} install rubocop package_cloud
fi

#
# Start bulding ( build under /tmp )
#
run_cmd cp -rp ${SRCTOP} /tmp
TMPSRCTOP=`basename ${SRCTOP}`
SRCTOP="/tmp/${TMPSRCTOP}"

run_cmd cd ${SRCTOP}
run_cmd ./autogen.sh
run_cmd ./configure --prefix=/usr ${CONFIGUREOPT}
run_cmd make
run_cmd make check

#
# Start packaging
#
if [ ${PKGTYPE_RPM} -eq 1 ]; then
	#
	# Create debian packages
	#
	prn_cmd ./buildutils/rpm_build.sh -buildnum ${BUILD_NUMBER} -y
	./buildutils/rpm_build.sh -buildnum ${BUILD_NUMBER} -y
else
	#
	# Create debian packages
	#
	DEBUILD_OPT=""
	if [ ${IS_PACKAGING} -ne 1 ]; then
		DEBUILD_OPT="-nodebuild"
	fi
	prn_cmd CONFIGUREOPT=${CONFIGUREOPT} ./buildutils/debian_build.sh -buildnum ${BUILD_NUMBER} ${DEBUILD_OPT} -y
	CONFIGUREOPT=${CONFIGUREOPT} ./buildutils/debian_build.sh -buildnum ${BUILD_NUMBER} ${DEBUILD_OPT} -y
fi
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : Failed to build packages" 1>&2
	exit 1
fi

#
# Publishing
#
if [ ${IS_PACKAGING} -eq 1 ]; then
	if [ "X${PCTOKEN}" != "X" ]; then
		prn_cmd PACKAGECLOUD_TOKEN=${PCTOKEN} package_cloud push ${PCPUBLISH_PATH} ${SRCTOP}/${PKGDIR}/*.${PKGEXT}
		PACKAGECLOUD_TOKEN=${PCTOKEN} package_cloud push ${PCPUBLISH_PATH} ${SRCTOP}/${PKGDIR}/*.${PKGEXT}
		if [ $? -ne 0 ]; then
			echo "[ERROR] ${PRGNAME} : Failed to publish *.${PKGEXT} packages" 1>&2
			exit 1
		fi
	fi
fi

echo "[SUCCESS] ${PRGNAME} : Finished without error." 1>&2
exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
