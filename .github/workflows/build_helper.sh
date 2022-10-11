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
# CREATE:   Mon, Nov 16 2020
# REVISION:	1.1
#

#---------------------------------------------------------------------
# Helper for container on Github Actions
#---------------------------------------------------------------------
func_usage()
{
	echo ""
	echo "Usage: $1 [options...]"
	echo ""
	echo "  Required option:"
	echo "    --help(-h)                                             print help"
	echo "    --ostype(-os)                             <os:version> specify os and version as like \"ubuntu:trusty\""
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
	echo "  Note:"
	echo "    This program uses the GITHUB_REF and GITHUB_EVENT_NAME environment variable internally."
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
		echo "[ERROR] ${PRGNAME} : \"$@\""
		exit 1
	fi
}

#---------------------------------------------------------------------
# Common Variables
#---------------------------------------------------------------------
PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
MYSCRIPTDIR=`cd ${MYSCRIPTDIR}; pwd`
SRCTOP=`cd ${MYSCRIPTDIR}/../..; pwd`

#---------------------------------------------------------------------
# Parse Options
#---------------------------------------------------------------------
echo "[INFO] ${PRGNAME} : Start the parsing of options."

OPT_OSTYPE=
OPT_OSTYPEVARS_FILE=
OPT_IS_FORCE_PUBLISH=
OPT_BUILD_NUMBER=
OPT_DEBEMAIL=
OPT_DEBFULLNAME=
OPT_USE_PC_REPO=
OPT_PC_TOKEN=
OPT_PC_OWNER=
OPT_PC_PUBLISH_REPO=
OPT_PC_DOWNLOAD_REPO=

while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break

	elif [ "X$1" = "X-h" -o "X$1" = "X-H" -o "X$1" = "X--help" -o "X$1" = "X--HELP" ]; then
		func_usage $PRGNAME
		exit 0

	elif [ "X$1" = "X-os" -o "X$1" = "X-OS" -o "X$1" = "X--ostype" -o "X$1" = "X--OSTYPE" ]; then
		if [ "X${OPT_OSTYPE}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--ostype(-os)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--ostype(-os)\" option is specified without parameter."
			exit 1
		fi
		OPT_OSTYPE=$1

	elif [ "X$1" = "X-f" -o "X$1" = "X-F" -o "X$1" = "X--ostype-vars-file" -o "X$1" = "X--OSTYPE-VARS-FILE" ]; then
		if [ "X${OPT_OSTYPEVARS_FILE}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--ostype-vars-file(-f)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--ostype-vars-file(-f)\" option is specified without parameter."
			exit 1
		fi
		if [ ! -f $1 ]; then
			echo "[ERROR] ${PRGNAME} : $1 file is not existed, it is specified \"--ostype-vars-file(-f)\" option."
			exit 1
		fi
		OPT_OSTYPEVARS_FILE=$1

	elif [ "X$1" = "X-p" -o "X$1" = "X-P" -o "X$1" = "X--force-publish" -o "X$1" = "X--FORCE-PUBLISH" ]; then
		if [ "X${OPT_IS_FORCE_PUBLISH}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--force-publish(-p)\" or \"--not-publish(-np)\" option."
			exit 1
		fi
		OPT_IS_FORCE_PUBLISH="true"

	elif [ "X$1" = "X-np" -o "X$1" = "X-NP" -o "X$1" = "X--not-publish" -o "X$1" = "X--NOT-PUBLISH" ]; then
		if [ "X${OPT_IS_FORCE_PUBLISH}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--force-publish(-p)\" or \"--not-publish(-np)\" option."
			exit 1
		fi
		OPT_IS_FORCE_PUBLISH="false"

	elif [ "X$1" = "X-num" -o "X$1" = "X-NUM" -o "X$1" = "X--build-number" -o "X$1" = "X--BUILD-NUMBER" ]; then
		if [ "X${OPT_BUILD_NUMBER}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--build-number(-num)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--build-number(-num)\" option is specified without parameter."
			exit 1
		fi
		expr $1 + 0 >/dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--build-number(-num)\" option specify with positive NUMBER parameter."
			exit 1
		fi
		if [ $1 -le 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--build-number(-num)\" option specify with positive NUMBER parameter."
			exit 1
		fi
		OPT_BUILD_NUMBER=$1

	elif [ "X$1" = "X-devname" -o "X$1" = "X-DEVNAME" -o "X$1" = "X--developer-fullname" -o "X$1" = "X--DEVELOPER-FULLNAME" ]; then
		if [ "X${OPT_DEBFULLNAME}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--developer-fullname(-devname)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--developer-fullname(-devname)\" option is specified without parameter."
			exit 1
		fi
		OPT_DEBFULLNAME=$1

	elif [ "X$1" = "X-devmail" -o "X$1" = "X-DEVMAIL" -o "X$1" = "X--developer-email" -o "X$1" = "X--DEVELOPER-EMAIL" ]; then
		if [ "X${OPT_DEBEMAIL}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--developer-email(-devmail)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--developer-email(-devmail)\" option is specified without parameter."
			exit 1
		fi
		OPT_DEBEMAIL=$1

	elif [ "X$1" = "X-usepc" -o "X$1" = "X-USEPC" -o "X$1" = "X--use-packagecloudio-repo" -o "X$1" = "X--USE-PACKAGECLOUDIO-REPO" ]; then
		if [ "X${OPT_USE_PC_REPO}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--use-packagecloudio-repo(-usepc)\" or \"--not-use-packagecloudio-repo(-notpc)\" option."
			exit 1
		fi
		OPT_USE_PC_REPO="true"

	elif [ "X$1" = "X-notpc" -o "X$1" = "X-NOTPC" -o "X$1" = "X--not-use-packagecloudio-repo" -o "X$1" = "X--NOT-USE-PACKAGECLOUDIO-REPO" ]; then
		if [ "X${OPT_USE_PC_REPO}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--use-packagecloudio-repo(-usepc)\" or \"--not-use-packagecloudio-repo(-notpc)\" option."
			exit 1
		fi
		OPT_USE_PC_REPO="false"

	elif [ "X$1" = "X-pctoken" -o "X$1" = "X-PCTOKEN" -o "X$1" = "X--packagecloudio-token" -o "X$1" = "X--PACKAGECLOUDIO-TOKEN" ]; then
		if [ "X${OPT_PC_TOKEN}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--packagecloudio-token(-pctoken)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--packagecloudio-token(-pctoken)\" option is specified without parameter."
			exit 1
		fi
		OPT_PC_TOKEN=$1

	elif [ "X$1" = "X-pcowner" -o "X$1" = "X-PCOWNER" -o "X$1" = "X--packagecloudio-owner" -o "X$1" = "X--PACKAGECLOUDIO-OWNER" ]; then
		if [ "X${OPT_PC_OWNER}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--packagecloudio-owner(-pcowner)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--packagecloudio-owner(-pcowner)\" option is specified without parameter."
			exit 1
		fi
		OPT_PC_OWNER=$1

	elif [ "X$1" = "X-pcprepo" -o "X$1" = "X-PCPREPO" -o "X$1" = "X--packagecloudio-publish-repo" -o "X$1" = "X--PACKAGECLOUDIO-PUBLICH-REPO" ]; then
		if [ "X${OPT_PC_PUBLISH_REPO}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--packagecloudio-publish-repo(-pcprepo)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--packagecloudio-publish-repo(-pcprepo)\" option is specified without parameter."
			exit 1
		fi
		OPT_PC_PUBLISH_REPO=$1

	elif [ "X$1" = "X-pcdlrepo" -o "X$1" = "X-PCDLREPO" -o "X$1" = "X--packagecloudio-download-repo" -o "X$1" = "X--PACKAGECLOUDIO-DOWNLOAD-REPO" ]; then
		if [ "X${OPT_PC_DOWNLOAD_REPO}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--packagecloudio-download-repo(-pcdlrepo)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--packagecloudio-download-repo(-pcdlrepo)\" option is specified without parameter."
			exit 1
		fi
		OPT_PC_DOWNLOAD_REPO=$1
	fi
	shift
done

#
# Check only options that must be specified
#
if [ "X${OPT_OSTYPE}" = "X" ]; then
	echo "[ERROR] ${PRGNAME} : \"--ostype(-os)\" option is not specified."
	exit 1
else
	CI_OSTYPE=${OPT_OSTYPE}
fi

#---------------------------------------------------------------------
# Load variables from file
#---------------------------------------------------------------------
echo "[INFO] ${PRGNAME} : Load local variables with an external file."

if [ "X${OPT_OSTYPEVARS_FILE}" = "X" ]; then
	OSTYPEVARS_FILE="${MYSCRIPTDIR}/ostypevars.sh"
elif [ ! -f ${OPT_OSTYPEVARS_FILE} ]; then
	echo "[WARNING] ${PRGNAME} : not found ${OPT_OSTYPEVARS_FILE} file, then default(ostypevars.sh) file is used."
	OSTYPEVARS_FILE="${MYSCRIPTDIR}/ostypevars.sh"
else
	OSTYPEVARS_FILE=${OPT_OSTYPEVARS_FILE}
fi
if [ -f ${OSTYPEVARS_FILE} ]; then
	echo "[INFO] ${PRGNAME} : Load ${OSTYPEVARS_FILE} for local variables by os:type(${CI_OSTYPE})"
	. ${OSTYPEVARS_FILE}
fi
if [ "X${DIST_TAG}" = "X" ]; then
	echo "[ERROR] ${PRGNAME} : Distro/Version is not set, please check ${OSTYPEVARS_FILE} and check \"DIST_TAG\" varibale."
	exit 1
fi

#---------------------------------------------------------------------
# Merge other variables
#---------------------------------------------------------------------
echo "[INFO] ${PRGNAME} : Set and check local variables."

#
# Check GITHUB Environment
#
if [ "X${GITHUB_EVENT_NAME}" = "Xschedule" ]; then
	IN_SCHEDULE_PROCESS=1
else
	IN_SCHEDULE_PROCESS=0
fi
PUBLISH_TAG_NAME=
if [ "X${GITHUB_REF}" != "X" ]; then
	echo ${GITHUB_REF} | grep 'refs/tags/' >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		PUBLISH_TAG_NAME=`echo ${GITHUB_REF} | sed 's#refs/tags/##g'`
	fi
fi

#
# Check whether to publish
#
if [ "X${OPT_IS_FORCE_PUBLISH}" = "X" ]; then
	if [ ${IN_SCHEDULE_PROCESS} -ne 1 ]; then
		if [ "X${PUBLISH_TAG_NAME}" != "X" ]; then
			IS_PUBLISH=1
		else
			IS_PUBLISH=0
		fi
	else
		IS_PUBLISH=0
	fi
elif [ "X${OPT_IS_FORCE_PUBLISH}" = "Xtrue" ]; then
	#
	# FORCE PUBLISH
	#
	if [ ${IN_SCHEDULE_PROCESS} -ne 1 ]; then
		if [ "X${PUBLISH_TAG_NAME}" != "X" ]; then
			echo "[INFO] ${PRGNAME} : specified \"--force-publish(-p)\" option, then forcibly publish"
			IS_PUBLISH=1
		else
			echo "[WARNING] ${PRGNAME} : specified \"--force-publish(-p)\" option, but not find relase tag."
			IS_PUBLISH=0
		fi
	else
		echo "[WARNING] ${PRGNAME} : specified \"--force-publish(-p)\" option, but not publish because this process is kicked by scheduler."
		IS_PUBLISH=0
	fi
else
	#
	# FORCE NOT PUBLISH
	#
	IS_PUBLISH=0
fi

#
# Set variables for packaging
#
if [ "X${OPT_BUILD_NUMBER}" != "X" ]; then
	BUILD_NUMBER=${BUILD_NUMBER}
else
	BUILD_NUMBER=1
fi
if [ "X${OPT_DEBEMAIL}" != "X" ]; then
	export DEBEMAIL=${OPT_DEBEMAIL}
fi
if [ "X${OPT_DEBFULLNAME}" != "X" ]; then
	export DEBFULLNAME=${OPT_DEBFULLNAME}
fi

#
# Set variables for packagecloud.io
#
if [ "X${OPT_USE_PC_REPO}" = "Xfalse" ]; then
	USE_PC_REPO=0
else
	USE_PC_REPO=1
fi
if [ "X${OPT_PC_TOKEN}" != "X" ]; then
	PC_TOKEN=${OPT_PC_TOKEN}
else
	PC_TOKEN=
fi
if [ "X${OPT_PC_OWNER}" != "X" ]; then
	PC_OWNER=${OPT_PC_OWNER}
else
	PC_OWNER="antpickax"
fi
if [ "X${OPT_PC_PUBLISH_REPO}" != "X" ]; then
	PC_PUBLISH_REPO=${OPT_PC_PUBLISH_REPO}
else
	PC_PUBLISH_REPO="current"
fi
if [ "X${OPT_PC_DOWNLOAD_REPO}" != "X" ]; then
	PC_DOWNLOAD_REPO=${OPT_PC_DOWNLOAD_REPO}
else
	PC_DOWNLOAD_REPO="stable"
fi

#
# Information
#
echo "[INFO] ${PRGNAME} : All local variables for building and packaging."
echo "  PRGNAME             = ${PRGNAME}"
echo "  MYSCRIPTDIR         = ${MYSCRIPTDIR}"
echo "  SRCTOP              = ${SRCTOP}"
echo "  CI_OSTYPE           = ${CI_OSTYPE}"
echo "  IS_OS_UBUNTU        = ${IS_OS_UBUNTU}"
echo "  IS_OS_DEBIAN        = ${IS_OS_DEBIAN}"
echo "  IS_OS_CENTOS        = ${IS_OS_CENTOS}"
echo "  IS_OS_FEDORA        = ${IS_OS_FEDORA}"
echo "  OSTYPEVARS_FILE     = ${OSTYPEVARS_FILE}"
echo "  DIST_TAG            = ${DIST_TAG}"
echo "  INSTALL_PKG_LIST    = ${INSTALL_PKG_LIST}"
echo "  CONFIGURE_EXT_OPT   = ${CONFIGURE_EXT_OPT}"
echo "  IN_SCHEDULE_PROCESS = ${IN_SCHEDULE_PROCESS}"
echo "  INSTALLER_BIN       = ${INSTALLER_BIN}"
echo "  PKG_TYPE_DEB        = ${PKG_TYPE_DEB}"
echo "  PKG_TYPE_RPM        = ${PKG_TYPE_RPM}"
echo "  PKG_OUTPUT_DIR      = ${PKG_OUTPUT_DIR}"
echo "  PKG_EXT             = ${PKG_EXT}"
echo "  IS_PUBLISH          = ${IS_PUBLISH}"
echo "  PUBLISH_TAG_NAME    = ${PUBLISH_TAG_NAME}"
echo "  BUILD_NUMBER        = ${BUILD_NUMBER}"
echo "  DEBEMAIL            = ${DEBEMAIL}"
echo "  DEBFULLNAME         = ${DEBFULLNAME}"
echo "  USE_PC_REPO         = ${USE_PC_REPO}"
echo "  PC_TOKEN            = **********"
echo "  PC_OWNER            = ${PC_OWNER}"
echo "  PC_PUBLISH_REPO     = ${PC_PUBLISH_REPO}"
echo "  PC_DOWNLOAD_REPO    = ${PC_DOWNLOAD_REPO}"

#---------------------------------------------------------------------
# Set package repository on packagecloud.io before build
#---------------------------------------------------------------------
if [ ${USE_PC_REPO} -eq 1 ]; then
	echo "[INFO] ${PRGNAME} : Setup packagecloud.io repository."

	#
	# Check curl
	#
	curl --version >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		run_cmd ${INSTALLER_BIN} update -y ${INSTALL_QUIET_ARG}
		run_cmd ${INSTALLER_BIN} install -y ${INSTALL_QUIET_ARG} curl
	fi

	#
	# Download and set packagecloud.io repository
	#
	if [ ${IS_OS_CENTOS} -eq 1 -o ${IS_OS_FEDORA} -eq 1 ]; then
		PC_REPO_ADD_SH="script.rpm.sh"
	else
		PC_REPO_ADD_SH="script.deb.sh"
	fi
	prn_cmd "curl -s https://packagecloud.io/install/repositories/${PC_OWNER}/${PC_DOWNLOAD_REPO}/${PC_REPO_ADD_SH} | bash"
	curl -s https://packagecloud.io/install/repositories/${PC_OWNER}/${PC_DOWNLOAD_REPO}/${PC_REPO_ADD_SH} | bash
	if [ $? -ne 0 ]; then
		echo "[ERROR] ${PRGNAME} : could not add packagecloud.io repository."
		exit 1
	fi
fi

#---------------------------------------------------------------------
# Install packages
#---------------------------------------------------------------------
#
# Update
#
if [ ${IS_OS_UBUNTU} -eq 1 -o ${IS_OS_DEBIAN} -eq 1 ]; then
	# [NOTE]
	# When start to update, it may come across an unexpected interactive interface.
	# (May occur with time zone updates)
	# Set environment variables to avoid this.
	#
	export DEBIAN_FRONTEND=noninteractive 
fi
echo "[INFO] ${PRGNAME} : Update local packages."
run_cmd ${INSTALLER_BIN} update -y ${INSTALL_QUIET_ARG}

#
# Install
#
if [ "X${INSTALL_PKG_LIST}" != "X" ]; then
	echo "[INFO] ${PRGNAME} : Install packages."
	run_cmd ${INSTALLER_BIN} install -y ${INSTALL_QUIET_ARG} ${INSTALL_PKG_LIST}
fi

#
# Install packagecloud.io CLI tool(package_cloud)
#
if [ ${IS_PUBLISH} -eq 1 ]; then
	echo "[INFO] ${PRGNAME} : Install packagecloud.io CLI tool(package_cloud)."
	GEM_BIN="gem"

	if [ ${IS_OS_CENTOS} -eq 1 ]; then
		#
		# For CentOS
		#
		if [ "X${CI_OSTYPE}" = "Xcentos:7" -o "X${CI_OSTYPE}" = "Xcentos:centos7" -o "X${CI_OSTYPE}" = "Xcentos:6" -o "X${CI_OSTYPE}" = "Xcentos:centos6" ]; then
			#
			# For CentOS6/7, using RHSCL because centos has older ruby
			#
			run_cmd ${INSTALLER_BIN} install -y ${INSTALL_QUIET_ARG} centos-release-scl
			run_cmd ${INSTALLER_BIN} install -y ${INSTALL_QUIET_ARG} rh-ruby24 rh-ruby24-ruby-devel rh-ruby24-rubygem-rake
			source /opt/rh/rh-ruby24/enable
		fi
		run_cmd ${GEM_BIN} install package_cloud
	else
		#
		# For other than CentOS
		#
		run_cmd ${GEM_BIN} install rake package_cloud
	fi
fi

#
# Install cppcheck
#
echo "[INFO] ${PRGNAME} : Install cppcheck."

if [ ${IS_OS_CENTOS} -eq 1 ]; then
	if [ "X${CI_OSTYPE}" = "Xcentos:7" -o "X${CI_OSTYPE}" = "Xcentos:centos7" -o "X${CI_OSTYPE}" = "Xcentos:6" -o "X${CI_OSTYPE}" = "Xcentos:centos6" ]; then
		#
		# For CentOS6/7, it need to allow EPEL(with setting disable)
		#
		run_cmd ${INSTALLER_BIN} install -y ${INSTALL_QUIET_ARG} epel-release
		run_cmd yum-config-manager --disable epel
		run_cmd ${INSTALLER_BIN} --enablerepo=epel install -y ${INSTALL_QUIET_ARG} cppcheck
	else
		#
		# For CentOS8, it is installed from PowerTools( PwoerTools -> powertools at 2020/12 )
		#
		run_cmd ${INSTALLER_BIN} --enablerepo=powertools install -y ${INSTALL_QUIET_ARG} cppcheck
	fi
else
	#
	# For other than CentOS
	#
	run_cmd ${INSTALLER_BIN} install -y ${INSTALL_QUIET_ARG} cppcheck
fi

#---------------------------------------------------------------------
# Build (using /tmp directory)
#---------------------------------------------------------------------
#
# Copy sources to /tmp directory
#
echo "[INFO] ${PRGNAME} : Copy sources to /tmp directory."
run_cmd cp -rp ${SRCTOP} /tmp
TMPSRCTOP=`basename ${SRCTOP}`
BUILD_SRCTOP="/tmp/${TMPSRCTOP}"

#
# Change current directory
#
run_cmd cd ${BUILD_SRCTOP}

#
# Start build
#
echo "[INFO] ${PRGNAME} : Build - run autogen."
run_cmd ./autogen.sh

echo "[INFO] ${PRGNAME} : Build - run configure."
run_cmd ./configure --prefix=/usr ${CONFIGURE_EXT_OPT}

echo "[INFO] ${PRGNAME} : Build - run cppcheck."
run_cmd make cppcheck

echo "[INFO] ${PRGNAME} : Build - run make."
run_cmd make

echo "[INFO] ${PRGNAME} : Build - run check(test)."
run_cmd make check

#---------------------------------------------------------------------
# Start packaging
#---------------------------------------------------------------------
echo "[INFO] ${PRGNAME} : Start packaging."

if [ ${PKG_TYPE_RPM} -eq 1 ]; then
	#
	# Create rpm packages
	#
	prn_cmd CONFIGUREOPT=${CONFIGURE_EXT_OPT} ./buildutils/rpm_build.sh --buildnum ${BUILD_NUMBER} -y
	CONFIGUREOPT=${CONFIGURE_EXT_OPT} ./buildutils/rpm_build.sh --buildnum ${BUILD_NUMBER} -y
else
	#
	# Create debian packages
	#
	DEBUILD_OPT=""
	if [ ${IS_PUBLISH} -ne 1 ]; then
		DEBUILD_OPT="-nodebuild"
	fi
	prn_cmd CONFIGUREOPT=${CONFIGURE_EXT_OPT} ./buildutils/debian_build.sh -buildnum ${BUILD_NUMBER} -disttype ${DIST_TAG} ${DEBUILD_OPT} -y
	CONFIGUREOPT=${CONFIGURE_EXT_OPT} ./buildutils/debian_build.sh -buildnum ${BUILD_NUMBER} -disttype ${DIST_TAG} ${DEBUILD_OPT} -y
fi
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : Failed to build packages"
	exit 1
fi

#---------------------------------------------------------------------
# Start publishing
#---------------------------------------------------------------------
if [ ${IS_PUBLISH} -eq 1 ]; then
	echo "[INFO] ${PRGNAME} : Start publishing."

	if [ "X${PC_TOKEN}" = "X" ]; then
		echo "[ERROR] ${PRGNAME} : Token for uploading to packagecloud.io is not specified."
		exit 1
	fi
	PC_PUBLISH_PATH="${PC_OWNER}/${PC_PUBLISH_REPO}/${DIST_TAG}"

	prn_cmd PACKAGECLOUD_TOKEN=${PC_TOKEN} package_cloud push ${PC_PUBLISH_PATH} ${BUILD_SRCTOP}/${PKG_OUTPUT_DIR}/*.${PKG_EXT}
	PACKAGECLOUD_TOKEN=${PC_TOKEN} package_cloud push ${PC_PUBLISH_PATH} ${BUILD_SRCTOP}/${PKG_OUTPUT_DIR}/*.${PKG_EXT}
	if [ $? -ne 0 ]; then
		echo "[ERROR] ${PRGNAME} : Failed to publish *.${PKG_EXT} packages"
		exit 1
	fi
fi

#---------------------------------------------------------------------
# Finish
#---------------------------------------------------------------------
echo "[SUCCESS] ${PRGNAME} : Finished without error."
exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
