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
K2HASHをビルドした後で、動作確認をしてみます。

## 1. ビルド成功

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
Attached paraemters:
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
