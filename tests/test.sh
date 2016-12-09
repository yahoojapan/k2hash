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
# CREATE:   Tue Nov 29 2013
# REVISION:
#

##############################################################
## library path & programs path
##
MYSCRIPTDIR=`dirname $0`
if [ "X${SRCTOP}" = "X" ]; then
	SRCTOP=`cd ${MYSCRIPTDIR}/..; pwd`
fi
cd ${MYSCRIPTDIR}
if [ "X${OBJDIR}" = "X" ]; then
	LD_LIBRARY_PATH="${SRCTOP}/lib/.lib"
	TESTPROGDIR=${MYSCRIPTDIR}
else
	LD_LIBRARY_PATH="${SRCTOP}/lib/${OBJDIR}"
	TESTPROGDIR=${MYSCRIPTDIR}/${OBJDIR}
fi
export LD_LIBRARY_PATH
export TESTPROGDIR

##############################################################
## variables
##
DATE=`date`
PROCID=$$
FILEPATH="/tmp/k2hash_test_$PROCID.k2h"

if [ "X$1" = "X" ]; then
	LOGFILE="/dev/null"
#	LOGFILE="/tmp/k2hash_test_$PROCID.log"
else
	LOGFILE="$1"
fi

echo "================= $DATE ====================" > $LOGFILE

##############################################################
###
### Memory test
###
echo "-- TEST Memory mode --" >> $LOGFILE
echo "" >> $LOGFILE
./testtool.sh -m memkey 100 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Memory mode test --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Memory mode test --->> OK" >> $LOGFILE


##############################################################
###
### Temp file test
###
echo "" >> $LOGFILE
echo "-- TEST Temp file mode --" >> $LOGFILE
echo "" >> $LOGFILE
./testtool.sh -t $FILEPATH tkey 100 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Temp file mode test --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Temp file mode test --->> OK" >> $LOGFILE


##############################################################
###
### Permanent file test
###
echo "" >> $LOGFILE
echo "-- TEST file mode --" >> $LOGFILE
echo "" >> $LOGFILE
./testtool.sh -f $FILEPATH tkey 100 >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "File mode test --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "File mode test --->> OK" >> $LOGFILE


##############################################################
###
### Dump file test
###
echo "" >> $LOGFILE
echo "-- DUMP file mode --" >> $LOGFILE
echo "" >> $LOGFILE
./testtool.sh -r $FILEPATH >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "Dump mode test --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "Dump mode test --->> OK" >> $LOGFILE

##############################################################
###
### API test by k2hlinetool
###
echo "" >> $LOGFILE
echo "-- k2hlinetool test --" >> $LOGFILE
echo "" >> $LOGFILE
./test_linetool.sh >> $LOGFILE

if [ $? -ne 0 ]; then
	echo "API k2hlinetool test --->> ERROR" >> $LOGFILE
	exit 1
fi
echo "API k2hlinetool test --->> OK" >> $LOGFILE

##############################################################
###
### Remove file
###
rm -f $FILEPATH

exit 0

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
