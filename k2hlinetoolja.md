---
layout: contents
language: ja
title: Tools
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: tools.html
lang_opp_word: To English
prev_url: toolsja.html
prev_string: Tools
top_url: indexja.html
top_string: TOP
next_url: 
next_string: 
---

# k2hlinetool
K2HASHのオンメモリ/ファイルの作成、K2HASHライブラリのほぼ全ての機能を対話形式でテスト実行できるツールです。 コマンドファイルをロードして自動実行もできます。以下では大きく二つの内容を説明します。

* k2hlinetoolの起動オプション
  * k2hlinetoolの起動オプションを説明します。

* k2hlinetoolのインタプリタ機能
  * k2hlinetoolのインタプリタ機能を説明します。

## k2hlinetoolの起動オプション

k2hlinetoolの起動オプションを説明します。本ツールは、対話形式のツールであり、起動後はコマンド入力を行うプロンプトが表示されます。このプロンプトにて、ヘルプ（help）を入力することで、コマンドのヘルプを表示することができます。

| オプション | 説明 |
| --- | --- |
| -h | ヘルプを表示します。|
| -f <filename> | メモリマップドファイルを使用する場合にファイルパスを指定します。|
| -t <filename> | メモリマップドファイルを使用する場合にファイルパスを指定します。このオプションの場合には、ライブラリがファイルからデタッチしたタイミングでファイルが削除されます。（一時ファイルとしての利用用途）|
| -m | オンメモリのみだけの場合に指定します。|
| -mask <bit count> | MASK値をビット数で指定します。8以上を指定してください。この値は初期化時のサイズであり、動的にMASK値は変更（増加）するため、長期動作においては意味が無く、最初からキー数が多い場合などには大きめに設定しておくことをお勧めします。なお、初期化済のファイルを指定している場合には、このオプションは無視されます。|
| -cmask <bit count> | サブHASHテーブルのCMASK値のビット数です。この値は初期化後は変更できません。4～5程度を指定することをお勧めします。なお、初期化済のファイルを指定している場合には、このオプションは無視されます。|
| -elementcnt <count> | HASH値の衝突（サブHASHテーブルでの衝突）におけるエレメントリストの最大数を指定します。この値を超過したときにHASHテーブル（MASK値）が増加します。1024程度を指定することをお勧めします。なお、初期化済のファイルを指定している場合には、このオプションは無視されます。|
| -pagesize <number> | ページサイズを指定します。キー（Key）、値（Value）、サブキーリストのデータを保存する単位ブロックのサイズです。デフォルトは128バイトになっています。大きい値を保存するなどの場合には、1024バイトなど大きめに設定しても良いかもしれません。小さくすると利用率（ブロックに対して保存されているデータのサイズ）はよくなりますが、管理領域の割合（24バイトが管理領域のバイト数）が大きくなります。この値を適正にするために、本ツールを使い、ステータス情報などを見てチューニングを行ってください。なお、初期化済のファイルを指定している場合には、このオプションは無視されます。|
| -fullmap | メモリマップドファイルを指定した場合に、ファイル全体をマップすることを指定します。本オプションの指定が無い場合には、部分マッピングとなります。|
| -ext <library path> | HASH関数のDSOモジュールのロードをします。ファイルパスを指定します。|
| -ro | 読み出し専用でK2HASHデータファイルにアタッチします。|
| -init | メモリマップドファイルの初期化のみ行います。|
| -lap | 起動後のコマンド単位でのラップタイムを表示するようにします。|
| -capi | ツール内部で利用しているK2HASHライブラリのC++ I/Fを使わず、C I/Fを利用するようにします。|
| -g <debug level> | メッセージ出力レベルの指定をします。ERR、WAN、INFOのいずれかを指定します。メッセージ出力レベルは、SIGUSR1でBumpupすることもできますので、本ツール起動後にプロセスIDを指定して、killして変更することが可能です。また環境変数K2HDBGMODEに、SILENT、ERR、WAN、INFOの指定をして、本ツールを起動し、メッセージ出力レベルを指定することもできます。|
| -glog <file path> | メッセージの出力先ファイルを指定します。デフォルトでは、stderrとなっています。また、環境変数K2HDBGFILEで出力ファイルを指定することもできます。|
| -his <count> | 起動後のコマンド履歴の最大数を指定します。デフォルトは1000です。|
| -libversion | libk2hash.soのバージョンおよびクレジットを表示します。|
| -run <file path> | コマンドファイル（履歴含む）を指定します。指定されたコマンドファイル（履歴含む）の内容を起動後に自動的に実行します。|

## k2hlinetoolのインタプリタ機能

k2hlinetoolのインタプリタ機能を説明します。本ツールは、対話形式のツールであり、起動後はコマンド入力を行うプロンプトが表示されます。このプロンプトにて、ヘルプ（help）を入力することで、コマンドのヘルプを表示することができます。使い方などは、ヘルプを参照してください。コマンドのオプションは以下にまとめておきます。

| queue(que) [prefix] dump <fifo | lifo> | K2HASHでサポートするキューをダンプします。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。|
| queue(que) [prefix] remove(rm) <fifo | lifo> <count> [c] [pass=...] | K2HASHでサポートするキューから指定数（count）分のデータを削除ます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。削除されたデータは表示されません。最後のパラメータ"c（confirm）"をつけた場合には、削除する前に対話形式で、確認がなされます。対話形式には"y(yes)"、"n(no)"、"b(break)"で回答してください。キューが暗号化されている場合にはパスフレーズを指定してください。| 
| keyqueue(kque) [prefix] empty | K2HASHでサポートするキューが空であるか確認します。|
| keyqueue(kque) [prefix] count | K2HASHでサポートするキューに蓄積されているデータ数を返します。|
| keyqueue(kque) [prefix] read <fifo | lifo> <pos> [pass=...] | K2HASHでサポートするキューからデータ（keyとvalue）をコピーます。prefixはキューにて使用されるキー名のプレフィック スです。FIFO/LIFOを指定してください。キューが暗号化されている場合にはパスフレーズを指定してください。|
| keyqueue(kque) [prefix] push <fifo | lifo> <key> <value> [pass=....] [expire=sec] | K2HASHでサポートするキューへデータ（key/value）を蓄積（プッシュ）します。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。指定されたkey/valueはキューだけではなく、K2HASHファイルにkey/valueとして書き込まれます。キューを暗号化する場合はパスフレーズを指定してください。またExpire時間を指定する場合には、expireを秒で指定してください。|
| keyqueue(kque) [prefix] pop <fifo | lifo> [pass=...] | K2HASHでサポートするキューからデータ（keyとvalue）を取り出し（ポップ）ます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。取り出されたkey/valueはK2HASHファイルからも削除されます。キューが暗号化されている場合にはパスフレーズを指定してください。|
| keyqueue(kque) [prefix] dump <fifo | lifo> | K2HASHでサポートするキューをダンプします。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。|
| keyqueue(kque) [prefix] remove(rm) <fifo | lifo> <count> [c] [pass=...] | K2HASHでサポートするキューから指定数（count）分のデータを削除ます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。削除されたデータは表示されません。キューから削除されるkey/valueはK2HASHファイルからも削除されます。最後のパラメータ"c（confirm）"をつけた場合には、削除する前に対話形式で、確認がなされます。対話形式には"y(yes)"、"n(no)"、"b(break)"で回答してください。キューが暗号化されている場合にはパスフレーズを指定してください。|
| builtinattr(ba) [mtime] [history] [expire=second] [enc] [pass=file path] | Builtin属性の設定をします。有効にする機能を指定します。expireには秒を指定してください。passにパスフレーズファイルを指定してください。暗号化機能を有効にする場合にはencを指定します。|
| loadpluginattr(lpa) filepath | Plugin属性を利用する場合に、シェアードライブラリ（DSOモジュール）を指定してロードします。|
| addpassphrese(app) <pass phrase> [default] | Builtin属性で利用するパスフレーズを1つ追加します。同時に暗号化機能を有効にし、このパスフレーズを暗号化で利用する場合いは、defaultを指定してください。| 
cleanallattr(caa) | 全ての属性（Attribute）設定を破棄し、初期状態にします。|
| shell |  shellに抜けます。|
| echo <string>... | UNIXのechoコマンドと同じ動作をします。指定されたパラメータを表示します。|
| sleep <second> | UNIXのsleepコマンドと同じ動作をします。指定された秒数のsleepを行い、コマンドを一時停止させます。|

