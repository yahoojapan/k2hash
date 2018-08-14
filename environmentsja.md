---
layout: contents
language: ja
title: Environments
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: environments.html
lang_opp_word: To English
prev_url: developerja.html
prev_string: Developer
top_url: indexja.html
top_string: TOP
next_url: toolsja.html
next_string: Tools
---

# 環境変数
K2HASHライブラリは、以下の環境変数を読み込みます。
- K2HDBGMODE  
  K2HASHライブラリのメッセージ出力レベルを指定します。 指定できる値は、silent / err / wan / info です。レベルに応じたメッセージが出力されます。
- K2HDBGFILE  
  K2HASHライブラリのメッセージを指定したファイルに出力します。
- K2HATTR_MTIME  
  ONを設定することで、組み込み属性の更新時刻を有効にします。
- K2HATTR_HISTORY  
  ONを設定することで、組み込み属性の履歴機能を有効にします。
- K2HATTR_EXPIRE_SEC  
  組み込み属性の有効時間を指定した値（秒）に設定します。
- K2HATTR_DEFENC  
  ONを設定することで、組み込み属性の暗号化機能を有効にします。
- K2HATTR_ENCFILE  
  組み込み属性の暗号化機能が有効であれば、 暗号化に利用する共通暗号化鍵（パスフレーズ）のファイルを指定できます。
<br />
これらの設定は、C、C++言語のAPIで設定された場合、APIの設定が優先されます。
