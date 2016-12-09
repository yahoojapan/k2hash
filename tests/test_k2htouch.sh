#/bin/sh
#
# K2HASH
#
# Copyright 2016 Yahoo! JAPAN corporation.
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
# AUTHOR:   Tetsuya Mochizuki
# CREATE:   Wed Jan 27 2016
# REVISION:
#
rm my.k2
./k2htouch my.k2 createmini
echo -e "\e[31m// mtime //\e[m"
echo "----------------------------------"
echo "test print mtime mode"
echo "- - - - - - - - -"
./k2htouch my.k2 mtime
echo "-----------------"
echo "test mtime on"
echo "- - - - - - - - -"
./k2htouch my.k2 mtime on
./k2htouch my.k2 mtime 
echo "-----------------"
echo "test mtime off"
echo "- - - - - - - - -"
./k2htouch my.k2 mtime off
./k2htouch my.k2 mtime 
echo -e "\e[31m// history //\e[m"
echo "----------------------------------"
echo "test print history mode"
echo "- - - - - - - - -"
./k2htouch my.k2 history
echo "-----------------"
echo "test history on"
echo "- - - - - - - - -"
./k2htouch my.k2 history on
./k2htouch my.k2 history
echo "-----------------"
echo "test history off"
echo "- - - - - - - - -"
./k2htouch my.k2 history off
./k2htouch my.k2 history
echo -e "\e[31m// expire //\e[m"
echo "----------------------------------"
echo "test print expire mode"
echo "- - - - - - - - -"
./k2htouch my.k2 expire
echo "-----------------"
echo "test history on"
echo "- - - - - - - - -"
./k2htouch my.k2 expire 10
./k2htouch my.k2 expire
echo "-----------------"
echo "test history off"
echo "- - - - - - - - -"
./k2htouch my.k2 expire off
./k2htouch my.k2 expire
echo -e "\e[31m// mtime/his off  //\e[m"
./k2htouch my.k2 set temochiz temo
./k2htouch my.k2 info
echo -e "\e[31m// mtime on  //\e[m"
./k2htouch my.k2 mtime on
./k2htouch my.k2 set temochiz temoold
./k2htouch my.k2 getattr temochiz
echo -e "\e[31m// history off  //\e[m"
./k2htouch my.k2 list | grep temochiz
echo -e "\e[31m// history on  //\e[m"
./k2htouch my.k2 history on
./k2htouch my.k2 set temochiz temonew
./k2htouch my.k2 get tetmochiz
echo ""
./k2htouch my.k2 list | grep temochiz
echo -e "\e[31m// expire  //\e[m"
./k2htouch my.k2 expire 5
./k2htouch my.k2 set temochiz expire5
echo "get"
./k2htouch my.k2 get temochiz
echo ""
echo "wait 3sec. and get"
sleep 3
./k2htouch my.k2 get temochiz
echo ""
echo "wait 3sec. and get"
sleep 3
./k2htouch my.k2 get temochiz
echo ""
echo "get end."
#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
