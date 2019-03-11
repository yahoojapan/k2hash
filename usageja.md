---
layout: contents
language: ja
title: Usage
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: usage.html
lang_opp_word: To English
prev_url: featureja.html
prev_string: Feature
top_url: indexja.html
top_string: TOP
next_url: buildja.html
next_string: Build
---

# 使い方
K2HASHの実行環境を整え、動作確認する方法を示します。

## 1. 利用環境構築

**K2HASH** をご利用の環境にインストールするには、2つの方法があります。  
ひとつは、[packagecloud.io](https://packagecloud.io/)から **K2HASH** のパッケージをダウンロードし、インストールする方法です。  
もうひとつは、ご自身で **K2HASH** をソースコードからビルドし、インストールする方法です。  
これらの方法について、以下に説明します。

### パッケージを使ったインストール
**K2HASH** は、誰でも利用できるように[packagecloud.io - AntPickax stable repository](https://packagecloud.io/antpickax/stable/)で[パッケージ](https://packagecloud.io/app/antpickax/stable/search?q=k2hash)を公開しています。  
**K2HASH** のパッケージは、Debianパッケージ、RPMパッケージの形式で公開しています。  
お使いのOSによりインストール方法が異なりますので、以下の手順を確認してインストールしてください。  

#### Debian(Stretch) / Ubuntu(Bionic Beaver)
```
$ sudo apt-get update -y
$ sudo apt-get install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh | sudo bash
$ sudo apt-get install k2hash
```
開発者向けパッケージをインストールする場合は、以下のパッケージをインストールしてください。
```
$ sudo apt-get install k2hash-dev
```

#### Fedora28 / CentOS7.x(6.x)
```
$ sudo yum makecache
$ sudo yum install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.rpm.sh | sudo bash
$ sudo yum install k2hash
```
開発者向けパッケージをインストールする場合は、以下のパッケージをインストールしてください。
```
$ sudo yum install k2hash-devel
```

#### 上記以外のOS
上述したOS以外をお使いの場合は、パッケージが準備されていないため、直接インストールすることはできません。  
この場合には、後述の[ソースコード](https://github.com/yahoojapan/k2hash)からビルドし、インストールするようにしてください。

### ソースコードからビルド・インストール
**K2HASH** を[ソースコード](https://github.com/yahoojapan/k2hash)からビルドし、インストールする方法は、[ビルド](https://k2hash.antpick.ax/buildja.html)を参照してください。

## 2. k2hlinetoolの実行
```
$ cd k2hash/tests
$ ./k2hlinetool -m
```
_メモリタイプで起動します。_

## 3. set/print  
k2hlinetoolを実行すると、対話式のプロンプトが出ています。  
このプロンプトにてコマンドを実行できます。  
以下のようにSet（設定）とprint（表示）を行ってみます。
```
$ ./k2hlinetool -m
-------------------------------------------------------
 K2HASH TOOL
-------------------------------------------------------
On memory mode
Attached parameters:
    Full are mapping:                   true
    Key Index mask count:               8
    Collision Key Index mask count:     4
    Max element count:                  32
-------------------------------------------------------
K2HTOOL> set mykey myvalue
K2HTOOL> print mykey
  +"mykey" => "myvalue"
```

## 4. その他のテストと終了  
k2hlinetoolの対話式のプロンプトにて、helpコマンドを実行することで利用できるコマンドが一覧表示されます。  
テストを行いたいコマンドを実行してください。

終了させるには、"quit"コマンドを実行すると対話形式を終了し、k2hlinetoolが終了します。
