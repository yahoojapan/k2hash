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
# This script gets commit hash(sha1) from github.com or .git directory
#
func_usage()
{
	echo ""
	echo "Usage:  $1 [-o org] [-r reponame] [-ep endpoint] [-t tag] [-short]"
	echo "        -o organization   specify organazation name if .git directory is not existed"
	echo "        -r repository     specify repository name if .git directory is not existed"
	echo "        -ep endpoint      if not github.com, specify endpoint for api"
	echo "        -t tag            if not find release version tag, use this"
	echo "        -short            get short commit hash(sha1)"
	echo "        -h                print help"
	echo ""
}
PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`
COMMITHASHFILE="${SRCTOP}/COMMIT_HASH"

#
# Check options
#
GITDIR=0
ORGNAME=""
REPONAME=""
DEFAULTENDPOINT="https://api.github.com/repos/"
ENDPOINT=${DEFAULTENDPOINT}
TAGNAME=""
ISSHORT=0
while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break
	elif [ "X$1" = "X-h" -o "X$1" = "X-help" ]; then
		func_usage $PRGNAME
		exit 0
	elif [ "X$1" = "X-short" ]; then
		ISSHORT=1
	elif [ "X$1" = "X-o" ]; then
		if [ "X$ORGNAME" != "X" ]; then
			echo "ERROR: already -o option is specified." 1>&2
			echo "unknown"
			exit 1
		fi
		shift
		if [ "X$1" = "X" ]; then
			echo "ERROR: -o option needs parameter" 1>&2
			echo "unknown"
			exit 1
		fi
		ORGNAME=$1
	elif [ "X$1" = "X-r" ]; then
		if [ "X$REPONAME" != "X" ]; then
			echo "ERROR: already -r option is specified." 1>&2
			echo "unknown"
			exit 1
		fi
		shift
		if [ "X$1" = "X" ]; then
			echo "ERROR: -r option needs parameter" 1>&2
			echo "unknown"
			exit 1
		fi
		REPONAME=$1
	elif [ "X$1" = "X-t" ]; then
		if [ "X$TAGNAME" != "X" ]; then
			echo "ERROR: already -t option is specified." 1>&2
			echo "unknown"
			exit 1
		fi
		shift
		if [ "X$1" = "X" ]; then
			echo "ERROR: -t option needs parameter" 1>&2
			echo "unknown"
			exit 1
		fi
		TAGNAME=$1
	elif [ "X$1" = "X-ep" ]; then
		if [ "X$ENDPOINT" != "X$DEFAULTENDPOINT" ]; then
			echo "ERROR: already -ep option is specified." 1>&2
			echo "unknown"
			exit 1
		fi
		shift
		if [ "X$1" = "X" ]; then
			echo "ERROR: -ep option needs parameter" 1>&2
			echo "unknown"
			exit 1
		fi
		ENDPOINT=$1
	else
		echo "ERROR: unknown option $1" 1>&2
		echo "unknown"
		exit 1
	fi
	shift
done

#
# Check .git directory
#
if [ -d ${SRCTOP}/.git ]; then
	GITDIR=1
fi

#
# Do
#
if [ $GITDIR -eq 1 ]; then
	#
	# use git command
	#
	if [ -f ${COMMITHASHFILE} ]; then
		rm -f ${COMMITHASHFILE}
	fi
	SHORTOPT=""
	if [ $ISSHORT -eq 1 ]; then
		SHORTOPT="--short"
	fi
	SHA1RESULT=`git rev-parse ${SHORTOPT} HEAD`

elif [ -f ${COMMITHASHFILE} ]; then
	#
	# get from COMMIT_HASH file
	#
	SHA1RESULT=`cat ${COMMITHASHFILE}`

	if [ $ISSHORT -eq 1 ]; then
		SHA1RESULT=`echo ${SHA1RESULT} 2>/dev/null | cut -c 1-7 2>/dev/null`
	fi

else
	#
	# access to github api
	#
	if [ "X$ORGNAME" = "X" -o "X$REPONAME" = "X" ]; then
		echo "ERROR: need to specify -o and -r options, because you do not have .git directory." 1>&2
		echo "unknown"
		exit 1
	fi
	if [ "X$TAGNAME" = "X" ]; then
		if [ -f ${SRCTOP}/RELEASE_VERSION ]; then
			#
			# release tag from RELEASE_VERSION file
			#
			TAGNAME=`cat ${SRCTOP}/RELEASE_VERSION`
		else
			#
			# release tag from source top directory name(expects source files from tar.gz file on github)
			#
			SRCTOPNAME=`basename $SRCTOP`
			TAGNAME=`echo ${SRCTOPNAME} | sed 's/-/ /g' | awk '{print $NF}'`
		fi
	fi

	#
	# first, access github api with adding "v" to tagname
	#
	SHA1RESULT=`curl ${ENDPOINT}${ORGNAME}/${REPONAME}/git/refs/tags/v${TAGNAME} 2>/dev/null | grep '"sha":' 2>/dev/null | awk '{print $2}' 2>/dev/null | sed 's/[",]//g' 2>/dev/null`
	if [ $? -ne 0 -o "X$SHA1RESULT" = "X" ]; then
		#
		# retry without "v"
		#
		SHA1RESULT=`curl ${ENDPOINT}${ORGNAME}/${REPONAME}/git/refs/tags/${TAGNAME} 2>/dev/null | grep '"sha":' 2>/dev/null | awk '{print $2}' 2>/dev/null | sed 's/[",]//g' 2>/dev/null`
		if [ $? -ne 0 -o "X$SHA1RESULT" = "X" ]; then
			echo "ERROR: Could not get commit hash for ${ORGNAME}/${REPONAME} - ${TAGNAME}" 1>&2
			echo "unknown"
			exit 1
		fi
	fi
	#
	# put commit hash file(as long)
	#
	echo "${SHA1RESULT}" > ${COMMITHASHFILE}

	if [ $ISSHORT -eq 1 ]; then
		SHA1RESULT=`echo ${SHA1RESULT} 2>/dev/null | cut -c 1-7 2>/dev/null`
	fi
fi

#
# confirm
#
if [ $? -ne 0 -o "X$SHA1RESULT" = "X" ]; then
	echo "ERROR: Could not get commit hash" 1>&2
	echo "unknown"
	exit 1
fi

echo ${SHA1RESULT}
exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
