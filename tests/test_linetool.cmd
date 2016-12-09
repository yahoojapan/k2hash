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
# AUTHOR:   Takeshi Nakatani
# CREATE:   Wed Jan 20 2016
# REVISION:
#

###### help
h

###### set normal
set testkey testvalue
p testkey

###### set with pass
set testkey testvalue pass=testpass
p testkey
p testkey pass=testpass

###### set with expire
set testkey testvalue expire=1
p testkey
sleep 2
p testkey

###### set with pass & expire
set testkey testvalue pass=testpass expire=1
p testkey
p testkey pass=testpass
sleep 2
p testkey
p testkey pass=testpass

###### cleanup key
rm testkey
l

###### set subkey
set parentkey parentvalue
setsub parentkey childkey childvalue
p parentkey
p childkey
l

###### remove subkey
rmsub parentkey childkey
p parentkey
p childkey
l

###### remove all
setsub parentkey childkey childvalue
rm parentkey all
p childkey
p parentkey
l

###### fill test
fill testkey_ fillvalue 10
l
rm testkey_-1
rm testkey_-2
rm testkey_-0
rm testkey_-3
rm testkey_-4
rm testkey_-5
rm testkey_-6
rm testkey_-7
rm testkey_-8
rm testkey_-9

###### fillsub test
set parentkey parentvalue
fillsub parentkey testsub_ fillsubvalue 10
l
rm parentkey all
l

###### attribute test
set attrkey attrvalue
addattr attrkey testattr testvalue
printattr attrkey
rm attrkey
l

##### rename key
set testkey testvalue
p testkey
p testkey_renamed
ren testkey testkey_renamed
p testkey
p testkey_renamed
rm testkey_renamed

###### queue test(normal)
que push fifo quevalue1
que push fifo quevalue2
que push fifo quevalue3
que push fifo quevalue4
que push fifo quevalue5
que count
que empty
que read fifo 0
que read fifo 1
que read fifo 2
que read fifo 3
que read fifo 4
que read fifo 5
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo

###### queue test(with pass)
que push fifo quevalue1 pass=testpass
que push fifo quevalue2 pass=testpass
que push fifo quevalue3 pass=testpass
que push fifo quevalue4 pass=testpass
que push fifo quevalue5 pass=testpass
que count
que empty
que read fifo 0
que read fifo 1
que read fifo 2
que read fifo 3
que read fifo 4
que read fifo 5
que read fifo 0 pass=testpass
que read fifo 1 pass=testpass
que read fifo 2 pass=testpass
que read fifo 3 pass=testpass
que read fifo 4 pass=testpass
que read fifo 5 pass=testpass
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que push fifo quevalue1 pass=testpass
que push fifo quevalue2 pass=testpass
que push fifo quevalue3 pass=testpass
que push fifo quevalue4 pass=testpass
que push fifo quevalue5 pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass

###### queue test(with expire)
que push fifo quevalue1 expire=1
que push fifo quevalue2 expire=1
que push fifo quevalue3 expire=1
que push fifo quevalue4 expire=1
que push fifo quevalue5 expire=1
que count
que empty
que read fifo 0
que read fifo 1
que read fifo 2
que read fifo 3
que read fifo 4
que read fifo 5
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que push fifo quevalue1 expire=1
que push fifo quevalue2 expire=1
que push fifo quevalue3 expire=1
que push fifo quevalue4 expire=1
que push fifo quevalue5 expire=1
sleep 2
que count
que empty
que read fifo 0
que read fifo 1
que read fifo 2
que read fifo 3
que read fifo 4
que read fifo 5
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo

###### queue test(with pass & expire)
que push fifo quevalue1 pass=testpass expire=1
que push fifo quevalue2 pass=testpass expire=1
que push fifo quevalue3 pass=testpass expire=1
que push fifo quevalue4 pass=testpass expire=1
que push fifo quevalue5 pass=testpass expire=1
que count
que empty
que read fifo 0
que read fifo 1
que read fifo 2
que read fifo 3
que read fifo 4
que read fifo 5
que read fifo 0 pass=testpass
que read fifo 1 pass=testpass
que read fifo 2 pass=testpass
que read fifo 3 pass=testpass
que read fifo 4 pass=testpass
que read fifo 5 pass=testpass
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que push fifo quevalue1 pass=testpass expire=1
que push fifo quevalue2 pass=testpass expire=1
que push fifo quevalue3 pass=testpass expire=1
que push fifo quevalue4 pass=testpass expire=1
que push fifo quevalue5 pass=testpass expire=1
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass

que push fifo quevalue1 pass=testpass expire=1
que push fifo quevalue2 pass=testpass expire=1
que push fifo quevalue3 pass=testpass expire=1
que push fifo quevalue4 pass=testpass expire=1
que push fifo quevalue5 pass=testpass expire=1
sleep 2
que count
que empty
que read fifo 0
que read fifo 1
que read fifo 2
que read fifo 3
que read fifo 4
que read fifo 5
que read fifo 0 pass=testpass
que read fifo 1 pass=testpass
que read fifo 2 pass=testpass
que read fifo 3 pass=testpass
que read fifo 4 pass=testpass
que read fifo 5 pass=testpass
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que pop fifo
que push fifo quevalue1 pass=testpass expire=1
que push fifo quevalue2 pass=testpass expire=1
que push fifo quevalue3 pass=testpass expire=1
que push fifo quevalue4 pass=testpass expire=1
que push fifo quevalue5 pass=testpass expire=1
sleep 2
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass
que pop fifo pass=testpass

###### keyqueue test(normal)
kque push fifo kquekey1 kquevalue1
kque push fifo kquekey2 kquevalue2
kque push fifo kquekey3 kquevalue3
kque push fifo kquekey4 kquevalue4
kque push fifo kquekey5 kquevalue5
kque count
kque empty
kque read fifo 0
kque read fifo 1
kque read fifo 2
kque read fifo 3
kque read fifo 4
kque read fifo 5
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo

###### keyqueue test(with pass)
kque push fifo kquekey1 kquevalue1 pass=testpass
kque push fifo kquekey2 kquevalue2 pass=testpass
kque push fifo kquekey3 kquevalue3 pass=testpass
kque push fifo kquekey4 kquevalue4 pass=testpass
kque push fifo kquekey5 kquevalue5 pass=testpass
kque count
kque empty
kque read fifo 0
kque read fifo 1
kque read fifo 2
kque read fifo 3
kque read fifo 4
kque read fifo 5
kque read fifo 0 pass=testpass
kque read fifo 1 pass=testpass
kque read fifo 2 pass=testpass
kque read fifo 3 pass=testpass
kque read fifo 4 pass=testpass
kque read fifo 5 pass=testpass
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque push fifo kquekey1 kquevalue1 pass=testpass
kque push fifo kquekey2 kquevalue2 pass=testpass
kque push fifo kquekey3 kquevalue3 pass=testpass
kque push fifo kquekey4 kquevalue4 pass=testpass
kque push fifo kquekey5 kquevalue5 pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass

###### keyqueue test(with expire)
kque push fifo kquekey1 kquevalue1 expire=1
kque push fifo kquekey2 kquevalue2 expire=1
kque push fifo kquekey3 kquevalue3 expire=1
kque push fifo kquekey4 kquevalue4 expire=1
kque push fifo kquekey5 kquevalue5 expire=1
kque count
kque empty
kque read fifo 0
kque read fifo 1
kque read fifo 2
kque read fifo 3
kque read fifo 4
kque read fifo 5
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque push fifo kquekey1 kquevalue1 expire=1
kque push fifo kquekey2 kquevalue2 expire=1
kque push fifo kquekey3 kquevalue3 expire=1
kque push fifo kquekey4 kquevalue4 expire=1
kque push fifo kquekey5 kquevalue5 expire=1
sleep 2
kque count
kque empty
kque read fifo 0
kque read fifo 1
kque read fifo 2
kque read fifo 3
kque read fifo 4
kque read fifo 5
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo

###### keyqueue test(with pass & expire)
kque push fifo kquekey1 kquevalue1 pass=testpass expire=1
kque push fifo kquekey2 kquevalue2 pass=testpass expire=1
kque push fifo kquekey3 kquevalue3 pass=testpass expire=1
kque push fifo kquekey4 kquevalue4 pass=testpass expire=1
kque push fifo kquekey5 kquevalue5 pass=testpass expire=1
kque count
kque empty
kque read fifo 0
kque read fifo 1
kque read fifo 2
kque read fifo 3
kque read fifo 4
kque read fifo 5
kque read fifo 0 pass=testpass
kque read fifo 1 pass=testpass
kque read fifo 2 pass=testpass
kque read fifo 3 pass=testpass
kque read fifo 4 pass=testpass
kque read fifo 5 pass=testpass
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque push fifo kquekey1 kquevalue1 pass=testpass expire=1
kque push fifo kquekey2 kquevalue2 pass=testpass expire=1
kque push fifo kquekey3 kquevalue3 pass=testpass expire=1
kque push fifo kquekey4 kquevalue4 pass=testpass expire=1
kque push fifo kquekey5 kquevalue5 pass=testpass expire=1
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass

kque push fifo kquekey1 kquevalue1 pass=testpass expire=1
kque push fifo kquekey2 kquevalue2 pass=testpass expire=1
kque push fifo kquekey3 kquevalue3 pass=testpass expire=1
kque push fifo kquekey4 kquevalue4 pass=testpass expire=1
kque push fifo kquekey5 kquevalue5 pass=testpass expire=1
sleep 2
kque count
kque empty
kque read fifo 0
kque read fifo 1
kque read fifo 2
kque read fifo 3
kque read fifo 4
kque read fifo 5
kque read fifo 0 pass=testpass
kque read fifo 1 pass=testpass
kque read fifo 2 pass=testpass
kque read fifo 3 pass=testpass
kque read fifo 4 pass=testpass
kque read fifo 5 pass=testpass
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque pop fifo
kque push fifo kquekey1 kquevalue1 pass=testpass expire=1
kque push fifo kquekey2 kquevalue2 pass=testpass expire=1
kque push fifo kquekey3 kquevalue3 pass=testpass expire=1
kque push fifo kquekey4 kquevalue4 pass=testpass expire=1
kque push fifo kquekey5 kquevalue5 pass=testpass expire=1
sleep 2
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass
kque pop fifo pass=testpass

###### builtin attribute test
ba mtime history expire=10 enc pass=test_encrypt_keys
set testkey testvalue
p testkey
rm testkey
clanallattr
cleanallattr
set testkey testvalue
p testkey
printattr testkey
rm testkey

###### exit
exit

#
# VIM modelines
#
# vim:set ts=4 fenc=utf-8:
#
