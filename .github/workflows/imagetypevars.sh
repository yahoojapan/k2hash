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
# [NOTE]
# If IMAGE_CMD_BASE(DEV) is set to empty, "['/bin/sh', '-c', 'tail -f /dev/null']"
# will be set as the default value.
#
PKGMGR_NAME=
PKGMGR_UPDATE_OPT=
PKGMGR_INSTALL_OPT=
PKGMGR_UNINSTALL_OPT=
PRE_PROCESS=
POST_PROCESS=
IMAGE_CMD_BASE=
IMAGE_CMD_DEV=
PKG_INSTALL_CURL=
PKG_INSTALL_BASE=
PKG_INSTALL_DEV=
PKG_UNINSTALL_BASE=
PKG_UNINSTALL_DEV=
BUILDER_CONFIGURE_FLAG=
SETUP_ENVIRONMENT=
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
	PKGMGR_UNINSTALL_OPT="del -q --purge --no-progress --no-cache"
	PKG_INSTALL_CURL="curl"
	PKG_INSTALL_BASE="k2hash"
	PKG_INSTALL_DEV="k2hash-dev"

elif [ "${CI_DOCKER_IMAGE_OSTYPE}" = "ubuntu" ]; then
	PKGMGR_NAME="apt-get"
	PKGMGR_UPDATE_OPT="update -q -y"
	PKGMGR_INSTALL_OPT="install -q -y"
	PKGMGR_UNINSTALL_OPT="purge --auto-remove -q -y"
	PKG_INSTALL_CURL="curl"
	PKG_INSTALL_BASE="k2hash"
	PKG_INSTALL_DEV="k2hash-dev"
	UPDATE_LIBPATH="ldconfig"

	#
	# For installing tzdata with another package(ex. git)
	#
	SETUP_ENVIRONMENT="ENV DEBIAN_FRONTEND=noninteractive"
fi

#---------------------------------------------------------------
# Override function for processing
#---------------------------------------------------------------
# [NOTE]
# The following functions allow customization of processing.
# You can write your own processing by overriding each function.
#
# set_custom_variables()
#	This function sets common variables used in the following
#	customizable functions.
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
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
