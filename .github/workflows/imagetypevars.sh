#
# K2HASH
#
# Utility tools for building configure/packages by AntPickax
#
# Copyright 2021 Yahoo Japan Corporation.
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
# CREATE:   Tue, Aug 10 2021
# REVISION:
#

#---------------------------------------------------------------------
# About this file
#---------------------------------------------------------------------
# This file is loaded into the docker_helper.sh script.
# The docker_helper.sh script is a Github Actions helper script that
# builds docker images and pushes it to Docker Hub.
# This file is mainly created to define variables that differ depending
# on the base docker image.
# It also contains different information(such as packages to install)
# for each repository.
#
# Set following variables according to the CI_DOCKER_IMAGE_OSTYPE
# variable. The value of the CI_DOCKER_IMAGE_OSTYPE variable matches
# the name of the base docker image.(ex, alpine/ubuntu/...)
#

#---------------------------------------------------------------------
# Default values
#---------------------------------------------------------------------
PKGMGR_NAME=
PKGMGR_UPDATE_OPT=
PKGMGR_INSTALL_OPT=
PKG_INSTALL_LIST_BUILDER=
PKG_INSTALL_LIST_BIN=
BUILDER_CONFIGURE_FLAG=
BUILDER_MAKE_FLAGS=
BUILDER_ENVIRONMENT=
UPDATE_LIBPATH=

#
# List the package names that contain pacakgecloud.io to install on Github Actions Runner.
#
RUNNER_INSTALL_PACKAGES="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config procps libfullock-dev libgcrypt20-dev"

#
# Directory name to Dockerfile.templ file
#
DOCKERFILE_TEMPL_SUBDIR="buildutils"

#---------------------------------------------------------------------
# Variables for each Docker image Type
#---------------------------------------------------------------------
if [ -z "${CI_DOCKER_IMAGE_OSTYPE}" ]; then
	#
	# Unknown image OS type : Nothing to do
	#
	:
elif [ "${CI_DOCKER_IMAGE_OSTYPE}" = "alpine" ]; then
	PKGMGR_NAME="apk"
	PKGMGR_UPDATE_OPT="update -q --no-progress"
	PKGMGR_INSTALL_OPT="add -q --no-progress --no-cache"
	PKG_INSTALL_LIST_BUILDER="git build-base openssl-dev libtool automake autoconf pkgconf procps"
	PKG_INSTALL_LIST_BIN="libstdc++"

	#
	# Special build flags
	#
	BUILDER_MAKE_FLAGS="CXXFLAGS=-Wno-address-of-packed-member"

elif [ "${CI_DOCKER_IMAGE_OSTYPE}" = "ubuntu" ]; then
	PKGMGR_NAME="apt-get"
	PKGMGR_UPDATE_OPT="update -qq -y"
	PKGMGR_INSTALL_OPT="install -qq -y"
	PKG_INSTALL_LIST_BUILDER="git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config procps libgcrypt20-dev"
	PKG_INSTALL_LIST_BIN=""
	UPDATE_LIBPATH="ldconfig"

	BUILDER_CONFIGURE_FLAG="--with-gcrypt"

	#
	# For installing tzdata with another package(ex. git)
	#
	BUILDER_ENVIRONMENT="ENV DEBIAN_FRONTEND=noninteractive"
fi

#---------------------------------------------------------------
# Override function for processing
#---------------------------------------------------------------
# [NOTE]
# The following functions allow customization of processing.
# You can write your own processing by overriding each function.
#
# get_repository_package_version()
#	Definition of a function that sets a variable to give the
#	version number of the Docker image when no tag or version
#	number is explicitly given.
#
# print_custom_variabels()
#	Definition of a function to display(for github actions logs)
#	if variables other than those required by default are defined
#	in this file(imagetypevar.sh) or if variables are created,
#
# run_preprocess()
#	Define this function when preprocessing for Docker Image
#	creation is required.
#
# custom_dockerfile_conversion()
#	Define this function when you need to modify your Dockerfile.
#

#
# Variables in using following function
#
BUILDUTILS_DIR="${SRCTOP}/${DOCKERFILE_TEMPL_SUBDIR}"
AUTOGEN_TOOL="${SRCTOP}/autogen.sh"
CONFIGURE_TOOL="${SRCTOP}/configure"
MAKE_VAR_TOOL="${BUILDUTILS_DIR}/make_variables.sh"

#
# Get version from repository package
#
# [NOTE]
# Set "REPOSITORY_PACKAGE_VERSION" environment
#
# The following variables will be used, so please set them in advance:
#	AUTOGEN_TOOL	: path to autogen.sh
#	MAKE_VAR_TOOL	: path to configure
#
get_repository_package_version()
{
	if [ -z "${AUTOGEN_TOOL}" ] || [ ! -f "${AUTOGEN_TOOL}" ] || [ -z "${MAKE_VAR_TOOL}" ] || [ ! -f "${MAKE_VAR_TOOL}" ]; then
		PRNERR "AUTOGEN_TOOL(${AUTOGEN_TOOL}) or MAKE_VAR_TOOL(${MAKE_VAR_TOOL}) file path is empty or not existed"
		return 1
	fi

	_CURRENT_DIR=$(pwd)
	cd "${SRCTOP}" || return 1

	if ! "${AUTOGEN_TOOL}"; then
		PRNERR "Failed to run ${AUTOGEN_TOOL} for creating RELEASE_VERSION file"
		return 1
	fi
	if ! REPOSITORY_PACKAGE_VERSION="$("${MAKE_VAR_TOOL}" -pkg_version | tr -d '\n')"; then
		PRNERR "Failed to run \"${MAKE_VAR_TOOL} -pkg_version\" for creating RELEASE_VERSION file"
		return 1
	fi
	return 0
}

#
# Print custom variables
#
print_custom_variabels()
{
	echo "  BUILDUTILS_DIR              = ${BUILDUTILS_DIR}"
	echo "  BUILDER_CONFIGURE_FLAG      = ${BUILDER_CONFIGURE_FLAG}"
	echo "  BUILDER_MAKE_FLAGS          = ${BUILDER_MAKE_FLAGS}"
	return 0
}

#
# Preprocessing
#
# The following variables will be used, so please set them in advance:
#	AUTOGEN_TOOL	: path to autogen.sh
#	MAKE_VAR_TOOL	: path to configure
#
run_preprocess()
{
	if [ -z "${AUTOGEN_TOOL}" ] || [ ! -f "${AUTOGEN_TOOL}" ] || [ -z "${MAKE_VAR_TOOL}" ] || [ ! -f "${MAKE_VAR_TOOL}" ]; then
		PRNERR "AUTOGEN_TOOL(${AUTOGEN_TOOL}) or MAKE_VAR_TOOL(${MAKE_VAR_TOOL}) file path is empty or not existed"
		return 1
	fi

	if ! "${AUTOGEN_TOOL}"; then
		PRNERR "Failed to run ${AUTOGEN_TOOL} for preprocessing"
		return 1
	fi
	if ! /bin/sh -c "${CONFIGURE_TOOL} ${BUILDER_CONFIGURE_FLAG}"; then
		PRNERR "Failed to run \"${CONFIGURE_TOOL} ${BUILDER_CONFIGURE_FLAG}\" for preprocessing"
		return 1
	fi
	return 0
}

#
# Custom dockerfile conversion
#
# $1	: Dockerfile path
#
# The following variables will be used, so please set them in advance:
#	BUILDER_CONFIGURE_FLAG
#	BUILDER_MAKE_FLAGS
#
custom_dockerfile_conversion()
{
	if [ -z "$1" ] || [ ! -f "$1" ]; then
		PRNERR "Dockerfile path($1) is empty or not existed."
		return 1
	fi
	_TMP_DOCKERFILE_PATH="$1"

	sed -e "s#%%CONFIGURE_FLAG%%#${BUILDER_CONFIGURE_FLAG}#g"	\
		-e "s#%%BUILD_FLAGS%%#${BUILDER_MAKE_FLAGS}#g"			\
		-i "${_TMP_DOCKERFILE_PATH}"

	# shellcheck disable=SC2181
	if [ $? -ne 0 ]; then
		PRNERR "Failed to converting ${_TMP_DOCKERFILE_PATH}"
		return 1
	fi
	return 0
}

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
