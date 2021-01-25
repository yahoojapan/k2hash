---
layout: contents
language: ja
title: Tools
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: tools.html
lang_opp_word: To English
prev_url:
prev_string:
top_url: toolsja.html
top_string: Tools
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
| -f \<filename\> | メモリマップドファイルを使用する場合にファイルパスを指定します。|
| -t \<filename\> | メモリマップドファイルを使用する場合にファイルパスを指定します。このオプションの場合には、ライブラリがファイルからデタッチしたタイミングでファイルが削除されます。（一時ファイルとしての利用用途）|
| -m | オンメモリのみだけの場合に指定します。|
| -mask \<bit count\> | MASK値をビット数で指定します。8以上を指定してください。この値は初期化時のサイズであり、動的にMASK値は変更（増加）するため、長期動作においては意味が無く、最初からキー数が多い場合などには大きめに設定しておくことをお勧めします。なお、初期化済のファイルを指定している場合には、このオプションは無視されます。|
| -cmask \<bit count\> | サブHASHテーブルのCMASK値のビット数です。この値は初期化後は変更できません。4～5程度を指定することをお勧めします。なお、初期化済のファイルを指定している場合には、このオプションは無視されます。|
| -elementcnt \<count\> | HASH値の衝突（サブHASHテーブルでの衝突）におけるエレメントリストの最大数を指定します。この値を超過したときにHASHテーブル（MASK値）が増加します。1024程度を指定することをお勧めします。なお、初期化済のファイルを指定している場合には、このオプションは無視されます。|
| -pagesize \<number\> | ページサイズを指定します。キー（Key）、値（Value）、サブキーリストのデータを保存する単位ブロックのサイズです。デフォルトは128バイトになっています。大きい値を保存するなどの場合には、1024バイトなど大きめに設定しても良いかもしれません。小さくすると利用率（ブロックに対して保存されているデータのサイズ）はよくなりますが、管理領域の割合（24バイトが管理領域のバイト数）が大きくなります。この値を適正にするために、本ツールを使い、ステータス情報などを見てチューニングを行ってください。なお、初期化済のファイルを指定している場合には、このオプションは無視されます。|
| -fullmap | メモリマップドファイルを指定した場合に、ファイル全体をマップすることを指定します。本オプションの指定が無い場合には、部分マッピングとなります。|
| -ext \<library path\> | HASH関数のDSOモジュールのロードをします。ファイルパスを指定します。|
| -ro | 読み出し専用でK2HASHデータファイルにアタッチします。|
| -init | メモリマップドファイルの初期化のみ行います。|
| -lap | 起動後のコマンド単位でのラップタイムを表示するようにします。|
| -capi | ツール内部で利用しているK2HASHライブラリのC++ I/Fを使わず、C I/Fを利用するようにします。|
| -g \<debug level\> | メッセージ出力レベルの指定をします。ERR、WAN、INFOのいずれかを指定します。メッセージ出力レベルは、SIGUSR1でBumpupすることもできますので、本ツール起動後にプロセスIDを指定して、killして変更することが可能です。また環境変数K2HDBGMODEに、SILENT、ERR、WAN、INFOの指定をして、本ツールを起動し、メッセージ出力レベルを指定することもできます。|
| -glog \<file path\> | メッセージの出力先ファイルを指定します。デフォルトでは、stderrとなっています。また、環境変数K2HDBGFILEで出力ファイルを指定することもできます。|
| -his \<count\> | 起動後のコマンド履歴の最大数を指定します。デフォルトは1000です。|
| -libversion | libk2hash.soのバージョンおよびクレジットを表示します。|
| -run \<file path\> | コマンドファイル（履歴含む）を指定します。指定されたコマンドファイル（履歴含む）の内容を起動後に自動的に実行します。|

## k2hlinetoolのインタプリタ機能

k2hlinetoolのインタプリタ機能を説明します。本ツールは、対話形式のツールであり、起動後はコマンド入力を行うプロンプトが表示されます。このプロンプトにて、ヘルプ（help）を入力することで、コマンドのヘルプを表示することができます。使い方などは、ヘルプを参照してください。コマンドのオプションは以下にまとめておきます。

| コマンド | 説明 |
| --- | --- |
| help(h) | コマンドのヘルプを表示します。 |
| quit(q)/exit |  ツールを終了します。|
| info(i) [state] | K2HASHデータの情報を表示します。stateオプションを指定した場合には、利用率も一緒に表示します。 |
| dump(d) \<parameter\> | K2HASHデータのダンプを行います。ダンプにはレベルがあり、head(デフォルト) / kindex / ckindex / element / fullを指定できます。 |
| set(s) \<key\> \<value\> [rmsub] [pass=....] [expire=sec] | キー（Key）と値（Value）を設定します。rmsubを付けた場合には、キーにサブキーが設定されている場合には、そのサブキーをすべて削除します。rmsubを付けた場合には、もしキー（Key）が既に存在しており、サブキーリストを持っている場合には、そのサブキーリストおよび全サブキーを削除します。 passパラメータにパスフレーズを指定して暗号化することができます。またexpireに秒数を指定してExpire時刻の指定ができます。|
| settrial(st) \<key\> [pass=....] | キー（Key）を読み出し、既存であれば値（Value)を表示します。未設定であればキー（Key）のみを表示します。その後、対話形式にて値（Value）を更新するか否かを問い合わせします。対話形式にて"n（no）"と入力した場合には何もしません。それ以外の文字列を入力した場合には、その入力された内容で値（Value）を更新します。passパラメータにパスフレーズを指定して暗号化することができます。 |
| setsub \<parent key\> \<key\> \<value\> | キー（Key）にサブキー（Subkey）とその値（Value）を作成し、紐付けます。 |
| directset(dset) \<key\> \<value\> \<offset\>  | キー （Key）に値（Value）を設定します。値はオフセット\<offse\t>バイトを開始位置として書き込みされます。書き込む前にオフセッ ト位置よりも値の長さが小さい場合には、オフセット位置まで値の長さが拡張されます。拡張された部分の値は不定です。 |
| setf(sf) \<key\> \<offset\> \<file\> | キー（Key）に値（Value）を設定します。値はファイル\<file\>から読み取られます。オフセットについては、directsetと同じです。 |
| fill(f) \<prefix\> \<value\> \<count\> | "prefix"を持つキー（Key）と値（Value）を設定します。キーはcount分作成されます。例えば、prefixが"key"でcountが10の場合には、"key-0"～"key-9"までの10個のキーが作成され、すべて値（value）で設定されます。 |
| fillsub \<parent\> \<prefix\> \<val\> \<cnt\> | fillとほぼ同じですが、parentで示されるキーに、prefixから開始されるサブキーをcnt個作成します。|
| rm \<key\> [all] | キー（Key）を削除します。allを付けた場合には、キーの持つサブキーリストの全サブキーも削除します。|
| rmsub \<parent key\> \<key\> | キー（parent key）のサブキーリストにあるサブキー（key）を削除します。サブキー自体も削除されます。|
| print(p) \<key\> [all] [noattrcheck] [pass=....] | キー（Key）の内容を表示します。allを指定した場合には、サブキーリストにあるサブキーの値（value）までネストして表示します。暗号化されたキーの場合には、パスフレーズを指定して読み出すことができます。またnoattrcheckを指定した場合には、復号化を行わず、Expire時刻もチェックされず値を読み出します。 |
| printattr(pa) \<key\> | キー（Key）に設定されている属性（Attribute）を表示します。 |
| addattr(aa) \<key\> \<attr name\> \<attr value\> | キー（Key）に属性（Attribute）を追加します。属性は、属性名、属性値で指定してください。 | 
| directprint(dp) \<key\> \<length\> \<offset\> | キー（Key）の内容をオフセット\<offset\>位置から長さ\<length\>バイト読み出します。|
| directsave(dsave) \<start hash\> \<file path\> | 指定HASH値を検索し、検出した値をバイナリデータとしてファイルに出力します。（検出は、MASKされた値で実行され、複数の値が同一のHASH値に存在する場合には、それら全てがファイルに出力されます） |
| directload(dload) \<file path\> [unixtime] | directsaveで保存したファイルからデータを設定します。unixtimeが指定されている場合には、unixtime以降に設定されている既存の同一のキーが存在した場合には、上書きされません。 |
| copyfile(cf) \<key\> \<offset\> \<file\> | キー（Key）の内容をオフセット\<offset\>位置から読み出し、ファイルに出力します。 |
| list(l) \<key\> | すべてのキー（Key）のリストアップをします。keyを指定した場合には、printコマンドのallとほぼ同じ表示になります。 |
| stream(str) \<key\> \< input \| output\> | キー（Key）に対してstreamとして値を操作します。\<input\>と\<output\>にて出力の方向を指定します。（iostramクラスと同等です）このコマンドは、対話形式で値の読み書きを行います。（サンプルを参考にしてください。） |
| history(his) | コマンドの履歴を表示します。 |
| save \<file path\> | コマンド履歴をファイルに保存します。 |
| load \<file path\> | コマンド履歴のファイルをロードし、実行します。 |
| trans(tr) \<on [filename [prefix [param]]] \| off\> [expire=sec] | トランザクションを有効/無効にします。expireを指定した場合にはトランザクションの有効期限時間を秒で指定できます。 |
| archive(ar) \<put \| load\> \<filename\> | K2HASH全体をアーカイブファイルとして出力します。または、アーカイブファイル（トランザクションログ含む）をロードします。 |
| queue(que) [prefix] empty | K2HASHでサポートするキューが空であるか確認します。|
| queue(que) [prefix] count | K2HASHでサポートするキューに蓄積されているデータ数を返します。|
| queue(que) [prefix] read \<fifo \| lifo\> \<pos\> [pass=...] | K2HASHでサポートするキューからデータをコピーます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。キューが暗号化されている場合にはパスフレーズを指定してください。|
| queue(que) [prefix] push \<fifo \| lifo\> \<value\> [pass=....] [expire=sec] | K2HASHでサポートするキューへデータ（value）を蓄積（プッシュ）します。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。キューを暗号化する場合はパスフレーズを指定してください。またExpire時間を指定する場合には、expireを秒で指定してください。|
| queue(que) [prefix] pop \<fifo \| lifo\> [pass=...] | K2HASHでサポートするキューからデータを取り出し（ポップ）ます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。キューが暗号化されている場合にはパスフレーズを指定してください。|
| queue(que) [prefix] dump \<fifo \| lifo\> | K2HASHでサポートするキューをダンプします。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。|
| queue(que) [prefix] remove(rm) \<fifo \| lifo\> \<count\> [c] [pass=...] | K2HASHでサポートするキューから指定数（count）分のデータを削除ます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。削除されたデータは表示されません。最後のパラメータ"c（confirm）"をつけた場合には、削除する前に対話形式で、確認がなされます。対話形式には"y(yes)"、"n(no)"、"b(break)"で回答してください。キューが暗号化されている場合にはパスフレーズを指定してください。| 
| keyqueue(kque) [prefix] empty | K2HASHでサポートするキューが空であるか確認します。|
| keyqueue(kque) [prefix] count | K2HASHでサポートするキューに蓄積されているデータ数を返します。|
| keyqueue(kque) [prefix] read \<fifo \| lifo\> \<pos\> [pass=...] | K2HASHでサポートするキューからデータ（keyとvalue）をコピーます。prefixはキューにて使用されるキー名のプレフィック スです。FIFO/LIFOを指定してください。キューが暗号化されている場合にはパスフレーズを指定してください。|
| keyqueue(kque) [prefix] push \<fifo \| lifo\> \<key\> \<value\> [pass=....] [expire=sec] | K2HASHでサポートするキューへデータ（key/value）を蓄積（プッシュ）します。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。指定されたkey/valueはキューだけではなく、K2HASHファイルにkey/valueとして書き込まれます。キューを暗号化する場合はパスフレーズを指定してください。またExpire時間を指定する場合には、expireを秒で指定してください。|
| keyqueue(kque) [prefix] pop \<fifo \| lifo\> [pass=...] | K2HASHでサポートするキューからデータ（keyとvalue）を取り出し（ポップ）ます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。取り出されたkey/valueはK2HASHファイルからも削除されます。キューが暗号化されている場合にはパスフレーズを指定してください。|
| keyqueue(kque) [prefix] dump \<fifo \| lifo\> | K2HASHでサポートするキューをダンプします。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。|
| keyqueue(kque) [prefix] remove(rm) \<fifo \| lifo\> \<count\> [c] [pass=...] | K2HASHでサポートするキューから指定数（count）分のデータを削除ます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。削除されたデータは表示されません。キューから削除されるkey/valueはK2HASHファイルからも削除されます。最後のパラメータ"c（confirm）"をつけた場合には、削除する前に対話形式で、確認がなされます。対話形式には"y(yes)"、"n(no)"、"b(break)"で回答してください。キューが暗号化されている場合にはパスフレーズを指定してください。|
| builtinattr(ba) [mtime] [history] [expire=second] [enc] [pass=file path] | Builtin属性の設定をします。有効にする機能を指定します。expireには秒を指定してください。passにパスフレーズファイルを指定してください。暗号化機能を有効にする場合にはencを指定します。|
| loadpluginattr(lpa) filepath | Plugin属性を利用する場合に、シェアードライブラリ（DSOモジュール）を指定してロードします。|
| addpassphrese(app) \<pass phrase\> [default] | Builtin属性で利用するパスフレーズを1つ追加します。同時に暗号化機能を有効にし、このパスフレーズを暗号化で利用する場合いは、defaultを指定してください。| 
cleanallattr(caa) | 全ての属性（Attribute）設定を破棄し、初期状態にします。|
| shell |  shellに抜けます。|
| echo \<string\>... | UNIXのechoコマンドと同じ動作をします。指定されたパラメータを表示します。|
| sleep \<second\> | UNIXのsleepコマンドと同じ動作をします。指定された秒数のsleepを行い、コマンドを一時停止させます。|

## stream サンプル

数値、文字列をstreamとして、"key"に設定しています。ends/endlが利用でき、終了する場合には、"."を指定してください。

```
     K2HTOOL> stream key output
      
     *** Output stream test : "key" key *********************
      You can specify any string for writing value to stream.
      The string does not terminate null charactor.
      If you need to terminate it, specify "ends".
      If you specify "endl", puts 0x0a.
      Specify "." only to exit this interactive mode.
     ********************************************************
      
     << 1234567890
     << endl
     << string1
     << endl
     << string2
     << .
      
     Lap time: 0h 0m 27s 245ms 722us(27s 245722us)
      
     K2HTOOL>
```

続いて、数値、文字列をstreamとして、"key"から順番に取り出しています。利用できる型は、string/char/int/longとなっています。終了する場合には、"."を指定してください。値（Value）の終端まで読み出した場合には自動的に終了します。

```
     K2HTOOL> stream key input
      
     *** Input stream test : "key" key *********************
      You can specify below value types for reading value from stream.
        - "string", "char", "int", "long"
      Specify "." only to exit this interactive mode.
     ********************************************************
      
     >> int
      int = "1234567890(499602d2)"
     >> string
      string = "string1"
     >> string
      string = "string2"
      
     K2HTOOL>
```

