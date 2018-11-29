#!/bin/sh
#
# K2HASH
#
# Copyright 2016 Yahoo Japan Corporation.
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
# CREATE:   Wed Jan 20 2016
# REVISION:
#

##############
# environment
##############
MYSCRIPTDIR=`dirname $0`
if [ "X${TESTPROGDIR}" = "X" ]; then
	TESTPROGDIR=`cd ${MYSCRIPTDIR}; pwd`
fi

PROCID=$$
#LINETOOL="${TESTPROGDIR}/k2hlinetool"
LINETOOL="${TESTPROGDIR}/k2hlinetool -g INFO"
K2HFILE="/tmp/k2hash_test_linetool.k2h"
CMDFILE="${MYSCRIPTDIR}/test_linetool.cmd"
DSAVECMDFILE="${MYSCRIPTDIR}/test_linetool_dsave.cmd"
LOGFILE="/tmp/k2hash_test_linetool_$PROCID.log"
LOGSUBFILE="/tmp/k2hash_test_linetool_$PROCID_noverinfo.log"
MASTERLOGFILE="${MYSCRIPTDIR}/test_linetool.log"
MASTERLOGSUBFILE="/tmp/test_linetool_noverinfo.log"

rm -f ${K2HFILE}
rm -f ${LOGFILE}

export K2HATTR_ENC_TYPE=AES256_PBKDF1

#################
# Initialize test
#################
${LINETOOL} -f ${K2HFILE} -init >> ${LOGFILE} 2>/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi

#################
# version test
#################
${LINETOOL} -libversion >> ${LOGFILE} 2>/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi

#################
# memory type test
#################
#${LINETOOL} -m -mask 4 -cmask 2 -elementcnt 32 -pagesize 128 -fullmap -run ${CMDFILE} >> ${LOGFILE} 2>/dev/null
${LINETOOL} -m -mask 4 -cmask 2 -elementcnt 32 -pagesize 128 -fullmap -run ${CMDFILE} >> ${LOGFILE} 2>&1
if [ $? -ne 0 ]; then
	exit 1
fi

#### Test for CAPI
${LINETOOL} -m -mask 4 -cmask 2 -elementcnt 32 -pagesize 128 -fullmap -capi -run ${CMDFILE} >> ${LOGFILE} 2>/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi

#################
# tmp file type test
#################
rm -f ${K2HFILE}
${LINETOOL} -t ${K2HFILE} -mask 4 -cmask 2 -elementcnt 32 -pagesize 128 -fullmap -run ${CMDFILE} >> ${LOGFILE} 2>/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi

#################
# file type test
#################
rm -f ${K2HFILE}
${LINETOOL} -f ${K2HFILE} -mask 4 -cmask 2 -elementcnt 32 -pagesize 128 -fullmap -run ${CMDFILE} >> ${LOGFILE} 2>/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi

#################
# test for direct save/load
#################
${LINETOOL} -m -mask 2 -cmask 2 -elementcnt 2 -fullmap -run ${DSAVECMDFILE} >> ${LOGFILE} 2>/dev/null
if [ $? -ne 0 ]; then
	exit 1
fi

#################
# check result
#################
diff ${LOGFILE} ${MASTERLOGFILE} >/dev/null 2>&1
if [ $? -ne 0 ]; then
	#
	# Version number is difference
	#
	cat ${LOGFILE}       | grep -v '^[a-z][0-9]*[ ][0-9]*$' | grep -v 'K2HASH library Version' | grep -v 'FULLOCK Version' > ${LOGSUBFILE} 2>/dev/null
	cat ${MASTERLOGFILE} | grep -v '^[a-z][0-9]*[ ][0-9]*$' | grep -v 'K2HASH library Version' | grep -v 'FULLOCK Version' > ${MASTERLOGSUBFILE} 2>/dev/null
	diff ${LOGSUBFILE} ${MASTERLOGSUBFILE} >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "============================================"
		echo "TEST LINETOOL IS FAILED"
		echo "============================================"
		echo "diff ${LOGFILE} ${MASTERLOGFILE} "
		echo ""
		diff ${LOGFILE} ${MASTERLOGFILE}
		echo "============================================"
		rm -f ${LOGFILE}
		rm -f ${LOGSUBFILE}
		rm -f ${MASTERLOGSUBFILE}
		exit 1
	fi
	rm -f ${LOGSUBFILE}
	rm -f ${MASTERLOGSUBFILE}
fi

rm -f ${LOGFILE}
rm -f ${K2HFILE}

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
