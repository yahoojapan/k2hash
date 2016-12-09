#!/bin/sh
#
# K2HASH
#
# Copyright 2013 Yahoo! JAPAN corporation.
#
# K2HASH is key-valuew store base libraries.
# K2HASH is made for the purpose of the construction of
# original KVS system and the offer of the library.
# The characteristic is this KVS library which Key can
# layer. And can support multi-processing and multi-thread,
# and is provided safely as available KVS.
#
# For the full copyright and license information, please view
# the LICENSE file that was distributed with this source code.
#
# AUTHOR:   Takeshi Nakatani
# CREATE:   Mon Feb 24 2014
# REVISION:
#

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
        echo "Mode:   -m                    anoymous memory mode"
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

##############
# environment
##############
if [ "X${TESTPROGDIR}" = "X" ]; then
	MYSCRIPTDIR=`dirname $0`
	TESTPROGDIR=`cd ${MYSCRIPTDIR}; pwd`
fi

##############
# Paraemters
##############
echo ""

PRGNAME=`basename $0`
MYDBG=
MODE=
FILENAME=
MASK=8
CMASK=4
MAXELECNT=32
PAGESIZE=128
RONLY=
DUMP=
DBG=
KEYPREFIX=
MAXCOUNT=
VALUE="ABCDEFGHIJKLMNOPQRSTUVWXYZ@ABCDEFGHIJKLMNOPQRSTUVWXYZ@ABCDEFGHIJKLMNOPQRSTUVWXYZ@ABCDEFGHIJKLMNOPQRSTUVWXYZ@"
PREVALUE=

while [ true ]; do
	if [ "X$1" = "X" ]; then
		break;
	fi
	OPTION=$1

	if [ "X$OPTION" = "X-h" -o "X$OPTION" = "X-H" ]; then
		func_usage $PRGNAME
		exit 0
	fi
	if [ "X$OPTION" = "X-mydbg" -o "X$OPTION" = "X-MYDBG" ]; then
		MYDBG=DBG
		OPTION=
	fi
	if [ "X$OPTION" = "X-m" -o "X$OPTION" = "X-M" ]; then
		MODE="M"
		OPTION=
	fi
	if [ "X$OPTION" = "X-f" -o "X$OPTION" = "X-F" ]; then
		MODE="F"
		shift
		if [ "X$1" = "X" ]; then
			echo "[ERR] -f option needs parameter."
			echo ""
			func_usage $PRGNAME
			exit 1
		fi
		FILENAME=$1
		OPTION=
	fi
	if [ "X$OPTION" = "X-t" -o "X$OPTION" = "X-T" ]; then
		MODE="T"
		shift
		if [ "X$1" = "X" ]; then
			echo "[ERR] -f option needs parameter."
			echo ""
			func_usage $PRGNAME
			exit 1
		fi
		FILENAME=$1
		OPTION=
	fi
	if [ "X$OPTION" = "X-r" -o "X$OPTION" = "X-R" ]; then
		MODE="R"
		shift
		if [ "X$1" = "X" ]; then
			echo "[ERR] -r option needs parameter."
			echo ""
			func_usage $PRGNAME
			exit 1
		fi
		FILENAME=$1
		OPTION=
	fi
	if [ "X$OPTION" = "X-mask" -o "X$OPTION" = "X-MASK" ]; then
		shift
		if [ "X$1" = "X" ]; then
			echo "[ERR] -mask option needs parameter."
			func_usage $PRGNAME
			exit 1
		fi
		MASK=$1
		OPTION=
	fi
	if [ "X$OPTION" = "X-cmask" -o "X$OPTION" = "X-CMASK" ]; then
		shift
		if [ "X$1" = "X" ]; then
			echo "[ERR] -cmask option needs parameter."
			func_usage $PRGNAME
			exit 1
		fi
		CMASK=$1
		OPTION=
	fi
	if [ "X$OPTION" = "X-elementcnt" -o "X$OPTION" = "X-ELEMENTCNT" ]; then
		shift
		if [ "X$1" = "X" ]; then
			echo "[ERR] -elementcnt option needs parameter."
			func_usage $PRGNAME
			exit 1
		fi
		MAXELECNT=$1
		OPTION=
	fi
	if [ "X$OPTION" = "X-pagesize" -o "X$OPTION" = "X-PAGESIZE" ]; then
		shift
		if [ "X$1" = "X" ]; then
			echo "[ERR] -pagesize option needs parameter."
			func_usage $PRGNAME
			exit 1
		fi
		PAGESIZE=$1
		OPTION=
	fi
	if [ "X$OPTION" = "X-ro" -o "X$OPTION" = "X-RO" ]; then
		RONLY=-ro
		OPTION=
	fi
	if [ "X$OPTION" = "X-d" -o "X$OPTION" = "X-D" ]; then
		shift
		if [ "X$1" = "X" ]; then
			echo "[ERR] -p option needs parameter."
			func_usage $PRGNAME
			exit 1
		fi
		DUMP="-d $1"
		OPTION=
	fi
	if [ "X$OPTION" = "X-g" -o "X$OPTION" = "X-G" ]; then
		shift
		if [ "X$1" = "X" ]; then
			echo "[ERR] -g option needs parameter."
			func_usage $PRGNAME
			exit 1
		fi
		DBG="-g $1"
		OPTION=
	fi

	if [ "X$OPTION" != "X" ]; then
		if [ "X$KEYPREFIX" = "X" ]; then
			KEYPREFIX=$OPTION
		elif [ "X$MAXCOUNT" = "X" ]; then
			MAXCOUNT=$OPTION
		elif [ "X$PREVALUE" = "X" ]; then
			PREVALUE=$OPTION
		else
			echo "[ERR] $OPTION is unknown parameter."
			func_usage $PRGNAME
			exit 1
		fi
	fi
	shift
done

if [ "X$PREVALUE" != "X" ]; then
	VALUE=$PREVALUE
fi

if [ "X$MODE" = "XM" -o "X$MODE" = "XF" -o "X$MODE" = "XT" ]; then
	if [ "X$RONLY" != "X" ]; then
		echo "[ERR] -m option can not use with -ro option."
		func_usage $PRGNAME
		exit 1
	fi
fi
if [ "X$MODE" = "XT" -a "X$RONLY" != "X" ]; then
	echo "[ERR] -t option can not use with -ro option."
	func_usage $PRGNAME
	exit 1
fi
if [ "X$MODE" = "XR" ]; then
	if [ "X$KEYPREFIX" != "X" -o "X$MAXCOUNT" != "X" -o "X$PREVALUE" != "X" ]; then
		echo "[ERR] -r option can not use with KEYNAME-PREFIX / REPEAT-COUNT / VALUE option."
		func_usage $PRGNAME
		exit 1
	fi
	if [ "X$DUMP" = "X" ]; then
		echo "[WAN] -d option is not specified, so use default FULL."
		echo ""
		DUMP="-d FULL"
	fi
	RONLY=-ro
fi

##############
# Internal Debug
##############
if [ "X$MYDBG" != "X" ]; then
	echo "----------------"
	echo "OWN DEBUGGING"
	echo "----------------"
	echo ""
	echo "MODE=$MODE"
	echo "FILENAME=$FILENAME"
	echo "MASK=$MASK"
	echo "CMASK=$CMASK"
	echo "MAXELECNT=$MAXELECNT"
	echo "PAGESIZE=$PAGESIZE"
	echo "RONLY=$RONLY"
	echo "DUMP=$DUMP"
	echo "DBG=$DBG"
	echo "KEYPREFIX=$KEYPREFIX"
	echo "MAXCOUNT=$MAXCOUNT"
	echo "PREVALUE=$PREVALUE"
	echo "VALUE=$VALUE"
	echo ""
	echo "---------------"
	echo ""
fi

##############
# Execte
##############

if [ "X$MODE" = "XR" ]; then
	echo "k2hinittest -f $FILENAME $RONLY $DUMP $DBG"
	echo ""
	${TESTPROGDIR}/k2hinittest -f $FILENAME $RONLY $DUMP $DBG
	echo ""

elif [ "X$MODE" = "XM" ]; then
	echo "k2hmemtest -mask $MASK -cmask $CMASK -elementcnt $MAXELECNT -pagesize $PAGESIZE $DUMP $DBG $KEYPREFIX $MAXCOUNT value..."
	echo ""
	${TESTPROGDIR}/k2hmemtest -mask $MASK -cmask $CMASK -elementcnt $MAXELECNT -pagesize $PAGESIZE $DUMP $DBG $KEYPREFIX $MAXCOUNT $VALUE
	echo ""

elif [ "X$MODE" = "XF" ]; then
	if [ -f $FILENAME ]; then
		rm -f $FILENAME
	fi

	echo "[ Initialize... ]"
	echo "k2hinittest -f $FILENAME -mask $MASK -cmask $CMASK -elementcnt $MAXELECNT -pagesize $PAGESIZE $DUMP $DBG"
	echo ""
	${TESTPROGDIR}/k2hinittest -f $FILENAME -mask $MASK -cmask $CMASK -elementcnt $MAXELECNT -pagesize $PAGESIZE $DUMP $DBG
	echo ""

	echo "[ Writing... ]"
	echo ""
	COUNT=0
	while [ true ]; do
		${TESTPROGDIR}/k2hrwtest -f $FILENAME $DBG ${KEYPREFIX}-${COUNT} $VALUE
		COUNT=`expr $COUNT + 1`
		if [ $COUNT -ge $MAXCOUNT ]; then
			break;
		fi
	done
	echo ""

	echo "[ Dumping... ]"
	echo ""
	echo "k2hinittest -f $FILENAME -ro $DUMP $DBG"
	${TESTPROGDIR}/k2hinittest -f $FILENAME -ro $DUMP $DBG
	echo ""

elif [ "X$MODE" = "XT" ]; then
	echo "[ Initialize... ]"
	echo "k2hinittest -f $FILENAME -tmp -mask $MASK -cmask $CMASK -elementcnt $MAXELECNT -pagesize $PAGESIZE $DUMP $DBG"
	echo ""
	${TESTPROGDIR}/k2hinittest -f $FILENAME -tmp -mask $MASK -cmask $CMASK -elementcnt $MAXELECNT -pagesize $PAGESIZE $DUMP $DBG
	echo ""

	echo "[ Writing... ]"
	echo ""
	COUNT=0
	while [ true ]; do
		${TESTPROGDIR}/k2hrwtest -f $FILENAME -tmp $DBG ${KEYPREFIX}-${COUNT} $VALUE
		COUNT=`expr $COUNT + 1`
		if [ $COUNT -ge $MAXCOUNT ]; then
			break;
		fi
	done
	echo ""

	echo "[ Dumping... ]"
	echo ""
	echo "k2hinittest -f $FILENAME -tmp -mask $MASK -cmask $CMASK -elementcnt $MAXELECNT -pagesize $PAGESIZE $DUMP $DBG"
	${TESTPROGDIR}/k2hinittest -f $FILENAME -tmp -mask $MASK -cmask $CMASK -elementcnt $MAXELECNT -pagesize $PAGESIZE $DUMP $DBG
	echo ""

else
	echo "[ERR] -m/-f/-t option not exists."
	func_usage $PRGNAME
	exit 1
fi

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
