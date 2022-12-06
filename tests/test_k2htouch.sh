#!/bin/bash
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
# AUTHOR:   Tetsuya Mochizuki
# CREATE:   Fri Feb 2 2015
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
#SRCTOP=$(cd "${SCRIPTDIR}/.." || exit 1; pwd)

#
# Variables
#
K2HTOUCH_BIN="${SCRIPTDIR}/k2htouch"
TEST_FILENAME="my.k2h"
TEST_KEYNAME="test_key_name"

#==============================================================
# Main
#==============================================================
#
# Initialize
#
echo "[TEST] Initialize for testing"

rm -f "${TEST_FILENAME}"
if ! "${K2HTOUCH_BIN}" "${TEST_FILENAME}" createmini; then
	echo "  --> ERROR : Failed to initialize ${TEST_FILENAME}"
	exit 1
fi

#--------------------------------------------------------------
# Test set key
#--------------------------------------------------------------
echo "[TEST] Set key"

IS_ERROR=0
echo "* Set key"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" set key1 key1value || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set key1 key1value"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" set key2 key2value || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set key2 key2value"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" set key3 key3value || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set key3 key3value"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test get key
#--------------------------------------------------------------
echo "[TEST] Get key"

IS_ERROR=0
echo "* Get key"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" get key1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to get key1"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" get key2 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to get key2"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" get key3 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to get key2"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test add subkey
#--------------------------------------------------------------
echo "[TEST] Add subkey"

IS_ERROR=0
echo "* Add subkey"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" addsubkey key1 key1-1sub key1-1subvalue || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to add subkey( key1 -> key1-1sub = key1-1subvalue)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" addsubkey key1 key1-2sub key1-2subvalue || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to add subkey( key1 -> key1-2sub = key1-2subvalue)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" addsubkey key1 key1-3sub key1-3subvalue || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to add subkey( key1 -> key1-3sub = key1-3subvalue)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" addsubkey key2 key2-1sub key2-1subvalue || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to add subkey( key2 -> key2-1sub = key2-1subvalue)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" addsubkey key2 key2-2sub key2-2subvalue || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to add subkey( key2 -> key2-2sub = key2-2subvalue)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" addsubkey key2 key2-3sub key2-3subvalue || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to add subkey( key2 -> key2-3sub = key2-3subvalue)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" addsubkey key3 key3-1sub key3-1subvalue || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to add subkey( key3 -> key3-1sub = key3-1subvalue)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" addsubkey key3 key3-2sub key3-2subvalue || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to add subkey( key3 -> key3-2sub = key3-2subvalue)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" addsubkey key3 key3-3sub key3-3subvalue || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to add subkey( key3 -> key3-3sub = key3-3subvalue)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test list
#--------------------------------------------------------------
echo "[TEST] List"

echo "* List"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" list || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to test list"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test Get subkey
#--------------------------------------------------------------
echo "[TEST] Get subkey"

echo "* Get subkey"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" getsubkey key1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to get subkey(key1)"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test Delete keys
#--------------------------------------------------------------
echo "[TEST] Delete keys"

IS_ERROR=0
echo "* Delete key(key1, key3)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" delete key1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to delete key(key1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" delete key3 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to delete key(key1)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

echo "* Delete key(key2-2sub)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" delete key2-2sub || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to delete key(key2-2sub)"
else
	echo "  --> OK"
fi

echo "* List(check)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" list || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to list(for checking)"
else
	echo "  --> OK"
fi

echo "* Delete key(key2)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" delete key2 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to delete key(key2)"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test Queue
#--------------------------------------------------------------
echo "[TEST] Queue"

IS_ERROR=0
echo "* Push queue"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 2 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 3 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

IS_ERROR=0
echo "* Pop queue"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop queue(1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop queue(2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop queue(3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

echo "* Pop queue(no data)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> OK : Failed to pop queue, because queue does not have any key."
else
	echo "  --> ERROR :Succeed to pop queue but it was not any key"
fi

#--------------------------------------------------------------
# Test Queue(FIFO)
#--------------------------------------------------------------
echo "[TEST] Queue(FIFO)"

IS_ERROR=0
echo "* Push queue(FIFO)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 1 -fifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(FIFO:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 2 -fifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(FIFO:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 3 -fifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(FIFO:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

IS_ERROR=0
echo "* Pop queue(FIFO)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop queue(FIFO:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop queue(FIFO:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop queue(FIFO:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

echo "* Pop queue(FIFO:no data)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> OK : Failed to pop queue(FIFO), because queue does not have any key."
else
	echo "  --> ERROR : Succeed to pop queue(FIFO) but it was not any key"
fi

#--------------------------------------------------------------
# Test Queue(LIFO)
#--------------------------------------------------------------
echo "[TEST] Queue(LIFO)"

IS_ERROR=0
echo "* Push queue(LIFO)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 1 -lifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(LIFO:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 2 -lifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(LIFO:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 3 -lifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(LIFO:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

IS_ERROR=0
echo "* Pop queue(LIFO)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop queue(LIFO:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop queue(LIFO:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" pop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop queue(LIFO:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test Clear queue
#--------------------------------------------------------------
echo "[TEST] Clear queue"

IS_ERROR=0
echo "* Push queue"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 2 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" push "${TEST_KEYNAME}" 3 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

echo "* Clear queue"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" clear "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push queue(3)"
else
	echo "  --> OK"
fi

echo "* Clear queue"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" list || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to test list"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test Key Queue
#--------------------------------------------------------------
echo "[TEST] Key Queue"

IS_ERROR=0
echo "* Push Key Queue"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num1 1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num2 2 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num3 3 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

IS_ERROR=0
echo "* Pop Key Queue"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

echo "* Pop Key Queue(no-data)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> OK : Failed to pop key queue, because queue does not have any key."
else
	echo "  --> ERROR : Succeed to pop key queue but it was not any key"
fi

#--------------------------------------------------------------
# Test Key Queue(FIFO)
#--------------------------------------------------------------
echo "[TEST] Key Queue(FIFO)"

IS_ERROR=0
echo "* Push Key Queue(FIFO)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num1 1 -fifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(FIFO:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num2 2 -fifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(FIFO:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num3 3 -fifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(FIFO:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

IS_ERROR=0
echo "* Pop Key Queue(FIFO)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(FIFO:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(FIFO:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(FIFO:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test Key Queue(LIFO)
#--------------------------------------------------------------
echo "[TEST] Key Queue(LIFO)"

IS_ERROR=0
echo "* Push Key Queue(LIFO)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num1 1 -lifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(LIFO:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num2 2 -lifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(LIFO:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num3 3 -lifo || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(LIFO:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

IS_ERROR=0
echo "* Pop Key Queue(LIFO)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(LIFO:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(LIFO:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(LIFO:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test Key Queue(normal)
#--------------------------------------------------------------
echo "[TEST] Key Queue(normal)"

IS_ERROR=0
echo "* Push Key Queue(normal)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num1 1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(normal:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num2 2 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(normal:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num3 3 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(normal:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

IS_ERROR=0
echo "* Pop Key Queue(normal)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(normal:1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(normal:2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpop "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to pop key queue(normal:3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test Clear Key Queue
#--------------------------------------------------------------
echo "[TEST] Clear Key Queue"

IS_ERROR=0
echo "* Push Key Queue"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num1 1 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(1)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num2 2 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(2)"
	IS_ERROR=1
fi
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kpush "${TEST_KEYNAME}" num3 3 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to push key queue(3)"
	IS_ERROR=1
fi
if [ "${IS_ERROR}" -eq 0 ]; then
	echo "  --> OK"
fi

echo "* Clear Key Queue"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" kclear "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to clear key queue"
else
	echo "  --> OK"
fi

echo "* Test list"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" list || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to test list"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test error file(permission)
#--------------------------------------------------------------
echo "[TEST] Error file(permission)"

echo "* Set permission to test file"
if ! chmod 0444 "${TEST_FILENAME}"; then
	echo "  --> ERROR : Failed to set permisstion to ${TEST_FILENAME}"
else
	echo "  --> OK"
fi

echo "* Test list"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" list || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to test list"
else
	echo "  --> OK"
fi
chmod 0644 "${TEST_FILENAME}"

#--------------------------------------------------------------
# Test mtime mode
#--------------------------------------------------------------
echo "[TEST] mtime mode"

echo "* Print mtime mode"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" mtime || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print mtime mode"
else
	echo "  --> OK"
fi

echo "* Set mtime on"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" mtime on || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set mtime mode on"
else
	echo "  --> OK"
fi

echo "* Print mtime mode(on)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" mtime || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print mtime mode(on)"
else
	echo "  --> OK"
fi

echo "* Set mtime off"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" mtime off || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set mtime mode off"
else
	echo "  --> OK"
fi

echo "* Print mtime mode(off)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" mtime || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print mtime mode(off)"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test history mode
#--------------------------------------------------------------
echo "[TEST] history mode"

echo "* Print history mode"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" history || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print history mode"
else
	echo "  --> OK"
fi

echo "* Set history on"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" history on || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set history mode on"
else
	echo "  --> OK"
fi

echo "* Print history mode(on)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" history || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print history mode(on)"
else
	echo "  --> OK"
fi

echo "* Set history off"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" history off || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set history mode off"
else
	echo "  --> OK"
fi

echo "* Print history mode(off)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" history || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print history mode(off)"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Test expire
#--------------------------------------------------------------
echo "[TEST] expire"

echo "* Print expire"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" expire || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print expire"
else
	echo "  --> OK"
fi

echo "* Set expire 10"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" expire 10 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set expire 10"
else
	echo "  --> OK"
fi

echo "* Print expire(10)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" expire || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print expire(10)"
else
	echo "  --> OK"
fi

echo "* Set expire off"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" expire off || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set expire off"
else
	echo "  --> OK"
fi

echo "* Print expire(off)"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" expire || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print expire(off)"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Set value with mtime(off) and history(off)
#--------------------------------------------------------------
echo "[TEST] Set value with mtime(off) and history(off)"

echo "* Set value ${TEST_KEYNAME}=first_value"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" set "${TEST_KEYNAME}" first_value || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set value ${TEST_KEYNAME}=first_value"
else
	echo "  --> OK"
fi

echo "* Print info"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" info || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print information"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Set value with mtime(on) and history(off)
#--------------------------------------------------------------
echo "[TEST] Set value with mtime(on) and history(off)"

echo "* Set mtime mode on"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" mtime on || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set mtime mode on"
else
	echo "  --> OK"
fi

echo "* Set value ${TEST_KEYNAME}=second_value"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" set "${TEST_KEYNAME}" second_value || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set value ${TEST_KEYNAME}=second_value"
else
	echo "  --> OK"
fi

echo "* Get ${TEST_KEYNAME} attribute"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" getattr "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to get ${TEST_KEYNAME} attribute"
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Set value with mtime(on) and history(on)
#--------------------------------------------------------------
echo "[TEST] Set value with mtime(on) and history(on)"

echo "* Print list"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" list | grep "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print list"
else
	echo "  --> OK"
fi

echo "* Set history mode on"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" history on || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set history mode on"
else
	echo "  --> OK"
fi

echo "* Set value ${TEST_KEYNAME}=third_value"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" set "${TEST_KEYNAME}" third_value || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set value ${TEST_KEYNAME}=third_value"
else
	echo "  --> OK"
fi

echo "* Get ${TEST_KEYNAME} value"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" get "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to get ${TEST_KEYNAME} value."
else
	echo "  --> OK"
fi

#--------------------------------------------------------------
# Set/Read value with expire
#--------------------------------------------------------------
echo "[TEST] Set/Read value with expire"

echo "* Print list"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" list | grep "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to print list"
else
	echo "  --> OK"
fi

echo "* Set expire 5"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" expire 5 || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set expire 5"
else
	echo "  --> OK"
fi

echo "* Set value ${TEST_KEYNAME}=fourth_value"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" set "${TEST_KEYNAME}" fourth_value || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to set value ${TEST_KEYNAME}=fourth_value"
else
	echo "  --> OK"
fi

echo "* Get ${TEST_KEYNAME} value as soon as setting"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" get "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to get ${TEST_KEYNAME} value as soon as setting"
else
	echo "  --> OK"
fi

#
# Wait 3 second
#
sleep 3

echo "* Get ${TEST_KEYNAME} value after 3 second"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" get "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  --> ERROR : Failed to get ${TEST_KEYNAME} value after 3 second"
else
	echo "  --> OK"
fi

#
# Wait more 3 second
#
sleep 3

echo "* Get ${TEST_KEYNAME} value after more 3 second"
if ({ "${K2HTOUCH_BIN}" "${TEST_FILENAME}" get "${TEST_KEYNAME}" || echo > "${PIPEFAILURE_FILE}"; } | sed -e 's/^/    /g') && rm "${PIPEFAILURE_FILE}" >/dev/null 2>&1; then
	echo "  -->OK (Failed to get ${TEST_KEYNAME} value after more 3 second, because it was expired)"
else
	echo "  --> ERROR : Succeed to get ${TEST_KEYNAME} value after 3 second, it should be expired."
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
