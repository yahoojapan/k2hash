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
# This script gets short/long description from man page.
#
func_usage()
{
        echo ""
        echo "Usage:  $1 <man page file path> [-short | -long | -esclong | -deblong]"
        echo ""
        echo "        man page file path    Input file path for man page file in source directory"
        echo "        -short                return short(summary) description"
        echo "        -long                 return long description"
        echo "        -esclong              return long description with escaped LF"
        echo "        -deblong              return long description for debian in debian control file"
        echo ""
}
PRGNAME=`basename $0`
MYSCRIPTDIR=`dirname $0`
SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`

#
# Check options
#
INFILE=""
ISSHORT=0
SPACECHAR=0
ESCLF=""
while [ $# -ne 0 ]; do
	if [ "X$1" = "X" ]; then
		break;

	elif [ "X$1" = "X-h" -o "X$1" = "X-help" ]; then
		func_usage $PRGNAME
		exit 0

	elif [ "X$1" = "X-short" ]; then
		ISSHORT=1

	elif [ "X$1" = "X-long" ]; then
		ISSHORT=0
		SPACECHAR=0

	elif [ "X$1" = "X-esclong" ]; then
		ISSHORT=0
		ESCLF="\\n\\"

	elif [ "X$1" = "X-deblong" ]; then
		ISSHORT=0
		SPACECHAR=1

	else
		if [ "X${INFILE}" != "X" ]; then
			echo "ERROR: already ${INFILE} file is specified." 1>&2
			echo "No description by $PRGNAME with error."
			exit 1
		fi
		if [ ! -f $1 ]; then
			echo "ERROR: $1 file is not existed." 1>&2
			echo "No description by $PRGNAME with error."
			exit 1
		fi
		INFILE=$1
	fi
	shift
done

#
# put man page formatted by nroff and insert space head.
#
TEMPFILE=/tmp/${PRGNAME}_$$.tmp
nroff -man ${INFILE} 2>/dev/null | col -b 2>/dev/null | sed 's/[0-9][0-9]*m//g' 2>/dev/null | sed 's/^\s/_____/g' > ${TEMPFILE} 2>/dev/null
if [ $? -ne 0 ]; then
	echo "ERROR: Could not read ${INFILE} file with converting." 1>&2
	echo "No description by $PRGNAME with error."
	rm -f ${TEMPFILE} > /dev/null 2>&1
	exit 1
fi

#
# Loop for printing with converting
#
LINELEVEL=0
LONGDEST_START="no"
while read LINE; do
	#
	# revert inserted special chars.
	#
	CHKLINE=`echo ${LINE} | sed 's/^_____//g'`
	CHKLINE=`echo ${CHKLINE}`

	if [ $LINELEVEL -eq 0 ]; then
		if [ "X$CHKLINE" = "XNAME" ]; then
			LINELEVEL=1
		fi
	elif [ $LINELEVEL -eq 1 ]; then
		if [ $ISSHORT -eq 1 ]; then
			echo "${CHKLINE}${ESCLF}" | sed 's/.* - //g'
			break
		fi
		LINELEVEL=2

	elif [ $LINELEVEL -eq 2 ]; then
		if [ "X$CHKLINE" = "XDESCRIPTION" ]; then
			LINELEVEL=3
		fi
	elif [ $LINELEVEL -eq 3 ]; then
		if [ "X$CHKLINE" = "X" ]; then
			if [ $SPACECHAR -eq 1 ]; then
				echo " .${ESCLF}"
			else
				echo "${ESCLF}"
			fi
		else
			if [ "X$LINE" = "X$CHKLINE" ]; then
				#
				# This is new section
				#
				break
			fi
			if [ $SPACECHAR -eq 0 ]; then
				echo "${CHKLINE}${ESCLF}"
			else
				if [ "X$LONGDEST_START" = "Xno" ]; then
					echo " ${CHKLINE}${ESCLF}"
					LONGDEST_START="yes"
				else
					echo "  ${CHKLINE}${ESCLF}"
				fi
			fi
		fi
	fi
done < ${TEMPFILE}

rm -f ${TEMPFILE} > /dev/null 2>&1

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
