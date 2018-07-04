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

export hr=" ------------------------- "
echo $hr "test info" $hr
./k2htouch z.k2 info

if [ -e z.k2 ]; then
	rm z.k2
fi

echo -e "\e[31m// print status? //\e[m"
# -----------------------------------------------
# basic command
# -----------------------------------------------
echo $hr "test createmini" $hr
./k2htouch z.k2 createmini
echo -e "\e[31m// no error? //\e[m"


echo $hr "test set" $hr
./k2htouch z.k2 set key1 key1value
./k2htouch z.k2 set key2 key2value
./k2htouch z.k2 set key3 key3value
echo -e "\e[31m// no error? //\e[m"

echo $hr "test get" $hr
./k2htouch z.k2 get key1
./k2htouch z.k2 get key2
./k2htouch z.k2 get key3
echo ""
echo -e "\e[31m// key1-3 value? //\e[m"

echo $hr "test addsubkey" $hr
./k2htouch z.k2 addsubkey key1 key1-1sub key1-1subvalue
./k2htouch z.k2 addsubkey key1 key1-2sub key1-2subvalue
./k2htouch z.k2 addsubkey key1 key1-3sub key1-3subvalue
./k2htouch z.k2 addsubkey key2 key2-1sub key2-1subvalue
./k2htouch z.k2 addsubkey key2 key2-2sub key2-2subvalue
./k2htouch z.k2 addsubkey key2 key2-3sub key2-3subvalue
./k2htouch z.k2 addsubkey key3 key3-1sub key3-1subvalue
./k2htouch z.k2 addsubkey key3 key3-2sub key3-2subvalue
./k2htouch z.k2 addsubkey key3 key3-3sub key3-3subvalue
echo -e "\e[31m// no error? //\e[m"

echo $hr "test list" $hr
./k2htouch z.k2 list
echo -e "\e[31m// 12 records? //\e[m"

echo $hr "test getsubkey Key1" $hr
./k2htouch z.k2 getsubkey key1
echo -e "\e[31m// 3 records? //\e[m"

echo $hr "test delete key1 Key3" $hr
./k2htouch z.k2 delete key1
./k2htouch z.k2 delete key3
echo -e "\e[31m// no error? //\e[m"

echo $hr "test delete key2-2" $hr
./k2htouch z.k2 delete key2-2sub
echo -e "\e[31m// no error? //\e[m"

echo $hr "test list" $hr
./k2htouch z.k2 list
echo -e "\e[31m// 3 records? //\e[m"

# all delete
./k2htouch z.k2 delete key2
# -----------------------------------------------
# que
# -----------------------------------------------
echo $hr "test push/pop(default)" $hr
./k2htouch z.k2 push temochiz 1
./k2htouch z.k2 push temochiz 2
./k2htouch z.k2 push temochiz 3
./k2htouch z.k2 pop temochiz
./k2htouch z.k2 pop temochiz
./k2htouch z.k2 pop temochiz
# nodata
./k2htouch z.k2 pop temochiz
echo -e "\e[31m// result = 1 2 3 (null)? //\e[m"

echo $hr "test push/pop(fifo)" $hr
./k2htouch z.k2 push temochiz 1 -fifo
./k2htouch z.k2 push temochiz 2 -fifo
./k2htouch z.k2 push temochiz 3 -fifo
./k2htouch z.k2 pop temochiz
./k2htouch z.k2 pop temochiz
./k2htouch z.k2 pop temochiz
./k2htouch z.k2 pop temochiz
echo -e "\e[31m// result = 1 2 3 (null)? //\e[m"

echo $hr "test push/pop(lifo)" $hr
./k2htouch z.k2 push temochiz 1 -lifo
./k2htouch z.k2 push temochiz 2 -lifo
./k2htouch z.k2 push temochiz 3 -lifo
./k2htouch z.k2 pop temochiz
./k2htouch z.k2 pop temochiz
./k2htouch z.k2 pop temochiz
echo -e "\e[31m// result = 3 2 1 (null)? //\e[m"

echo $hr "test que clear" $hr
./k2htouch z.k2 push temochiz 1
./k2htouch z.k2 push temochiz 2
./k2htouch z.k2 push temochiz 3
./k2htouch z.k2 clear temochiz
./k2htouch z.k2 list

echo ""
echo -e "\e[31m// Result = Only Marker? //\e[m"

# -----------------------------------------------
# keyque
# -----------------------------------------------
echo $hr "test kpush/kpop(default)" $hr
./k2htouch z.k2 kpush temochiz num1 1
./k2htouch z.k2 kpush temochiz num2 2
./k2htouch z.k2 kpush temochiz num3 3
./k2htouch z.k2 kpush temochiz num2 22
./k2htouch z.k2 kpop temochiz
./k2htouch z.k2 kpop temochiz
./k2htouch z.k2 kpop temochiz
# nodata
./k2htouch z.k2 kpop temochiz
echo -e "\e[31m// result = 1 22 3 (null)? //\e[m"

echo $hr "test kpush/kpop(fifo)" $hr
./k2htouch z.k2 kpush temochiz num1 1 -fifo
./k2htouch z.k2 kpush temochiz num2 2 -fifo
./k2htouch z.k2 kpush temochiz num3 3 -fifo
./k2htouch z.k2 kpop temochiz
./k2htouch z.k2 kpop temochiz
./k2htouch z.k2 kpop temochiz
echo -e "\e[31m// result = 1 2 3 (null)? //\e[m"

echo $hr "test kpush/kpop(lifo)" $hr
./k2htouch z.k2 kpush temochiz num1 1 -lifo
./k2htouch z.k2 kpush temochiz num2 2 -lifo
./k2htouch z.k2 kpush temochiz num3 3 -lifo
./k2htouch z.k2 kpop temochiz
./k2htouch z.k2 kpop temochiz
./k2htouch z.k2 kpop temochiz
echo -e "\e[31m// result = 3 2 1 (null)? //\e[m"

echo $hr "test kpush and normal pop" $hr
./k2htouch z.k2 kpush temochiz num1 1
./k2htouch z.k2 kpush temochiz num2 2
./k2htouch z.k2 kpush temochiz num3 3
./k2htouch z.k2 kpop temochiz
./k2htouch z.k2 kpop temochiz
./k2htouch z.k2 kpop temochiz
echo -e "\e[31m// result = 1 2 3 (null)? //\e[m"

echo $hr "test keyedque clear" $hr
./k2htouch z.k2 kpush temochiz num1 1
./k2htouch z.k2 kpush temochiz num2 2
./k2htouch z.k2 kpush temochiz num3 3
./k2htouch z.k2 kclear temochiz
./k2htouch z.k2 list

echo ""
echo -e "\e[31m// Result = Only Marker? //\e[m"

echo $hr "test dtor mode" $hr
if [ -e z.k2.tr ]; then
	rm z.k2.tr
fi
./k2htouch z.k2 dtor on -conf /home/temochiz/dtor/ini.slave.ini
./k2htouch z.k2 set k2htouchdtor1 key1value
./k2htouch z.k2 dtor off
./k2htouch z.k2 set k2htouchdtor2 key1value
./k2htouch z.k2 dtor on
./k2htouch z.k2 set k2htouchdtor3 `date +"%Y/%m/%d-%p%I:%M:%S"`
./k2htouch z.k2 delete k2htouchdtor1
./k2htouch z.k2 dtor
./k2htouch z.k2 dtor off
./k2htouch z.k2 dtor
echo ""
echo -e "\e[31m// no error? dtor off? server k2 have a k2htouchdtor3? //\e[m"


echo $hr "test attach error" $hr
chmod 444 z.k2
./k2htouch z.k2 list
echo -e "\e[31m// print error message? //\e[m"

chmod 644 z.k2

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
