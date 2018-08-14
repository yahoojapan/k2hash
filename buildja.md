---
layout: contents
language: ja
title: Build
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: build.html
lang_opp_word: To English
prev_url: usageja.html
prev_string: Usage
top_url: indexja.html
top_string: TOP
next_url: developerja.html
next_string: Developer
---

# ビルド方法
K2HASHをビルドする方法を説明します。

## 1. 事前環境
- Debian / Ubuntu
```
$ sudo aptitude update
$ sudo aptitude install git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config libssl-dev
```
- Fedora / CentOS
```
$ sudo yum install git autoconf automake gcc libstdc++-devel gcc-c++ make libtool openssl-devel
```

## 2. ビルド、インストール：FULLOCK
```
$ git clone https://github.com/yahoojapan/fullock.git
$ cd fullock
$ ./autogen.sh
Debian / Ubuntuの場合
$ ./configure --prefix=/usr
Fedora / CentOSの場合
$ ./configure --prefix=/usr --libdir=/usr/lib64
$ make
$ sudo make install
$ cd ..
```

## 3. clone
```
$ git clone git@github.com:yahoojapan/k2hash.gif ; cd k2hash
```

## 4. ビルド、インストール：K2HASH
```
$ ./autogen.sh
Debian / Ubuntuの場合
$ ./configure --prefix=/usr
Fedora / CentOSの場合
$ ./configure --prefix=/usr --libdir=/usr/lib64
$ make
$ sudo make install
```
### 暗号化ライブラリの変更
- OpenSSLを使う場合  
"--with-openssl"をconfigureコマンドのオプションとして指定してください。暗号化ライブラリの指定オプションが省略されている場合のデフォルトはOpenSSLになります。
- NSS(mozilla)を使う場合  
"--with-nss"をconfigureコマンドのオプションとして指定してください。
- Nettle(gnutls)を使う場合  
"--with-nettle"をconfigureコマンドのオプションとして指定してください。
- Gcrypt(gnutls)を使う場合  
"--with-gcrypt"をconfigureコマンドのオプションとして指定してください。


