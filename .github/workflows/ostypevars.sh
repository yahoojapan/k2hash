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
# CREATE:   Fri, Nov 13 2020
# REVISION:	1.0
#

#---------------------------------------------------------------------
# About this file
#---------------------------------------------------------------------
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
#   PKG_TYPE_DEB      : Set to 1 for debian packages, 0 otherwise
#   PKG_TYPE_RPM      : Set to 1 for rpm packages, 0 otherwise
#   PKG_OUTPUT_DIR    : Set the directory path where the package will
#                       be created relative to the top directory of the
#                       source
#   PKG_EXT           : The extension of the package file
#   IS_OS_UBUNTU      : Set to 1 for Ubuntu, 0 otherwise
#   IS_OS_DEBIAN      : Set to 1 for Debian, 0 otherwise
#   IS_OS_CENTOS      : Set to 1 for CentOS, 0 otherwise
#   IS_OS_FEDORA      : Set to 1 for Fedora, 0 otherwise
#
# Set these variables according to the CI_OSTYPE variable.
# The value of the CI_OSTYPE variable matches the name of the
# Container (docker image) used in Github Actions.
# Check the ".github/workflow/***.yml" file for the value.
#

#---------------------------------------------------------------------
# Default values
#---------------------------------------------------------------------
DIST_TAG=
INSTALL_PKG_LIST=
CONFIGURE_EXT_OPT=
INSTALLER_BIN=
PKG_TYPE_DEB=0
PKG_TYPE_RPM=0
PKG_OUTPUT_DIR=
PKG_EXT=
IS_OS_UBUNTU=0
IS_OS_DEBIAN=0
IS_OS_CENTOS=0
IS_OS_FEDORA=0

#---------------------------------------------------------------------
# Variables for each OS Type
#---------------------------------------------------------------------
if [ "X${CI_OSTYPE}" = "Xubuntu:20.04" -o "X${CI_OSTYPE}" = "Xubuntu:focal" ]; then
	DIST_TAG="ubuntu/focal"
	INSTALL_PKG_LIST="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config ruby-dev rubygems rubygems-integration procps libfullock-dev libgcrypt20-dev"
	CONFIGURE_EXT_OPT="--with-gcrypt"
	INSTALLER_BIN="apt-get"
	INSTALL_QUIET_ARG="-qq"
	PKG_TYPE_DEB=1
	PKG_TYPE_RPM=0
	PKG_OUTPUT_DIR="debian_build"
	PKG_EXT="deb"
	IS_OS_UBUNTU=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2

elif [ "X${CI_OSTYPE}" = "Xubuntu:18.04" -o "X${CI_OSTYPE}" = "Xubuntu:bionic" ]; then
	DIST_TAG="ubuntu/bionic"
	INSTALL_PKG_LIST="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config ruby-dev rubygems rubygems-integration procps libfullock-dev libgcrypt20-dev"
	CONFIGURE_EXT_OPT="--with-gcrypt"
	INSTALLER_BIN="apt-get"
	INSTALL_QUIET_ARG="-qq"
	PKG_TYPE_DEB=1
	PKG_TYPE_RPM=0
	PKG_OUTPUT_DIR="debian_build"
	PKG_EXT="deb"
	IS_OS_UBUNTU=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2

elif [ "X${CI_OSTYPE}" = "Xubuntu:16.04" -o "X${CI_OSTYPE}" = "Xubuntu:xenial" ]; then
	DIST_TAG="ubuntu/xenial"
	INSTALL_PKG_LIST="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config ruby-dev rubygems rubygems-integration procps libfullock-dev libgcrypt20-dev"
	CONFIGURE_EXT_OPT="--with-gcrypt"
	INSTALLER_BIN="apt-get"
	INSTALL_QUIET_ARG="-qq"
	PKG_TYPE_DEB=1
	PKG_TYPE_RPM=0
	PKG_OUTPUT_DIR="debian_build"
	PKG_EXT="deb"
	IS_OS_UBUNTU=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2

elif [ "X${CI_OSTYPE}" = "Xdebian:10" -o "X${CI_OSTYPE}" = "Xdebian:buster" ]; then
	DIST_TAG="debian/buster"
	INSTALL_PKG_LIST="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config ruby-dev rubygems rubygems-integration procps libfullock-dev libgcrypt20-dev"
	CONFIGURE_EXT_OPT="--with-gcrypt"
	INSTALLER_BIN="apt-get"
	INSTALL_QUIET_ARG="-qq"
	PKG_TYPE_DEB=1
	PKG_TYPE_RPM=0
	PKG_OUTPUT_DIR="debian_build"
	PKG_EXT="deb"
	IS_OS_DEBIAN=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2

elif [ "X${CI_OSTYPE}" = "Xdebian:9" -o "X${CI_OSTYPE}" = "Xdebian:stretch" ]; then
	DIST_TAG="debian/stretch"
	INSTALL_PKG_LIST="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config ruby-dev rubygems rubygems-integration procps libfullock-dev libgcrypt20-dev"
	CONFIGURE_EXT_OPT="--with-gcrypt"
	INSTALLER_BIN="apt-get"
	INSTALL_QUIET_ARG="-qq"
	PKG_TYPE_DEB=1
	PKG_TYPE_RPM=0
	PKG_OUTPUT_DIR="debian_build"
	PKG_EXT="deb"
	IS_OS_DEBIAN=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2

elif [ "X${CI_OSTYPE}" = "Xcentos:8" -o "X${CI_OSTYPE}" = "Xcentos:centos8" ]; then
	DIST_TAG="el/8"
	INSTALL_PKG_LIST="git autoconf automake gcc gcc-c++ gdb make libtool pkgconfig redhat-rpm-config rpm-build ruby-devel rubygems procps libfullock-devel nss-devel"
	CONFIGURE_EXT_OPT="--with-nss"
	INSTALLER_BIN="dnf"
	INSTALL_QUIET_ARG="-qq"
	PKG_TYPE_DEB=0
	PKG_TYPE_RPM=1
	PKG_OUTPUT_DIR="."
	PKG_EXT="rpm"
	IS_OS_CENTOS=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2

	#
	# Change mirrorlist
	#
	sed -i -e 's|^mirrorlist|#mirrorlist|g' -e 's|^#baseurl=http://mirror|baseurl=http://vault|g' /etc/yum.repos.d/CentOS-*repo

elif [ "X${CI_OSTYPE}" = "Xcentos:7" -o "X${CI_OSTYPE}" = "Xcentos:centos7" ]; then
	DIST_TAG="el/7"
	INSTALL_PKG_LIST="git autoconf automake gcc gcc-c++ gdb make libtool pkgconfig redhat-rpm-config rpm-build ruby-devel rubygems procps libfullock-devel nss-devel"
	CONFIGURE_EXT_OPT="--with-nss"
	INSTALLER_BIN="yum"
	INSTALL_QUIET_ARG=""
	PKG_TYPE_DEB=0
	PKG_TYPE_RPM=1
	PKG_OUTPUT_DIR="."
	PKG_EXT="rpm"
	IS_OS_CENTOS=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2

elif [ "X${CI_OSTYPE}" = "Xfedora:32" ]; then
	DIST_TAG="fedora/32"
	INSTALL_PKG_LIST="git autoconf automake gcc gcc-c++ gdb make libtool pkgconfig redhat-rpm-config rpm-build ruby-devel rubygems procps libfullock-devel nss-devel"
	CONFIGURE_EXT_OPT="--with-nss"
	INSTALLER_BIN="dnf"
	INSTALL_QUIET_ARG="-qq"
	PKG_TYPE_DEB=0
	PKG_TYPE_RPM=1
	PKG_OUTPUT_DIR="."
	PKG_EXT="rpm"
	IS_OS_FEDORA=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2

elif [ "X${CI_OSTYPE}" = "Xfedora:31" ]; then
	DIST_TAG="fedora/31"
	INSTALL_PKG_LIST="git autoconf automake gcc gcc-c++ gdb make libtool pkgconfig redhat-rpm-config rpm-build ruby-devel rubygems procps libfullock-devel nss-devel"
	CONFIGURE_EXT_OPT="--with-nss"
	INSTALLER_BIN="dnf"
	INSTALL_QUIET_ARG="-qq"
	PKG_TYPE_DEB=0
	PKG_TYPE_RPM=1
	PKG_OUTPUT_DIR="."
	PKG_EXT="rpm"
	IS_OS_FEDORA=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2

elif [ "X${CI_OSTYPE}" = "Xfedora:30" ]; then
	DIST_TAG="fedora/30"
	INSTALL_PKG_LIST="git autoconf automake gcc gcc-c++ gdb make libtool pkgconfig redhat-rpm-config rpm-build ruby-devel rubygems procps libfullock-devel nss-devel"
	CONFIGURE_EXT_OPT="--with-nss"
	INSTALLER_BIN="dnf"
	INSTALL_QUIET_ARG="-qq"
	PKG_TYPE_DEB=0
	PKG_TYPE_RPM=1
	PKG_OUTPUT_DIR="."
	PKG_EXT="rpm"
	IS_OS_FEDORA=1
	# special variables
	export K2HATTR_ENC_TYPE=AES256_PBKDF2
fi

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
