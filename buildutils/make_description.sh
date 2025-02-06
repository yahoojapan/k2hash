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

#
# Common variables
#
PRGNAME=$(basename "${0}")
#MYSCRIPTDIR=$(dirname "${0}")
#SRCTOP=$(cd "${MYSCRIPTDIR}/.." || exit 1; pwd)

#
# Variables
#
EXCLUSIVE_OPT=0
MAN_FILE=""
IS_SHORT=0
IS_DEB_CONTROL_FILE=0
ESC_LF_CHAR=""
TEMP_FILE="/tmp/${PRGNAME}_$$.tmp"

#
# Utility functions
#
func_usage()
{
	echo ""
	echo "Usage:  $1 <man page file path> [--help(-h)] [--short(-s) | --long(-l) | --esclong(-e) | --deblong(-d)]"
	echo ""
	echo "	<man page file path>  input file path for man page file in source directory"
	echo "  --help(-h)            print help"
	echo "	--short(-s)           return short(summary) description"
	echo "	--long(-l)            return long description"
	echo "	--esclong(-e)         return long description with escaped LF"
	echo "	--deblong(-d)         return long description for debian in debian control file"
	echo ""
}

#
# Check options
#
while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		break

	elif echo "$1" | grep -q -i -e "^-h$" -e "^--help$"; then
		func_usage "${PRGNAME}"
		exit 0

	elif echo "$1" | grep -q -i -e "^-s$" -e "^--short$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --short(-s), --long(-l), --esclong(-e), --deblong(-d) ) is specified." 1>&2
			echo "No description because the ${PRGNAME} program failed to extract the description."
			exit 1
		fi
		IS_SHORT=1
		EXCLUSIVE_OPT=1

	elif echo "$1" | grep -q -i -e "^-l$" -e "^--long$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --short(-s), --long(-l), --esclong(-e), --deblong(-d) ) is specified." 1>&2
			echo "No description because the ${PRGNAME} program failed to extract the description."
			exit 1
		fi
		IS_SHORT=0
		IS_DEB_CONTROL_FILE=0
		EXCLUSIVE_OPT=1

	elif echo "$1" | grep -q -i -e "^-e$" -e "^--esclong$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --short(-s), --long(-l), --esclong(-e), --deblong(-d) ) is specified." 1>&2
			echo "No description because the ${PRGNAME} program failed to extract the description."
			exit 1
		fi
		IS_SHORT=0
		EXCLUSIVE_OPT=1
		# [NOTE]
		# I want to set ESC_LF_CHAR to "\n\".
		# We can write as follows to set this, but to be compatible with vim and ShellCheck, so ex3 is used.
		#	ex1) ESC_LF_CHAR="\\n\\"	-> vim will confuse it.
		#	ex2) ESC_LF_CHAR='\n\'		-> ShellCheck will output a warning.
		#	ex3) ESC_LF_CHAR='\n'\\		-> This is the correct.
		#
		ESC_LF_CHAR='\n'\\

	elif echo "$1" | grep -q -i -e "^-d$" -e "^--deblong$"; then
		if [ "${EXCLUSIVE_OPT}" -eq 1 ]; then
			echo "[ERROR] already one of eclusive options( --short(-s), --long(-l), --esclong(-e), --deblong(-d) ) is specified." 1>&2
			echo "No description because the ${PRGNAME} program failed to extract the description."
			exit 1
		fi
		IS_SHORT=0
		IS_DEB_CONTROL_FILE=1
		EXCLUSIVE_OPT=1

	else
		if [ -n "${MAN_FILE}" ]; then
			echo "[ERROR] already man page file path(${MAN_FILE}) is specified." 1>&2
			echo "No description because the ${PRGNAME} program failed to extract the description."
			exit 1
		fi
		if [ ! -f "$1" ]; then
			echo "[ERROR] $1 file is not existed." 1>&2
			echo "No description because the ${PRGNAME} program failed to extract the description."
			exit 1
		fi
		MAN_FILE="$1"
	fi
	shift
done

#
# Extract formatted man output from a man file using nroff
#
# [NOTE]
# A special character string("_____") is inserted at the beginning
# of lines other than section lines.
# There are spaces at the beginning of lines other than lines that
# indicate sections, so use them as markers to insert characters.
# This allows you to see the section breaks.
#
if ! nroff -man "${MAN_FILE}" 2>/dev/null | sed -e 's/[[:cntrl:]]\[[0-9][0-9]*m//g' -e 's/[[:cntrl:]][[:graph:]]//g' -e 's/^[[:space:]]/_____/g' >"${TEMP_FILE}" 2>/dev/null; then
	echo "[ERROR] Could not read ${MAN_FILE} file with converting." 1>&2
	echo "No description because the ${PRGNAME} program failed to extract the description."
	rm -f "${TEMP_FILE}"
	exit 1
fi

#
# Loop for printing with converting
#
LINE_LEVEL=0
START_LONG_DESCRIPT=0
while IFS= read -r ONE_LINE; do
	#
	# revert inserted special chars.
	#
	REVERTED_LINE=$(echo "${ONE_LINE}" | sed -e 's/^_____//g' -e 's/^[[:space:]]*//g' -e 's/[[:space:]]*$//g')

	if [ "${LINE_LEVEL}" -eq 0 ]; then
		if [ -n "${REVERTED_LINE}" ] && [ "${REVERTED_LINE}" = "NAME" ]; then
			LINE_LEVEL=1
		fi

	elif [ "${LINE_LEVEL}" -eq 1 ]; then
		if [ "${IS_SHORT}" -eq 1 ]; then
			echo "${REVERTED_LINE}${ESC_LF_CHAR}" | sed -e 's/.* - //g' -e 's/[[:space:]]\+/ /g'
			break
		fi
		LINE_LEVEL=2

	elif [ "${LINE_LEVEL}" -eq 2 ]; then
		if [ -n "${REVERTED_LINE}" ] && [ "${REVERTED_LINE}" = "DESCRIPTION" ]; then
			LINE_LEVEL=3
		fi

	elif [ "${LINE_LEVEL}" -eq 3 ]; then
		if [ -z "${REVERTED_LINE}" ]; then
			if [ "${IS_DEB_CONTROL_FILE}" -eq 0 ]; then
				echo "${ESC_LF_CHAR}"
			else
				echo " .${ESC_LF_CHAR}"
			fi
		else
			if [ -n "${ONE_LINE}" ] && [ "${ONE_LINE}" = "${REVERTED_LINE}" ]; then
				#
				# This is new section
				#
				break
			fi

			OUTPUT_LINE=$(echo "${REVERTED_LINE}${ESC_LF_CHAR}" | sed -e 's/[[:space:]]\+/ /g')
			if [ "${IS_DEB_CONTROL_FILE}" -eq 0 ]; then
				echo "${OUTPUT_LINE}"
			else
				if [ "${START_LONG_DESCRIPT}" -eq 0 ]; then
					echo " ${OUTPUT_LINE}"
					START_LONG_DESCRIPT=1
				else
					echo "  ${OUTPUT_LINE}"
				fi
			fi
		fi
	fi
done < "${TEMP_FILE}"

rm -f "${TEMP_FILE}"

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
