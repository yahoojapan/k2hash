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
# Build

This chapter consists of three parts:

* how to set up **K2HASH** for local development
* how to build **K2HASH** from the source code
* how to install **K2HASH**

## 1. Install prerequisites

**K2HASH** primarily depends on [FULLOCK](https://fullock.antpick.ax/index.html). [FULLOCK](https://fullock.antpick.ax/index.html) and its header files are required to build **K2HASH**. We provide two ways to install them. You can select your favorite one.

* Use [GitHub](https://github.com/yahoojapan)  
  Install **fullock** source code and the header files. You will **build** them and install them.
* Use [packagecloud.io](https://packagecloud.io/antpickax/stable)  
  Install packages of **fullock** and its header files. You just install them. Libraries are already built.

### 1.1. Install fullock and its header files from GitHub

Read the following documents for details:  
* [fullock](https://fullock.antpick.ax/build.html)

### 1.2. Install fullock and its header files from packagecloud.io

This section instructs how to install fullock and the header files from [packagecloud.io - AntPickax stable repository](https://packagecloud.io/antpickax/stable). 

**Note**: Skip reading this section if you have installed each dependent library and the header files from [GitHub](https://github.com/yahoojapan) in the previous section.

[K2HASH](https://k2hash.antpick.ax/) requires a SSL/TLS library and header files. Table1 shows Development packages to build the [K2HASH](https://k2hash.antpick.ax/).

Table1. Development packages to build the [K2HASH](https://k2hash.antpick.ax/):

| SSL/TLS library | pkg |
|:--|:--|
| [OpenSSL](https://www.openssl.org/) | libssl-dev(deb) / openssl-devel(rpm) |
| [GnuTLS](https://gnutls.org/) (gcrypt) | libgcrypt11-dev(deb) |
| [GnuTLS](https://gnutls.org/) (nettle) | nettle-dev(deb) |
| [Mozilla NSS](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/NSS) | nss-devel(rpm) |

For DebianStretch or Ubuntu(Bionic Beaver) users, follow the steps below. You can replace `libgcrypt11-dev` with the other SSL/TLS package your application requires:
```bash
$ sudo apt-get update -y
$ sudo apt-get install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh \
    | sudo bash
$ sudo apt-get install autoconf autotools-dev gcc g++ make gdb libtool pkg-config \
    libyaml-dev libfullock-dev libgcrypt11-dev -y
$ sudo apt-get install git -y
```

For Fedora28 or CentOS7.x(6.x) users, follow the steps below. You can replace `nss-devel` with the other SSL/TLS package your application requires:
```bash
$ sudo yum makecache
$ sudo yum install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.rpm.sh \
    | sudo bash
$ sudo yum install autoconf automake gcc gcc-c++ gdb make libtool pkgconfig \
    libyaml-devel libfullock-devel nss-devel -y
$ sudo yum install git -y
```

## 2. Clone the source code from GitHub

Download the **K2HASH**'s source code from [GitHub](https://github.com/yahoojapan/k2hash).
```bash
$ git clone https://github.com/yahoojapan/k2hash.git
```

## 3. Build and install

Just follow the steps below to build **K2HASH** and install it. We use [GNU Automake](https://www.gnu.org/software/automake/) to build **K2HASH**.

You need consider **K2HASH** also requires one SSL/TLS library. This means the [K2HASH](https://k2hash.antpick.ax/index.html) build option affects the [CHMPX](https://chmpx.antpick.ax/index.html) build option. Table1 shows possible configure options.

Table1. possible configure option:

| SSL/TLS library | K2HASH configure options | CHMPX configure options |
|:--|:--|:--|
| [OpenSSL](https://www.openssl.org/) | ./configure --prefix=/usr --with-openssl | ./configure --prefix=/usr --with-openssl |
| [GnuTLS](https://gnutls.org/) (gcrypt) | ./configure \-\-prefix=/usr \-\-with-gcrypt | ./configure \-\-prefix=/usr \-\-with-gnutls |
| [GnuTLS](https://gnutls.org/) (nettle) | ./configure \-\-prefix=/usr \-\-with-nettle | ./configure \-\-prefix=/usr \-\-with-gnutls |
| [Mozilla NSS](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/NSS) | ./configure \-\-prefix=/usr \-\-with-nss | ./configure \-\-prefix=/usr \-\-with-nss |

For DebianStretch or Ubuntu(Bionic Beaver) users, follow the steps below:
```bash
$ cd k2hash
$ sh autogen.sh
$ ./configure --prefix=/usr --with-gcrypt
$ make
$ sudo make install
```

For Fedora28 or CentOS7.x(6.x) users, follow the steps below:
```bash
$ cd k2hash
$ sh autogen.sh
$ ./configure --prefix=/usr --with-nss
$ make
$ sudo make install
```

After successfully installing **K2HASH**, you will see the **k2himport** help text:
```bash
$ k2himport -h

usage: 

k2himport inputfile outputfile [-mdbm]
k2himport -h
```
