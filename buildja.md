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

# ビルド

この章は3つの部分から構成されています。

* ローカル開発用に**K2HASH**を設定する方法
* ソースコードから**K2HASH**を構築する方法
* **K2HASH**のインストール方法

## 1. 前提条件をインストールする

**K2HASH**は主に、fullockに依存します。依存ライブラリとヘッダファイルは**K2HASH**をビルドするために必要です。依存ライブラリとヘッダファイルをインストールする方法は2つあります。好きなものを選ぶことができます。

* GitHubを使う  
  依存ライブラリのソースコードとヘッダファイルをインストールします。あなたは依存ライブラリとヘッダファイルをビルドしてインストールします。
* packagecloud.ioを使用する  
  依存ライブラリのパッケージとヘッダファイルをインストールします。あなたは依存ライブラリとヘッダファイルをインストールするだけです。ライブラリはすでに構築されています。

### 1.1. GitHubから各依存ライブラリとヘッダファイルをインストールします。

詳細については以下の文書を読んでください。  
* [fullock](https://fullock.antpick.ax/build.html)

### 1.2. packagecloud.ioから各依存ライブラリとヘッダファイルをインストールします。

このセクションでは、packagecloud.ioから各依存ライブラリとヘッダーファイルをインストールする方法を説明します。

**注**：前のセクションでGitHubから依存ライブラリとヘッダーファイルをインストールした場合は、このセクションを読み飛ばしてください。

[K2HASH](https://k2hash.antpick.ax/build.html)をビルドするときには、ひとつのSSL/TLSライブラリーを選択します。[K2HASH](https://k2hash.antpick.ax/build.html)がサポートしているSSL/TLSライブラリとヘッダファイルのパッケージ名は次のようになっています。

| SSL/TLS library | pkg |
|:--|:--|
| [OpenSSL](https://www.openssl.org/) | libssl-dev(deb) / openssl-devel(rpm) |
| [GnuTLS](https://gnutls.org/) (gcrypt) | libgcrypt11-dev(deb) |
| [GnuTLS](https://gnutls.org/) (nettle) | nettle-dev(deb) |
| [Mozilla NSS](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/NSS) | nss-devel(rpm) |

DebianStretchまたはUbuntu（Bionic Beaver）をお使いの場合は、以下の手順に従ってください。SSL/TLSライブラリとヘッダファイルのパッケージ名（libgcrypt11-dev）は、あなたアプリケーションが要求するSSL/TLSライブラリとヘッダファイルのパッケージ名と置き換えても良いです。
```bash
$ sudo apt-get update -y
$ sudo apt-get install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh \
    | sudo bash
$ sudo apt-get install autoconf autotools-dev gcc g++ make gdb libtool pkg-config \
    libyaml-dev libfullock-dev libgcrypt11-dev -y
$ sudo apt-get install git -y
```

Fedora28またはCentOS7.x（6.x）ユーザーの場合は、以下の手順に従ってください。SSL/TLSライブラリとヘッダファイルのパッケージ名（nss-devel）は、あなたアプリケーションが要求するSSL/TLSライブラリとヘッダファイルのパッケージ名と置き換えても良いです。
```bash
$ sudo yum makecache
$ sudo yum install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.rpm.sh \
    | sudo bash
$ sudo yum install autoconf automake gcc gcc-c++ gdb make libtool pkgconfig \
    libyaml-devel libfullock-devel nss-devel -y
$ sudo yum install git -y
```

## 2. GitHubからソースコードを複製する

GitHubから**k2hash**のソースコードをダウンロードしてください。
```bash
$ git clone https://github.com/yahoojapan/k2hash.git
```

## 3. ビルドしてインストールする

以下の手順に従って**K2HASH**をビルドしてインストールしてください。 [GNU Automake](https://www.gnu.org/software/automake/)を使って**K2HASH**を構築します。

**K2HASH**は、1つのSSL/TLSライブラリが必要であることを考慮する必要があります。[K2HASH](https://k2hash.antpick.ax/build.html)ビルドオプションは、**CHMPX**のビルドオプションに影響します。 表1は、可能な構成オプションを示しています。

表1. 可能な構成オプション:

| SSL/TLS library | K2HASH configure options | CHMPX configure options |
|:--|:--|:--|
| [OpenSSL](https://www.openssl.org/) | ./configure \-\-prefix=/usr \-\-with-openssl | ./configure \-\-prefix=/usr \-\-with-openssl |
| [GnuTLS](https://gnutls.org/) (gcrypt)| ./configure \-\-prefix=/usr \-\-with-gcrypt | ./configure \-\-prefix=/usr \-\-with-gnutls |
| [GnuTLS](https://gnutls.org/) (nettle)| ./configure \-\-prefix=/usr \-\-with-nettle | ./configure \-\-prefix=/usr \-\-with-gnutls |
| [Mozilla NSS](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/NSS) | ./configure \-\-prefix=/usr \-\-with-nss | ./configure \-\-prefix=/usr \-\-with-nss |

DebianStretchまたはUbuntu（Bionic Beaver）をお使いの場合は、以下の手順に従ってください。
```bash
$ cd k2hash
$ sh autogen.sh
$ ./configure --prefix=/usr --with-gcrypt
$ make
$ sudo make install
```

Fedora28またはCentOS7.x（6.x）ユーザーの場合は、以下の手順に従ってください。
```bash
$ cd k2hash
$ sh autogen.sh
$ ./configure --prefix=/usr --with-nss
$ make
$ sudo make install
```

**K2HASH**を正常にインストールすると、**k2himport**のヘルプテキストが表示されます。
```bash
$ k2himport -h

usage: 

k2himport inputfile outputfile [-mdbm]
k2himport -h

```