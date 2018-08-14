---
layout: contents
language: ja
title: Overview
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: home.html
lang_opp_word: To English
prev_url: 
prev_string: 
top_url: indexja.html
top_string: TOP
next_url: featureja.html
next_string: Feature
---

# K2HASH
**K2HASH** (NoSQL Key Value Store(KVS) library)は、いくつかの特徴を備えた独自の **KVS** (key-value store)です。  
**K2HASH** は、キー（Key）に対して、一意のHash値を割り当て（計算）、そのHash値に対応したデータ格納場所へ、値（Value）を保存、読み出し、削除を行うライブラリです。

## 背景
単純で一時的なデータ保管、またウェブサービスのバックエンドシステムで利用される大規模なデータの取り扱いにおいて、KVS（NoSQL）は有効に機能します。  
私たちは、これらの小規模から大規模なデータの取り扱いができる共通のKVSライブラリおよびシステムを必要としました。  
特に、共通のライブラリの提供を行い、このライブラリにより派生システムが構築できることにより、開発、保守、運用を楽にしたいと考えました。  
また、多様な要求（対象データ量、速度など）を共通に取り扱えるKVSライブラリがあるとよいと考えました。

この背景から、K2HASHは、小規模から大規模までのデータを取り扱うことができ、高速であり、また派生システムのためのいくつかの特徴を持って作成されたKVSライブラリです。
