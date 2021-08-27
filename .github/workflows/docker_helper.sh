#!/bin/sh
#
# Utility helper tools for Github Actions by AntPickax
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
# CREATE:   Tue, Aug 3 2021
# REVISION:	1.0
#

#---------------------------------------------------------------------
# Docker Image Helper for container on Github Actions
#---------------------------------------------------------------------
func_usage()
{
	echo ""
	echo "Usage: $1 [options...]"
	echo ""
	echo "  Option:"
	echo "    --help(-h)                                print help"
	echo "    --imagetype-vars-file(-f) <file path>     specify the file path to imagetype variable(deafult. \"imagetypevars.sh\")"
	echo "    --imageinfo(-i)           <image info>    specify infomration about "base docker image", "base docker dev image", "os type tag" and "default flag" (ex. \"libfullock:1.0.41,libfullock-dev:1.0.41,alpine,default\")"
	echo "    --organization(-o)        <organization>  specify organaization name on DockerHub(default. \"antpickax\")"
	echo "    --imagenames(-n)          <image name>    specify build image names, separate multiple names with commas(ex. \"target,target-dev\")"
	echo "    --imageversion(-v)        <version>       the value of this option is set automatically and is usually not specified.(ex. \"1.0.0\")"
	echo "    --push(-p)                                specify this when force pushing the image to Docker Hub, normally the images is pushed only when it is tagged(determined from GITHUB_REF/GITHUB_EVENT_NAME)"
	echo "    --notpush(-np)                            specify this when force never pushing the image to Docker Hub."
	echo ""
	echo "  Note:"
	echo "    Specifying the above options will create the image shown in the example below:"
	echo "      antpickax/image:1.0.0-alpine  (imagetag is \"alpine\")"
	echo "      antpickax/image:1.0.0         (imagetag is not specified)"
	echo ""
	echo "    This program uses folowing environment variable internally."
	echo "      GITHUB_REPOSITORY"
	echo "      GITHUB_REF"
	echo "      GITHUB_EVENT_NAME"
	echo "      GITHUB_EVENT_PATH"
	echo ""
}

#---------------------------------------------------------------------
# Common Variables
#---------------------------------------------------------------------
PRGNAME=$(basename $0)
MYSCRIPTDIR=$(dirname $0)
MYSCRIPTDIR=$(cd ${MYSCRIPTDIR}; pwd)
SRCTOP=$(cd ${MYSCRIPTDIR}/../..; pwd)
BUILDUTILS_DIR=${SRCTOP}/buildutils

MAKE_VAR_TOOL="${BUILDUTILS_DIR}/make_variables.sh"
DOCKER_TEMPL_FILE=Dockerfile.templ
DOCKER_FILE=Dockerfile

#---------------------------------------------------------------------
# Parse Options
#---------------------------------------------------------------------
echo "[INFO] ${PRGNAME} : Start the parsing of options."

IMAGEVAR_FILE=
DOCKER_IMAGE_INFO=
DOCKER_HUB_ORG=
IMAGE_NAMES=
IMAGE_VERSION=
FORCE_PUSH=
DO_PUSH=

while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break

	elif [ "X$1" = "X-h" ] || [ "X$1" = "X-H" ] || [ "X$1" = "X--help" ] || [ "X$1" = "X--HELP" ]; then
		func_usage $PRGNAME
		exit 0

	elif [ "X$1" = "X-f" ] || [ "X$1" = "X-F" ] || [ "X$1" = "X--imagetype-vars-file" ] || [ "X$1" = "X--IMAGETYPE-VARS-FILE" ]; then
		if [ "X${IMAGEVAR_FILE}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--imagetype-vars-file(-f)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--imagetype-vars-file(-f)\" option is specified without parameter."
			exit 1
		fi
		IMAGEVAR_FILE=$1

		if [ ! -f ${IMAGEVAR_FILE} ]; then
			if [ ! -f ${MYSCRIPTDIR}/${IMAGEVAR_FILE} ]; then
				echo "[ERROR] ${PRGNAME} : Could not file : ${IMAGEVAR_FILE}."
				exit 1
			else
				IMAGEVAR_FILE=${MYSCRIPTDIR}/${IMAGEVAR_FILE}
			fi
		fi

	elif [ "X$1" = "X-i" ] || [ "X$1" = "X-I" ] || [ "X$1" = "X--imageinfo" ] || [ "X$1" = "X--IMAGEINFO" ]; then
		if [ "X${DOCKER_IMAGE_INFO}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--imageinfo(-i)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--imageinfo(-i)\" option is specified without parameter."
			exit 1
		fi
		DOCKER_IMAGE_INFO=$1

	elif [ "X$1" = "X-o" ] || [ "X$1" = "X-O" ] || [ "X$1" = "X--organization" ] || [ "X$1" = "X--ORGANIZATION" ]; then
		if [ "X${DOCKER_HUB_ORG}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--organization(-o)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--organization(-o)\" option is specified without parameter."
			exit 1
		fi
		DOCKER_HUB_ORG=$1

	elif [ "X$1" = "X-n" ] || [ "X$1" = "X-N" ] || [ "X$1" = "X--imagenames" ] || [ "X$1" = "X--IMAGENAMES" ]; then
		if [ "X${IMAGE_NAMES}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--imagenames(-n)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--imagenames(-n)\" option is specified without parameter."
			exit 1
		fi
		IMAGE_NAMES=$1

	elif [ "X$1" = "X-v" ] || [ "X$1" = "X-V" ] || [ "X$1" = "X--imageversion" ] || [ "X$1" = "X--IMAGEVERSION" ]; then
		if [ "X${IMAGE_VERSION}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--imageversion(-v)\" option."
			exit 1
		fi
		shift
		if [ $# -eq 0 ]; then
			echo "[ERROR] ${PRGNAME} : \"--imageversion(-v)\" option is specified without parameter."
			exit 1
		fi
		IMAGE_VERSION=$1

	elif [ "X$1" = "X-p" ] || [ "X$1" = "X-P" ] || [ "X$1" = "X--push" ] || [ "X$1" = "X--PUSH" ]; then
		if [ "X${FORCE_PUSH}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--push(-p)\" or \"--notpush(-np)\" option."
			exit 1
		fi
		FORCE_PUSH="true"

	elif [ "X$1" = "X-np" ] || [ "X$1" = "X-NP" ] || [ "X$1" = "X--notpush" ] || [ "X$1" = "X--NOTPUSH" ]; then
		if [ "X${FORCE_PUSH}" != "X" ]; then
			echo "[ERROR] ${PRGNAME} : already set \"--push(-p)\" or \"--notpush(-np)\" option."
			exit 1
		fi
		FORCE_PUSH="false"

	else
		echo "[ERROR] ${PRGNAME} : Unknown \"$1\" option."
		exit 1
	fi
	shift
done

#
# Check options for default value
#
if [ "X${DOCKER_HUB_ORG}" = "X" ]; then
	DOCKER_HUB_ORG="antpickax"
fi

if [ "X${IMAGE_NAMES}" = "X" ]; then
	echo "[ERROR] ${PRGNAME} : The \"--imagenames(-n)\" option is required."
	exit 1
fi

if [ "X${DOCKER_IMAGE_INFO}" = "X" ]; then
	echo "[ERROR] ${PRGNAME} : The \"--imageinfo(-i)\" option is required."
	exit 1
else
	#
	# Parse inmage information
	#
	DOCKER_IMAGE_INFO=$(echo ${DOCKER_IMAGE_INFO} | sed -e 's#,# #g')
	DOCKER_IMAGE_BASE=$(echo ${DOCKER_IMAGE_INFO} | awk '{print $1}')
	DOCKER_IMAGE_DEV_BASE=$(echo ${DOCKER_IMAGE_INFO} | awk '{print $2}')
	DOCKER_IMAGE_OSTYPE=$(echo ${DOCKER_IMAGE_INFO} | awk '{print $3}')
	DOCKER_IMAGE_DEFAULT_TAG=$(echo ${DOCKER_IMAGE_INFO} | awk '{print $4}')

	#
	# DOCKER_IMAGE_BASE
	#
	if [ "X${DOCKER_IMAGE_BASE}" = "X" ] || [ "X${DOCKER_IMAGE_DEV_BASE}" = "X" ] ; then
		echo "[ERROR] ${PRGNAME} : The \"--imageinfo(-i)\" option value does not have base image name."
		exit 1
	fi

	#
	# DOCKER_IMAGE_OSTYPE / DOCKER_IMAGE_OSTYPE_TAG
	#
	if [ "X${DOCKER_IMAGE_OSTYPE}" = "X" ]; then
		#
		# Get os type from base image name
		#
		DOCKER_IMAGE_OSTYPE=$(echo ${DOCKER_IMAGE_BASE} | sed -e 's#^.*/##g' -e 's#:.*$##g')
		if [ "X${DOCKER_IMAGE_OSTYPE}" = "X" ]; then
			echo "[ERROR] ${PRGNAME} : The \"--imageinfo(-i)\" option value does not have image os type."
			exit 1
		fi
		echo "[WARNING] ${PRGNAME} : The \"--imageinfo(-i)\" option value does not have image os type, but get it from base image name."
	fi
	DOCKER_IMAGE_OSTYPE_TAG="-${DOCKER_IMAGE_OSTYPE}"

	#
	# DEFAULT_IMAGE_TAGGING
	#
	if [ "X${DOCKER_IMAGE_DEFAULT_TAG}" = "Xdefault" ]; then
		DEFAULT_IMAGE_TAGGING=1
	else
		DEFAULT_IMAGE_TAGGING=0
	fi
fi

#
# Version
#
TAGGED_VERSION=
if [ "X${GITHUB_REF}" != "X" ]; then
	echo ${GITHUB_REF} | grep 'refs/tags/' >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		TAGGED_VERSION=$(echo ${GITHUB_REF} | sed -e 's#refs/tags/v##g' -e 's#refs/tags/##g')
	fi
fi
if [ "X${IMAGE_VERSION}" = "X" ]; then
	if [ "X${TAGGED_VERSION}" != "X" ]; then
		#
		# From Github ref
		#
		IMAGE_VERSION=${TAGGED_VERSION}
	else
		#
		# From RELEASE_VERSION file
		#
		cd ${SRCTOP}
		./autogen.sh

		IMAGE_VERSION=$(${MAKE_VAR_TOOL} -pkg_version)
		if [ "X${IMAGE_VERSION}" = "X" ]; then
			IMAGE_VERSION="0.0.0"
		fi
	fi
fi

#
# Push mode
#
if [ "X${FORCE_PUSH}" = "Xtrue" ]; then
	#
	# FORCE PUSH
	#
	if [ "X${GITHUB_EVENT_NAME}" = "Xschedule" ]; then
		echo "[WARNING] ${PRGNAME} : specified \"--push(-p)\" option, but not push images because this process is kicked by scheduler."
		DO_PUSH=0
	else
		DO_PUSH=1
	fi
elif [ "X${FORCE_PUSH}" = "Xfalse" ]; then
	#
	# FORCE NOT PUSH
	#
	DO_PUSH=0
else
	if [ "X${GITHUB_EVENT_NAME}" = "Xschedule" ]; then
		DO_PUSH=0
	else
		if [ "X${TAGGED_VERSION}" != "X" ]; then
			DO_PUSH=1
		else
			DO_PUSH=0
		fi
	fi
fi

#
# Base Repository/SHA1 for creating Docker images
#
if [ "X${GITHUB_EVENT_NAME}" = "Xpull_request" ]; then
	# [NOTE]
	# In the PR case, GITHUB_REPOSITORY and GITHUB_REF cannot be used
	# because they are the information on the merging side.
	# Then the Organization/Repository/branch(or SHA1) of the PR source
	# is fetched from the file(json) indicated by the GITHUB_EVENT_PATH
	# environment variable.
	#	ex.
	#		{
	#		  "pull_request": {
	#		    "head": {
	#		      "repo": {
	#		        "full_name": "org/repo",
	#		      },
	#		      "sha": "776........",
	#		    },
	#		  },
	#		}
	#
	if [ ! -f ${GITHUB_EVENT_PATH} ]; then
		echo "[ERROR] ${PRGNAME} : \"GITHUB_EVENT_PATH\" environment is empty or not file path."
		exit 1
	fi

	# [NOTE]
	# we need "jq" for parsing json
	#
	PR_GITHUB_REPOSITORY=$(jq -r '.pull_request.head.repo.full_name' ${GITHUB_EVENT_PATH})
	DOCKER_GIT_ORGANIZATION=$(echo ${PR_GITHUB_REPOSITORY} | sed 's#/# #g' | awk '{print $1}')
	DOCKER_GIT_REPOSITORY=$(echo ${PR_GITHUB_REPOSITORY} | sed 's#/# #g' | awk '{print $2}')
	DOCKER_GIT_BRANCH=$(jq -r '.pull_request.head.sha' ${GITHUB_EVENT_PATH})

else
	DOCKER_GIT_ORGANIZATION=$(echo ${GITHUB_REPOSITORY} | sed 's#/# #g' | awk '{print $1}')
	DOCKER_GIT_REPOSITORY=$(echo ${GITHUB_REPOSITORY} | sed 's#/# #g' | awk '{print $2}')
	DOCKER_GIT_BRANCH=$(echo ${GITHUB_REF} | sed 's#^refs/.*/##g')
fi
if [ "X${DOCKER_GIT_ORGANIZATION}" = "X" ] || [ "X${DOCKER_GIT_REPOSITORY}" = "X" ] || [ "X${DOCKER_GIT_BRANCH}" = "X" ]; then
	echo "[ERROR] ${PRGNAME} : Not found Organization/Repository/branch(or SHA1)."
	exit 1
fi

#
# Load variables file
#
if [ "X${IMAGEVAR_FILE}" = "X" ]; then
	IMAGEVAR_FILE="${MYSCRIPTDIR}/imagetypevars.sh"
fi
if [ -f ${IMAGEVAR_FILE} ]; then
	echo "[INFO] ${PRGNAME} : Load ${IMAGEVAR_FILE} for local variables."
	. ${IMAGEVAR_FILE}
fi

#---------------------------------------------------------------------
# Print information
#---------------------------------------------------------------------
echo "[INFO] ${PRGNAME} : All local variables."
echo "  PRGNAME                  = ${PRGNAME}"
echo "  MYSCRIPTDIR              = ${MYSCRIPTDIR}"
echo "  BUILDUTILS_DIR           = ${BUILDUTILS_DIR}"
echo "  MAKE_VAR_TOOL            = ${MAKE_VAR_TOOL}"
echo "  DOCKER_TEMPL_FILE        = ${DOCKER_TEMPL_FILE}"
echo "  DOCKER_FILE              = ${DOCKER_FILE}"
echo "  IMAGEVAR_FILE            = ${IMAGEVAR_FILE}"
echo "  DOCKER_IMAGE_INFO        = ${DOCKER_IMAGE_INFO}"
echo "  DOCKER_IMAGE_BASE        = ${DOCKER_IMAGE_BASE}"
echo "  DOCKER_IMAGE_DEV_BASE    = ${DOCKER_IMAGE_DEV_BASE}"
echo "  DOCKER_IMAGE_OSTYPE      = ${DOCKER_IMAGE_OSTYPE}"
echo "  DOCKER_GIT_ORGANIZATION  = ${DOCKER_GIT_ORGANIZATION}"
echo "  DOCKER_GIT_REPOSITORY    = ${DOCKER_GIT_REPOSITORY}"
echo "  DOCKER_GIT_BRANCH        = ${DOCKER_GIT_BRANCH}"
echo "  DEFAULT_IMAGE_TAGGING    = ${DEFAULT_IMAGE_TAGGING}"
echo "  DOCKER_HUB_ORG           = ${DOCKER_HUB_ORG}"
echo "  IMAGE_NAMES              = ${IMAGE_NAMES}"
echo "  IMAGE_VERSION            = ${IMAGE_VERSION}"
echo "  FORCE_PUSH               = ${FORCE_PUSH}"
echo "  DO_PUSH                  = ${DO_PUSH}"
echo "  PKGMGR_NAME              = ${PKGMGR_NAME}"
echo "  PKGMGR_UPDATE_OPT        = ${PKGMGR_UPDATE_OPT}"
echo "  PKGMGR_INSTALL_OPT       = ${PKGMGR_INSTALL_OPT}"
echo "  PKG_INSTALL_LIST_BUILDER = ${PKG_INSTALL_LIST_BUILDER}"
echo "  PKG_INSTALL_LIST_BIN     = ${PKG_INSTALL_LIST_BIN}"
echo "  BUILDER_CONFIGURE_FLAG   = ${BUILDER_CONFIGURE_FLAG}"
echo "  BUILDER_MAKE_FLAGS       = ${BUILDER_MAKE_FLAGS}"
echo "  BUILDER_ENVIRONMENT      = ${BUILDER_ENVIRONMENT}"
echo "  UPDATE_LIBPATH           = ${UPDATE_LIBPATH}"
echo "  RUNNER_INSTALL_PACKAGES  = ${RUNNER_INSTALL_PACKAGES}"

#---------------------------------------------------------------------
# Initialize Runner for creating Dockerfile
#---------------------------------------------------------------------
# [NOTE]
# Github Actions Runner uses Ubuntu to create Docker images.
# Therefore, the below code is written here assuming that Ubuntu is used.
#

#
# Update pacakges
#
echo "[INFO] ${PRGNAME} : Update local packages and caches"

export DEBIAN_FRONTEND=noninteractive 
sudo apt-get update -y -qq

#
# Setup packagecloud.io repository
#
echo "[INFO] ${PRGNAME} : Setup packagecloud.io repository."

curl --version >/dev/null 2>&1
if [ $? -ne 0 ]; then
	sudo apt-get install -y -qq curl
fi

sudo /bin/sh -c "curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh | bash"
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : could not add packagecloud.io repository."
	exit 1
fi

#
# Install packages
#
echo "[INFO] ${PRGNAME} : Install packages for Github Actions Runner."
if [ "X${RUNNER_INSTALL_PACKAGES}" != "X" ]; then
	sudo apt-get install -y -qq ${RUNNER_INSTALL_PACKAGES}
fi

#---------------------------------------------------------------------
# Create Dockerfile from template
#---------------------------------------------------------------------
cd ${SRCTOP}

#
# For generating Dockerfile.templ
#
echo "[INFO] ${PRGNAME} : Initialize repository files"

./autogen.sh
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : Failed to run autogen.sh."
	exit 1
fi
./configure
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : Failed to run configure."
	exit 1
fi

#
# Create each Dockerfile
#
echo "[INFO] ${PRGNAME} : Create Dockerfile from ${DOCKER_TEMPL_FILE}"

if [ "X${PKG_INSTALL_LIST_BUILDER}" != "X" ]; then
	PKG_INSTALL_BUILDER_COMMAND="${PKGMGR_NAME} ${PKGMGR_INSTALL_OPT} ${PKG_INSTALL_LIST_BUILDER}"
else
	#
	# Set no-operation command
	#
	PKG_INSTALL_BUILDER_COMMAND=":"
fi

if [ "X${PKG_INSTALL_LIST_BIN}" != "X" ]; then
	PKG_INSTALL_BIN_COMMAND="${PKGMGR_NAME} ${PKGMGR_INSTALL_OPT} ${PKG_INSTALL_LIST_BIN}"
else
	#
	# Set no-operation command
	#
	PKG_INSTALL_BIN_COMMAND=":"
fi

if [ "X${UPDATE_LIBPATH}" = "X" ]; then
	#
	# Set no-operation command
	#
	UPDATE_LIBPATH=":"
fi

cat ${BUILDUTILS_DIR}/${DOCKER_TEMPL_FILE} |							\
	sed -e "s#%%DOCKER_IMAGE_BASE%%#${DOCKER_IMAGE_BASE}#g"				\
		-e "s#%%DOCKER_IMAGE_DEV_BASE%%#${DOCKER_IMAGE_DEV_BASE}#g"		\
		-e "s#%%DOCKER_GIT_ORGANIZATION%%#${DOCKER_GIT_ORGANIZATION}#g"	\
		-e "s#%%DOCKER_GIT_REPOSITORY%%#${DOCKER_GIT_REPOSITORY}#g"		\
		-e "s#%%DOCKER_GIT_BRANCH%%#${DOCKER_GIT_BRANCH}#g"				\
		-e "s#%%PKG_UPDATE%%#${PKGMGR_NAME} ${PKGMGR_UPDATE_OPT}#g"		\
		-e "s#%%PKG_INSTALL_BUILDER%%#${PKG_INSTALL_BUILDER_COMMAND}#g"	\
		-e "s#%%PKG_INSTALL_BIN%%#${PKG_INSTALL_BIN_COMMAND}#g"			\
		-e "s#%%CONFIGURE_FLAG%%#${BUILDER_CONFIGURE_FLAG}#g"			\
		-e "s#%%BUILD_FLAGS%%#${BUILDER_MAKE_FLAGS}#g"					\
		-e "s#%%UPDATE_LIBPATH%%#${UPDATE_LIBPATH}#g"					\
		-e "s#%%BUILD_ENV%%#${BUILDER_ENVIRONMENT}#g"					> ${SRCTOP}/${DOCKER_FILE}
if [ $? -ne 0 ]; then
	echo "[ERROR] ${PRGNAME} : Failed to creating ${DOCKER_FILE} from ${DOCKER_TEMPL_FILE}."
	exit 1
fi

echo "[INFO] ${PRGNAME} : Success to create ${SRCTOP}/${DOCKER_FILE}"
echo ""
cat ${SRCTOP}/${DOCKER_FILE}
echo ""

#---------------------------------------------------------------------
# Build Docker Images and Tagging
#---------------------------------------------------------------------
echo "[INFO] ${PRGNAME} : Build docker images"

IMAGE_NAMES=$(echo ${IMAGE_NAMES} | sed -e 's/,/ /g')

for ONE_IMAGE_NAME in ${IMAGE_NAMES}; do
	echo "[INFO] ${PRGNAME} : Build docker image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG}, ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG} from ${ONE_IMAGE_NAME}"

	if [ ${DEFAULT_IMAGE_TAGGING} -eq 1 ]; then
		docker image build -f ${SRCTOP}/${DOCKER_FILE} --target ${ONE_IMAGE_NAME} -t ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG} -t ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG} -t ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION} -t ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest .
	else
		docker image build -f ${SRCTOP}/${DOCKER_FILE} --target ${ONE_IMAGE_NAME} -t ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG} -t ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG} .
	fi
	if [ $? -ne 0 ]; then
		echo "[ERROR] ${PRGNAME} : Failed to build image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG}, ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG} from ${ONE_IMAGE_NAME}"
		exit 1
	fi
	echo "[INFO] ${PRGNAME} : Success to build image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG}, ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG}"
done

#---------------------------------------------------------------------
# Push Docker Images
#---------------------------------------------------------------------
if [ ${DO_PUSH} -eq 1 ]; then
	echo "[INFO] ${PRGNAME} : Push docker images."

	for ONE_IMAGE_NAME in ${IMAGE_NAMES}; do
		#
		# Push images
		#
		echo "[INFO] ${PRGNAME} : Push docker image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG}"
		docker push ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG}
		if [ $? -ne 0 ]; then
			echo "[ERROR] ${PRGNAME} : Failed to push image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG}"
			exit 1
		fi

		echo "[INFO] ${PRGNAME} : Push docker image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG}"
		docker push ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG}
		if [ $? -ne 0 ]; then
			echo "[ERROR] ${PRGNAME} : Failed to push image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG}"
			exit 1
		fi

		#
		# Push image as default tag
		#
		if [ ${DEFAULT_IMAGE_TAGGING} -eq 1 ]; then
			echo "[INFO] ${PRGNAME} : Push docker image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}"
			docker push ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}
			if [ $? -ne 0 ]; then
				echo "[ERROR] ${PRGNAME} : Failed to push image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}"
				exit 1
			fi

			echo "[INFO] ${PRGNAME} : Push docker image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest"
			docker push ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest
			if [ $? -ne 0 ]; then
				echo "[ERROR] ${PRGNAME} : Failed to push image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest"
				exit 1
			fi

			echo "[INFO] ${PRGNAME} : Success to build image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG}, ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG}, ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}, ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest"
		else
			echo "[INFO] ${PRGNAME} : Success to build image : ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:${IMAGE_VERSION}${DOCKER_IMAGE_OSTYPE_TAG}, ${DOCKER_HUB_ORG}/${ONE_IMAGE_NAME}:latest${DOCKER_IMAGE_OSTYPE_TAG}"
		fi
	done
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
