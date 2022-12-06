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
# CREATE:   Tue Nov 29 2013
# REVISION:
#

#--------------------------------------------------------------
# Common Variables
#--------------------------------------------------------------
#
# Instead of pipefail(for shells not support "set -o pipefail")
#
PIPEFAILURE_FILE="/tmp/.pipefailure.$(od -An -tu4 -N4 /dev/random | tr -d ' \n')"

#PRGNAME=$(basename "${0}")
SCRIPTDIR=$(dirname "${0}")
SCRIPTDIR=$(cd "${SCRIPTDIR}" || exit 1; pwd)
SRCTOP=$(cd "${SCRIPTDIR}/.." || exit 1; pwd)

#
# Directories / Files
#
TESTDIR="${SRCTOP}/tests"
LIBOBJDIR="${SRCTOP}/lib/.libs"
#TESTOBJDIR="${TESTDIR}/.libs"

TEST_TOOL="${TESTDIR}/test_tool.sh"
TEST_LINETOOL="${TESTDIR}/test_linetool.sh"

#
# LD_LIBRARY_PATH / TESTPROGDIR
#
LD_LIBRARY_PATH="${LIBOBJDIR}"
export LD_LIBRARY_PATH

TESTPROGDIR="${TESTDIR}"
export TESTPROGDIR

#--------------------------------------------------------------
# Variables
#--------------------------------------------------------------
DATE=$(date)
PROCID=$$

TEST_FILEPATH="/tmp/k2hash_test_$PROCID.k2h"

#--------------------------------------------------------------
# Input Variables
#--------------------------------------------------------------
if [ -z "$1" ]; then
	LOGFILE="/dev/null"
#	LOGFILE="/tmp/k2hash_test_$PROCID.log"
else
	LOGFILE="$1"
fi

#==============================================================
# Main
#==============================================================
{
	echo "================= ${DATE} ===================="

	#----------------------------------------------------------
	# Memory test
	#----------------------------------------------------------
	echo "[TEST] Memory mode"

	if ({ "${TEST_TOOL}" -m memkey 100 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"

	#----------------------------------------------------------
	# Temp file test
	#----------------------------------------------------------
	echo "[TEST] Temp file mode"

	if ({ "${TEST_TOOL}" -t "${TEST_FILEPATH}" tkey 100 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"

	#----------------------------------------------------------
	# Permanent file test
	#----------------------------------------------------------
	echo "[TEST] file mode"

	if ({ "${TEST_TOOL}" -f "${TEST_FILEPATH}" tkey 100 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"

	#----------------------------------------------------------
	# Dump file test
	#----------------------------------------------------------
	echo "[TEST] Dump - file mode"

	if ({ "${TEST_TOOL}" -r "${TEST_FILEPATH}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"

	#----------------------------------------------------------
	# API test by k2hlinetool
	#----------------------------------------------------------
	echo "[TEST] Run k2hlinetool"

	if ({ "${TEST_LINETOOL}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
		echo "    [Result] ERROR"
		exit 1
	fi
	echo "    [Result] OK"

	#----------------------------------------------------------
	# Remove file
	#----------------------------------------------------------
	rm -f "${TEST_FILEPATH}"

} | tee "${LOGFILE}"

exit 0

#
# Local variables:
# tab-width: 4
# c-basic-offset: 4
# End:
# vim600: noexpandtab sw=4 ts=4 fdm=marker
# vim<600: noexpandtab sw=4 ts=4
#
