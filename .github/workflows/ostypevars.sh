#
# K2HASH
#
# Utility tools for building configure/packages by AntPickax
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
# CREATE:   Fri, Nov 13 2020
# REVISION:	1.1
#

#===============================================================
# Configuration for build_helper.sh
#===============================================================
# This file is loaded into the build_helper.sh script.
# The build_helper.sh script is a Github Actions helper script that
# builds and packages the target repository.
# This file is mainly created to define variables that differ depending
# on the OS and version.
# It also contains different information(such as packages to install)
# for each repository.
#
# In the initial state, you need to set the following variables:
#   DIST_TAG          : "Distro/Version" for publishing packages
#   INSTALL_PKG_LIST  : A list of packages to be installed for build and
#                       packaging
#   CONFIGURE_EXT_OPT : Options to specify when running configure
#   INSTALLER_BIN     : Package management command
#   UPDATE_CMD        : Update sub command for package management command
#   UPDATE_CMD_ARG    : Update sub command arguments for package management command
#   INSTALL_CMD       : Install sub command for package management command
#   INSTALL_CMD_ARG   : Install sub command arguments for package management command
#   INSTALL_AUTO_ARG  : No interaption arguments for package management command
#   INSTALL_QUIET_ARG : Quiet arguments for package management command
#   PKG_OUTPUT_DIR    : Set the directory path where the package will
#                       be created relative to the top directory of the
#                       source
#   PKG_EXT           : The extension of the package file
#   IS_OS_UBUNTU      : Set to 1 for Ubuntu, 0 otherwise
#   IS_OS_DEBIAN      : Set to 1 for Debian, 0 otherwise
#   IS_OS_FEDORA      : Set to 1 for Fedora, 0 otherwise
#   IS_OS_ROCKY       : Set to 1 for Rocky, 0 otherwise
#   IS_OS_ALPINE      : Set to 1 for Alpine, 0 otherwise
#   IS_OPENSSL_TYPE   : Set to 1 for using openssl library, 0 otherwise
#
# Set these variables according to the CI_OSTYPE variable.
# The value of the CI_OSTYPE variable matches the name of the
# Container (docker image) used in Github Actions.
# Check the ".github/workflow/***.yml" file for the value.
#

#----------------------------------------------------------
# Default values
#----------------------------------------------------------
DIST_TAG=""
INSTALL_PKG_LIST=""
CONFIGURE_EXT_OPT=""
INSTALLER_BIN=""
UPDATE_CMD=""
UPDATE_CMD_ARG=""
INSTALL_CMD=""
INSTALL_CMD_ARG=""
INSTALL_AUTO_ARG=""
INSTALL_QUIET_ARG=""
PKG_OUTPUT_DIR=""
PKG_EXT=""

IS_OS_UBUNTU=0
IS_OS_DEBIAN=0
IS_OS_FEDORA=0
IS_OS_ROCKY=0
IS_OS_ALPINE=0
IS_OPENSSL_TYPE=1

#----------------------------------------------------------
# Variables for each OS Type
#----------------------------------------------------------
if [ -z "${CI_OSTYPE}" ]; then
	#
	# Unknown OS : Nothing to do
	#
	:

elif echo "${CI_OSTYPE}" | grep -q -i -e "ubuntu:24.04" -e "ubuntu:noble"; then
	DIST_TAG="ubuntu/noble"
	INSTALL_PKG_LIST="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config ruby-dev rubygems rubygems-integration procps libfullock-dev libssl-dev"
	INSTALLER_BIN="apt-get"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG=""
	INSTALL_CMD="install"
	INSTALL_CMD_ARG=""
	INSTALL_AUTO_ARG="-y"
	INSTALL_QUIET_ARG="-qq"
	PKG_OUTPUT_DIR="debian_build"
	PKG_EXT="deb"
	IS_OS_UBUNTU=1

elif echo "${CI_OSTYPE}" | grep -q -i -e "ubuntu:22.04" -e "ubuntu:jammy"; then
	DIST_TAG="ubuntu/jammy"
	INSTALL_PKG_LIST="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config ruby-dev rubygems rubygems-integration procps libfullock-dev libssl-dev"
	INSTALLER_BIN="apt-get"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG=""
	INSTALL_CMD="install"
	INSTALL_CMD_ARG=""
	INSTALL_AUTO_ARG="-y"
	INSTALL_QUIET_ARG="-qq"
	PKG_OUTPUT_DIR="debian_build"
	PKG_EXT="deb"
	IS_OS_UBUNTU=1

elif echo "${CI_OSTYPE}" | grep -q -i -e "debian:12" -e "debian:bookworm"; then
	DIST_TAG="debian/bookworm"
	INSTALL_PKG_LIST="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config ruby-dev rubygems rubygems-integration procps libfullock-dev libssl-dev"
	INSTALLER_BIN="apt-get"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG=""
	INSTALL_CMD="install"
	INSTALL_CMD_ARG=""
	INSTALL_AUTO_ARG="-y"
	INSTALL_QUIET_ARG="-qq"
	PKG_OUTPUT_DIR="debian_build"
	PKG_EXT="deb"
	IS_OS_DEBIAN=1

elif echo "${CI_OSTYPE}" | grep -q -i -e "debian:11" -e "debian:bullseye"; then
	DIST_TAG="debian/bullseye"
	INSTALL_PKG_LIST="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config ruby-dev rubygems rubygems-integration procps libfullock-dev libgcrypt20-dev"
	INSTALLER_BIN="apt-get"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG=""
	INSTALL_CMD="install"
	INSTALL_CMD_ARG=""
	INSTALL_AUTO_ARG="-y"
	INSTALL_QUIET_ARG="-qq"
	PKG_OUTPUT_DIR="debian_build"
	PKG_EXT="deb"
	IS_OS_DEBIAN=1
	IS_OPENSSL_TYPE=0

elif echo "${CI_OSTYPE}" | grep -q -i "rockylinux:9"; then
	DIST_TAG="el/9"
	INSTALL_PKG_LIST="git autoconf automake gcc gcc-c++ gdb make libtool pkgconfig redhat-rpm-config rpm-build ruby-devel rubygems procps libfullock-devel openssl-devel"
	INSTALLER_BIN="dnf"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG=""
	INSTALL_CMD="install"
	INSTALL_CMD_ARG=""
	INSTALL_AUTO_ARG="-y"
	INSTALL_QUIET_ARG="-q"
	PKG_OUTPUT_DIR="."
	PKG_EXT="rpm"
	IS_OS_ROCKY=1

elif echo "${CI_OSTYPE}" | grep -q -i "rockylinux:8"; then
	DIST_TAG="el/8"
	INSTALL_PKG_LIST="git autoconf automake gcc gcc-c++ gdb make libtool pkgconfig redhat-rpm-config rpm-build ruby-devel rubygems procps libfullock-devel nss-devel"
	INSTALLER_BIN="dnf"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG=""
	INSTALL_CMD="install"
	INSTALL_CMD_ARG=""
	INSTALL_AUTO_ARG="-y"
	INSTALL_QUIET_ARG="-q"
	PKG_OUTPUT_DIR="."
	PKG_EXT="rpm"
	IS_OS_ROCKY=1
	IS_OPENSSL_TYPE=0

elif echo "${CI_OSTYPE}" | grep -q -i "fedora:41"; then
	DIST_TAG="fedora/41"
	INSTALL_PKG_LIST="git autoconf automake gcc gcc-c++ gdb make libtool pkgconfig redhat-rpm-config rpm-build ruby-devel rubygems procps libfullock-devel openssl-devel"
	INSTALLER_BIN="dnf"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG=""
	INSTALL_CMD="install"
	INSTALL_CMD_ARG=""
	INSTALL_AUTO_ARG="-y"
	INSTALL_QUIET_ARG="-q"
	PKG_OUTPUT_DIR="."
	PKG_EXT="rpm"
	IS_OS_FEDORA=1

elif echo "${CI_OSTYPE}" | grep -q -i "fedora:40"; then
	DIST_TAG="fedora/40"
	INSTALL_PKG_LIST="git autoconf automake gcc gcc-c++ gdb make libtool pkgconfig redhat-rpm-config rpm-build ruby-devel rubygems procps libfullock-devel openssl-devel"
	INSTALLER_BIN="dnf"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG=""
	INSTALL_CMD="install"
	INSTALL_CMD_ARG=""
	INSTALL_AUTO_ARG="-y"
	INSTALL_QUIET_ARG="-q"
	PKG_OUTPUT_DIR="."
	PKG_EXT="rpm"
	IS_OS_FEDORA=1

elif echo "${CI_OSTYPE}" | grep -q -i "alpine:3.21"; then
	DIST_TAG="alpine/v3.21"
	INSTALL_PKG_LIST="bash sudo alpine-sdk automake autoconf libtool groff util-linux-misc musl-locales ruby-dev procps libfullock libfullock-dev openssl-dev"
	INSTALLER_BIN="apk"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG="--no-progress"
	INSTALL_CMD="add"
	INSTALL_CMD_ARG="--no-progress --no-cache"
	INSTALL_AUTO_ARG=""
	INSTALL_QUIET_ARG="-q"
	PKG_OUTPUT_DIR="apk_build"
	PKG_EXT="apk"
	IS_OS_ALPINE=1

elif echo "${CI_OSTYPE}" | grep -q -i "alpine:3.20"; then
	DIST_TAG="alpine/v3.20"
	INSTALL_PKG_LIST="bash sudo alpine-sdk automake autoconf libtool groff util-linux-misc musl-locales ruby-dev procps libfullock libfullock-dev openssl-dev"
	INSTALLER_BIN="apk"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG="--no-progress"
	INSTALL_CMD="add"
	INSTALL_CMD_ARG="--no-progress --no-cache"
	INSTALL_AUTO_ARG=""
	INSTALL_QUIET_ARG="-q"
	PKG_OUTPUT_DIR="apk_build"
	PKG_EXT="apk"
	IS_OS_ALPINE=1

elif echo "${CI_OSTYPE}" | grep -q -i "alpine:3.19"; then
	DIST_TAG="alpine/v3.19"
	INSTALL_PKG_LIST="bash sudo alpine-sdk automake autoconf libtool groff util-linux-misc musl-locales ruby-dev procps libfullock libfullock-dev openssl-dev"
	INSTALLER_BIN="apk"
	UPDATE_CMD="update"
	UPDATE_CMD_ARG="--no-progress"
	INSTALL_CMD="add"
	INSTALL_CMD_ARG="--no-progress --no-cache"
	INSTALL_AUTO_ARG=""
	INSTALL_QUIET_ARG="-q"
	PKG_OUTPUT_DIR="apk_build"
	PKG_EXT="apk"
	IS_OS_ALPINE=1
fi

#---------------------------------------------------------------
# Enable/Disable processing
#---------------------------------------------------------------
# [NOTE]
# Specify the phase of processing to use.
# The phases that can be specified are the following values, and
# the default is set for C/C++ processing.
# Setting this value to 1 enables the corresponding processing,
# setting it to 0 disables it.
#
#	<variable name>		<default value>
#	RUN_PRE_CONFIG			1
#	RUN_CONFIG				1
#	RUN_PRE_CLEANUP			0
#	RUN_CLEANUP				1
#	RUN_POST_CLEANUP		0
#	RUN_CPPCHECK			1
#	RUN_SHELLCHECK			1
#	RUN_CHECK_OTHER			0
#	RUN_PRE_BUILD			0
#	RUN_BUILD				1
#	RUN_POST_BUILD			0
#	RUN_PRE_TEST			0
#	RUN_TEST				1
#	RUN_POST_TEST			0
#	RUN_PRE_PACKAGE			0
#	RUN_PACKAGE				1
#	RUN_POST_PACKAGE		0
#	RUN_PUBLISH_PACKAGE		1
#

#---------------------------------------------------------------
# Variables for each process
#---------------------------------------------------------------
# [NOTE]
# Specify the following variables that can be specified in each phase.
# Each value has a default value for C/C++ processing.
#
#	AUTOGEN_EXT_OPT_RPM				""
#	AUTOGEN_EXT_OPT_DEBIAN			""
#	AUTOGEN_EXT_OPT_ALPINE			""
#	AUTOGEN_EXT_OPT_OTHER			""
#
#	CONFIGURE_EXT_OPT_RPM			""
#	CONFIGURE_EXT_OPT_DEBIAN		""
#	CONFIGURE_EXT_OPT_ALPINE		""
#	CONFIGURE_EXT_OPT_OTHER			""
#
#	BUILD_MAKE_EXT_OPT_RPM			""
#	BUILD_MAKE_EXT_OPT_DEBIAN		""
#	BUILD_MAKE_EXT_OPT_ALPINE		""
#	BUILD_MAKE_EXT_OPT_OTHER		""
#
#	MAKE_TEST_OPT_RPM				"check"
#	MAKE_TEST_OPT_DEBIAN			"check"
#	MAKE_TEST_OPT_ALPINE			"check"
#	MAKE_TEST_OPT_OTHER				"check"
#
#	CREATE_PACKAGE_TOOL_RPM			"buildutils/rpm_build.sh"
#	CREATE_PACKAGE_TOOL_DEBIAN		"buildutils/debian_build.sh"
#	CREATE_PACKAGE_TOOL_ALPINE		"buildutils/apline_build.sh"
#	CREATE_PACKAGE_TOOL_OTHER		""
#
#	CREATE_PACKAGE_TOOL_OPT_AUTO	"-y"
#	CREATE_PACKAGE_TOOL_OPT_RPM		""
#	CREATE_PACKAGE_TOOL_OPT_DEBIAN	""
#	CREATE_PACKAGE_TOOL_OPT_ALPINE	""
#	CREATE_PACKAGE_TOOL_OPT_OTHER	""
#
if [ "${IS_OS_UBUNTU}" -eq 1 ] && [ "${IS_OPENSSL_TYPE}" -eq 0 ]; then
	CONFIGURE_EXT_OPT_DEBIAN="--with-gcrypt"
elif [ "${IS_OS_DEBIAN}" -eq 1 ] && [ "${IS_OPENSSL_TYPE}" -eq 0 ]; then
	CONFIGURE_EXT_OPT_DEBIAN="--with-gcrypt"
elif [ "${IS_OS_ROCKY}" -eq 1 ] && [ "${IS_OPENSSL_TYPE}" -eq 0 ]; then
	CONFIGURE_EXT_OPT_RPM="--with-nss"
fi

if [ "${IS_OS_UBUNTU}" -eq 1 ] || [ "${IS_OS_DEBIAN}" -eq 1 ]; then
	CREATE_PACKAGE_TOOL_OPT_DEBIAN="--disttype ${DIST_TAG}"
fi

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
