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

#--------------------------------------------------------------
# Common Variables
#--------------------------------------------------------------
#PRGNAME=$(basename "${0}")
SCRIPTDIR=$(dirname "${0}")
SCRIPTDIR=$(cd "${SCRIPTDIR}" || exit 1; pwd)
#SRCTOP=$(cd "${SCRIPTDIR}/.." || exit 1; pwd)

#
# Environment
#
if [ -z "${TESTPROGDIR}" ]; then
	TESTPROGDIR="${SCRIPTDIR}"
fi

#
# Variables
#
PROCID=$$
LINETOOL="${TESTPROGDIR}/k2hlinetool"
K2HFILE="/tmp/k2hash_test_linetool.k2h"
CMDFILE="${SCRIPTDIR}/test_linetool.cmd"
DSAVECMDFILE="${SCRIPTDIR}/test_linetool_dsave.cmd"

LOGFILE="/tmp/k2hash_test_linetool_${PROCID}.log"
LOGSUBFILE="/tmp/k2hash_test_linetool_${PROCID}_noverinfo.log"
MASTERLOGFILE="${SCRIPTDIR}/test_linetool.log"
MASTERLOGSUBFILE="/tmp/test_linetool_noverinfo.log"

#==============================================================
# Run test
#==============================================================
rm -f "${K2HFILE}" "${LOGFILE}"

{
	#----------------------------------------------------------
	# Initialize test
	#----------------------------------------------------------
	if ! "${LINETOOL}" -f "${K2HFILE}" -init; then
		exit 1
	fi

	#----------------------------------------------------------
	# version test
	#----------------------------------------------------------
	if ! "${LINETOOL}" -libversion; then
		exit 1
	fi

	#----------------------------------------------------------
	# memory type test
	#----------------------------------------------------------
	#
	# Test for CPP API
	#
	if ! "${LINETOOL}" -m -mask 4 -cmask 2 -elementcnt 32 -pagesize 128 -fullmap -run "${CMDFILE}"; then
		exit 1
	fi
	#
	# Test for CAPI
	#
	if ! "${LINETOOL}" -m -mask 4 -cmask 2 -elementcnt 32 -pagesize 128 -fullmap -capi -run "${CMDFILE}"; then
		exit 1
	fi

	#----------------------------------------------------------
	# tmp file type test
	#----------------------------------------------------------
	rm -f "${K2HFILE}"

	if ! "${LINETOOL}" -t "${K2HFILE}" -mask 4 -cmask 2 -elementcnt 32 -pagesize 128 -fullmap -run "${CMDFILE}"; then
		exit 1
	fi

	#----------------------------------------------------------
	# file type test
	#----------------------------------------------------------
	rm -f "${K2HFILE}"

	if ! "${LINETOOL}" -f "${K2HFILE}" -mask 4 -cmask 2 -elementcnt 32 -pagesize 128 -fullmap -run "${CMDFILE}"; then
		exit 1
	fi

	#----------------------------------------------------------
	# test for direct save/load
	#----------------------------------------------------------
	if ! "${LINETOOL}" -m -mask 2 -cmask 2 -elementcnt 2 -fullmap -run "${DSAVECMDFILE}"; then
		exit 1
	fi

} > "${LOGFILE}" 2>/dev/null

rm -f "${K2HFILE}"

#==============================================================
# Check result
#==============================================================
# [NOTE]
# Exclude the version number as it may be different.
#
grep -v '^[a-z][0-9]*[ ][0-9]*$' "${LOGFILE}"       | grep -v 'K2HASH library Version' | grep -v 'FULLOCK Version' > "${LOGSUBFILE}"       2>/dev/null
grep -v '^[a-z][0-9]*[ ][0-9]*$' "${MASTERLOGFILE}" | grep -v 'K2HASH library Version' | grep -v 'FULLOCK Version' > "${MASTERLOGSUBFILE}" 2>/dev/null

if ! diff "${LOGSUBFILE}" "${MASTERLOGSUBFILE}" >/dev/null 2>&1; then
	echo "==========================================================="
	echo "[FAILED] TEST LINETOOL"
	echo "-----------------------------------------------------------"
	echo "Result : \"diff ${LOGFILE} ${MASTERLOGFILE}\""
	echo "-----------------------------------------------------------"
	diff "${LOGSUBFILE}" "${MASTERLOGSUBFILE}"
	echo "-----------------------------------------------------------"

	rm -f ${LOGFILE}
	rm -f ${LOGSUBFILE}
	rm -f ${MASTERLOGSUBFILE}
	exit 1
fi

rm -f "${LOGFILE}"
rm -f "${LOGSUBFILE}"
rm -f "${MASTERLOGSUBFILE}"

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
