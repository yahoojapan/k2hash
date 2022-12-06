#!/bin/sh
#
# K2HASH
#
# Copyright 2013 Yahoo Japan Corporation.
#
# K2HASH is key-valuew store base libraries.
# K2HASH is made for the purpose of the construction of
# original KVS system and the offer of the library.
# The characteristic is this KVS library which Key can
# layer. And can support multi-processing and multi-thread,
# and is provided safely as available KVS.
#
# For the full copyright and license information, please view
# the license file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Mon Feb 24 2014
# REVISION:
#

#--------------------------------------------------------------
# Common Variables
#--------------------------------------------------------------
PRGNAME=$(basename "${0}")
SCRIPTDIR=$(dirname "${0}")
SCRIPTDIR=$(cd "${SCRIPTDIR}" || exit 1; pwd)
#SRCTOP=$(cd "${SCRIPTDIR}/.." || exit 1; pwd)

#
# Environment
#
if [ -z "${TESTPROGDIR}" ]; then
	TESTPROGDIR="${SCRIPTDIR}"
fi

#--------------------------------------------------------------
# Usage
#--------------------------------------------------------------
func_usage()
{
        echo ""
        echo "Usage:  $1 -h"
        echo "        $1 -m          [options] KEYNAME-PREFIX REPEAT-COUNT [VALUE]"
        echo "        $1 -f FILENAME [options] KEYNAME-PREFIX REPEAT-COUNT [VALUE]"
        echo "        $1 -t FILENAME [options] KEYNAME-PREFIX REPEAT-COUNT [VALUE]"
        echo "        $1 -r FILENAME [options]"
        echo ""
        echo "        KEYNAME-PREFIX        key name prefix for test data"
        echo "        REPEAT-COUNT          repeat writing count"
        echo "        VALUE                 value for test data(default is already set)"
        echo ""
        echo "Mode:   -m                    anonymous memory mode"
        echo "        -f FILENAME           using shm file(data area is not mmap)"
        echo "        -t FILENAME           using temporary shm file(full mmap)"
        echo "        -r FILENAME           dumping shm file"
        echo ""
        echo "Option: -mask <maskbit>       start cur_mask bit count(default 8)"
        echo "        -cmask <maskbit>      collision_mask bit count(default 4)"
        echo "        -elementcnt <maxcnt>  max element count in ckey list(default 32)"
        echo "        -pagesize <pagesize>  page size(default 128 byte)"
        echo "        -ro                   read only mode(not use with -mem)"
        echo "        -d <dump level>       HEAD / KINDEX / CKINDEX / ELEMENT / FULL"
        echo "        -g <debug level>      ERR / WAN / INF"
        echo ""
        echo "Example:"
        echo "       $1 -m -mask 8 -cmask 4 -elementcnt 32 -pagesize 1024 -d full -g inf MEMKEY 100 MYVALUE"
        echo "       $1 -f shmfile -mask 8 -cmask 4 -elementcnt 32 -d full -g inf FILEKEY 100 MYVALUE"
        echo "       $1 -t shmfile -mask 8 -cmask 4 -elementcnt 32 -d full -g inf TMPKEY 100 MYVALUE"
        echo "       $1 -r shmfile -d full -g inf"
        echo ""
}

#--------------------------------------------------------------
# Parse options
#--------------------------------------------------------------
#
# Variables
#
IS_PRINT_INFO=0
SCRIPT_MODE=""
K2H_FILENAME=""

OPT_MASK=
OPT_CMASK=
OPT_MAXELECNT=
OPT_PAGESIZE=

RONLY_OPT=""
DUMP_OPT=""
DEBUG_OPT=""

KEYPREFIX=""
MAXCOUNT=""
PREVALUE=""

while [ $# -ne 0 ]; do
	if [ -z "$1" ]; then
		break;

	elif [ "$1" = "-h" ] || [ "$1" = "-H" ] || [ "$1" = "--help" ] || [ "$1" = "--HELP" ]; then
		func_usage "${PRGNAME}"
		exit 0

	elif [ "$1" = "--printinfo" ] || [ "$1" = "--PRINTINFO" ]; then
		if [ "${IS_PRINT_INFO}" -ne 0 ]; then
			echo "[ERROR] Already specify --printinfo option."
			exit 1
		fi
		IS_PRINT_INFO=1

	elif [ "$1" = "-m" ] || [ "$1" = "-M" ]; then
		if [ -n "${SCRIPT_MODE}" ]; then
			echo "[ERROR] Already specify -m, -f, -t or -r option."
			exit 1
		fi
		SCRIPT_MODE="M"

	elif [ "$1" = "-f" ] || [ "$1" = "-F" ]; then
		if [ -n "${SCRIPT_MODE}" ]; then
			echo "[ERROR] Already specify -m, -f, -t or -r option."
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] -f option needs parameter."
			exit 1
		fi
		SCRIPT_MODE="F"
		K2H_FILENAME="$1"

	elif [ "$1" = "-t" ] || [ "$1" = "-T" ]; then
		if [ -n "${SCRIPT_MODE}" ]; then
			echo "[ERROR] Already specify -m, -f, -t or -r option."
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] -f option needs parameter."
			exit 1
		fi
		SCRIPT_MODE="T"
		K2H_FILENAME="$1"

	elif [ "$1" = "-r" ] || [ "$1" = "-R" ]; then
		if [ -n "${SCRIPT_MODE}" ]; then
			echo "[ERROR] Already specify -m, -f, -t or -r option."
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] -r option needs parameter."
			exit 1
		fi
		SCRIPT_MODE="R"
		K2H_FILENAME="$1"

	elif [ "$1" = "-mask" ] || [ "$1" = "-MASK" ]; then
		if [ -n "${OPT_MASK}" ]; then
			echo "[ERROR] Already specify -mask option."
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] -mask option needs parameter."
			exit 1
		fi
		if echo "$1" | grep -q '[^0-9]'; then
			echo "[ERROR] -mask option parameter must be number."
			exit 1
		fi
		OPT_MASK="$1"

	elif [ "$1" = "-cmask" ] || [ "$1" = "-CMASK" ]; then
		if [ -n "${OPT_CMASK}" ]; then
			echo "[ERROR] Already specify -cmask option."
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] -cmask option needs parameter."
			exit 1
		fi
		if echo "$1" | grep -q '[^0-9]'; then
			echo "[ERROR] -cmask option parameter must be number."
			exit 1
		fi
		OPT_CMASK="$1"

	elif [ "$1" = "-elementcnt" ] || [ "$1" = "-ELEMENTCNT" ]; then
		if [ -n "${OPT_MAXELECNT}" ]; then
			echo "[ERROR] Already specify -elementcnt option."
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] -elementcnt option needs parameter."
			exit 1
		fi
		if echo "$1" | grep -q '[^0-9]'; then
			echo "[ERROR] -elementcnt option parameter must be number."
			exit 1
		fi
		OPT_MAXELECNT="$1"

	elif [ "$1" = "-pagesize" ] || [ "$1" = "-PAGESIZE" ]; then
		if [ -n "${OPT_PAGESIZE}" ]; then
			echo "[ERROR] Already specify -pagesize option."
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] -pagesize option needs parameter."
			exit 1
		fi
		if echo "$1" | grep -q '[^0-9]'; then
			echo "[ERROR] -pagesize option parameter must be number."
			exit 1
		fi
		OPT_PAGESIZE="$1"

	elif [ "$1" = "-ro" ] || [ "$1" = "-RO" ]; then
		if [ -n "${RONLY_OPT}" ]; then
			echo "[ERROR] Already specify -ro option."
			exit 1
		fi
		RONLY_OPT="-ro"

	elif [ "$1" = "-d" ] || [ "$1" = "-D" ]; then
		if [ -n "${DUMP_OPT}" ]; then
			echo "[ERROR] Already specify -d option."
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] -p option needs parameter."
			exit 1
		fi
		DUMP_OPT="-d $1"

	elif [ "$1" = "-g" ] || [ "$1" = "-G" ]; then
		if [ -n "${DEBUG_OPT}" ]; then
			echo "[ERROR] Already specify -g option."
			exit 1
		fi
		shift
		if [ -z "$1" ]; then
			echo "[ERROR] -g option needs parameter."
			exit 1
		fi
		DEBUG_OPT="-g $1"

	elif [ -n "$1" ]; then
		if [ -z "${KEYPREFIX}" ]; then
			KEYPREFIX="$1"
		elif [ -z "${MAXCOUNT}" ]; then
			MAXCOUNT="$1"
		elif [ -z "${PREVALUE}" ]; then
			PREVALUE="$1"
		else
			echo "[ERROR] $1 is unknown parameter."
			exit 1
		fi
	fi

	shift
done

#
# Check and Set fedault value
#
if [ -n "${OPT_MASK}" ]; then
	MASK="${OPT_MASK}"
else
	MASK=8
fi
if [ -n "${OPT_CMASK}" ]; then
	CMASK="${OPT_CMASK}"
else
	CMASK=4
fi
if [ -n "${OPT_MAXELECNT}" ]; then
	MAXELECNT="${OPT_MAXELECNT}"
else
	MAXELECNT=32
fi
if [ -n "${OPT_PAGESIZE}" ]; then
	PAGESIZE="${OPT_PAGESIZE}"
else
	PAGESIZE=128
fi
if [ -n "${PREVALUE}" ]; then
	VALUE="${PREVALUE}"
else
	VALUE="ABCDEFGHIJKLMNOPQRSTUVWXYZ@ABCDEFGHIJKLMNOPQRSTUVWXYZ@ABCDEFGHIJKLMNOPQRSTUVWXYZ@ABCDEFGHIJKLMNOPQRSTUVWXYZ@"
fi

#
# Mode and etc
#
if [ -z "${SCRIPT_MODE}" ]; then
	echo "[ERROR] Not specified -m, -f or -t option."
	exit 1
elif [ "${SCRIPT_MODE}" = "M" ] || [ "${SCRIPT_MODE}" = "F" ] || [ "${SCRIPT_MODE}" = "T" ]; then
	if [ -n "${RONLY_OPT}" ]; then
		echo "[ERROR] The -m, -f and -t option cannot be used with the -ro option."
		exit 1
	fi
elif [ "${SCRIPT_MODE}" = "R" ]; then
	if [ -n "${KEYPREFIX}" ] || [ -n "${MAXCOUNT}" ] || [ -n "${PREVALUE}" ]; then
		echo "[ERROR] -r option cannot be used with <KEYNAME-PREFIX>, <REPEAT-COUNT> and <VALUE>."
		exit 1
	fi
	if [ -z "${DUMP_OPT}" ]; then
		# set default dump option
		echo "[WARNING] -d option is not specified, so use default FULL."
		DUMP_OPT="-d FULL"
	fi
	if [ -z "${RONLY_OPT}" ]; then
		# force set read only
		RONLY_OPT="-ro"
	fi
else
	echo "[ERROR] Not specified unknown option(internal error)."
	exit 1
fi

#
# Debug print
#
if [ "${IS_PRINT_INFO}" -eq 1 ]; then
	echo "-----------------------------------------------------------"
	echo "Infomration"
	echo "-----------------------------------------------------------"
	echo " SCRIPT_MODE : ${SCRIPT_MODE}"
	echo " K2H_FILENAME    : ${K2H_FILENAME}"
	echo " MASK        : ${MASK}"
	echo " CMASK       : ${CMASK}"
	echo " MAXELECNT   : ${MAXELECNT}"
	echo " PAGESIZE    : ${PAGESIZE}"
	echo " RONLY_OPT       : ${RONLY_OPT}"
	echo " DUMP_OPT        : ${DUMP_OPT}"
	echo " DEBUG_OPT         : ${DEBUG_OPT}"
	echo " KEYPREFIX   : ${KEYPREFIX}"
	echo " MAXCOUNT    : ${MAXCOUNT}"
	echo " PREVALUE    : ${PREVALUE}"
	echo " VALUE       : ${VALUE}"
	echo "-----------------------------------------------------------"
fi

#==============================================================
# Main
#==============================================================
if [ "${SCRIPT_MODE}" = "R" ]; then
	echo "[TEST] k2hinittest -f ${K2H_FILENAME} ${RONLY_OPT} ${DUMP_OPT} ${DEBUG_OPT}"

	if ! /bin/sh -c "${TESTPROGDIR}/k2hinittest -f ${K2H_FILENAME} ${RONLY_OPT} ${DUMP_OPT} ${DEBUG_OPT}"; then
		echo "    [Result] : ERROR"
	fi
	echo "    [Result] OK"

elif [ "${SCRIPT_MODE}" = "M" ]; then
	echo "[TEST] k2hmemtest -mask ${MASK} -cmask ${CMASK} -elementcnt ${MAXELECNT} -pagesize ${PAGESIZE} ${DUMP_OPT} ${DEBUG_OPT} ${KEYPREFIX} ${MAXCOUNT} value..."

	if ! /bin/sh -c "${TESTPROGDIR}/k2hmemtest -mask ${MASK} -cmask ${CMASK} -elementcnt ${MAXELECNT} -pagesize ${PAGESIZE} ${DUMP_OPT} ${DEBUG_OPT} ${KEYPREFIX} ${MAXCOUNT} ${VALUE}"; then
		echo "    [Result] : ERROR"
	fi
	echo "    [Result] OK"

elif [ "${SCRIPT_MODE}" = "F" ]; then
	if [ -f "${K2H_FILENAME}" ]; then
		rm -f "${K2H_FILENAME}"
	fi

	#
	# Initialize
	#
	echo "[TEST] (Initializing) k2hinittest -f ${K2H_FILENAME} -mask ${MASK} -cmask ${CMASK} -elementcnt ${MAXELECNT} -pagesize ${PAGESIZE} ${DUMP_OPT} ${DEBUG_OPT}"

	if ! /bin/sh -c "${TESTPROGDIR}/k2hinittest -f ${K2H_FILENAME} -mask ${MASK} -cmask ${CMASK} -elementcnt ${MAXELECNT} -pagesize ${PAGESIZE} ${DUMP_OPT} ${DEBUG_OPT}"; then
		echo "    [Result] : ERROR"
	fi
	echo "    [Result] OK"
	echo ""

	#
	# Writing
	#
	echo "[TEST] (Writing - ${COUNT}) k2hrwtest -f ${K2H_FILENAME} ${DEBUG_OPT} ${KEYPREFIX}-${COUNT} value..."
	COUNT=0
	while [ "${COUNT}" -lt "${MAXCOUNT}" ]; do
		if ! /bin/sh -c "${TESTPROGDIR}/k2hrwtest -f ${K2H_FILENAME} ${DEBUG_OPT} ${KEYPREFIX}-${COUNT} ${VALUE}"; then
			echo "    [Result] : ERROR(Failed to writing)"
		fi
		COUNT=$((COUNT + 1))
	done
	echo "    [Result] OK"
	echo ""

	#
	# Dumping
	#
	echo "[TEST](Dumping) k2hinittest -f ${K2H_FILENAME} -ro ${DUMP_OPT} ${DEBUG_OPT}"

	if ! /bin/sh -c "${TESTPROGDIR}/k2hinittest -f ${K2H_FILENAME} -ro ${DUMP_OPT} ${DEBUG_OPT}"; then
		echo "    [Result] : ERROR"
	fi
	echo "    [Result] OK"

elif [ "${SCRIPT_MODE}" = "T" ]; then
	#
	# Initialize
	#
	echo "[TEST] (Initializing) k2hinittest -f ${K2H_FILENAME} -tmp -mask ${MASK} -cmask ${CMASK} -elementcnt ${MAXELECNT} -pagesize ${PAGESIZE} ${DUMP_OPT} ${DEBUG_OPT}"

	if ! /bin/sh -c "${TESTPROGDIR}/k2hinittest -f ${K2H_FILENAME} -tmp -mask ${MASK} -cmask ${CMASK} -elementcnt ${MAXELECNT} -pagesize ${PAGESIZE} ${DUMP_OPT} ${DEBUG_OPT}"; then
		echo "    [Result] : ERROR"
	fi
	echo "    [Result] OK"
	echo ""

	#
	# Writing
	#
	echo "[TEST] (Writing - ${COUNT}) k2hrwtest -f ${K2H_FILENAME} -tmp ${DEBUG_OPT} ${KEYPREFIX}-${COUNT} value..."
	COUNT=0
	while [ "${COUNT}" -lt "${MAXCOUNT}" ]; do
		if ! /bin/sh -c "${TESTPROGDIR}/k2hrwtest -f ${K2H_FILENAME} -tmp ${DEBUG_OPT} ${KEYPREFIX}-${COUNT} ${VALUE}"; then
			echo "    [Result] : ERROR(Failed to writing)"
		fi
		COUNT=$((COUNT + 1))
	done
	echo "    [Result] OK"
	echo ""

	#
	# Dumping
	#
	echo "[TEST](Dumping) k2hinittest -f ${K2H_FILENAME} -tmp -mask ${MASK} -cmask ${CMASK} -elementcnt ${MAXELECNT} -pagesize ${PAGESIZE} ${DUMP_OPT} ${DEBUG_OPT}"

	if ! /bin/sh -c "${TESTPROGDIR}/k2hinittest -f ${K2H_FILENAME} -tmp -mask ${MASK} -cmask ${CMASK} -elementcnt ${MAXELECNT} -pagesize ${PAGESIZE} ${DUMP_OPT} ${DEBUG_OPT}"; then
		echo "    [Result] : ERROR"
	fi
	echo "    [Result] OK"
fi

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
