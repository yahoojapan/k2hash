---
layout: contents
language: en-us
title: Build
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: buildja.html
lang_opp_word: To Japanese
prev_url: usage.html
prev_string: Usage
top_url: index.html
top_string: TOP
next_url: developer.html
next_string: Developer
---

# Building
The build method for K2HASH is explained below.

## 1. Install prerequisites before compiling
- Debian / Ubuntu
```
$ sudo aptitude update
$ sudo aptitude install git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config libssl-dev
```
- Fedora / CentOS
```
$ sudo yum install git-core gcc-c++ make libtool openssl-devel
```
## 2. Building and installing FULLOCK before compiling K2HASH
```
$ git clone https://github.com/yahoojapan/fullock.git
$ cd fullock
$ ./autogen.sh
If you use Debian / Ubuntu, then
$ ./configure --prefix=/usr
If you use Fedora / CentOS, then
$ ./configure --prefix=/usr --libdir=/usr/lib64
$ make
$ sudo make install
$ cd ..
```

## 3. Clone source codes from Github
```
$ git clone git@github.com:yahoojapan/k2hash.gif ; cd k2hash
```

## 4. Building and installing K2HASH
```
$ ./autogen.sh
If you use Debian / Ubuntu, then
$ ./configure --prefix=/usr
If you use Fedora / CentOS, then
$ ./configure --prefix=/usr --libdir=/usr/lib64
$ make
$ sudo make install
```
### Switch Crypt library
- Linking OpenSSL  
Specify "--with-openssl" for configure command option, OpenSSL is default if you do not specify any option for crypt library.
- Linking NSS(mozilla)  
Specify "--with-nss" for configure command option.
- Linking Nettle(gnutls)  
Specify "--with-nettle" for configure command option.
- Linking Gcrypt(gnutls)  
Specify "--with-gcrypt" for configure command option.

