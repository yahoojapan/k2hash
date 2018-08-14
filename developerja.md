---
layout: contents
language: ja
title: Developer
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: developer.html
lang_opp_word: To English
prev_url: buildja.html
prev_string: Build
top_url: indexja.html
top_string: TOP
next_url: environmentsja.html
next_string: Environments
---

<!-- -----------------------------------------------------------　-->
# 開発者向け

#### [C言語インタフェース](#CAPI)
[デバッグ関連(C I/F)](#DEBUG)  
[DSOロード関連(C I/F)](#DSO)  
[Create/Open/Close関連(C I/F)](#COC)  
[トランザクション・アーカイブ関連(C I/F)](#TRANSACTION)  
[属性関連(C I/F)](#ATTR)  
[GET関連(C I/F)](#GET)  
[SET関連(C I/F)](#SET)  
[リネーム関連(C I/F)](#RENAME)  
[ダイレクトバイナリデータ取得/設定関連（C I/F）](#DIRECTBINARY)   
[削除関連(C I/F)](#DELETE)  
[探索関連(C I/F)](#FIND)  
[ダイレクトアクセス関連(C I/F)](#DIRECTACCESS)  
[キュー関連(C I/F)](#QUE)  
[キュー（キー＆値）関連(C I/F)](#KVQUE)  
[ダンプ・ステータス関連(C I/F)](#DUMP)  
#### [C++言語インタフェース](#CPP)
[デバッグ関連(C/C++ I/F)](#DEBUGCPP)  
[K2HashDynLibクラス](#K2HASHDYNLIB)  
[K2HTransDynLibクラス](#K2HTRANSDYNLIB)  
[K2HDAccessクラス](#K2HDACCESS)  
[k2hstream（ik2hstream、ok2hstream）クラス](#K2HSTREAM)  
[K2HArchiveクラス](#K2HARCHIVE)  
[K2HQueueクラス](#K2HQUEUE)  
[K2HKeyQueueクラス](#K2HKEYQUEUE)  
[K2HShmクラス](#K2HSHM)  

<!-- -----------------------------------------------------------　-->
***

## <a name="CAPI"> C言語インタフェース
C言語用のインタフェースです。

開発時には以下のヘッダファイルをインクルードしてください。
 ```
#include <k2hash/k2hash.h>
 ```

リンク時には以下をオプションとして指定してください。
 ```
-lk2hash
 ```

以下にC言語用の関数の説明をします。

<!-- -----------------------------------------------------------　-->
***

### <a name="DEBUG"> デバッグ関連(C I/F)
K2HASHライブラリは、内部動作およびAPIの動作の確認をするためにメッセージ出力を行うことができます。
本関数群は、メッセージ出力を制御するための関数群です。

#### 書式
- void k2h_bump_debug_level(void)
- void k2h_set_debug_level_silent(void)
- void k2h_set_debug_level_error(void)
- void k2h_set_debug_level_warning(void)
- void k2h_set_debug_level_message(void)
- bool k2h_set_debug_file(const char\* filepath)
- bool k2h_unset_debug_file(void)
- bool k2h_load_debug_env(void)
- bool k2h_set_bumup_debug_signal_user1(void)

#### 説明
- k2h_bump_debug_level  
  メッセージ出力レベルを、非出力→エラー→ワーニング→インフォメーション→非出力・・・とBump upします。
- k2h_set_debug_level_silent  
  メッセージ出力レベルを、出力しないようにします。
- k2h_set_debug_level_error  
  メッセージ出力レベルを、エラーにします。
- k2h_set_debug_level_warning  
  メッセージ出力レベルを、ワーニング以上にします。
- k2h_set_debug_level_message  
  メッセージ出力レベルを、インフォメーション以上にします。
- k2h_set_debug_file  
  メッセージを出力するファイルを指定します。設定されていない場合には stderr に出力します。
- k2h_unset_debug_file  
  メッセージを stderr に出力するように戻します。
- k2h_load_debug_env  
  環境変数 K2HDBGMODE、K2HDBGFILE を読み込み、その値にしたがってメッセージ出力、出力先を設定します。
- k2h_set_bumup_debug_signal_user1  
  SIGUSR1 シグナルハンドラを設定します。設定された場合には SIGUSR1 を受ける毎にメッセージ出力レベルを Bump upします。

#### 返り値
k2h_set_debug_file、 k2h_unset_debug_file、k2h_load_debug_env、k2h_set_bumup_debug_signal_user1 は成功した場合には、true を返します。失敗した場合には false を返します。

#### 注意
環境変数 K2HDBGMODE、K2HDBGFILEについては、[環境変数](environmentsja.html)を参照してください。

#### サンプル
 ```
k2h_set_bumup_debug_signal_user1();
k2h_set_debug_file("/var/log/k2hash/error.log");
k2h_set_debug_level_message();
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="DSO"> DSOロード関連(C I/F) 
K2HASHライブラリは、内部で利用するHASH関数およびトランザクション処理のための関数を外部のシェアードライブラリ（DSOモジュール）として読み込むことができます。
本関数群は、これらのシェアードライブラリのロード、アンロードを行う関数群です。

#### 書式
- bool k2h_load_hash_library(const char\* libpath)
- bool k2h_unload_hash_library(void)
- bool k2h_load_transaction_library(const char\* libpath)
- bool k2h_unload_transaction_library(void)

#### 説明
- k2h_load_hash_library  
  K2HASHライブラリで使用するHASH関数のDSOモジュールをロードします。ファイルパスを指定してください。
- k2h_unload_hash_library  
  ロードしたHASH関数（DSOモジュール）をアンロードします。
- k2h_load_transaction_library  
  トランザクションプラグインのDSOモジュールをロードします。ファイルパスを指定してください。
- k2h_unload_transaction_library  
  ロードしたトランザクションプラグイン（DSOモジュール）をアンロードします。

#### 返り値
成功した場合には、true を返します。失敗した場合には false を返します。

#### 注意
K2HASHは予めHASH関数（FNV-1A）、単純な処理を行うトランザクションプラグイン（Bultinトランザクションプラグイン）が組み込まれています。
本関数群によりDSOモジュールをロードした場合には、これら内部で実装されているHASH関数およびトランザクションプラグインより優先され、置き換えられます。

#### サンプル
 ```
if(!k2h_load_hash_library("/usr/lib64/myhashfunc.so")){
    return false;
}
if(!k2h_load_transaction_library("/usr/lib64/mytrunsfunc.so")){
    return false;
}
    //...
    //...
    //...
k2h_unload_hash_library();
k2h_unload_transaction_library();
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="COC"> Create/Open/Close関連(C I/F)
K2HASHファイル（もしくはオンメモリ）の初期化、オープン（アタッチ）、クローズ（デタッチ）を行う関数群です。

#### 書式
- bool k2h_create(const char\* filepath, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open(const char\* filepath, bool readonly, bool removefile, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open_rw(const char\* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open_ro(const char\* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open_tempfile(const char\* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open_mem(int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- bool k2h_close(k2h_h handle)
- bool k2h_close_wait(k2h_h handle, long waitms)

#### 説明
- k2h_create  
  K2HASHファイルの初期化（生成）をします。
- k2h_open  
  K2HASHファイルへのアタッチをします。filepath を NULL とした場合にはオンメモリとして動作します。
- k2h_open_rw  
  K2HASHファイルへ Read / Write 属性でアタッチをします。 k2h_open関数のラッパー関数です。
- k2h_open_ro  
  K2HASHファイルへRead属性のみでアタッチをします。 k2h_open関数のラッパー関数です。
- k2h_open_tempfile  
  K2HASHファイルへ Read / Write 属性でアタッチをします。 K2HASHファイルをテンポラリファイルとしてオープンし、k2h_close が呼ばれた時点で削除されます。 k2h_open関数のラッパー関数です。
- k2h_open_mem  
  オンメモリとしてアタッチします。 k2h_open関数のラッパー関数です。
- k2h_close  
  K2HASHハンドルをクローズします。
- k2h_close_wait  
  K2HASHハンドルをクローズします。引数にms（ミリ秒）を指定してトランザクション処理の完了を待つことができます。トランザクションが完全に完了するか、指定ms待つか、すぐに終了（k2h_closeと同等）するかを指定できます。

#### パラメータ
- filepath  
  K2HASHファイルのパスを指定してください。
- maskbitcnt  
  初期MASKビットカウント数を指定します。8以上を指定します。 既存 K2HASHファイルをオープンする場合には、ファイルに設定されている値が優先され、この値は無視されます。
- cmaskbitcnt  
  CMASKビットカウント数を指定します。4以上を推奨します。 既存 K2HASHファイルをオープンする場合には、ファイルに設定されている値が優先され、この値は無視されます。
- maxelementcnt  
  HASH値の衝突時の重複されたエレメント（要素）数の上限を設定します。 32以上を推奨します。既存 K2HASHファイルをオープンする場合には、ファイルに設定されている値が優先され、この値は無視されます。
- pagesize  
  データ保管時のブロックサイズであり、ページサイズとして設定します。 128以上を指定します。既存 K2HASHファイルをオープンする場合には、ファイルに設定されている値が優先され、この値は無視されます。
- readonly  
  読み出し属性のみでファイルをアタッチします。
- removefile  
  k2h_closeが呼び出され、完全にファイルがデタッチ（他プロセス含む）されたときにファイルを削除します。 ファイルを一時的なテンポラリとして利用するときに指定します（例：マルチプロセスで共有されたキャッシュKVSとして利用したい）。
- fullmap  
  アタッチしたファイルの全領域をメモリマッピングする場合に true を指定します。 false の場合には、K2HASHの管理領域のみがメモリマッピングされ、データなどの領域はメモリマッピングされません。  
  オンメモリのタイプの場合は、常にfullmap=trueと同じ状態となり、この値にfalseを指定しても無視されます。
- handle  
  k2h_open系の関数から返された K2HASHハンドルを指定します。
- waitms  
  トランザクション処理の完了を待つ ms（ミリ秒）を指定します。 0 を指定した場合は、k2h_close と同等の動作になります。 正数を指定した場合にはそのms分だけトランザクションの処理の終了をまちます。 指定ms秒以内に完了しない場合には完了を待たずにクローズします。 -1 を指定した場合にはトランザクションが完了するまでブロックされます。

#### 返り値
- k2h_create  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_close  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_open  
  成功した場合には、有効な K2HASHハンドルを返します。失敗した場合には K2H_INVALID_HANDLE(0) を返します。
- k2h_open_rw  
  成功した場合には、有効な K2HASHハンドルを返します。失敗した場合には K2H_INVALID_HANDLE(0) を返します。
- k2h_open_ro  
  成功した場合には、有効な K2HASHハンドルを返します。失敗した場合には K2H_INVALID_HANDLE(0) を返します。
- k2h_open_tempfile  
  成功した場合には、有効な K2HASHハンドルを返します。失敗した場合には K2H_INVALID_HANDLE(0) を返します。
- k2h_open_mem  
  成功した場合には、有効な K2HASHハンドルを返します。失敗した場合には K2H_INVALID_HANDLE(0) を返します。

#### 注意 
各関数から返されるK2HASHハンドルは、他のAPI関数でK2HASHファイル（もしくはオンメモリ）を操作するために指定するハンドルです。
アタッチ（オープン）したK2HASHハンドルは必ずデタッチ（クローズ）してください。
MASK、CMASK、elementcntは、K2HASHライブラリが内部で管理しているデータツリーの構造（HASHテーブル）を決定する値です。付属しているk2hlintoolツールを試用して、その値の効果を試して、値を設定することを推奨します。
既に存在するK2HASHファイルをアタッチする場合は、これらの値は無視され、設定された値が使用されます。

#### サンプル
 ```
if(!k2h_create("/home/myhome/mydata.k2h", 8, 4, 1024, 512)){
    return false;
}
k2h_h k2handle;
if(K2H_INVALID_HANDLE == (k2handle = k2h_open_rw("/home/myhome/mydata.k2h", true, 8, 4, 1024, 512))){
    return false;
}
    //...
    //...
    //...
k2h_close(k2handle);
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="TRANSACTION"> トランザクション・アーカイブ関連(C I/F)
K2HASHライブラリの提供するトランザクション処理に関連する関数群です。

#### 書式
- bool k2h_transaction(k2h_h handle, bool enable, const char\* transfile)
- bool k2h_enable_transaction(k2h_h handle, const char\* transfile)
- bool k2h_disable_transaction(k2h_h handle)
- bool k2h_transaction_prefix(k2h_h handle, bool enable, const char\* transfile, const unsigned char\* pprefix, size_t prefixlen)
- bool k2h_transaction_param(k2h_h handle, bool enable, const char\* transfile, const unsigned char\* pprefix, size_t prefixlen, const unsigned char\* pparam, size_t paramlen)
- bool k2h_transaction_param_we(k2h_h handle, bool enable, const char\* transfile, const unsigned char\* pprefix, size_t prefixlen, const unsigned char\* pparam, size_t paramlen, const time_t\* expire)
- bool k2h_enable_transaction_prefix(k2h_h handle, const char\* transfile, const unsigned char\* pprefix, size_t prefixlen)
- bool k2h_enable_transaction_param(k2h_h handle, const char\* transfile, const unsigned char\* pprefix, size_t prefixlen, const unsigned char\* pparam, size_t paramlen)
- bool k2h_enable_transaction_param_we(k2h_h handle, const char\* transfile, const unsigned char\* pprefix, size_t prefixlen, const unsigned char\* pparam, size_t paramlen, const time_t\* expire)
- int k2h_get_transaction_archive_fd(k2h_h handle)
- int k2h_get_transaction_thread_pool(void)
- bool k2h_set_transaction_thread_pool(int count)
- bool k2h_unset_transaction_thread_pool(void)
- bool k2h_load_archive(k2h_h handle, const char\* filepath, bool errskip)
- bool k2h_put_archive(k2h_h handle, const char\* filepath, bool errskip)

#### 説明
- k2h_transaction  
  トランザクション機能を有効/無効にします。
- k2h_enable_transaction  
  トランザクション機能を有効にします。 k2h_transaction のラッパー関数です。
- k2h_disable_transaction  
  トランザクション機能を無効にします。 k2h_transaction のラッパー関数です。
- k2h_transaction_prefix  
  トランザクション機能を有効/無効にします。本関数は、トランザクションのための内部キューに使用されるデフォルトのキープレフィックスを使用せず、独自で指定できるようになっています。
- k2h_transaction_param  
  トランザクション機能を有効/無効にします。 本関数は、トランザクションのための内部キューに使用されるデフォルトのキープレフィックスを使用せず、独自で指定できるようになっています。また、トランザクションプラグインに対して独自のパラメータを渡せるようになっています。
- k2h_transaction_param_we  
  k2h_transaction_param と同等です。 トランザクション処理において、そのトランザクションデータに予め Expire時間を指定することができます。
- k2h_enable_transaction_prefix  
  トランザクション機能を有効にします。 k2h_transaction_prefix のラッパー関数です。
- k2h_enable_transaction_param  
  トランザクション機能を有効にします。 k2h_transaction_param のラッパー関数です。
- k2h_enable_transaction_param_we  
  k2h_enable_transaction_param と同等です。 トランザクション処理において、そのトランザクションデータに予め Expire時間を指定することができます。
- k2h_get_transaction_archive_fd  
  トランザクションが有効であり、トランザクションを記録するファイルパスが設定されている場合に、そのファイルディスクリプタを取得する関数です。
- k2h_get_transaction_thread_pool  
  トランザクション処理のためのマルチスレッドワーカーのプール数を取得します。 このプール数は K2HASHライブラリインスタンスすべてにおいて共通となります（複数のK2HASHファイル（もしくはオンメモリ）にアタッチしている場合でもライブラリとして共通のプール数となります）。
- k2h_set_transaction_thread_pool  
  トランザクション処理のためのマルチスレッドワーカーのプール数を設定します。
- k2h_unset_transaction_thread_pool  
  トランザクション処理にマルチスレッドを使用しないようにします。プール数が0となります。
- k2h_load_archive  
  アーカイブファイルから K2HASHデータをロードします。 ロード前のK2HASHデータはそのままであり、アーカイブデータにより上書きします。  
  アーカイブファイルは、以下のファイルが指定できます。
 - k2h_put_archive によって出力されるファイル
 - k2h_transaction系関数により出力されるトランザクションファイル（トランザクション関連関数を独自のものに置き換えていないデフォルトの形式のファイル）
 - k2hlinetoolツールにて作成されるトランザクション/アーカイブファイル
- k2h_put_archive  
  K2HASHファイル（もしくはオンメモリ）の全データをアーカイブ（シリアライズ）し、ファイルに出力します。

#### パラメータ
- handle  
  k2h_open系の関数から返された K2HASHハンドルを指定します。
- enable  
  トランザクション機能の有効(true) / 無効(false) を指定します。
- transfile  
  トランザクションを記録するファイルパスを指定します。ファイルを使用しない場合にはNULLを指定できます。
- pprefix  
  トランザクションのキューイングを行うためのキー（K2HASHデータ内に作成される）のプレフィックスを指定します。
- prefixlen  
  トランザクションのキューイングを行うためのキー（K2HASHデータ内に作成される）のプレフィックスを指定したときの pprefix のバッファ長を指定します。
- pparam  
  トランザクションプラグインに引き渡す独自のパラメータを指定します。
- paramlen  
  トランザクションプラグインに引き渡す独自のパラメータを指定した際の、pparam バッファ長をしていします。（pparam に指定できるバッファの最大長は32Kバイトまでです。）
- expire  
  トランザクションデータの有効時間（Expire）を time_t のポインタで指定します。Expire時間を指定しない場合には、NULL を指定します。
- count  
  トランザクション処理のマルチスレッドワーカーのプール数を指定します。 0 を指定した場合には、マルチスレッドによる処理をしない設定となります。
- filepath  
  アーカイブファイルのファイルパスを指定します。
- errskip  
  アーカイブファイルへの出力/ロードを行っている最中にエラーが発生した場合に、そのまま継続するか否かを指定します。エラー発生後に継続する場合には true を指定してください。それ以外は false を指定します。

#### 返り値
- k2h_get_transaction_archive_fd  
  トランザクションが有効であり、トランザクションを記録するファイルパスが設定されている場合に、そのファイルディスクリプタを返します。無効、未設定、エラーなどの場合には-1を返します。
- k2h_get_transaction_thread_pool  
  トランザクション処理のためのマルチスレッドワーカーのプール数を返します。
- 上記以外の関数  
  成功した場合には、true を返します。失敗した場合には false を返します。

#### 注意
- K2HASHライブラリのトランザクション処理は、内部でキュー（K2HQueueを参照）を利用し、実現されています。このキューにはキー名が必要であり、変更をしない場合にはデフォルトのキープレフィックスが使用されます。デフォルトキープレフィックスを使用せず、別のキープレフィックスを使用する場合は、k2h_transaction_prefix、k2h_enable_transaction_prefix関数で変更できます。
- k2h_load_archive でロードできるファイルは、K2HASHライブラリのBultinトランザクションプラグインにより出力されたファイル、もしくはk2h_put_archive で出力されたファイルです。
- k2h_load_archive関数を呼び出す前にK2HASHデータが存在する場合は、そのK2HASHデータにアーカイブファイルの内容が上書きされます。
- k2h_transaction、k2h_enable_transaction、k2h_transaction_prefix、k2h_enable_transaction_prefix は、transfile引数（トランザクションの出力ファイルパス）に NULL を許可します。ただし、独自のトランザクションプラグインをロードし、そのトランザクションプラグインがトランザクションファイルを不要とする場合のみNULLを許容できます。K2HASHライブラリのBultinトランザクションプラグインはトランザクションファイルを必要とします。
- マルチスレッドでトランザクション処理を行う場合は、ワーカースレッドのプール数を指定できます。プール数は、K2HASHライブラリのインスタンスで共通で設定される値となります。 複数のK2HASHファイル（もしくはオンメモリ）にアタッチした場合でも、インスタンスとしてこのプール数が使用されます。

#### サンプル
 ```
if(!k2h_enable_transaction(k2handle, "/tmp/mytrans.log")){
    return false;
}
    //...
    //...
    //...
k2h_disable_transaction(k2handle);
   
//
// K2HASH can load the file which is made by transaction.
//
if(!k2h_load_archive(k2handle, "/tmp/mytrans.log", true)){
    return false;
}
// Full backup
if(!k2h_put_archive(k2handle, "/tmp/myfullbackup.ar", true)){
    return false;
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="ATTR"> 属性関連(C I/F)
K2HASHライブラリの提供する属性（Attribute）に関連する関数群です。

#### 書式
- bool k2h_set_common_attr(k2h_h handle, const bool\* is_mtime, const bool\* is_defenc, const char\* passfile, const bool\* is_history, const time_t\* expire)
- bool k2h_clean_common_attr(k2h_h handle)
- bool k2h_add_attr_plugin_library(k2h_h handle, const char\* libpath)
- bool k2h_add_attr_crypt_pass(k2h_h handle, const char\* pass, bool is_default_encrypt)
- bool k2h_print_attr_version(k2h_h handle, FILE\* stream)
- bool k2h_print_attr_information(k2h_h handle, FILE\* stream)

#### 説明
- k2h_set_common_attr  
  K2HASHライブラリに組み込まれている属性（Builtin属性）の設定を行います。Builtin属性のタイプ（更新時刻スタンプ、履歴機能、Expire機能、暗号化機能、パスフレーズファイル指定）に応じて引数を設定します。
- k2h_clean_common_attr  
  Builtin属性の設定を初期化（クリア）します。
- k2h_add_attr_plugin_library  
  K2HASHライブラリは、独自の属性のシェアードライブラリ（DSOモジュール）をロードし、組み込むことができます。これをPlugin属性と呼びます。  
  本関数は、Plugin属性のDSOモジュールをロードします。
- k2h_add_attr_crypt_pass  
  Builtin属性の暗号化のパスフレーズを登録します。また暗号化機能の有効/無効を設定します。（無効の場合でパスフレーズが登録されるケースは、復号のみ行うということになります。）
- k2h_print_attr_version  
  Builtin属性、Plugin属性のバージョンを表示（取得）します。
- k2h_print_attr_information  
  Builtin属性、Plugin属性の情報を表示（取得）します。

#### パラメータ
- handle  
  k2h_open系の関数から返されたK2HASHハンドルを指定します。
- is_mtime  
  Builtin属性の更新時刻スタンプ機能の有効/無効を指定する場合には、bool値のポインタを渡します。現在設定されている状態から変更しない場合には、NULL を指定します。（Builtin属性は環境変数からの設定の読み込みも行われるので、その状態を変更しない場合には NULL を指定します。）
- is_defenc  
  Builtin属性の暗号化機能の有効/無効を指定する場合には、bool値のポインタを渡します。現在設定されている状態から変更しない場合には、NULL を指定します。（Builtin属性は環境変数からの設定の読み込みも行われるので、その状態を変更しない場合には NULL を指定します。）
- passfile  
  Builtin属性の暗号化・復号化で使用されるパスフレーズのファイルを指定します。指定しない場合には、NULL を指定します。（Builtin属性は環境変数からの設定の読み込みも行われるので、その状態を変更しない場合には NULL を指定します。）
- is_history  
  Builtin属性の履歴機能の有効/無効を指定する場合には、bool値のポインタを渡します。現在設定されている状態から変更しない場合には、NULL を指定します。（Builtin属性は環境変数からの設定の読み込みも行われるので、その状態を変更しない場合には NULL を指定します。）
- expire  
  Builtin属性のExpire機能で利用される有効期限（秒）を指定する場合には、time_t 値のポインタを渡します。現在設定されている状態から変更しない場合 には、NULL を指定します。（Builtin属性は環境変数からの設定の読み込みも行われるので、その状態を変更しない場合には NULL を指定します。）

#### 返り値
成功した場合には、true を返します。失敗した場合には false を返します。

#### 注意
Builtin属性の設定は、k2h_set_common_attr関数で行います。また、K2HASHライブラリは環境変数からBuiltin属性の設定値を読み出すこともできます。
まず、K2HASHライブラリの初期化時に環境変数から属性に関する設定値を読み込みます。その後、k2h_set_common_attr関数などの呼び出しによりBuiltin属性の設定は上書きされます。
よって、初期値（環境変数の設定含む）から変更しない値に関しては、NULL を指定することで初期値を維持できます。

#### サンプル
 ```
bool    is_mtime = true;
bool    is_defenc= true;
char*   passfile = "/etc/k2hpass";
bool    is_history = true;
time_t  expire = 120;
if(!k2h_set_common_attr(handle, &is_mtime, &is_defenc, passfile, &is_history, &expire)){
    return false;
}
    //...
    //...
    //...
k2h_clean_common_attr(k2handle);
   
if(!k2h_add_attr_plugin_library(k2handle, "/usr/lib/myattrs.so")){
    return false;
}
// Full backup
if(!k2h_put_archive(k2handle, "/tmp/myfullbackup.ar", true)){
    return false;
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="GET"> GET関連(C I/F)
K2HASHファイル（もしくはオンメモリ）からデータを読み出す関数群です。

#### 書式
- bool k2h_get_value(k2h_h handle, const unsigned char\* pkey, size_t keylength, unsigned char\*\* ppval, size_t\* pvallength)
- unsigned char\* k2h_get_direct_value(k2h_h handle, const unsigned char\* pkey, size_t keylength, size_t\* pvallength)
- bool k2h_get_str_value(k2h_h handle, const char\* pkey, char\*\* ppval)
- char\* k2h_get_str_direct_value(k2h_h handle, const char\* pkey)
<br />
<br />
- bool k2h_get_value_wp(k2h_h handle, const unsigned char\* pkey, size_t keylength, unsigned char\*\* ppval, size_t\* pvallength, const char\* pass)
- unsigned char\* k2h_get_direct_value_wp(k2h_h handle, const unsigned char\* pkey, size_t keylength, size_t\* pvallength, const char\* pass)
- bool k2h_get_str_value_wp(k2h_h handle, const char\* pkey, char\*\* ppval, const char\* pass)
- char\* k2h_get_str_direct_value_wp(k2h_h handle, const char\* pkey, const char\* pass)
<br />
<br />
- bool k2h_get_value_np(k2h_h handle, const unsigned char\* pkey, size_t keylength, unsigned char\*\* ppval, size_t\* pvallength)
- unsigned char\* k2h_get_direct_value_np(k2h_h handle, const unsigned char\* pkey, size_t keylength, size_t\* pvallength)
- bool k2h_get_str_value_np(k2h_h handle, const char\* pkey, char\*\* ppval)
- char\* k2h_get_str_direct_value_np(k2h_h handle, const char\* pkey)
<br />
<br />
- bool k2h_get_value_ext(k2h_h handle, const unsigned char\* pkey, size_t keylength, unsigned char\*\* ppval, size_t\* pvallength, k2h_get_trial_callback fp, void\* pExtData)
- unsigned char\* k2h_get_direct_value_ext(k2h_h handle, const unsigned char\* pkey, size_t keylength, size_t\* pvallength, k2h_get_trial_callback fp, void\* pExtData)
- bool k2h_get_str_value_ext(k2h_h handle, const char\* pkey, char\*\* ppval, k2h_get_trial_callback fp, void\* pExtData)
- char\* k2h_get_str_direct_value_ext(k2h_h handle, const char\* pkey, k2h_get_trial_callback fp, void\* pExtData)
<br />
<br />
- bool k2h_get_value_wp_ext(k2h_h handle, const unsigned char\* pkey, size_t keylength, unsigned char\*\* ppval, size_t\* pvallength, k2h_get_trial_callback fp, void\* pExtData, const char\* pass)
- unsigned char\* k2h_get_direct_value_wp_ext(k2h_h handle, const unsigned char\* pkey, size_t keylength, size_t\* pvallength, k2h_get_trial_callback fp, void\* pExtData, const char\* pass)
- bool k2h_get_str_value_wp_ext(k2h_h handle, const char\* pkey, char\*\* ppval, k2h_get_trial_callback fp, void\* pExtData, const char\* pass)
- char\* k2h_get_str_direct_value_wp_ext(k2h_h handle, const char\* pkey, k2h_get_trial_callback fp, void\* pExtData, const char\* pass)
<br />
<br />
- bool k2h_get_value_np_ext(k2h_h handle, const unsigned char\* pkey, size_t keylength, unsigned char\*\* ppval, size_t\* pvallength, k2h_get_trial_callback fp, void\* pExtData)
- unsigned char\* k2h_get_direct_value_np_ext(k2h_h handle, const unsigned char\* pkey, size_t keylength, size_t\* pvallength, k2h_get_trial_callback fp, void\* pExtData)
- bool k2h_get_str_value_np_ext(k2h_h handle, const char\* pkey, char\*\* ppval, k2h_get_trial_callback fp, void\* pExtData)
- char\* k2h_get_str_direct_value_np_ext(k2h_h handle, const char\* pkey, k2h_get_trial_callback fp, void\* pExtData)
<br />
<br />
- bool k2h_get_subkeys(k2h_h handle, const unsigned char\* pkey, size_t keylength, PK2HKEYPCK\* ppskeypck, int\* pskeypckcnt)
- PK2HKEYPCK k2h_get_direct_subkeys(k2h_h handle, const unsigned char\* pkey, size_t keylength, int\* pskeypckcnt)
- int k2h_get_str_subkeys(k2h_h handle, const char\* pkey, char\*\*\* ppskeyarray)
- char\*\* k2h_get_str_direct_subkeys(k2h_h handle, const char\* pkey)
<br />
<br />
- bool k2h_get_subkeys_np(k2h_h handle, const unsigned char\* pkey, size_t keylength, PK2HKEYPCK\* ppskeypck, int\* pskeypckcnt)
- PK2HKEYPCK k2h_get_direct_subkeys_np(k2h_h handle, const unsigned char\* pkey, size_t keylength, int\* pskeypckcnt)
- int k2h_get_str_subkeys_np(k2h_h handle, const char\* pkey, char\*\*\* ppskeyarray)
- char\*\* k2h_get_str_direct_subkeys_np(k2h_h handle, const char\* pkey)
<br />
<br />
- bool k2h_free_keypack(PK2HKEYPCK pkeys, int keycnt)
- bool k2h_free_keyarray(char\*\* pkeys)
<br />
<br />
- bool k2h_get_attrs(k2h_h handle, const unsigned char\* pkey, size_t keylength, PK2HATTRPCK\* ppattrspck, int\* pattrspckcnt)
- PK2HATTRPCK k2h_get_direct_attrs(k2h_h handle, const unsigned char\* pkey, size_t keylength, int\* pattrspckcnt)
- PK2HATTRPCK k2h_get_str_direct_attrs(k2h_h handle, const char\* pkey, int\* pattrspckcnt)
<br />
<br />
- bool k2h_free_attrpack(PK2HATTRPCK pattrs, int attrcnt)

#### 説明
- k2h_get_value  
  キー（Key）に設定されている値（Value）を取り出します。（暗号化されている場合にはパスフレーズの登録がなされており、合致するパスフレーズがあれば復号された値が取得できます。 復号に失敗した場合には値は取り出せません）
- k2h_get_direct_value  
  キー（Key）に設定されている値（Value）を取り出します。 戻り値で値（Value）を返します。（複合化については k2h_get_value と同じ）
- k2h_get_str_value  
  キー（Key）に設定されている値（Value）を取り出します。 キー、値共に文字列であることを前提とした関数です。バイナリ列である場合には本関数は利用しないでください。（複合化については k2h_get_value と同じ）
- k2h_get_str_direct_value  
  キー（Key）に設定されている値（Value）を取り出します。 戻り値で値（Value）を返します。キー、値共に文字列であることを前提とした関数です。 バイナリ列である場合には本関数は利用しないでください。（複合化については k2h_get_value と同じ）
- k2h_get_value_wp  
  k2h_get_value 同等であり、復号用のパスフレーズを指定できます。
- k2h_get_direct_value_wp  
  k2h_get_direct_value 同等であり、復号用のパスフレーズを指定できます。
- k2h_get_str_value_wp  
  k2h_get_str_value 同等であり、復号用のパスフレーズを指定できます。
- k2h_get_str_direct_value_wp  
  k2h_get_str_direct_value 同等であり、復号用のパスフレーズを指定できます。
- k2h_get_value_np  
  k2h_get_value 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_direct_value_np  
  k2h_get_direct_value 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_str_value_np  
  k2h_get_str_value 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_str_direct_value_np  
  k2h_get_str_direct_value 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_value_ext  
  キー（Key）に設定されている値（Value）を取り出します。ただし、コールバック関数（後述）が引数 pExtData を伴い、呼び出されます。このコールバック関数にて読み出した時点の値（Value）の変更（上書き）を行うことができます。キー（Key）がK2HASHデータに存在していない場合もコールバック関数は呼び出され、値（Value）を設定することができます。これにより未設定のキー（Key）の読み出しにおいて、初期値を設定するタイミングを与えることができます。（複合化については k2h_get_value と同じ）
- k2h_get_direct_value_ext  
  k2h_get_value_ext と同等の関数であり、ダイレクトに値（Value）を返します。
- k2h_get_str_value_ext  
  k2h_get_value_ext と同等の関数であり、値（Value）を文字列として取り扱います。
- k2h_get_str_direct_value_ext  
  k2h_get_str_value_ext と同等の関数であり、ダイレクトに文字列として値（Value）を返します。
- k2h_get_value_wp_ext  
  k2h_get_value_ext 同等であり、復号用のパスフレーズを指定できます。
- k2h_get_direct_value_wp_ext  
  k2h_get_direct_value_ext 同等であり、復号用のパスフレーズを指定できます。
- k2h_get_str_value_wp_ext  
  k2h_get_str_value_ext 同等であり、復号用のパスフレーズを指定できます。
- k2h_get_str_direct_value_wp_ext  
  k2h_get_str_direct_value_ext 同等であり、復号用のパスフレーズを指定できます。
- k2h_get_value_np_ext  
  k2h_get_value_ext 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_direct_value_np_ext  
  k2h_get_direct_value_ext 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_str_value_np_ext  
  k2h_get_str_value_ext 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_str_direct_value_np_ext  
  k2h_get_str_direct_value_ext 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_subkeys  
  キー（Key）に設定されているサブキー（Subkey）リストを取り出します。
- k2h_get_direct_subkeys  
  キー（Key）に設定されているサブキー（Subkey）リストを取り出します。戻り値でサブキー（Subkey）リストを返します。
- k2h_get_str_subkeys  
  キー（Key）に設定されているサブキー（Subkey）リストを取り出します。キー、サブキー共に文字列であることを前提とした関数です。バイナリ列である場合には本関数は利用しないでください。
- k2h_get_str_direct_subkeys  
  キー （Key）に設定されているサブキー（Subkey）リストを取り出します。戻り値でサブキー（Subkey）リストを返します。キーが文字列であること を前提とした関数です。キー、サブキー共に文字列であることを前提とした関数です。バイナリ列である場合には本関数は利用しないでください。
- k2h_get_subkeys_np  
  k2h_get_subkeys 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_direct_subkeys_np  
  k2h_get_direct_subkeys 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_str_subkeys_np  
  k2h_get_str_subkeys 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_get_str_direct_subkeys_np  
  k2h_get_str_direct_subkeys 同等であり、暗号化されていた場合であっても復号しない値を取り出します。
- k2h_free_keypack  
  k2h_get_subkeys系関数が返す K2HKEYPCKポインタが示す領域を開放します。
- k2h_free_keyarray  
  k2h_get_str_subkeys系関数が返す文字列配列（char\*\*）が示す領域を開放します。
- k2h_get_attrs  
  キー（Key）に設定されている属性を取り出します。
- k2h_get_direct_attrs  
  キー（Key）に設定されている属性を取り出します。戻り値で属性を返します。
- k2h_get_str_direct_attrs  
  キー（Key）に設定されている属性名の属性を取り出します。戻り値で属性を返します。属性名は文字列を想定しています。
- k2h_free_attrpack  
  k2h_get_direct_attrs などが返す K2HATTRPCKポインタの示す領域を開放します。

#### パラメータ
- k2h_get_value
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - ppval  
   取り出した値（Value）のバイナリ列を保存するポインタを指定します。このポインタに設定される（返される）領域は、free()関数で開放してください。
 - pvallength  
   取り出した値（Value）のバイナリ列の長さを返すポインタを指定します。
 - pass  
   パスフレーズを指定します。
- k2h_get_direct_value
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - pvallength  
   取り出した値（Value）のバイナリ列の長さを返すポインタを指定します。
 - pass  
   パスフレーズを指定します。
- k2h_get_str_value
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）文字列のポインタを指定します。
 - ppval 
   取り出した値（Value）の文字列を保存するポインタを指定します。このポインタに設定される（返される）領域は、free()関数で開放してください。
 - pass  
   パスフレーズを指定します。
- k2h_get_str_direct_value
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）文字列のポインタを指定します。
 - pass  
   パスフレーズを指定します。
- k2h_get_value\*_ext
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - ppval  
   取り出した値（Value）のバイナリ列を保存するポインタを指定します。 このポインタに設定される（返される）領域は、free()関数で開放してください。
 - pvallength  
   取り出した値（Value）のバイナリ列の長さを返すポインタを指定します。
 - fp  
   コールバック関数（k2h_get_trial_callback）のポインタを指定します。
 - pExtData  
   コールバック関数（k2h_get_trial_callback）に引き渡す任意のポインタを指定します（NULL 可）。
 - pass  
   パスフレーズを指定します。
- k2h_get_direct_value\*_ext
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - pvallength  
   取り出した値（Value）のバイナリ列の長さを返すポインタを指定します。
 - fp  
   コールバック関数（k2h_get_trial_callback）のポインタを指定します。
 - pExtData  
   コールバック関数（k2h_get_trial_callback）に引き渡す任意のポインタを指定します（NULL 可）。
 - pass  
   パスフレーズを指定します。
- k2h_get_str_value\*_ext
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）文字列のポインタを指定します。
 - ppval  
   取り出した値（Value）の文字列を保存するポインタを指定します。 このポインタに設定される（返される）領域は、free()で開放してください。
 - fp  
   コールバック関数（k2h_get_trial_callback）のポインタを指定します。
 - pExtData  
   コールバック関数（k2h_get_trial_callback）に引き渡す任意のポインタを指定します（NULL 可）。
 - pass  
   パスフレーズを指定します。
- k2h_get_str_direct_value\*_ext  
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）文字列のポインタを指定します。
 - fp  
   コールバック関数（k2h_get_trial_callback）のポインタを指定します。
 - pExtData  
   コールバック関数（k2h_get_trial_callback）に引き渡す任意のポインタを指定します（NULL 可）。
 - pass  
   パスフレーズを指定します。
- k2h_get_subkeys
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - ppskeypck  
   取り出したサブキー（Subkey）リストの K2HKEYPCK構造体配列を保存するポインタを指定します。このポインタに設定される（返される）領域は、k2h_free_keypack()関数で開放してください。
 - pskeypckcnt  
    取り出したサブキー（Subkey）リストの K2HKEYPCK構造体配列の個数を返すポインタを指定します。
- k2h_get_direct_subkeys
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - pskeypckcnt  
   取り出したサブキー（Subkey）リストの K2HKEYPCK構造体配列の個数を返すポインタを指定します。
- k2h_get_str_subkeys
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）文字列のポインタを指定します。
 - ppskeyarray  
   取り出したサブキー（Subkey）リストの文字列配列（NULL終端）を保存するポインタを指定します。このポインタに設定される（返される）領域は、 k2h_free_keyarray()関数で開放してください。 文字列配列は終端が NULL となっています。
- k2h_get_str_direct_subkeys
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）文字列のポインタを指定します。
- k2h_free_keypack 	
 - pkeys  
   k2h_get_subkeys、k2h_get_direct_subkeys から返されたサブキー（Subkey）リストの K2HKEYPCK構造体配列のポインタを指定します。
 - keycnt  
   指定したサブキー（Subkey）リストの K2HKEYPCK構造体配列の個数を指定します。
- k2h_free_keyarray
 - pkeys  
   k2h_get_str_subkeys、k2h_get_str_subkeys から返されたサブキー（Subkey）リストの文字列配列のポインタを指定します。
- k2h_get_attrs
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。 
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - ppattrspck  
   取り出した属性の K2HATTRPCK構造体配列を保存するポインタを指定します。このポインタに設定される（返される）領域は、 k2h_free_attrpack()関数で開放してください。
  - pattrspckcnt  
    取り出した属性の K2HATTRPCK構造体配列の個数を返すポインタを指定します。
- k2h_get_direct_attrs
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - pattrspckcnt  
    取り出した属性の K2HATTRPCK構造体配列の個数を返すポインタを指定します。
- k2h_get_str_direct_attrs
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）文字列のポインタを指定します。
 - pattrspckcnt  
   取り出した属性の K2HATTRPCK構造体配列の個数を返すポインタを指定します。
- k2h_free_attrpack  
 - pattrs  
   k2h_get_direct_attrs、k2h_get_str_direct_attrs から返された属性情報の K2HATTRPCKポインタを指定します。
 - attrcnt  
   k2h_get_direct_attrs、k2h_get_str_direct_attrs から返された属性情報の K2HATTRPCKポインタに含まれる属性数を指定します。

#### 返り値
- k2h_get_value  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_get_str_value  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_get_value\*_ext  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_get_str_value\*_ext  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_get_subkeys  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_get_attrs  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_free_keypack  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_free_keyarray  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_free_attrpack  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_get_direct_value  
  成功した場合には、取り出す値（Value）のバイナリ列ポインタを返します。失敗した場合には NULL を返します。成功時に返されたポインタは free()関数で開放してください。
- k2h_get_direct_value\*_ext  
  成功した場合には、取り出す値（Value）のバイナリ列ポインタを返します。失敗した場合には NULL を返します。成功時に返されたポインタは free()関数で開放してください。
- k2h_get_str_direct_value  
  成功した場合には、取り出す値（Value）の文字列ポインタを返します。失敗した場合には NULL を返します。成功時に返されたポインタは free()関数で開放してください。
- k2h_get_str_direct_value\*_ext  
  成功した場合には、取り出す値（Value）の文字列ポインタを返します。失敗した場合には NULL を返します。成功時に返されたポインタは free()関数で開放してください。
- k2h_get_str_subkeys  
  成功した場合には、取り出すサブキー（Subkey）リストの K2HKEYPCK構造体ポインタに格納された要素数を返します。失敗した場合には、-1 を返します。
- k2h_get_direct_subkeys  
  成功した場合には、取り出すサブキー（Subkey）リストの K2HKEYPCK構造体ポインタを返します。失敗した場合には NULL を返します。成功時に返されたポインタは k2h_free_keypack()関数で開放してください。
- k2h_get_str_direct_subkeys  
  成功した場合には、取り出すサブキー（Subkey）リストの文字列配列ポインタを返します。失敗した場合には NULL を返します。成功時に返されたポインタは k2h_free_keyarray()関数で開放してください。
- k2h_get_direct_attrs  
  成功した場合には、取り出す属性の K2HATTRPCK構造体ポインタを返します。失敗した場合には NULL を返します。成功時に返されたポインタは k2h_free_attrpack()関数で開放してください。
- k2h_get_str_direct_attrs  
  成功した場合には、取り出す属性の K2HATTRPCK構造体ポインタを返します。失敗した場合には NULL を返します。成功時に返されたポインタは k2h_free_attrpack()関数で開放してください。

### k2h_get_trial_callback コールバック関数について
k2h_get_value\*_ext、k2h_get_direct_value\*_ext、k2h_get_str_value\*_ext、k2h_get_str_direct_value\*_ext関数は、引数にコールバック関数（k2h_get_trial_callback）を指定します。  
k2h_get_trial_callback コールバック関数は、キー（Key）を読み出した時に、その値（Value）を引数に呼び出（コールバック）されます。  
呼び出されたコールバック関数は、値（Value）を変更（上書き）することができます。  
このコールバック関数を使うことにより、特定のキー（Key）に対する値（Value）を変更するタイミングが与えられます。  
キー（Key）がK2HASHデータに存在していない場合もコールバック関数は呼び出されます。  
たとえば、キー（Key）が未設定の場合、初期値としての値（Value）を返すことができます。これは、未設定のキー（Key）の読み出しにおいて、初期値を設定するタイミングを得ることができます。  

コールバック関数は、以下のプロトタイプとなっています。
 ```
typedef K2HGETCBRES (*k2h_get_trial_callback)(const unsigned char* byKey, size_t keylen, const unsigned char* byValue, size_t vallen, unsigned char** ppNewValue, size_t* pnewvallen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData)
 ```
 
コールバック関数は、以下の引数と共に呼び出されます。
- byKey  
  \*_ext関数を呼び出したときに指定したキー（Key）のバイナリ列のポインタです。
- keylen  
  byKeyのバイナリ列長を示します。
- byValue  
  キー（Key）に対する値（Value)が存在する場合には、その値（Value）のバイナリ列のポインタです。 キー（Key）が未設定である場合には、NULL が設定されています。
- vallen  
  byValue のバイナリ列長を示します。
- ppNewValue  
  コールバック関数にて、値（Value）の上書きする（もしくはキー（Key）が未設定で値（Value）を新たに設定する）場合に、このバッファに新たな値（Value）をメモリ確保（Allocate）し、そのバイナリのポインタを設定します。
- pnewvallen  
  \*ppNewValue を設定した場合の、バイナリ列長を設定します。
- pattrs  
  キー（Key）に設定されている属性がK2HATTRPCKのポインタで渡されます。
- attrscnt  
  pattrs に含まれる属性の数が渡されます。
- pExtData  
  \*_ext関数を呼び出したときに指定した pExtData の値が設定されています。  

 
コールバック関数は以下の値を返します。
- K2HGETCB_RES_ERROR  
  コールバック関数にてエラーが発生した場合などに返します。
- K2HGETCB_RES_NOTHING  
  新たな値を設定しない（現状の値（Value）を返す）場合に、この値を返します。
- K2HGETCB_RES_OVERWRITE  
  新たな値を設定し、その新たな値を返す場合に、この値を返します。  

 
コールバック関数のサンプルを以下に示します。
 ```
static K2HGETCBRES GetTrialCallback(const unsigned char* byKey, size_t keylen, const unsigned char* byValue, size_t vallen, unsigned char** ppNewValue, size_t* pnewvallen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData)
{
    if(!byKey || 0 == keylen || !ppNewValue){
        return K2HGETCB_RES_ERROR;
    }
    //
    // Check get results which are byKey and byValue.
    // If you need to reset value for key, you set ppNewValue and return K2HGETCB_RES_OVERWRITE.
    // pExpData is the parameter when you call Get function.
    //
    return K2HGETCB_RES_NOTHING;
}
 ```

#### 注意
取り出した値、サブキーリスト、属性の領域は専用の関数を使い、領域を開放してください。
各構造体の型を以下に示します。
- K2HKEYPCK構造体  
 ```
typedef struct k2h_key_pack{
  unsigned char* pkey;
  size_t   length;
}K2HKEYPCK, *PK2HKEYPCK;
 ```

- K2HATTRPCK構造体  
 ```
typedef struct k2h_attr_pack{
    unsigned char*    pkey;
    size_t            keylength;
    unsigned char*    pval;
    size_t            vallength;
}K2HATTRPCK, *PK2HATTRPCK;
 ```

#### サンプル
 ```
char* pval;
if(NULL == (pval = k2h_get_str_direct_value(k2handle, "mykey"))){
    return false;
}
printf("KEY=mykey has VALUE=%s\n", pval);
free(pval);
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="SET"> SET関連(C I/F)
K2HASHファイル（もしくはオンメモリ）にデータを書き込む関数群です。

#### 書式
- bool k2h_set_all(k2h_h handle, const unsigned char\* pkey, size_t keylength, const unsigned char\* pval, size_t vallength, const PK2HKEYPCK pskeypck, int skeypckcnt)
- bool k2h_set_str_all(k2h_h handle, const char\* pkey, const char\* pval, const char\*\* pskeyarray)
- bool k2h_set_value(k2h_h handle, const unsigned char\* pkey, size_t keylength, const unsigned char\* pval, size_t vallength)
- bool k2h_set_str_value(k2h_h handle, const char\* pkey, const char\* pval)
- bool k2h_set_all_wa(k2h_h handle, const unsigned char\* pkey, size_t keylength, const unsigned char\* pval, size_t vallength, const PK2HKEYPCK pskeypck, int skeypckcnt, const char\* pass, const time_t\* expire)
- bool k2h_set_str_all_wa(k2h_h handle, const char\* pkey, const char\* pval, const char\*\* pskeyarray, const char\* pass, const time_t\* expire)
- bool k2h_set_value_wa(k2h_h handle, const unsigned char\* pkey, size_t keylength, const unsigned char\* pval, size_t vallength, const char\* pass, const time_t\* expire)
- bool k2h_set_str_value_wa(k2h_h handle, const char\* pkey, const char\* pval, const char\* pass, const time_t\* expire)
<br />
<br />
- bool k2h_set_subkeys(k2h_h handle, const unsigned char\* pkey, size_t keylength, const PK2HKEYPCK pskeypck, int skeypckcnt)
- bool k2h_set_str_subkeys(k2h_h handle, const char\* pkey, const char\*\* pskeyarray)
- bool k2h_add_subkey(k2h_h handle, const unsigned char\* pkey, size_t keylength, const unsigned char\* psubkey, size_t skeylength, const unsigned char\* pval, size_t vallength)
- bool k2h_add_str_subkey(k2h_h handle, const char\* pkey, const char\* psubkey, const char\* pval)
- bool k2h_add_subkey_wa(k2h_h handle, const unsigned char\* pkey, size_t keylength, const unsigned char\* psubkey, size_t skeylength, const unsigned char\* pval, size_t vallength, const char\* pass, const time_t\* expire)
- bool k2h_add_str_subkey_wa(k2h_h handle, const char\* pkey, const char\* psubkey, const char\* pval, const char\* pass, const time_t\* expire)
<br />
<br />
- bool k2h_add_attr(k2h_h handle, const unsigned char\* pkey, size_t keylength, const unsigned char\* pattrkey, size_t attrkeylength, const unsigned char\* pattrval, size_t attrvallength)
- bool k2h_add_str_attr(k2h_h handle, const char\* pkey, const char\* pattrkey, const char\* pattrval)

#### 説明
- k2h_set_all  
  キー（Key）に対して値（Value）、サブキー（Subkey）リストを設定します。キーが存在していた場合、すべて上書きされます。
- k2h_set_str_all  
  キー（Key）に対して値（Value）、サブキー（Subkey）リストを設定します。キー、値、サブキー（リスト）のすべてが文字列であることが前提です。 文字列以外の場合には使用しないでください。 キーが存在していた場合、すべて上書きされます。
- k2h_set_value  
  キー（Key）に対して値（Value）を設定します。 キーが存在していた場合、値のみ上書きされ、サブキーリストはそのまま残ります。
- k2h_set_str_value  
  キー（Key）に対して値（Value）を設定します。 キー、値共に文字列であることが前提です。 文字列以外の場合には使用しないでください。 キーが存在していた場合、値のみ上書きされ、サブキーリストはそのまま残ります。
- k2h_set_all_wa  
  k2h_set_all と同等であり、パスフレーズ、Expire時間の指定もできます。
- k2h_set_str_all_wa  
  k2h_set_str_all と同等であり、パスフレーズ、Expire時間の指定もできます。
- k2h_set_value_wa  
  k2h_set_value と同等であり、パスフレーズ、Expire時間の指定もできます。
- k2h_set_str_value_wa  
  k2h_set_str_value と同等であり、パスフレーズ、Expire時間の指定もできます。
- k2h_set_subkeys  
  キー（Key）に対してサブキー（Subkey）リストを設定します。 キーが存在していた場合、サブキーリストのみ上書きされ、値はそのまま残ります。
- k2h_set_str_subkeys  
  キー（Key）に対してサブキー（Subkey）リストを設定します。 キー、サブキー（リスト）共に文字列であることが前提です。 文字列以外の場合には使用しないでください。キーが存在していた場合、サブキーリストのみ上書きされ、値はそのまま残ります。
- k2h_add_subkey  
  キー（Key）に対してサブキー（Subkey）を追加します。 サブキー自体も値（Value）で新たに登録されます。既にサブキーが存在している場合には上書きされます。
- k2h_add_str_subkey  
  キー （Key）に対してサブキー（Subkey）を追加します。 キー、サブキー、値（サブキーの）のすべてが文字列であることが前提です。 文字列以外の場合に は使用しないでください。サブキー自体も値（Value）で新たに登録されます。既にサブキーが存在している場合には上書きされます。
- k2h_add_subkey_wa  
  k2h_add_subkey と同等であり、サブキーに対してパスフレーズ、Expire時間の指定もできます。
- k2h_add_str_subkey_wa  
  k2h_add_str_subkey と同等であり、サブキーに対してパスフレーズ、Expire時間の指定もできます。
- k2h_add_attr  
  キー （Key）に対して属性（属性名、属性値）を追加します。
- k2h_add_str_attr  
  キー （Key）に対して属性（属性名、属性値）を追加します。 キー、属性名、属性値のすべてが文字列であることが前提です。 文字列以外の場合に は使用しないでください。

#### パラメータ
- k2h_set_all
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - pval  
   値（Value）のバイナリ列を示すポインタを指定します。
 - vallength  
   値（Value）の長さを指定します。
 - pskeypck  
   サブキー（Subkey）リストを示す K2HKEYPCK構造体ポインタを指定します。
 - skeypckcnt  
   サブキー（Subkey）リストの個数を指定します。
 - pass  
   パスフレーズを指定します。
 - expire  
   Expire時間を指定します。（指定しない場合には NULL を指定してください。 Builtin属性で Expire機能が有効となっている場合には、その値で設定されます。）
- k2h_set_str_all
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）の文字列のポインタを指定します。
 - pval  
   値（Value）の文字列を示すポインタを指定します。
 - pskeyarray  
   サブキー（Subkey）リストを示す文字列配列ポインタを指定します。
 - pass  
   パスフレーズを指定します。
 - expire	
   Expire時間を指定します。（指定しない場合には NULL を指定してください。 Builtin属性で Expire機能が有効となっている場合には、その値で設定されます。）
- k2h_set_value
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - pval  
   値（Value）のバイナリ列を示すポインタを指定します。
 - vallength  
   値（Value）の長さを指定します。
 - pass  
   パスフレーズを指定します。
 - expire  
   Expire時間を指定します。（指定しない場合にはNULLを指定してください。 Builtin属性で Expire機能が有効となっている場合には、その値で設定されます。）
- k2h_set_str_value
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）の文字列のポインタを指定します。
 - pval  
   値（Value）の文字列を示すポインタを指定します。
 - pass  
   パスフレーズを指定します。
 - expire  
   Expire時間を指定します。（指定しない場合にはNULLを指定してください。 Builtin属性で Expire機能が有効となっている場合には、その値で設定されます。）
- k2h_set_subkeys
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。 
 - keylength  
   キー（Key）の長さを指定します。 
 - pskeypck  
   サブキー（Subkey）リストを示す K2HKEYPCK構造体ポインタを指定します。 
 - skeypckcnt  
   サブキー（Subkey）リストの個数を指定します。 
- k2h_set_str_subkeys
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。 
 - pkey  
   キー（Key）の文字列のポインタを指定します。 
 - pskeyarray  
   サブキー（Subkey）リストを示す文字列配列ポインタを指定します。 
- k2h_add_subkey
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。 
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。 
 - keylength  
   キー（Key）の長さを指定します。 
 - psubkey  
   追加するサブキー（Subkey）のバイナリ列のポインタを指定します。 
 - skeylength  
   追加するサブキー（Subkey）の長さを指定します。 
 - pval  
   サブキーに設定する値（Value）のバイナリ列を示すポインタを指定します。 
 - vallength  
   サブキーに設定する値（Value）の長さを指定します。 
 - pass  
   パスフレーズを指定します。
 - expire  
   Expire時間を指定します。（指定しない場合には NULL を指定してください。 Builtin属性でExpire機能が有効となっている場合には、その値で設定されます。）
- k2h_add_str_subkey
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。 
 - pkey  
   キー（Key）の文字列のポインタを指定します。 
 - psubkey  
   追加するサブキー（Subkey）の文字列のポインタを指定します。 
 - pval  
   サブキーに設定する値（Value）の文字列配列を示すポインタを指定します。 
 - pass  
   パスフレーズを指定します。
 - expire  
   Expire時間を指定します。（指定しない場合には NULL を指定してください。 Builtin属性で Expire機能が有効となっている場合には、その値で設定されます。）
- k2h_add_attr
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。 
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。 
 - keylength  
   キー（Key）の長さを指定します。 
 - pattrkey  
   追加する属性の属性名のバイナリ列のポインタを指定します。
 - attrkeylength  
   属性名の長さを指定します。
 - pattrval  
   追加する属性の属性値のバイナリ列のポインタを指定します。
 - attrvallength  
   属性値の長さを指定します。
- k2h_add_str_attr
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。 
 - pkey  
   キー（Key）の文字列のポインタを指定します。 
 - pattrkey  
   追加する属性の属性名の文字列のポインタを指定します。
 - pattrval  
   追加する属性の属性値の文字列のポインタを指定します。

#### 返り値 
成功した場合には、true を返します。失敗した場合には false を返します。

#### 注意
K2HKEYPCK構造体は、以下の構造となっています。
- K2HKEYPCK構造体  
 ```
typedef struct k2h_key_pack{
  unsigned char* pkey;
  size_t         length;
}K2HKEYPCK, *PK2HKEYPCK;
 ```

#### サンプル
 ```
if(!k2h_set_str_value(k2handle, "mykey", "myval")){
    return false;
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="RENAME"> リネーム関連(C I/F)
K2HASHファイル（もしくはオンメモリ）のデータのキー名を変更（リネーム）する関数群です。

#### 書式
- bool k2h_rename(k2h_h handle, const unsigned char\* pkey, size_t keylength, const unsigned char\* pnewkey, size_t newkeylength)
- bool k2h_rename_str(k2h_h handle, const char\* pkey, const char\* pnewkey)

#### 説明
- k2h_rename  
  キー（Key）を新しいキー名に変更します。 サブキー、属性はそのまま引き継がれ、キー名だけが変更されます。
- k2h_rename_str  
  k2h_renameと同等です。 キーは文字列であることが前提です。 文字列以外の場合には使用しないでください。


#### パラメータ
- k2h_rename
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - pnewkey  
   変更後のキー（Key）のバイナリ列のポインタを指定します。
 - newkeylength  
   変更後のキー（Key）の長さを指定します。
- k2h_rename_str
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）の文字列のポインタを指定します。
 - pnewkey  
   変更後のキー（Key）の文字列のポインタを指定します。

#### 返り値 
成功した場合には、true を返します。失敗した場合には false を返します。

#### サンプル
 ```
if(!k2h_rename_str(k2handle, "mykey", "newmykey")){
    return false;
} 
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="DIRECTBINARY"> ダイレクトバイナリデータ取得/設定関連（C I/F）
K2HASHファイル（もしくはオンメモリ）の複数のデータをHASH値等の範囲を指定して直接取得する関数群です。
本関数群は、複写（バックアップやレプリケーション）を目的として利用する特殊な関数群です。

#### 書式
- bool k2h_get_elements_by_hashbool k2h_get_elements_by_hash(k2h_h handle, const k2h_hash_t starthash, const struct timespec startts, const struct timespec endts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check, k2h_hash_t\* pnexthash, PK2HBIN\* ppbindatas, size_t\* pdatacnt)
- bool k2h_set_element_by_binary(k2h_h handle, const PK2HBIN pbindatas, const struct timespec\* pts)
- void free_k2hbin(PK2HBIN pk2hbin)
- void free_k2hbins(PK2HBIN pk2hbin, size_t count)

#### 説明
- k2h_get_elements_by_hash  
  開始HASH値、時刻範囲を指定して、指定範囲のHASH値（マスク後の開始HASH値、個数、最大HASH値）に合致する最初のデータを検出し、1つのマスク後のHASH値に紐付けられたデータ（キー、値、サブキー、属性）をバイナリ列として取得します（1つのマスクされたHASH値には複数のキー＆値が存在します）。検出後は次の開始HASH値も返します。
- k2h_set_element_by_binary  
  k2h_get_elements_by_hash を使って取り出した1つのバイナリデータ（キー、値、サブキー、属性）を K2HASHデータベースに設定します。
- free_k2hbin  
  K2HBIN構造体を開放します。
- free_k2hbins  
  K2HBIN構造体配列を開放します。

#### パラメータ
- k2h_get_elements_by_hash
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - starthash  
   検索を開始する HASH値を指定します。（0～最大値（64bit））
 - startts  
   検索対象とする K2HASHデータの最終更新時刻の範囲を指定する値であり、最小の最終更新時刻を示します（この値が有効に機能するには K2HASHにBuiltin属性として更新時刻の設定がなされている必要があります）。
 - endts  
   検索対象とする K2HASHデータの最終更新時刻の範囲を指定する値であり、最大の最終更新時刻を示します（この値が有効に機能するには K2HASHにBuiltin属性として更新時刻の設定がなされている必要があります）。
 - target_hash  
   検索対象とするHASH値の開始値を示します。 ただし、この値は対象の HASH値をtarget_max_hashで示す値で丸めた値（target_max_hashで割った剰余）に対して比較します。
 - target_max_hash  
   データのHASH値をこの値を使って丸めるための値です（検索される HASH値は、データの HASH値を target_max_hash で割った剰余を使います）。
 - old_hash  
   検索対象とするHASH値が、この値（およびtarget_hash_rangeを使った範囲）と合致する場合には、starttsとの比較を行います。 この値が不要の場合（startts の比較をしない）は、-1（=0xFFFFFFFFFFFFFFFF）を設定してください。
 - old_max_hash  
   old_hash の HASH値をこの値を使って丸めるための値です（old_hashのHASH値は、データの HASH値を old_hash で割った剰余を使います）。 old_hash を利用しない場合は、-1（=0xFFFFFFFFFFFFFFFF）を設定してください。
 - target_hash_range  
   検索対象とするHASH値の開始値（target_hash、old_hash）からの対象とするHASH値の個数を示します。
 - is_expire_check  
   検索対象のHASH値に合致するデータの有効期限を確認する場合には true を設定します（Builtin属性として更新時刻属性が有効になっていない場合には機能しません）。
 - pnexthash  
   検索後、次に検索を開始するHASH値を返します（検索結果がこれ以上存在しない場合には、0 を返します）。
 - ppbindatas  
   検出されたデータをバイナリデータ（キー、値、サブキー、属性）とし、その複数の検出結果を K2HBIN構造体配列として返します（非検出時には NULL を返します）。
 - pdatacnt  
   検出されたデータをバイナリデータ列（ppbindatas）のデータ数を返します（非検出時には 0 を返します）。
- k2h_set_element_by_binary
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pbindatas  
   k2h_get_elements_by_hashで検出された1つのバイナリデータを指定します（指定された値がそのまま K2HASH に設定されます）。
 - pts  
   指定されたバイナリデータと同じキーが存在した場合に、この指定した時刻より新しい場合には、設定をしないための値を指定します（NULL は指定できないため、常に設定する場合には現在時刻よりも未来の値を指定してください）。
- free_k2hbin
 - pk2hbin  
   開放する K2HBIN構造体ポインタを指定します。
- free_k2hbins
 - pk2hbin  
   開放する K2HBIN構造体配列のポインタを指定します。
 - count  
   開放する K2HBIN構造体配列の個数を指定します。

#### 注意
- 検索対象のHASH値は、以下のようになります。  
0x0 ～ 0xFFFFFFFFFFFFFFFの範囲でK2HASHにデータが分布していている前提において、starthash=100、target_hash=2、target_max_hash=10、target_hash_range=2 であった場合には、検索はHASH値=100 から開始されます。 そしてHASH値を、102、103、112、113、・・・と順番に検索されます。
- k2h_get_elements_by_hash関数は、主にchmpxなどの通信ミドルウエアと一緒にクラスタ構成を利用しているケースにおいて、データのオートマージを実装するために利用します。よって、old_hashなどの値は、オートマージ前のHASH値を想定しています。

#### K2HBIN構造体
 ```
typedef struct k2hash_binary_data{
    unsigned char*    byptr;
    size_t            length;
}K2HBIN, *PK2HBIN;
 ```

#### 返り値 
成功した場合には、trueを返します。失敗した場合にはfalseを返します。

#### サンプル
 ```
if(!k2h_get_elements_by_hash(k2hash, hashval, startts, endts, target_hash, target_max_hash, target_hash_range, &nexthash, &pbindatas, &datacnt)){
     return false;
}
 
if(!k2h_set_element_by_binary(k2hash, &pbindatas[cnt], &ts)){
     return false;
}
 ```

 - 注意  
この関数I/Fは、K2HASHデータの複製を行う場合などに利用するAPIであり、取り扱いには注意が必要です。

<!-- -----------------------------------------------------------　-->
***

### <a name="DELETE"> 削除関連(C I/F)
K2HASHファイル（もしくはオンメモリ）からデータを削除する関数群です。

#### 書式
- bool k2h_remove_all(k2h_h handle, const unsigned char\* pkey, size_t keylength)
- bool k2h_remove_str_all(k2h_h handle, const char\* pkey)
- bool k2h_remove(k2h_h handle, const unsigned char\* pkey, size_t keylength)
- bool k2h_remove_str(k2h_h handle, const char\* pkey)
- bool k2h_remove_subkey(k2h_h handle, const unsigned char\* pkey, size_t keylength, const unsigned char\* psubkey, size_t skeylength)
- bool k2h_remove_str_subkey(k2h_h handle, const char\* pkey, const char\* psubkey)

#### 説明
- k2h_remove_all  
  キー（Key）の削除を行います。 キーに登録されているすべてのサブキー（Subkey）も一緒に削除します。
- k2h_remove_str_all  
  キー（Key）の削除を行います。 キーは文字列であることが前提です。文字列以外の場合には使用しないでください。 キーに登録されているすべてのサブキー（Subkey）も一緒に削除します。
- k2h_remove  
  キー（Key）の削除を行います。 キーに登録されているサブキー（Subkey）は削除されず、そのまま残ります。
- k2h_remove_str  
  キー（Key）の削除を行います。 キーは文字列であることが前提です。文字列以外の場合には使用しないでください。 キーに登録されているサブキー（Subkey）は削除されず、そのまま残ります。
- k2h_remove_subkey  
  キー（Key）に登録されているサブキー（Subkey）をサブキーリストから削除し、そのサブキー自体も削除します。
- k2h_remove_str_subkey  
  キー（Key）に登録されているサブキー（Subkey）をサブキーリストから削除し、そのサブキー自体も削除します。 キー、サブキー共に文字列であることが前提です。文字列以外の場合には使用しないでください。

#### パラメータ 
- k2h_remove_all
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
- k2h_remove_str_all
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）の文字列のポインタを指定します。
- k2h_remove
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
- k2h_remove_str
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）の文字列のポインタを指定します。
- k2h_remove_subkey
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
 - psubkey  
   削除するサブキー（Subkey）のバイナリ列のポインタを指定します。
 - skeylength  
   削除するサブキー（Subkey）の長さを指定します。
- k2h_remove_str_subkey
 - handle  
   k2h_open系の関数から返されたK2HASHハンドルを指定します。
 - pkey  
   キー（Key）の文字列のポインタを指定します。
 - psubkey
   削除するサブキー（Subkey）の文字列のポインタを指定します。

#### 返り値 
成功した場合には、true を返します。失敗した場合には false を返します。

#### 注意
k2h_remove_subkey、k2h_remove_str_subkey は、キーが指定されたサブキーを持っていない場合には失敗します。 

#### サンプル
 ```
if(!k2h_remove_str_all(k2handle, "mykey")){
    return false;
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="FIND"> 探索関連(C I/F)
K2HASHファイル（もしくはオンメモリ）からデータを探索する関数群です。

#### 書式
- k2h_find_h k2h_find_first(k2h_h handle)
- k2h_find_h k2h_find_first_subkey(k2h_h handle, const unsigned char\* pkey, size_t keylength)
- k2h_find_h k2h_find_first_str_subkey(k2h_h handle, const char\* pkey)
- k2h_find_h k2h_find_next(k2h_find_h findhandle)
- bool k2h_find_free(k2h_find_h findhandle)
- bool k2h_find_get_key(k2h_find_h findhandle, unsigned char\*\* ppkey, size_t\* pkeylength)
- char\* k2h_find_get_str_key(k2h_find_h findhandle)
- bool k2h_find_get_value(k2h_find_h findhandle, unsigned char\*\* ppval, size_t\* pvallength)
- char\* k2h_find_get_direct_value(k2h_find_h findhandle)
- bool k2h_find_get_subkeys(k2h_find_h findhandle, PK2HKEYPCK\* ppskeypck, int\* pskeypckcnt)
- PK2HKEYPCK k2h_find_get_direct_subkeys(k2h_find_h findhandle, int\* pskeypckcnt)
- int k2h_find_get_str_subkeys(k2h_find_h findhandle, char\*\*\* ppskeyarray)
- char\*\* k2h_find_get_str_direct_subkeys(k2h_find_h findhandle)

#### 説明
- k2h_find_first  
  全K2HASHデータのキー探索用のハンドルを取得します。取得したハンドルは、k2h_find_free()関数で開放してください。
- k2h_find_first_subkey  
  キー（Key）に登録されているサブキー探索用のハンドルを取得します。
- k2h_find_first_str_subkey  
  キー（Key）に登録されているサブキー探索用のハンドルを取得します。 キーが文字列であることが前提です。文字列以外の場合には使用しないでください。
- k2h_find_next  
  探索を行い、次を示す探索用ハンドルを返します。
- k2h_find_free  
  探索ハンドルを開放します。
- k2h_find_get_key  
  探索用ハンドルが示す K2HASHデータのキー（Key）を取得します。 取得したキーは free()関数で開放してください。
- k2h_find_get_str_key  
  探索用ハンドルが示す K2HASHデータのキー（Key）を文字列として取得します。 キーが文字列でないときは使用しないでください。 取得したキーは free()関数で開放してください。
- k2h_find_get_value  
  探索用ハンドルが示す K2HASHデータの値（Value）を取得します。 取得した値は free()関数で開放してください。
- k2h_find_get_direct_value  
  探索用ハンドルが示す K2HASHデータの値（Value）を文字列として取得します。 値が文字列でないときは使用しないでください。 取得した値はfree()関数で開放してください。
- k2h_find_get_subkeys  
  探索用ハンドルが示す K2HASHデータのサブキー（Subkey）リストを取得します。 取得したサブキーリストは k2h_free_keypack()関数 で開放してください。
- k2h_find_get_direct_subkeys  
  探索用ハンドルが示す K2HASHデータのサブキー（Subkey）リストを取得します。 取得したサブキーリストは k2h_free_keypack()関数 で開放してください。
- k2h_find_get_str_subkeys  
  探索用ハンドルが示す K2HASHデータのサブキー（Subkey）リストを取得します。 サブキーが文字列ではない場合には使用しないでください。 取得したサブキーリストは k2h_free_keyarray()関数で開放してください。
- k2h_find_get_str_direct_subkeys  
  探索用ハンドルが示す K2HASHデータのサブキー（Subkey）リストを取得します。 サブキーが文字列ではない場合には使用しないでください。 取得したサブキーリストは k2h_free_keyarray()関数で開放してください。  

 
#### パラメータ
- k2h_find_first
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
- k2h_find_first_subkey
 - handle
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）のバイナリ列のポインタを指定します。
 - keylength  
   キー（Key）の長さを指定します。
- k2h_find_first_str_subkey
 - handle  
   k2h_open系の関数から返された K2HASHハンドルを指定します。
 - pkey  
   キー（Key）の文字列のポインタを指定します。
- k2h_find_next
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。
- k2h_find_free
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。
- k2h_find_get_key
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。
 - ppkey  
   取り出したキー（Key）のバイナリ列を保存するポインタを指定します。 このポインタに設定される（返される）領域は、free()関数で開放してください。
 - pkeylength  
   取り出したキー（Key）のバイナリ列の長さを返すポインタを指定します。
- k2h_find_get_str_key
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。
- k2h_find_get_value
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。
 - ppval  
   取り出した値（Value）のバイナリ列を保存するポインタを指定します。 このポインタに設定される（返される）領域は、free()関数で開放してください。
 - pvallength  
   取り出した値（Value）のバイナリ列の長さを返すポインタを指定します。
- k2h_find_get_direct_value
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。
- k2h_find_get_subkeys
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。
 - ppskeypck  
   取り出したサブキー（Subkey）リストの K2HKEYPCK構造体配列を保存するポインタを指定します。 このポインタに設定される（返される）領域は、 k2h_free_keypack()関数で開放してください。
 - pskeypckcnt  
   取り出したサブキー（Subkey）リストの K2HKEYPCK構造体配列の個数を返すポインタを指定します。
- k2h_find_get_direct_subkeys
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。
 - pskeypckcnt  
   取り出したサブキー（Subkey）リストの K2HKEYPCK構造体配列の個数を返すポインタを指定します。
- k2h_find_get_str_subkeys
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。
 - ppskeyarray  
   取り出したサブキー（Subkey）リストの文字列配列（NULL終端）を保存するポインタを指定します。 このポインタに設定される（返される）領域は、k2h_free_keyarray()関数で開放してください。文字列配列は終端がNULLとなっています。
- k2h_find_get_str_direct_subkeys
 - findhandle  
   k2h_find_first系の関数から返された探索ハンドルを指定します。

#### 返り値
- k2h_find_first  
  成功した場合には、探索ハンドルを指定します。 失敗した場合には、K2H_INVALID_HANDLE(0) を返します。 取得した探索ハンドルは k2h_find_free()関数で開放してください。
- k2h_find_first_subkey  
  成功した場合には、探索ハンドルを指定します。 失敗した場合には、K2H_INVALID_HANDLE(0) を返します。 取得した探索ハンドルは k2h_find_free()関数で開放してください。
- k2h_find_first_str_subkey  
  成功した場合には、探索ハンドルを指定します。 失敗した場合には、K2H_INVALID_HANDLE(0) を返します。 取得した探索ハンドルは k2h_find_free()関数で開放してください。
- k2h_find_next  
  成功した場合には、探索ハンドルを指定します。 失敗した場合には、K2H_INVALID_HANDLE(0) を返します。 取得した探索ハンドルは k2h_find_free()関数で開放してください。
- k2h_find_free  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_find_get_key  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_find_get_value  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_find_get_subkeys  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_find_get_str_key  
  成功した場合は、キー（Key）の文字列ポインタを返します。 失敗した場合は NULL を返します。 返された文字列ポインタは free()関数で開放してください。
- k2h_find_get_direct_value  
  成功した場合は、値（Value）の文字列ポインタを返します。 失敗した場合は NULL を返します。 返された文字列ポインタは free()関数で開放してください。
- k2h_find_get_direct_subkeys  
  成功した場合は、サブキー（Subkey）リストの K2HKEYPCK構造体配列へのポインタを返します。 失敗した場合は NULL を返します。返されたポインタは k2h_free_keypack()関数で開放してください。
- k2h_find_get_str_subkeys  
  成功した場合は、サブキー（Sunkey）リストのポインタ ppskeyarray の示す配列個数を返します。 失敗した場合には、-1を返します。
- k2h_find_get_str_direct_subkeys  
  成功した場合は、サブキー（Sunkey）リストのK2HKEYPCK構造体配列のポインタを返します。失敗した場合にはNULLを返します。返されたポインタは k2h_free_keyarray()関数で開放してください。

#### 注意
有効な探索ハンドル（k2h_find_h）を保持している期間（k2h_find_free により開放されるまでの期間）は、探索ハンドルが指し示しているK2HASHデータに対して読み取りのためのロックが設定されています。
ロックが設定されている期間は、そのキーの読み出し、書き込みがブロックされます。
よって、長期間この探索ハンドルを保持しないことを推奨します。
特に、探索ハンドルをキャッシュしたり、プログラム内部で長期間保持しないようにするべきです。

#### サンプル
 ```
// Full dump
for(k2h_find_h fhandle = k2h_find_first(k2handle); K2H_INIVALID_HANDLE != fhandle; fhandle = k2h_find_next(fhandle)){
    char*    pkey = k2h_find_get_str_key(fhandle);
    char*    pval = k2h_find_get_direct_value(fhandle);
    printf("KEY=%s  --> VAL=%s\n", pkey ? pkey : "null", pval ? pval : "null");
    if(pkey){
        free(pkey);
    }
    if(pval){
        free(pval);
    }
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="DIRECTACCESS"> ダイレクトアクセス関連(C I/F)
K2HASHファイル（もしくはオンメモリ）のデータへ直接アクセスする関数群です。
主に大きなサイズの値（Value）を持つデータへの読み書きを行う場合などに利用します。オフセットを指定して値（Value）の一部分の上書きや読み出しができます。

#### 書式
- k2h_da_h k2h_da_handle(k2h_h handle, const unsigned char\* pkey, size_t keylength, K2HDAMODE mode)
- k2h_da_h k2h_da_handle_read(k2h_h handle, const unsigned char\* pkey, size_t keylength)
- k2h_da_h k2h_da_handle_write(k2h_h handle, const unsigned char\* pkey, size_t keylength)
- k2h_da_h k2h_da_handle_rw(k2h_h handle, const unsigned char\* pkey, size_t keylength)
- k2h_da_h k2h_da_str_handle(k2h_h handle, const char\* pkey, K2HDAMODE mode)
- k2h_da_h k2h_da_str_handle_read(k2h_h handle, const char\* pkey)
- k2h_da_h k2h_da_str_handle_write(k2h_h handle, const char\* pkey)
- k2h_da_h k2h_da_str_handle_rw(k2h_h handle, const char\* pkey)
- bool k2h_da_free(k2h_da_h dahandle)
<br />
<br />
- ssize_t k2h_da_get_length(k2h_da_h dahandle)
- ssize_t k2h_da_get_buf_size(k2h_da_h dahandle)
- bool k2h_da_set_buf_size(k2h_da_h dahandle, size_t bufsize)
<br />
<br />
- off_t k2h_da_get_offset(k2h_da_h dahandle, bool is_read)
- off_t k2h_da_get_read_offset(k2h_da_h dahandle)
- off_t k2h_da_get_write_offset(k2h_da_h dahandle)
- bool k2h_da_set_offset(k2h_da_h dahandle, off_t offset, bool is_read)
- bool k2h_da_set_read_offset(k2h_da_h dahandle, off_t offset)
- bool k2h_da_set_write_offset(k2h_da_h dahandle, off_t offset)
<br />
<br />
- bool k2h_da_get_value(k2h_da_h dahandle, unsigned char\*\* ppval, size_t\* pvallength)
- bool k2h_da_get_value_offset(k2h_da_h dahandle, unsigned char\*\* ppval, size_t\* pvallength, off_t offset)
- bool k2h_da_get_value_to_file(k2h_da_h dahandle, int fd, size_t\* pvallength)
- unsigned char\* k2h_da_read(k2h_da_h dahandle, size_t\* pvallength)
- unsigned char\* k2h_da_read_offset(k2h_da_h dahandle, size_t\* pvallength, off_t offset)
- char\* k2h_da_read_str(k2h_da_h dahandle)
<br />
<br />
- bool k2h_da_set_value(k2h_da_h dahandle, const unsigned char\* pval, size_t vallength)
- bool k2h_da_set_value_offset(k2h_da_h dahandle, const unsigned char\* pval, size_t vallength, off_t offset)
- bool k2h_da_set_value_from_file(k2h_da_h dahandle, int fd, size_t\* pvallength)
- bool k2h_da_set_value_str(k2h_da_h dahandle, const char\* pval)

#### 説明
- k2h_da_handle  
  Keyを指定し、そのKeyの値（Value)への直接アクセスをするためのハンドル（k2h_da_h）を取得します
- k2h_da_handle_read  
  読み出し専用のハンドル（k2h_da_h）を取得します。
- k2h_da_handle_write  
  書き込み専用のハンドル（k2h_da_h）を取得します。
- k2h_da_handle_rw  
  読み書き可能なハンドル（k2h_da_h）を取得します。
- k2h_da_str_handle  
  Key文字列の指定、およびハンドル（k2h_da_h）を取得します
- k2h_da_str_handle_read  
  Key文字列の指定、および読み込み専用のハンドル（k2h_da_h）を取得します。
- k2h_da_str_handle_write  
  Key文字列の指定、および書き込み専用のハンドル（k2h_da_h）を取得します。
- k2h_da_str_handle_rw  
  Key文字列の指定、および読み書き可能なハンドル（k2h_da_h）を取得します。
- k2h_da_free  
  ハンドル（k2h_da_h）を開放します。
- k2h_da_get_length  
  値（Value）の長さを取得します。
- k2h_da_get_buf_size  
  任意のファイルからデータを読み書きするための内部バッファサイズを取得します。
- k2h_da_set_buf_size  
  任意のファイルからデータを読み書きするための内部バッファサイズを設定します。
- k2h_da_get_offset  
  ダイレクト読み書きをする開始オフセットを取得します。
- k2h_da_get_read_offset  
  ダイレクト読み込みをする開始オフセットを取得します。
- k2h_da_get_write_offset  
  ダイレクト書き込みをする開始オフセットを取得します。
- k2h_da_set_offset  
  ダイレクト読み書きをする開始オフセットを設定します。
- k2h_da_set_read_offset  
  ダイレクト読み込みをする開始オフセットを設定します。
- k2h_da_set_write_offset  
  ダイレクト書き込みをする開始オフセットを設定します。
- k2h_da_get_value  
  設定されているオフセットから値（Value）を読み出します。
- k2h_da_get_value_offset  
  オフセットを指定して値（Value）を読み出します。
- k2h_da_get_value_to_file  
  設定されているオフセットから値（Value）を読み出し、指定ファイルに書き出します。
- k2h_da_read  
  設定されているオフセットから値（Value）を読み出し、返り値で返します。
- k2h_da_read_offset  
  オフセットを指定して値（Value）を読み出し、返り値で返します。
- k2h_da_read_str  
  設定されているオフセットから値（Value）を読み出し、文字列として返り値で返します。
- k2h_da_set_value  
  設定されているオフセットから値（Value）を書き出します。
- k2h_da_set_value_offset  
  オフセットを指定して値（Value）を書き出します。
- k2h_da_set_value_from_file  
  指定ファイルから読み出した値を、設定されているオフセットから値（Value）に書き出します。
- k2h_da_set_value_str  
  設定されているオフセットから文字列値（Value）を書き出します。

#### パラメータ
- k2h_da_handle
 - handle  
   k2h_hハンドルを指定します。
 - pkey  
   Key値を指定します。
 - keylength  
   Key値の長さを指定します。
 - mode  
   取得する k2h_da_h ハンドルのモードを指定します。 モードは K2H_DA_READ（読み出し専用）、K2H_DA_WRITE（書き込み専用）、K2H_DA_RW（読み書き両用）を指定します。
- k2h_da_handle_read 
 - handle  
   k2h_hハンドルを指定します。
 - pkey  
   Key値を指定します。
 - keylength  
   Key値の長さを指定します。
- k2h_da_handle_write
 - handle  
   k2h_hハンドルを指定します。
 - pkey  
   Key値を指定します。
 - keylength  
   Key値の長さを指定します。
- k2h_da_handle_rw
 - handle  
   k2h_hハンドルを指定します。
 - pkey  
   Key値を指定します。
 - keylength  
   Key値の長さを指定します。
- k2h_da_str_handle
 - handle  
   k2h_hハンドルを指定します。
 - pkey  
   Key値を指定します。
 - mode  
   取得する k2h_da_hハンドルのモードを指定します。 モードは K2H_DA_READ（読み出し専用）、K2H_DA_WRITE（書き込み専用）、K2H_DA_RW（読み書き両用）を指定します。
- k2h_da_str_handle_read
 - handle  
   k2h_hハンドルを指定します。
 - pkey  
   Key値を指定します。
- k2h_da_str_handle_write
 - handle  
   k2h_hハンドルを指定します。
 - pkey  
   Key値を指定します。
- k2h_da_str_handle_rw
 - handle  
   k2h_hハンドルを指定します。
 - pkey  
   Key値を指定します。
- k2h_da_free
 - handle  
   k2h_hハンドルを指定します。
- k2h_da_get_length
 - handle  
   k2h_hハンドルを指定します。
- k2h_da_get_buf_size	
 - handle  
   k2h_hハンドルを指定します。
- k2h_da_set_buf_size	
 - handle  
   k2h_hハンドルを指定します。
 - bufsize  
   内部バッファサイズを指定します。
- k2h_da_get_offset
 - handle  
   k2h_hハンドルを指定します。
 - is_read  
   読み出し用のオフセットを取り出す場合には true を指定します。
- k2h_da_get_read_offset
 - handle  
   k2h_hハンドルを指定します。
- k2h_da_get_write_offset
 - handle  
   k2h_hハンドルを指定します。
- k2h_da_set_offset
 - handle  
   k2h_hハンドルを指定します。
 - offset  
   設定するオフセット値を指定します。
 - is_read  
   読み出し用のオフセットを取り出す場合には true を指定します。
- k2h_da_set_read_offset
 - handle  
   k2h_hハンドルを指定します。
 - offset  
   設定するオフセット値を指定します。
- k2h_da_set_write_offset
 - handle  
   k2h_hハンドルを指定します。
 - offset  
   設定するオフセット値を指定します。
- k2h_da_get_value
 - dahandle  
   k2h_hハンドルを指定します。
 - ppval  
   読み出した値を格納するポインタバッファを指定します。 成功し動的に確保されたメモリへのポインタが設定されます。 このポインタは free()関数で開放してください。
 - pvallength  
   読み出す長さを指定します。 成功した場合には、読み出したバイト数が設定されます。
- k2h_da_get_value_offset
 - dahandle  
   k2h_hハンドルを指定します。
 - ppval  
   読み出した値を格納するポインタバッファを指定します。 成功し動的に確保されたメモリへのポインタが設定されます。 このポインタは free()関数で開放してください。
 - pvallength  
   読み出す長さを指定します。成功した場合には、読み出したバイト数が設定されます。
 - offset  
   読み出し開始のオフセットを指定します。
- k2h_da_get_value_to_file
 - dahandle  
   k2h_hハンドルを指定します。
 - fd  
   読み出した値を出力するファイルディスクリプタを指定します。
 - pvallength  
   読み出す長さを指定します。 成功した場合には、読み出したバイト数が設定されます。
- k2h_da_read
 - dahandle  
   k2h_hハンドルを指定します
 - pvallength  
   読み出す長さを指定します。 成功した場合には、読み出したバイト数が設定されます。
- k2h_da_read_offset
 - dahandle  
   k2h_hハンドルを指定します
 - pvallength  
   読み出す長さを指定します。 成功した場合には、読み出したバイト数が設定されます。
 - offset  
   読み出し開始のオフセットを指定します。
- k2h_da_read_str
 - dahandle  
   k2h_hハンドルを指定します。
- k2h_da_set_value
 - dahandle  
   k2h_hハンドルを指定します。
 - pval  
   書き込むデータへのポインタを指定します。
 - vallength  
   書き込むデータへ長さを指定します。
- k2h_da_set_value_offset
 - dahandle  
   k2h_hハンドルを指定します。
 - pval  
   書き込むデータへのポインタを指定します。
 - vallength  
   書き込むデータへ長さを指定します。
 - offset  
   書き込み開始のオフセットを指定します。
- k2h_da_set_value_from_file
 - dahandle  
   k2h_hハンドルを指定します。
 - fd  
   書き込むデータのファイルディスクリプタを指定します。
 - pvallength  
   書き込むデータ長を指定し、成功した場合には書き込んだバイト数を返します。
- k2h_da_set_value_str	
 - dahandle  
    - k2h_h ハンドルを指定します。
 - pval  
    - 書き込む文字列へのポインタを指定します。

#### 返り値
- k2h_da_handle  
  ダイレクトアクセス用のハンドル（k2h_da_h）を返します。失敗時には、K2H_INVALID_HANDLE(0) を返します。
- k2h_da_handle_read  
  ダイレクトアクセス用のハンドル（k2h_da_h）を返します。失敗時には、K2H_INVALID_HANDLE(0) を返します。
- k2h_da_handle_write  
  ダイレクトアクセス用のハンドル（k2h_da_h）を返します。失敗時には、K2H_INVALID_HANDLE(0) を返します。
- k2h_da_handle_rw  
  ダイレクトアクセス用のハンドル（k2h_da_h）を返します。失敗時には、K2H_INVALID_HANDLE(0) を返します。
- k2h_da_str_handle  
  ダイレクトアクセス用のハンドル（k2h_da_h）を返します。失敗時には、K2H_INVALID_HANDLE(0) を返します。
- k2h_da_str_handle_read  
  ダイレクトアクセス用のハンドル（k2h_da_h）を返します。失敗時には、K2H_INVALID_HANDLE(0) を返します。
- k2h_da_str_handle_write  
  ダイレクトアクセス用のハンドル（k2h_da_h）を返します。失敗時には、K2H_INVALID_HANDLE(0) を返します。
- k2h_da_str_handle_rw  
  ダイレクトアクセス用のハンドル（k2h_da_h）を返します。失敗時には、K2H_INVALID_HANDLE(0) を返します。
- k2h_da_free  
  成功時には true、失敗時には false を返します。
- k2h_da_set_buf_size  
  成功時には true、失敗時には false を返します。
- k2h_da_set_offset  
  成功時には true、失敗時には false を返します。
- k2h_da_set_read_offset  
  成功時には true、失敗時には false を返します。
- k2h_da_set_write_offset  
  成功時には true、失敗時には false を返します。
- k2h_da_get_value  
  成功時には true、失敗時には false を返します。
- k2h_da_get_value_offset  
  成功時には true、失敗時には false を返します。
- k2h_da_get_value_to_file  
  成功時には true、失敗時には false を返します。
- k2h_da_set_value  
  成功時には true、失敗時には false を返します。
- k2h_da_set_value_offset  
  成功時には true、失敗時には false を返します。
- k2h_da_set_value_from_file  
  成功時には true、失敗時には false を返します。
- k2h_da_set_value_str  
  成功時には true、失敗時には false を返します。
- k2h_da_get_length  
  値（Value）の長さを返します。 エラーが発生した場合には、-1 を返します。
- k2h_da_get_buf_size  
  内部バッファの長さを返します。 エラーが発生した場合には、-1 を返します。
- k2h_da_get_offset  
  現在設定されている読み出しもしくは書き込み位置のオフセットを返します。 エラーが発生した場合には、-1 を返します。
- k2h_da_get_read_offset  
  現在設定されている読み出し位置のオフセットを返します。 エラーが発生した場合には、-1 を返します。
- k2h_da_get_write_offset  
  現在設定されている書き込み位置のオフセットを返します。 エラーが発生した場合には、-1 を返します。
- k2h_da_read  
  読み出した値（Value）をメモリ確保した領域で返します。 エラー時には NULL を返します。 返されたポインタは、free()関数で開放してください。
- k2h_da_read_offset  
  読み出した値（Value）をメモリ確保した領域で返します。 エラー時には NULL を返します。 返されたポインタは、free()関数で開放してください。
- k2h_da_read_str  
  読み出した値（Value）をメモリ確保した領域（文字列ポインタ）で返します。エラー時には NULL を返します。 返されたポインタは、free()関数で開放してください。

#### 注意
- ダイレクトアクセスのハンドル（k2h_da_h）を取得した場合、そのキー（Key）に対する変更はロックされます。  
  特に書き込み用のハンドルを取得した場合、書き込み用にロックがされており、読み出しもブロックされます。  
  不要な書き込みモードでのハンドル取得は避け、利用後は即座にハンドル（k2h_da_h）を開放することを推奨します。
- 書き込み前に存在しない領域（オフセットからの）への書き込みを行った場合には、オフセット以前の領域の値は不定となります。
- 暗号化されたキーに対して、本関数群は使用できません。  
  本関数で操作する値は、復号前のデータとなりますので、暗号化されたデータが破壊されることになりますので、注意してください。  
  また、新規作成する場合には、暗号化することはできません。  
  暗号化を行うキーに対しては、ダイレクトアクセスできませんので、他の関数（SET系関数）を利用してください。

#### サンプル
 ```
// get handle
k2h_da_h    dahandle;
if(K2H_INIVALID_HANDLE == (dahandle = k2h_da_str_handle_write(k2handle, "mykey"))){
    fprintf(stderr, "Could not get k2h_da_h handle.");
    return false;
}
// offset
if(!k2h_da_set_write_offset(dahandle, 100)){
    fprintf(stderr, "Could not set write offset.");
    k2h_da_free(dahandle);
    return false;
}
// write
if(!k2h_da_set_value_str(dahandle, "test data")){
    fprintf(stderr, "Failed writing value.");
    k2h_da_free(dahandle);
    return false;
}
k2h_da_free(dahandle);
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="QUE"> キュー関連(C I/F)
K2HASHライブラリは、キュー（FIFO/LIFO）としての機能を提供します。
キューは、値（Value）をFIFO/LIFOとして蓄積（PUSH）し、読み出し（POP）できます。
本関数群は、キューに関連する関数群です。

#### 書式
- k2h_q_h k2h_q_handle(k2h_h handle, bool is_fifo)
- k2h_q_h k2h_q_handle_prefix(k2h_h handle, bool is_fifo, const unsigned char\* pref, size_t preflen)
- k2h_q_h k2h_q_handle_str_prefix(k2h_h handle, bool is_fifo, const char\* pref)
- bool k2h_q_free(k2h_q_h qhandle)
- bool k2h_q_empty(k2h_q_h qhandle)
- int k2h_q_count(k2h_q_h qhandle)
- bool k2h_q_read(k2h_q_h qhandle, unsigned char\*\* ppdata, size_t\* pdatalen, int pos)
- bool k2h_q_str_read(k2h_q_h qhandle, char\*\* ppdata, int pos)
- bool k2h_q_read_wp(k2h_q_h qhandle, unsigned char\*\* ppdata, size_t\* pdatalen, int pos, const char\* encpass)
- bool k2h_q_str_read_wp(k2h_q_h qhandle, char\*\* ppdata, int pos, const char\* encpass)
- bool k2h_q_push(k2h_q_h qhandle, const unsigned char\* byval, size_t vallen)
- bool k2h_q_str_push(k2h_q_h qhandle, const char\* pval)
- bool k2h_q_push_wa(k2h_q_h qhandle, const unsigned char\* bydata, size_t datalen, const PK2HATTRPCK pattrspck, int attrspckcnt, const char\* encpass, const time_t\* expire)
- bool k2h_q_str_push_wa(k2h_q_h qhandle, const char\* pdata, const PK2HATTRPCK pattrspck, int attrspckcnt, const char\* encpass, const time_t\* expire)
- bool k2h_q_pop(k2h_q_h qhandle, unsigned char\*\* ppval, size_t\* pvallen)
- bool k2h_q_str_pop(k2h_q_h qhandle, char\*\* ppval)
- bool k2h_q_pop_wa(k2h_q_h qhandle, unsigned char\*\* ppdata, size_t\* pdatalen, PK2HATTRPCK\* ppattrspck, int\* pattrspckcnt, const char\* encpass)
- bool k2h_q_str_pop_wa(k2h_q_h qhandle, char\*\* ppdata, PK2HATTRPCK\* ppattrspck, int\* pattrspckcnt, const char\* encpass)
- bool k2h_q_pop_wp(k2h_q_h qhandle, unsigned char\*\* ppdata, size_t\* pdatalen, const char\* encpass)
- bool k2h_q_str_pop_wp(k2h_q_h qhandle, char\*\* ppdata, const char\* encpass)
- bool k2h_q_remove(k2h_q_h qhandle, int count)
- bool k2h_q_remove_wp(k2h_q_h qhandle, int count, const char\* encpass)
- int k2h_q_remove_ext(k2h_q_h qhandle, int count, k2h_q_remove_trial_callback fp, void\* pextdata)
- int k2h_q_remove_wp_ext(k2h_q_h qhandle, int count, k2h_q_remove_trial_callback fp, void\* pextdata, const char\* encpass)
- bool k2h_q_dump(k2h_q_h qhandle, FILE\* stream)

#### 説明
- k2h_q_handle  
  FIFO または LIFO のキューへのハンドルを取得します。
- k2h_q_handle_prefix  
  FIFO または LIFO のキューへのハンドルを取得します。 取得時にキューで使用される内部キー名におけるプレフィックスを指定することができます。
- k2h_q_handle_str_prefix  
  k2h_q_handle_prefixへのラッパ関数です。プレフィックスを文字列として指定することができます。
- k2h_q_free  
  キューへのハンドルを破棄します。
- k2h_q_empty  
  キューが空であるか調べます。
- k2h_q_count  
  キューに蓄積されているデータ数を取得します（注意参照）。
- k2h_q_read  
  キューから値をコピーします。 値はキューに残ったままとなります。 取得した値へのポインタは開放してください（注意参照）。
- k2h_q_str_read  
  k2h_q_readへのラッパ関数です。 値を文字列として取得することができます。 取得した値へのポインタは開放してください（注意参照）。
- k2h_q_read_wp  
  k2h_q_readと同等です。 暗号化されたキューを読み出すための復号のためのパスフレーズを指定できます。
- k2h_q_str_read_wp  
  k2h_q_str_readと同等です。 暗号化されたキューを読み出すための復号のためのパスフレーズを指定できます。
- k2h_q_push  
  キューに値を蓄積（push）します。 キューにのみpushします。
- k2h_q_str_push  
  k2h_q_pushへのラッパ関数です。 値を文字列として指定することができます。
- k2h_q_push_wa  
  k2h_q_pushと同等です。 Builtin属性を K2HATTRPCKポインタで指定することができます。 またパスフレーズ、Expire時間の指定ができます。  
  K2HATTRPCKポインタとパスフレーズ、Expire時間が指定されている場合には、K2HATTRPCKポインタで指定されている暗号化と Expire時間は指定されたパスフレーズ、Expire時間で上書きされます。   
  K2HATTRPCKポインタは主にトランザクション処理などで一旦キューから取り出したデータをキューに再pushするときに利用されることを想定しています。  
  これにより、最初にキューに登録された Expire時間を継続して維持することができトランザクションデータの再送を Expire することができるようになります。
- k2h_q_str_push_wa  
  k2h_q_str_push と同等です。 Builtin属性の引数については、k2h_q_push_wa と同等です。
- k2h_q_pop  
  キューから値を取り出し（pop）します。 pop した値はキューから削除されます。 キューからのみ削除します。 取得した値へのポインタは開放してください。
- k2h_q_str_pop  
  k2h_q_pop へのラッパ関数です。 値を文字列として取得することができます。 取得した値へのポインタは開放してください。
- k2h_q_pop_wa  
  k2h_q_pop と同等です。 パスフレーズを指定して復号化することができます。 またキューからデータを取り出す際に設定されているBuiltin属性も取り出すことができます。  
  この取り出したBuiltin属性は、キューに再登録する場合に k2h_q_push_wa の引数に引き渡すことにより、トランザクションなどを最初に登録された時刻からの Expire として動作させすことができます。
- k2h_q_str_pop_wa  
  k2h_q_str_pop と同等です。 パスフレーズ、Builtin属性に関しては、k2h_q_pop_wa と同じです。
- k2h_q_pop_wp  
  k2h_q_pop と同等です。 パスフレーズを指定して復号化することができます。
- k2h_q_str_pop_wp  
  k2h_q_str_pop と同等です。 パスフレーズを指定して復号化することができます。
- k2h_q_remove  
  キューに蓄積されている値を指定個数分削除します。 削除された値は返されません。 蓄積された数以上を指定してもエラーとはならず、キューの中身が空になり、正常終了します。
- k2h_q_remove_wp  
  k2h_q_remove と同等です。 暗号化されたキューに対してパスフレーズを指定できます。
- k2h_q_remove_ext  
  k2h_q_remove と同様にキューに蓄積されている値を、指定個数分削除します。 この関数にはコールバック関数（後述）を指定します。キューから値の削除毎にコールバック関数が呼び出され、値の削除の判定を独自に行うことができます。
- k2h_q_remove_wp_ext  
  k2h_q_remove_ext と同等です。暗号化されたキューに対してパスフレーズを指定できます。
- k2h_q_dump  
  キューのデバッグ用の関数であり、キュー内部をダンプします。

#### パラメータ
- k2h_q_handle	
 - handle  
   K2HASHハンドルを指定します。
 - is_fifo  
   取得するキューのタイプ（FIFO または LIFO）を指定します。
- k2h_q_handle_prefix	
 - handle  
   K2HASHハンドルを指定します。
 - is_fifo  
   取得するキューのタイプ（FIFO または LIFO）を指定します。
 - pref  
   キューとして使用する内部キー名のプレフィックスを指定します（バイナリ列）。
 - preflen  
   pref のバッファ長を指定します。
- k2h_q_handle_str_prefix
 - handle  
   K2HASHハンドルを指定します。
 - is_fifo  
   取得するキューのタイプ（FIFO または LIFO）を指定します。
 - pref  
   キューとして使用する内部キー名のプレフィックスを指定します（文字列であり、\0終端です）。
- k2h_q_free
 - qhandle  
   キューのハンドルを指定します。
- k2h_q_empty
 - qhandle  
   キューのハンドルを指定します。
- k2h_q_count
 - qhandle   
   キューのハンドルを指定します。
- k2h_q_read
 - qhandle  
   キューのハンドルを指定します。
 - ppdata  
   キューからコピーする値を格納するバッファへのポインタを指定します（バイナリ列）。 返されたポインタは開放してください。
 - pdatalen  
   ppdata のバッファ長が返されます。
 - pos  
   コピーするデータのキューの先頭からの位置を指定します（先頭が 0 となります）。
 - encpass  
   パスフレーズを指定します。
- k2h_q_str_read
 - qhandle  
   キューのハンドルを指定します。
 - ppdata  
   キューからコピーする値を格納するバッファへのポインタを指定します（文字列）。 返されたポインタは開放してください。
 - pos  
   コピーするデータのキューの先頭からの位置を指定します（先頭が 0 となります）。
 - encpass  
   パスフレーズを指定します。
- k2h_q_push	
 - qhandle  
   キューのハンドルを指定します。
 - byval  
   キューに蓄積（push）する値を指定します（バイナリ列）。
 - vallen  
   byvalのバッファ長を指定します。
 - pattrspck  
   Builtin属性およびPlugin属性の情報 K2HATTRPCKポインタを指定します。 この属性情報は主に k2h_q_pop_wa で返されたポインタを指定することを想定しています。
 - attrspckcnt  
   Builtin属性およびPlugin属性情報の属性数を指定します。
 - encpass  
   パスフレーズを指定します。
 - expire  
   Expire時間を指定します。 指定しない場合には NULL を指定します。
- k2h_q_str_push
 - qhandle  
   キューのハンドルを指定します。
 - pval  
   キューに蓄積（push）する値を指定します。（文字列であり、\0 終端です）
 - pattrspck  
   Builtin属性およびPlugin属性情報の K2HATTRPCKポインタを指定します。 この属性情報は主に k2h_q_str_pop_wp で返されたポインタを指定することを想定しています。
 - attrspckcnt  
   Builtin属性およびPlugin属性情報の属性数を指定します。
 - encpass  
   パスフレーズを指定します。
 - expire  
   Expire時間を指定します。 指定しない場合には NULL を指定します。
- k2h_q_pop	
 - qhandle	
    - キューのハンドルを指定します。
 - ppval	
    - キューから取り出す（pop）値を格納するバッファへのポインタを指定します（バイナリ列）。 返されたポインタは開放してください。
 - pvallen	
    - \*pvalのバッファ長が返されます。
 - ppattrspck	
    - キューに設定されていたBuiltin属性およびPlugin属性情報を取り出すポインタを指定します。
 - pattrspckcnt	
    - キューに設定され、取り出されたBuiltin属性およびPlugin属性情報の数を返すポインタを指定します。
 - encpass	
    - パスフレーズを指定します。
- k2h_q_str_pop
 - qhandle  
   キューのハンドルを指定します。
 - ppval  
   キューから取り出す（pop）値を格納するバッファへのポインタを指定します（文字列であり、\0 終端です）。 返されたポインタは開放してください。
 - ppattrspck  
   キューに設定されていたBuiltin属性およびPlugin属性情報を取り出すポインタを指定します。
 - pattrspckcnt  
   キューに設定され、取り出されたBuiltin属性およびPlugin属性情報の数を返すポインタを指定します。
 - encpass  
   パスフレーズを指定します。
- k2h_q_remove
 - qhandle  
   キューのハンドルを指定します。
 - count  	
   キューから削除したい値の数を指定してください。
 - encpass  
   パスフレーズを指定します。
- k2h_q_remove\*_ext
 - qhandle  
   キューのハンドルを指定します。
 - count  
   キューから削除したい値の数を指定してください。
 - fp  
   コールバック関数へのポインタを指定します。
 - pextdata  
   コールバック関数に引き渡す任意のデータを指定します（NULL 可）。
 - encpass  
   パスフレーズを指定します。
- k2h_q_dump
 - qhandle  
   キューのハンドルを指定します。
 - stream    
   デバッグ用の出力先を指定します。 NULL を指定した場合には、stdout に出力されます。

#### 返り値
- k2h_q_handle  
  正常に終了した場合には、キューのハンドルを返します。 エラーの場合には、K2H_INVALID_HANDLE(0) を返します。
- k2h_q_handle_prefix  
  正常に終了した場合には、キューのハンドルを返します。 エラーの場合には、K2H_INVALID_HANDLE(0) を返します。
- k2h_q_handle_str_prefix  
  正常に終了した場合には、キューのハンドルを返します。 エラーの場合には、K2H_INVALID_HANDLE(0) を返します。
- k2h_q_count  
  キューに蓄積されているデータの数を返します。エラーが発生した場合には0を返します。
- k2h_q_remove\*_ext  
  削除したキューの値の数を返します。エラー時には、-1を返します。
- 上記以外  
  正常終了した場合には true を返し、エラー時には false を返します。

#### k2h_q_remove_trial_callback コールバック関数
k2h_q_remove_ext関数は、引数にコールバック関数（k2h_q_remove_trial_callback）を指定します。  
コールバック関数は、キューの値の削除処理毎に呼び出され、値をキューから削除するか否かを判定することができます。  
このコールバック関数により、キューに蓄積された値の削除において、任意の条件で削除を実行することができるようになります。  

このコールバック関数は、以下のプロトタイプとなっています。
 ```
typedef K2HQRMCBRES (*k2h_q_remove_trial_callback)(const unsigned char* bydata, size_t datalen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData);
 ```
k2h_q_remove_ext関数は、キューの値の削除処理毎に指定されたコールバック関数を呼び出します。  


コールバック関数は、以下の引数と共に呼び出されます。
- bydata  
  削除しようとしているキューに蓄積されたデータへのバイナリ列のポインタが指定されます。
- datalen  
  bydataのバイナリ列長が設定されます。
- pattrs  
  削除しようとしているキューに設定されているBuiltin属性およびPlugin属性が K2HATTRPCK のポインタで渡されます。
- attrscnt  
  pattrs のBuiltin属性およびPlugin属性の数が渡されます。
- pExtData  
  k2h_q_remove_ext関数を呼び出したときに指定した pextdata ポインタが設定されています。  


コールバック関数は以下の値を返します。
- K2HQRMCB_RES_ERROR  
  コールバック関数にてエラーが発生した場合などに返します。
- K2HQRMCB_RES_CON_RM  
  削除しようとしている値をキューから削除し、次の値の検査を要求します。（中断しない限り、k2h_q_remove_ext関数を呼び出したときの count 引数分の検査が行われます。）
- K2HQRMCB_RES_CON_NOTRM  
  削除しようとしている値のキューからの削除を中止し（削除しない）、次の値の検査を要求します。（中断しない限り、k2h_q_remove_ext関数を呼び出したときの count 引数分の検査が行われます。）
- K2HQRMCB_RES_FIN_RM  
  削除しようとしている値をキューから削除し、次の値の検査を行わず、処理中断を要求します。（k2h_q_remove_ext関数を呼び出したときの count 引数分の検査を中断することになります。）
- K2HQRMCB_RES_FIN_NOTRM  
  削除しようとしている値のキューからの削除を中止し（削除しない）、次の値の検査を行わず、処理中断を要求します。（k2h_q_remove_ext関数を呼び出したときの count 引数分の検査を中断することになります。）  

 
コールバック関数のサンプルを以下に示します。
 ```
static K2HQRMCBRES QueueRemoveCallback(const unsigned char* bydata, size_t datalen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData)
{
    if(!bydata || 0 == datalen){
        return K2HQRMCB_RES_ERROR;
    }
    //
    // Check get queued data as bydata which is queued data(K2HQueue) or key(K2HKeyQueue).
    // If you need to remove it from queue, this function must return K2HQRMCB_RES_CON_RM or K2HQRMCB_RES_FIN_RM.
    // The other do not want to remove it, must return K2HQRMCB_RES_CON_NOTRM or K2HQRMCB_RES_FIN_NOTRM.
    // If you want to stop removing no more, this function can return K2HQRMCB_RES_FIN_*(RM/NOTRM).
    // pExpData is the parameter when you call Remove function.
    //
    return K2HQRMCB_RES_CON_RM;
}
 ```

#### 注意
- k2h_q_handle等の関数は、キューへのハンドルを返しますが、ハンドルが返された時点では K2HASH内部キューのキー作成（存在しない場合）および、キーへのアタッチ操作などは行われていません。  
  このハンドルを使用して、蓄積（push）や取り出し（pop）の操作を行った時点で、内部キューのキー作成が行われたり、キーへのアクセスが発生します。
- k2h_q_pop系の関数から返される値へのポインタは、呼び出し側で開放してください。  
  キューが多量に蓄積されている場合などに、k2h_q_count関数の呼び出し、および k2h_q_read、k2h_q_str_read系の関数に指定する pos引数（データ位置を示す）を後方を指定した場合、パフォーマンスはよくありません。利用する場合には留意してください。
- K2HASHライブラリのトランザクション処理は内部ではキューで実装されています。

#### サンプル

 ```
if(!k2h_create("/home/myhome/mydata.k2h", 8, 4, 1024, 512)){
    return false;
}
k2h_h k2handle;
if(K2H_INVALID_HANDLE == (k2handle = k2h_open_rw("/home/myhome/mydata.k2h", true, 8, 4, 1024, 512))){
    return false;
}
// get queue handle
k2h_q_h    qhandle;
if(K2H_INVALID_HANDLE == (qhandle = k2h_q_handle_str_prefix(k2handle, true/*FIFO*/, "my_queue_prefix_"))){
    k2h_close(k2handle);
    return false;
}
// push
if(!k2h_q_str_push(qhandle, "test_value")){
    k2h_q_free(qhandle);
    k2h_close(k2handle);
    return false;
}
// pop
char*    pdata = NULL;
if(!k2h_q_str_pop(qhandle, &pdata)){
    k2h_q_free(qhandle);
    k2h_close(k2handle);
    return false;
}
free(pdata);
k2h_q_free(qhandle);
k2h_close(k2handle);
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="KVQUE"> キュー（キー＆値）関連(C I/F)
K2HASHライブラリは、キュー（FIFO/LIFO）としての機能を提供します。
本関数群の提供するキューは、キー（Key）と値（Value）を1組として、FIFO/LIFOで蓄積（PUSH）し、読み出し（POP）できます。
本関数群は、このキーと値のキューに関連する関数群です。

#### 書式
- k2h_keyq_h k2h_keyq_handle(k2h_h handle, bool is_fifo)
- k2h_keyq_h k2h_keyq_handle_prefix(k2h_h handle, bool is_fifo, const unsigned char\* pref, size_t preflen)
- k2h_keyq_h k2h_keyq_handle_str_prefix(k2h_h handle, bool is_fifo, const char\* pref)
- bool k2h_keyq_free(k2h_keyq_h keyqhandle)
- bool k2h_keyq_empty(k2h_keyq_h keyqhandle)
- int k2h_keyq_count(k2h_keyq_h keyqhandle)
- bool k2h_keyq_read(k2h_keyq_h keyqhandle, unsigned char\*\* ppdata, size_t\* pdatalen, int pos)
- bool k2h_keyq_read_keyval(k2h_keyq_h keyqhandle, unsigned char\*\* ppkey, size_t\* pkeylen, unsigned char\*\* ppval, size_t\* pvallen, int pos)
- bool k2h_keyq_read_wp(k2h_keyq_h keyqhandle, unsigned char\*\* ppdata, size_t\* pdatalen, int pos, const char\* encpass)
- bool k2h_keyq_read_keyval_wp(k2h_keyq_h keyqhandle, unsigned char\*\* ppkey, size_t\* pkeylen, unsigned char\*\* ppval, size_t\* pvallen, int pos, const char\* encpass)
- bool k2h_keyq_str_read(k2h_keyq_h keyqhandle, char\*\* ppdata, int pos)
- bool k2h_keyq_str_read_keyval(k2h_keyq_h keyqhandle, char\*\* ppkey, char\*\* ppval, int pos)
- bool k2h_keyq_str_read_wp(k2h_keyq_h keyqhandle, char\*\* ppdata, int pos, const char\* encpass)
- bool k2h_keyq_str_read_keyval_wp(k2h_keyq_h keyqhandle, char\*\* ppkey, char\*\* ppval, int pos, const char\* encpass)
- bool k2h_keyq_push(k2h_keyq_h keyqhandle, const unsigned char\* bykey, size_t keylen)
- bool k2h_keyq_push_keyval(k2h_keyq_h keyqhandle, const unsigned char\* bykey, size_t keylen, const unsigned char\* byval, size_t vallen)
- bool k2h_keyq_push_wa(k2h_keyq_h keyqhandle, const unsigned char\* bykey, size_t keylen, const char\* encpass, const time_t\* expire)
- bool k2h_keyq_push_keyval_wa(k2h_keyq_h keyqhandle, const unsigned char\* bykey, size_t keylen, const unsigned char\* byval, size_t vallen, const char\* encpass, const time_t\* expire)
- bool k2h_keyq_str_push(k2h_keyq_h keyqhandle, const char\* pkey)
- bool k2h_keyq_str_push_keyval(k2h_keyq_h keyqhandle, const char\* pkey, const char\* pval)
- bool k2h_keyq_str_push_wa(k2h_keyq_h keyqhandle, const char\* pkey, const char\* encpass, const time_t\* expire)
- bool k2h_keyq_str_push_keyval_wa(k2h_keyq_h keyqhandle, const char\* pkey, const char\* pval, const char\* encpass, const time_t\* expire)
- bool k2h_keyq_pop(k2h_keyq_h keyqhandle, unsigned char\*\* ppval, size_t\* pvallen)
- bool k2h_keyq_pop_keyval(k2h_keyq_h keyqhandle, unsigned char\*\* ppkey, size_t\* pkeylen, unsigned char\*\* ppval, size_t\* pvallen)
- bool k2h_keyq_pop_wp(k2h_keyq_h keyqhandle, unsigned char\*\* ppval, size_t\* pvallen, const char\* encpass)
- bool k2h_keyq_pop_keyval_wp(k2h_keyq_h keyqhandle, unsigned char\*\* ppkey, size_t\* pkeylen, unsigned char\*\* ppval, size_t\* pvallen, const char\* encpass)
- bool k2h_keyq_str_pop(k2h_keyq_h keyqhandle, char\*\* ppval)
- bool k2h_keyq_str_pop_keyval(k2h_keyq_h keyqhandle, char\*\* ppkey, char\*\* ppval)
- bool k2h_keyq_str_pop_wp(k2h_keyq_h keyqhandle, char\*\* ppval, const char\* encpass)
- bool k2h_keyq_str_pop_keyval_wp(k2h_keyq_h keyqhandle, char\*\* ppkey, char\*\* ppval, const char\* encpass)
- bool k2h_keyq_remove(k2h_keyq_h keyqhandle, int count)
- bool k2h_keyq_remove_wp(k2h_keyq_h keyqhandle, int count, const char\* encpass)
- int k2h_keyq_remove_ext(k2h_q_h keyqhandle, int count, k2h_q_remove_trial_callback fp, void\* pextdata)
- int k2h_keyq_remove_wp_ext(k2h_keyq_h keyqhandle, int count, k2h_q_remove_trial_callback fp, void\* pextdata, const char\* encpass)
- bool k2h_keyq_dump(k2h_keyq_h keyqhandle, FILE\* stream)

#### 説明
- k2h_keyq_handle  
  FIFO または LIFO のキューへのハンドルを取得します。（このキューはキー＆値のセットとして一連の操作を行うことの出来るキューへのハンドルです。）
- k2h_keyq_handle_prefix  
  FIFO または LIFO のキューへのハンドルを取得します。 取得時にキューで使用される内部キー名におけるプレフィックスを指定することができます。（このキューはキー＆値のセットとして一連の操作を行うことの出来るキューへのハンドルです。）
- k2h_keyq_handle_str_prefix  
  k2h_keyq_handle_prefix へのラッパ関数です。プレフィックスを文字列として指定することができます。（このキューはキー＆値のセットとして一連の操作を行うことの出来るキューへのハンドルです。）
- k2h_keyq_free  
  キューへのハンドルを破棄します。
- k2h_keyq_empty  
  キューが空であるか確認します。
- k2h_keyq_count  
  キューに蓄積されているデータ数を取得します（注意参照）。
- k2h_keyq_read  
  キューに蓄積されているキーをコピーし、その値を返します。 取り出されたキーは削除されません。 取得したキーへのポインタは開放してください（注意参照）。
- k2h_keyq_read_keyval  
  キューに蓄積されているキーをコピーし、そのキーとキーに紐づく値を返します。 取り出されたキーは削除されません。 取得したキーと値へのポインタは開放してください（注意参照）。
- k2h_keyq_read_wp  
  k2h_keyq_read と同等です。 パスフレーズを指定して暗号化されたキューを読み出します。
- k2h_keyq_read_keyval_wp  
  k2h_keyq_read_keyval と同等です。 パスフレーズを指定して暗号化されたキューを読み出します。
- k2h_keyq_str_read  
  k2h_keyq_read へのラッパ関数です。 キーを文字列として取得することができます。 取得したキーへのポインタは開放してください（注意参照）。
- k2h_keyq_str_read_keyval  
  k2h_keyq_read_keyvalへのラッパ関数です。 キー＆値を文字列として取得することができます。 取得したキー＆値へのポインタは開放してください（注意参照）。
- k2h_keyq_str_read_wp  
  k2h_keyq_str_read と同等です。 パスフレーズを指定して暗号化されたキューを読み出します。
- k2h_keyq_str_read_keyval_wp  
  k2h_keyq_str_read_keyval と同等です。 パスフレーズを指定して暗号化されたキューを読み出します。
- k2h_keyq_push  
  キューにキーを蓄積（push）します。 キューにのみpushします。
- k2h_keyq_push_keyval  
  キー＆値をK2HASHに書き込み、そのキーをキューに蓄積（push）します。 キー＆値の書き込みと一連の操作としてキューにも蓄積（push）します。
- k2h_keyq_push_wa  
  k2h_keyq_push と同等です。 パスフレーズと Expire時間を指定して蓄積（push）ができます。
- k2h_keyq_push_keyval_wa  
  k2h_keyq_push_keyval と同等です。パスフレーズと Expire時間を指定して蓄積（push）ができます。
- k2h_keyq_str_push  
  k2h_keyq_push へのラッパ関数です。 キーを文字列として指定することができます。
- k2h_keyq_str_push_keyval  
  k2h_keyq_push_keyval へのラッパ関数です。 キーと値を文字列として指定することができます。
- k2h_keyq_str_push_wa  
  k2h_keyq_str_push と同等です。 パスフレーズと Expire時間を指定して蓄積（push）ができます。
- k2h_keyq_str_push_keyval_wa  
  k2h_keyq_str_push_keyval と同等です。パスフレーズと Expire時間を指定して蓄積（push）ができます。
- k2h_keyq_pop  
  キューに蓄積されているキーを取り出し（pop）し、そのキーに書き込まれている値を取り出し、値を返します。 取り出されたキー＆値は、K2HASHからも削除されます。 取得したキーへのポインタは開放してください。
- k2h_keyq_pop_keyval  
  キューに蓄積されているキーを取り出し（pop）し、そのキーに書き込まれている値も取り出し、キー＆値を一緒に返します。 取り出されたキー＆値は、K2HASHからも削除されます。 取得したキー＆値へのポインタは開放してください。
- k2h_keyq_pop_wp  
  k2h_keyq_popと 同等です。 パスフレーズを指定して暗号化されたキューを取り出します。
- k2h_keyq_pop_keyval_wp  
  k2h_keyq_pop_keyval と同等です。 パスフレーズを指定して暗号化されたキューを取り出します。
- k2h_keyq_str_pop  
  k2h_keyq_pop へのラッパ関数です。 キーを文字列として取得することができます。 取得したキーへのポインタは開放してください。
- k2h_keyq_str_pop_keyval  
  k2h_keyq_pop_keyval へのラッパ関数です。 キー＆値を文字列として取得することができます。 取得したキー＆値へのポインタは開放してください。
- k2h_keyq_str_pop_wp  
  k2h_keyq_str_pop と同等です。 パスフレーズを指定して暗号化されたキューを取り出します。
- k2h_keyq_str_pop_keyval_wp  
  k2h_keyq_str_pop_keyval と同等です。 パスフレーズを指定して暗号化されたキューを取り出します。
- k2h_keyq_remove  
  キューに蓄積されているキーを、指定個数分削除します。 削除されたキーとキーに紐付けられた値は、K2HASHからも削除されます。 削除されたキー＆値は返されません。 蓄積された数以上を指定してもエラーとはならず、キューの中身が空になり、正常終了します。
- k2h_keyq_remove_wp  
  k2h_keyq_removeと同等です。 パスフレーズを指定して暗号化されたキューを削除します。
- k2h_keyq_remove_ext  
  k2h_keyq_remove と同様にキューに蓄積されている値を、指定個数分削除します。 この関数にはコールバック関数を指定します。 キューから値の削除毎にコールバック関数が呼び出され、値の削除の判定を独自に行うことができます。（k2h_q_remove_trial_callbackコールバック関数および本関数の動作については、k2h_q_remove_ext関数の説明を参照してください。）
- k2h_keyq_remove_wp_ext  
  k2h_keyq_remove_extと同等です。パスフレーズを指定して暗号化されたキューを削除します。
- k2h_keyq_dump  
  キューのデバッグ用の関数であり、キュー内部をダンプします。

#### パラメータ
- k2h_keyq_handle	
 - handle  
   K2HASHハンドルを指定します。
 - is_fifo  
   取得するキューのタイプ（FIFO または LIFO）を指定します。
- k2h_keyq_handle_prefix
 - handle  
   K2HASHハンドルを指定します。
 - is_fifo  
   取得するキューのタイプ（FIFO または LIFO）を指定します。
 - pref  
   キューとして使用する内部キー名のプレフィックスを指定します。（バイナリ列）
 - preflen  
   pref のバッファ長を指定します。
- k2h_keyq_handle_str_prefix
 - handle  
   K2HASHハンドルを指定します。
 - is_fifo  
   取得するキューのタイプ（FIFO または LIFO）を指定します。
 - pref  
   キューとして使用する内部キー名のプレフィックスを指定します。（文字列であり、\0終端です）
- k2h_keyq_free
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
- k2h_keyq_empty
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
- k2h_keyq_count
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
- k2h_keyq_read
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - ppdata  
   キューからコピーしたキーに紐づく値（バイナリ列）を受け取るポインタを指定します。 ポインタは開放してください。
 - pdatalen  
   ppdata のバッファ長を返します。
 - pos  
   コピーするデータのキューの先頭からの位置を指定します。（先頭が0となります）
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_read_keyval
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - ppkey  
   キューからコピーしたキーに紐づく値（バイナリ列）を受け取るポインタを指定します。 ポインタは開放してください。
 - pkeylen  
   ppkey のバッファ長を返します。
 - ppval  
   キューからコピーしたキーに紐づく値（バイナリ列）を受け取るポインタを指定します。 ポインタは開放してください。
 - pvallen  
   ppval のバッファ長を返します。
 - pos  
   コピーするデータのキューの先頭からの位置を指定します。（先頭が0となります）
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_str_read
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - ppdata  
   キューからコピーしたキーに紐づく値（文字列であり、\0終端です）を受け取るポインタを指定します。ポインタは開放してください。
 - pos  
   コピーするデータのキューの先頭からの位置を指定します。（先頭が0となります）
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_str_read_keyval
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - ppkey  
   キューからコピーしたキーに紐づく値（文字列であり、\0終端です）を受け取るポインタを指定します。ポインタは開放してください。
 - ppval  
   キューからコピーしたキーに紐づく値（文字列であり、\0終端です）を受け取るポインタを指定します。ポインタは開放してください。
 - pos  
   コピーするデータのキューの先頭からの位置を指定します。（先頭が0となります）
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_push
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - bykey  
   キューに蓄積（push）するキーを指定します。（バイナリ列）
 - keylen  
   bykeyのバッファ長を指定します。
 - encpass  
   パスフレーズを指定します。
 - expire  
   Expire時間（秒）を指定する場合にはtime_tへのポインタを指定します。 指定しない場合にはNULLを指定します。
- k2h_keyq_push_keyval
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - bykey  
   キューに蓄積（push）するキーを指定します。（バイナリ列）
 - keylen  
   bykeyのバッファ長を指定します。
 - byval  
   キューに蓄積（push）するキーに紐づく値を指定します。（バイナリ列）
 - vallen  
   byvalのバッファ長を指定します。
 - encpass  
   パスフレーズを指定します。
 - expire  
   Expire時間（秒）を指定する場合には time_t へのポインタを指定します。 指定しない場合には NULL を指定します。
- k2h_keyq_str_push
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - pkey  
   キューに蓄積（push）するキーを指定します。（文字列であり、\0終端です）
 - encpass 
   パスフレーズを指定します。
 - expire  
   Expire時間（秒）を指定する場合には time_t へのポインタを指定します。 指定しない場合には NULL を指定します。
- k2h_keyq_str_push_keyval
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - pkey  
   キューに蓄積（push）するキーを指定します。（文字列であり、\0終端です）
 - pval  
   キューに蓄積（push）するキーに紐づく値を指定します。（文字列であり、\0終端です）
 - encpass  
   パスフレーズを指定します。
 - expire  
   Expire時間（秒）を指定する場合にはtime_tへのポインタを指定します。 指定しない場合には NULL を指定します。
- k2h_keyq_pop
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - ppval  
   キューから取り出した（pop）キーに紐づく値（バイナリ列）を受け取るポインタを指定します。 ポインタは開放してください。
 - pvallen  
   ppval のバッファ長を返します。
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_pop_keyval
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - ppkey  
   キューから取り出した（pop）キー（バイナリ列）を受け取るポインタを指定します。 ポインタは開放してください。
 - pkeylen  
   ppkey のバッファ長を返します。
 - ppval  
   キューから取り出した（pop）キーに紐づく値（バイナリ列）を受け取るポインタを指定します。 ポインタは開放してください。
 - pvallen  
   ppvalのバッファ長を返します。
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_str_pop
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - ppval  
   キューから取り出した（pop）キーに紐づく値（文字列であり、\0終端です）を受け取るポインタを指定します。 ポインタは開放してください。
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_str_pop_keyval
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - ppkey  
   キューから取り出した（pop）キー（文字列であり、\0終端です）を受け取るポインタを指定します。 ポインタは開放してください。
 - ppval  
   キューから取り出した（pop）キーに紐づく値（文字列であり、\0終端です）を受け取るポインタを指定します。 ポインタは開放してください。
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_remove
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - count  
   キューから削除したいキー＆値の数を指定してください。
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_remove\*_ext
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - count  
   キューから削除したいキー＆値の数を指定してください。
 - fp  
   k2h_q_remove_trial_callbackコールバック関数のポインタを指定します。
 - pextdata  
   k2h_q_remove_trial_callbackコールバック関数に引き渡すための任意のポインタを指定します。（NULL 可）
 - encpass  
   パスフレーズを指定します。
- k2h_keyq_dump
 - keyqhandle  
   キュー(キー＆値用)のハンドルを指定します。
 - stream  
   デバッグ用の出力先を指定します。 NULL を指定した場合には、stdout に出力されます。

##### 返り値
- k2h_keyq_handle  
  正常に終了した場合には、キューのハンドルを返します。 エラーの場合には、K2H_INVALID_HANDLE(0) を返します。
- k2h_keyq_handle_prefix  
  正常に終了した場合には、キューのハンドルを返します。 エラーの場合には、K2H_INVALID_HANDLE(0) を返します。
- k2h_keyq_handle_str_prefix  
  正常に終了した場合には、キューのハンドルを返します。 エラーの場合には、K2H_INVALID_HANDLE(0) を返します。
- k2h_keyq_count  
  キューに蓄積されたデータ数を返します。エラーが発生した場合には0を返します。
- k2h_keyq_remove\*_ext  
  削除したキューの値の数を返します。エラー時は、-1を返します。
- 上記以外	
  正常終了した場合には true を返し、エラー時には false を返します。

#### 注意
- 本関数群は、[キュー関連(C I/F)](#QUE)とほぼ同じです。  
  [キュー関連(C I/F)](#QUE)との違いは、キーと値をセットとして取り扱う点であり、キューへの蓄積（push）、取り出し（pop）操作と同時に、キーと値のセットを K2HASH からも削除します。  
  本関数群を使うことにより、キーと値を K2HASH に保管すると同時に、キューイングを行うことができ、そのキーと値の有効期限をキューを使って管理できます。  
  削除においては、キューからの削除（pop）することにより、K2HASH に保管されているキーと値の本体の削除もできます。  
  本関数群を使いキューに蓄積した場合、キーと値をセットで K2HASH に保管しているので、通常のキーに対する値のアクセスなどのKVSとして、キーへのアクセスができます。
- [キュー関連(C I/F)](#QUE)と同様に handle等の関数は、キューへのハンドルを返しますが、ハンドルが返された時点では K2HASH内部キューのキー作成（存在しない場合）および、キーへのアタッチ操作などは行われません。  
  これらのハンドルを使用して、蓄積（push）や取り出し（pop）の操作を行った時点で、内部キューのキー作成が行われたり、キーへのアクセスが発生します。
- k2h_keyq_pop系の関数から返される値へのポインタは、呼び出し側で開放してください。  
  キューが多量に蓄積されている場合などに、k2h_keyq_count関数の呼び出し、および k2h_keyq_read系の関数に指定する pos引数（データ位置を示す）を後方を指定した場合、パフォーマンスはよくありません。利用する場合は留意してください。
- Builtin属性の暗号化および Expire機能を利用した場合には、蓄積されるキューおよびそのキューのキーと値共に暗号化され、Expire時刻が設定されます。  
  キューを読み出すためには蓄積した際のパスフレーズが必要となり、またExpire時刻を経過すると読み出せなくなります。

#### サンプル
 ```
if(!k2h_create("/home/myhome/mydata.k2h", 8, 4, 1024, 512)){
    return false;
}
k2h_h k2handle;
if(K2H_INVALID_HANDLE == (k2handle = k2h_open_rw("/home/myhome/mydata.k2h", true, 8, 4, 1024, 512))){
    return false;
}
// get queue handle
k2h_q_h    keyqhandle;
if(K2H_INVALID_HANDLE == (keyqhandle = k2h_keyq_handle_str_prefix(k2handle, , true/*FIFO*/, "my_queue_prefix_"))){
    k2h_close(k2handle);
    return false;
}
// push
if(!k2h_keyq_str_push_keyval(keyqhandle, "test_key", "test_value")){
    k2h_keyq_free(keyqhandle);
    k2h_close(k2handle);
    return false;
}
// test for accessing the key
char*    pvalue = NULL;
if(NULL == (pvalue = k2h_get_str_direct_value(k2handle, "test_key"))){
    // error...
}else{
    if(0 != strcmp(pvalue, "test_value")){
        // error...
    }
    free(pvalue);
}
// pop
char*    pkey = NULL;
pvalue        = NULL;
if(!k2h_keyq_str_pop_keyval(keyqhandle, &pkey, &pval)){
    k2h_q_free(keyqhandle);
    k2h_close(k2handle);
    return false;
}
if(0 != strcmp(pkey, "test_key") || 0 != strcmp(pvalue, "test_value")){
    // error...
}
free(pkey);
free(pvalue);
// check no key
pvalue = NULL;
if(NULL != (pvalue = k2h_get_str_direct_value(k2handle, "test_key"))){
    // error...
    free(pvalue);
}
free(pdata);
k2h_q_free(keyqhandle);
k2h_close(k2handle);
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="DUMP"> ダンプ・ステータス関連(C I/F)
K2HASHデータをダンプするためのデバッグ用の関数群です。

#### 書式
- bool k2h_dump_head(k2h_h handle, FILE\* stream)
- bool k2h_dump_keytable(k2h_h handle, FILE\* stream)
- bool k2h_dump_full_keytable(k2h_h handle, FILE\* stream)
- bool k2h_dump_elementtable(k2h_h handle, FILE\* stream)
- bool k2h_dump_full(k2h_h handle, FILE\* stream)
- bool k2h_print_state(k2h_h handle, FILE\* stream)
- void k2h_print_version(FILE\* stream)
- PK2HSTATE k2h_get_state(k2h_h handle)

#### 説明
- k2h_dump_head  
  K2HASHデータのヘッダ情報をダンプします。ダンプ出力は stream に対して行われます。stream が NULL の場合には、stderr へ出力されます。
- k2h_dump_keytable  
  K2HASHデータのヘッダおよびHASHテーブル情報をダンプします。 ダンプ出力は stream に対して行われます。 stream が NULL の場合には、stderr へ出力されます。
- k2h_dump_full_keytable  
  K2HASHデータのヘッダ、HASHテーブル、サブHASHテーブル情報をダンプします。 ダンプ出力は stream に対して行われます。 stream が NULL の場合には、stderr へ出力されます。
- k2h_dump_elementtable  
  K2HASHデータのヘッダ、HASHテーブル、サブHASHテーブル、エレメント（要素）情報をダンプします。 ダンプ出力は stream に対して行われます。 stream が NULL の場合には、stderr へ出力されます。
- k2h_dump_full  
  K2HASHデータのフル（ヘッダ、HASHテーブル、サブHASHテーブル、エレメント（要素）、個々のデータ）情報をダンプします。 ダンプ出力は stream に対して行われます。stream が NULL の場合には、stderr へ出力されます。
- k2h_print_state  
  K2HASHデータのステータス（利用状況）を出力します。出力は stream に対して行われます。 stream が NULL の場合には、stderr へ出力されます。
- k2h_print_version  
  K2HASHライブラリのバージョンおよびクレジットの表示をします。
- k2h_get_state	  
  K2HASHデータのヘッダ情報およびステータス（利用状況）をまとめた構造体K2HSTATE を返します。

#### パラメータ 
- handle  
  k2h_open系の関数から返された K2HASHハンドルを指定します。
- stream  
  出力先のFILEポインタを指定します。 NULL を指定した場合には、stderr に出力します。

#### 返り値
- k2h_get_state以外  
  成功した場合には、true を返します。失敗した場合には false を返します。
- k2h_get_state  
  成功した場合には、領域確保された PK2HSTATEポインタを返します（このポインタは開放する必要があります）。 失敗した場合には NULL を返します。

#### 注意
- ダンプ結果を理解するためには、K2HASHファイル（もしくはオンメモリ）の構造を知っておく必要があります。  
  ダンプ結果は、K2HASHファイル（もしくはオンメモリ）のデータ（HASHテーブルを含む）が出力されます。
- フルダンプは、有効な全K2HASHデータをダンプしますので、出力ファイルの容量、出力のための処理時間に気をつけてください。

#### サンプル

 ```
k2h_print_state(k2handle, NULL);
k2h_dump_full(k2handle, NULL);
 ```

<!-- -----------------------------------------------------------　-->
***

## <a name="CPP"> C++言語インタフェース
K2HASHライブラリを利用するための、C++言語APIのI/Fです。  

 
開発時には以下のヘッダファイルをインクルードしてください。
 ```
#include <k2hash/k2hash.h>
#include <k2hash/k2hshm.h>
 ```
 
リンク時には以下をオプションとして指定してください。
 ```
-lk2hash
 ```
 
以下にC++言語用の関数の説明をします。

### <a name="DEBUGCPP"> デバッグ関連(C/C++ I/F)
K2HASHライブラリは、内部動作およびAPIの動作の確認をするためにメッセージ出力を行うことができます。
本関数群は、メッセージ出力を制御するための関数群です。

#### 書式
- K2hDbgMode SetK2hDbgMode(K2hDbgMode mode)
- K2hDbgMode BumpupK2hDbgMode(void)
- K2hDbgMode GetK2hDbgMode(void)
- bool LoadK2hDbgEnv(void)
- bool SetK2hDbgFile(const char\* filepath)
- bool UnsetK2hDbgFile(void)
- bool SetSignalUser1(void)

#### 説明
- SetK2hDbgMode  
  デバッグ用のメッセージ出力レベルを設定します。 デフォルトは 非出力となっています。
- BumpupK2hDbgMode  
  デバッグ用のメッセージ出力レベルをBump upします。 デフォルトは非出力であり、本関数を呼び出す毎に、非出力→エラー→ワーニング→メッセージ→非出力・・・と切り替わります。
- GetK2hDbgMode  
  現在のデバッグ用のメッセージ出力レベルを取得します。
- LoadK2hDbgEnv  
  デバッグ用のメッセージ出力レベル、出力ファイルに関する環境変数 K2HDBGMODE、K2HDBGFILE を再度読み込み、環境変数が設定されていれば、再設定します。
- SetK2hDbgFile  
  デバッグ用のメッセージ出力ファイルを設定します。 デフォルト（環境変数 K2HDBGFILE の指定も無い場合）では、stderr が設定されています。
- UnsetK2hDbgFile  
  デバッグ用のメッセージ出力ファイルが設定されている場合に、デフォルトである stderr に戻します。
- SetSignalUser1  
  SIGUSR1 シグナルにより、デバッグ用のメッセージ出力レベルを Bump upできるように、シグナルハンドラを設定します。

#### パラメータ
- mode  
  メッセージ出力レベルを指定する K2hDbgMode の値を設定します。 現在設定可能な値は、以下に示す4種類となります。
  - 非出力  
    K2hDbgMode::K2HDBG_SILENT
  - エラー  
    K2hDbgMode::K2HDBG_ERR
  - ワーニング  
    K2hDbgMode::K2HDBG_WARN
  - メッセージ（インフォメーション）  
    K2hDbgMode::K2HDBG_MSG
- filepath  
  メッセージ出力ファイルのファイルパスを指定します。

#### 返り値
- SetK2hDbgMode  
  直前に設定されていたデバッグ用のメッセージ出力レベルを返します。
- BumpupK2hDbgMode  
  直前に設定されていたデバッグ用のメッセージ出力レベルを返します。
- GetK2hDbgMode  
  現在設定されているデバッグ用のメッセージ出力レベルを返します。
- LoadK2hDbgEnv  
  成功した場合には、true を返します。失敗した場合には false を返します。
- SetK2hDbgFile  
  成功した場合には、true を返します。失敗した場合には false を返します。
- UnsetK2hDbgFile  
  成功した場合には、true を返します。失敗した場合には false を返します。
- SetSignalUser1  
  成功した場合には、true を返します。失敗した場合には false を返します。

#### 注意
- デバッグ用のメッセージ出力レベル、出力ファイルに関する環境変数 K2HDBGMODE、K2HDBGFILEは、K2HASHライブラリがロードされたときに設定されているので、本来は LoadK2hDbgEnv()関数により再設定する必要はありません。
- SetSignalUser1()関数による SIGUSR1 シグナルハンドラ設定は、実装するプログラムによっては動作に不都合を発生させる恐れがありますので、留意してください。  
  K2HASHライブラリは、シグナルハンドラを設定し、シグナルをハンドリングしますが、以前に設定されていたハンドラを呼び出すことはしません。  
  また、多重での SIGUSR1 シグナルをマルチスレッドで受け取った場合の動作確認は、実装する側で行ってください。  

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HASHDYNLIB"> K2HashDynLibクラス

#### 説明
K2HASHライブラリは、内部で利用するHASH関数を外部のシェアードライブラリ（DSOモジュール）として読み込むことができます。
本クラスは、このシェアードライブラリのロード、アンロードを行う管理クラスです。
本クラスオブジェクトは K2HASHライブラリで唯一しか存在しないシングルトンとなっています。
本クラスのメソッドを利用する場合には、シングルトンの K2HashDynLibオブジェクトポインタを取得して利用してください。

#### HASH系関数プロトタイプ
DSOモジュールには、以下の3つの関数を含んでいる必要があります。

 ```
// First hash value returns, the value is used Key Index
k2h_hash_t k2h_hash(const void* ptr, size_t length);
 
// Second hash value returns, the value is used the Element in collision keys.
k2h_hash_t k2h_second_hash(const void* ptr, size_t length);
 
// Hash function(library) version string, the value is stamped into SHM file.
// This retuned value length must be under 32 byte.
const char* k2h_hash_version(void);
 ```
詳しくは、k2hashfunc.hを参照してください。

#### メソッド
- static K2HashDynLib\* K2HashDynLib::get(void)
- bool K2HashDynLib::Load(const char\* path)
- bool K2HashDynLib::Unload(void)

#### メソッド説明
- K2HashDynLib::get  
  クラスメソッドであり、シングルトンである K2HASHライブラリ唯一の K2HashDynLib オブジェクトポインタを返します。このメソッドの返すオブジェクトを使って、本クラスのメソッドを呼び出してください。
- K2HashDynLib::Load  
  HASH関数の DSOモジュールをロードします。
- K2HashDynLib::Unload  
  ロードされているHASH関数の DSOモジュールがあれば、アンロードします。

#### メソッド返り値
- K2HashDynLib::get  
  シングルトンである K2HASHライブラリ唯一の K2HashDynLibオブジェクトポインタを返します。この関数は失敗しません。
- K2HashDynLib::Load  
  成功した場合には、true を返します。失敗した場合には false を返します。
- K2HashDynLib::Unload  
  成功した場合には、true を返します。失敗した場合には false を返します。

#### 注意
シングルトンの取り扱いに注意してください。

#### サンプル
 ```
if(!K2HashDynLib::get()->Load("/home/myhome/myhashfunc.so")){
 exit(-1);
}
 ・
 ・
 ・
K2HashDynLib::get()->Unload();
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HTRANSDYNLIB"> K2HTransDynLibクラス

#### 説明
K2HASHライブラリは、内部で利用するトランザクション処理を外部のシェアードライブラリ（DSOモジュール）として読み込むことができます。
本クラスは、このシェアードライブラリのロード、アンロードを行う管理クラスです。
本クラスオブジェクトは K2HASHライブラリで唯一しか存在しないシングルトンとなっています。
本クラスのメソッドを利用する場合には、シングルトンの K2HTransDynLibオブジェクトポインタを取得して利用してください。

#### トランザクション系関数プロトタイプ
DSOモジュールには、以下の3つの関数を含んでいる必要があります。

 ```
// transaction callback function
bool k2h_trans(k2h_h handle, PBCOM pBinCom);
 
// Transaction function(library) version string.
const char* k2h_trans_version(void);
 
// transaction control function
bool k2h_trans_cntl(k2h_h handle, PTRANSOPT pOpt);
 ```

 詳しくは、k2htransfunc.hを参照してください。

#### メソッド
- static K2HTransDynLib\* K2HTransDynLib::get(void)
- bool K2HTransDynLib::Load(const char\* path)
- bool K2HTransDynLib::Unload(void)

#### メソッド説明
- K2HTransDynLib::get  
  クラスメソッドであり、シングルトンである K2HASHライブラリ唯一の K2HTransDynLibオブジェクトポインタを返します。  
  このメソッドの返すオブジェクトを使って、本クラスのメソッドを呼び出してください。
- K2HTransDynLib::Load  
  トランザクション系関数の DSOモジュールをロードします。
- K2HTransDynLib::Unload  
  ロードされているトランザクション系関数の DSOモジュールがあれば、アンロードします。

#### メソッド返り値 
- K2HTransDynLib::get  
  シングルトンである K2HASHライブラリ唯一の K2HTransDynLibオブジェクトポインタを返します。 この関数は失敗しません。
- K2HTransDynLib::Load  
  成功した場合には、true を返します。失敗した場合には false を返します。
- K2HTransDynLib::Unload  
  成功した場合には、true を返します。失敗した場合には false を返します。

#### 注意 
シングルトンの取り扱いに注意してください。

#### サンプル
 ```
if(!K2HTransDynLib::get()->Load("/home/myhome/mytransfunc.so")){
 exit(-1);
}
 ・
 ・
 ・
K2HTransDynLib::get()->Unload();
 
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HDACCESS"> K2HDAccessクラス

#### 説明
K2HASHファイル（もしくはオンメモリ）のデータへ直接アクセスするクラスです。
k2hshmクラスから、キー（Key）を指定して、このクラスのオブジェクトを取得し、利用できます。
主に大きなサイズの値（Value）を持つデータへの読み書きを行う場合などに利用します。オフセットを指定して値（Value）の一部分の上書きや読み出しができます。

#### メソッド
- K2HDAccess::K2HDAccess(K2HShm\* pk2hshm, K2HDAccess::ACSMODE access_mode)
<br />
<br />
- bool K2HDAccess::IsInitialize(void) const
- bool K2HDAccess::Open(const char\* pKey)
- bool K2HDAccess::Open(const unsigned char\* byKey, size_t keylength)
- bool K2HDAccess::Close(void)
<br />
<br />
- bool K2HDAccess::SetOffset(off_t offset, size_t length, bool isRead)
- bool K2HDAccess::SetOffset(off_t offset)
- bool K2HDAccess::SetWriteOffset(off_t offset)
- bool K2HDAccess::SetReadOffset(off_t offset)
- off_t K2HDAccess::GetWriteOffset(void) const
- off_t K2HDAccess::GetReadOffset(void) const
<br />
<br />
- bool K2HDAccess::GetSize(size_t& size) const
- size_t K2HDAccess::SetFioSize(size_t size)
- size_t K2HDAccess::GetFioSize(void) const
<br />
<br />
- bool K2HDAccess::Write(const char\* pValue)
- bool K2HDAccess::Write(const unsigned char\* byValue, size_t vallength)
- bool K2HDAccess::Write(int fd, size_t& wlength)
- bool K2HDAccess::Read(char\*\* ppValue)
- bool K2HDAccess::Read(unsigned char\*\* byValue, size_t& vallength)
- bool K2HDAccess::Read(int fd, size_t& rlength)

#### メソッド説明
- K2HDAccess::K2HDAccess  
  コンストラクタです。 k2hshmオブジェクトのポインタと、アクセスするモードを指定します（このクラスを直接生成せず、k2hshmクラスのメソッドを利用して K2HDAccessオブジェクトを取得してください）。
- K2HDAccess::IsInitialize  
  初期化確認
- K2HDAccess::Open  
  キー（Key）を指定して、K2HDAccess用にオープンします（このクラスを直接生成することはせず、k2hshmクラスのメソッドを利用して K2HDAccessオブジェクトを取得してください）。
- K2HDAccess::Close  
  キー（key）をクローズします。 クローズすることによって、キーに対するロック解除ができますので、必要に応じて呼び出してください。
- K2HDAccess::SetOffset  
  読み出し、書き込み位置のオフセットを設定します。
- K2HDAccess::SetWriteOffset  
  書き込み位置のオフセットを設定します。
- K2HDAccess::SetReadOffset  
  読み出し位置のオフセットを設定します。
- K2HDAccess::GetWriteOffset  
  書き込み位置のオフセットを取得します。
- K2HDAccess::GetReadOffset  
  読み出し位置のオフセットを取得します。
- K2HDAccess::GetSize  
  値（Value）の長さを取得します。
- K2HDAccess::SetFioSize  
  ファイルからの読み込み、ファイルへの出力時に使用するバッファのサイズを指定します（デフォルトは400Kbyteです）。
- K2HDAccess::GetFioSize  
  ファイルからの読み込み、ファイルへの出力時に使用するバッファのサイズを取得します。
- K2HDAccess::Write  
  値（Value）へ書き込みをします。
- K2HDAccess::Read  
  値（Value）を読み出します。

#### メソッド返り値
- bool  
  成功した場合には true、失敗した場合は false を返します。
- off_t  
  成功した場合には、書き込み、読み出し位置をオフセットで返します。失敗した場合には -1 を返します。
- size_t  
  バッファのサイズを返します。

#### 注意
ダイレクトアクセスを利用した場合には、暗号化など属性（Attribute）の影響を受けません。 これは、例えば暗号化されたデータへの直接的なアクセスをすることを意味し、データそのものを破壊することになるため、属性を設定しているキーには利用しないようにしてください。

#### サンプル
 ```
k2hshm*    pk2hash;
    ・
    ・
    ・
 
// attach write object
K2HDAccess*    pAccess;
if(NULL == (pAccess = pk2hash->GetDAccessObj("meykey", K2HDAccess::WRITE_ACCESS, 0))){
    return false
}
 
// write
if(!pAccess->Write("my test data")){
    delete pAccess;
    return false;
}
delete pAccess;
 
// attach read object
if(NULL == (pAccess = pk2hash->GetDAccessObj("meykey", K2HDAccess::READ_ACCESS, 0))){
    return false
}
 
// read
unsigned char*    byValue   = NULL;
size_t        vallength = 20;        // this is about :-p
if(!pAccess->Read(&byValue, vallength)){
    delete pAccess;
    return false;
}
delete pAccess;
if(byValue){
    free(byValue);
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HSTREAM"> k2hstream（ik2hstream、ok2hstream）クラス

#### 説明
K2HASHファイル（もしくはオンメモリ）のデータへのアクセスを、iostreamとして取り扱うためのクラスです。
値（Value）への直接的なアクセス（読み込み、書き出し）を iostream 派生クラスとして実装しています。
k2hshmクラスとキー（Key）を指定し、ストリームクラス k2hstream、ik2hstream、ok2hstream を初期化し、iostream として利用できます。
std::stringstream と同等であり、また seekpos を利用できます。
詳細な説明は、std::stringstream などを参考にしてください。

#### 基本クラス
<table>
<tr><td>k2hstream </td><td>std::basic_stream</td></tr>
<tr><td>ik2hstream</td><td>std::basic_istream</td></tr>
<tr><td>ok2hstream</td><td>std::basic_ostream</td></tr>
</table>

#### サンプル
 ```
k2hshm*    pk2hash;
    ・
    ・
    ・
 
// output stream test
{
    ok2hstream    strm(pk2hash, "mykey");
    string        strTmp("test string");
 
    strm << strTmp << endl;
    strm << strTmp << ends;
 
    printf("output string     = \"%s\\n%s\\0\"", strTmp.c_str(), strTmp.c_str());
}
 
// input stream test
{
    ik2hstream    strm(pk2hash, "mykey");
    string        strTmp1;
    string        strTmp2;
 
    strm >> strTmp1;
    strm >> strTmp2;
 
    printf("string     = \"%s\",\"%s\"", strTmp1.c_str(), strTmp2.c_str());
 
    if(!strm.eof()){
        ・
        ・
        ・
    }
}
 ```

#### 注意
本クラスが作成された時点（正確には本クラスを通してキー（Key）がオープンされた時点）で、キーに対してロックが発生します。
ik2hstream クラスではリードロック、ok2hstream および k2hstream クラスの場合にはライトロックが適応されます。
不要なロック状態を回避するために、本クラスは利用を完了した時点で、破棄するかクローズするべきです。

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HARCHIVE"> K2HArchiveクラス

#### 説明
K2HASHデータをアーカイブするためのクラスです。
K2HShmクラスの管理するK2HASHデータを、K2HArchiveクラスがアーカイブ化します。

#### メソッド
- K2HArchive::K2HArchive(const char\* pFile = NULL, bool iserrskip = false)
- bool K2HArchive::Initialize(const char\* pFile, bool iserrskip)
- bool K2HArchive::Serialize(K2HShm\* pShm, bool isLoad) const

#### メソッド説明
- K2HArchive::K2HArchive  
  コンストラクタです。アーカイブファイルおよびエラー発生時の処理フラグ（エラー発生時に継続処理するか否かを示すフラグ）の指定ができます。
- K2HArchive::Initialize  
  初期化メソッドです。アーカイブファイルおよびエラー発生時の処理フラグ（エラー発生時に継続処理するか否かを示すフラグ）の指定ができます。
- K2HArchive::Serialize  
  シリアライズ（アーカイブ化）実行メソッドです。アーカイブしたい K2HShmクラスポインタの指定をしてください。 出力、ロードのフラグを指定してください。

#### メソッド返り値
成功した場合には、true を返します。失敗した場合には false を返します。



#### サンプル
 ```
K2HArchive    archiveobj;
if(!archiveobj.Initialize("/tmp/k2hash.ar", false)){
    return false;
}
if(!archiveobj.Serialize(&k2hash, false)){
    return false;
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HQUEUE"> K2HQueueクラス

#### 説明
K2HASHライブラリは、キュー（FIFO/LIFO）としての機能を提供します。
キューは、値（Value）をFIFO/LIFOとして蓄積（PUSH）し、読み出し（POP）できます。
本クラスは、キュー（Queue）の操作を行うクラスです。
本クラスにより、キューへのデータの蓄積（push）と取り出し（pop）、削除を操作することができます。

K2HASHライブラリの提供するキューは、キー（Key）と値（Value）で実装されており、そのためキューに蓄積される内部的なキー名には特定のプレフィックスが付与されています。
プレフィックスの指定がされない場合には、"\0K2HQUEUE_PREFIX_"（先頭バイトが '\0'(0x00)であることに注意）がデフォルトとして使用されます。
Builtin属性を利用して、キューの暗号化、有効時間（Expire）を指定しできます。

#### メソッド
- K2HQueue::K2HQueue(K2HShm\* pk2h, bool is_fifo, const unsigned char\* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
<br />
<br />
- bool K2HQueue::Init(K2HShm\* pk2h, bool is_fifo, const unsigned char\* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
- bool K2HQueue::Init(const unsigned char\* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
<br />
<br />
- bool K2HQueue::IsEmpty(void) const
- int K2HQueue::GetCount(void) const
- bool K2HQueue::Read(unsigned char\*\* ppdata, size_t& datalen, int pos, const char\* encpass) const
- bool K2HQueue::Push(const unsigned char\* bydata, size_t datalen, K2HAttrs\* pAttrs, const char\* encpass, const time_t\* expire)
- bool K2HQueue::Pop(unsigned char\*\* ppdata, size_t& datalen, K2HAttrs\*\* ppAttrs, const char\* encpass)
- int K2HQueue::Remove(int count, k2h_q_remove_trial_callback fp = NULL, void\* pExtData, const char\* encpass)
- bool K2HQueue::Dump(FILE\* stream)

#### メソッド説明
- K2HQueue::K2HQueue  
  K2HShmオブジェクト、FIFO / LIFOを指定してキューオブジェクトを生成します。 生成された時点では K2HASHデータへの操作（書き込みなど）は行いません。 引数にBuiltin属性のマスクを指定することができます（通常この引数を使うことはありません）。
- K2HQueue::Init  
  K2HShmオブジェクトを指定してキューオブジェクトを初期化します。 初期化された時点では K2HASHデータへの操作（書き込みなど）は行いません。 引数にBuiltin属性のマスクを指定することができます（通常この引数を使うことはありません）。
- K2HQueue::Init  
  プレフィックスを指定してキューオブジェクトを初期化します。 初期化された時点では K2HASHデータへの操作（書き込みなど）は行いません。引数にBuiltin属性のマスクを指定することができます（通常この引数を使うことはありません）。
- K2HQueue::IsEmpty  
  キューが空であるか確認します。
- K2HQueue::GetCount  
  キューに蓄積されているデータ数を返します（注意参照）。
- K2HQueue::Read  
  キューからデータをコピーします。 データはキューから削除されません。 返されたデータのポインタは開放してください（注意参照）。 暗号化されたキューの場合にはパスフレーズを指定してください。
- K2HQueue::Push  
  キューにデータを蓄積（push）します。 蓄積する場合にパスフレーズ、Expire時間の指定もできます。また Pop にて取り出したキューを再度蓄積する場合に利用できる属性情報クラスへのオブジェクトポインタも指定できます。
- K2HQueue::Pop  
  キューからデータを取り出し（pop）します。 返されたデータのポインタは開放してください。 暗号化されたキューの場合にはパスフレーズを指定してください。 必要であれば属性情報クラスへのオブジェクトポインタを取得することができます（この値は再度蓄積する場合に Push に引き渡すことができます）。
- K2HQueue::Remove  
  指定数分のデータをキューからデータを削除します。 削除されたデータは返されません。 蓄積されたデータ数以上を指定した場合はキューが空となり正常終了します。 暗号化されたキューの場合にはパスフレーズを指定してください。  
  引数 fp には、k2h_q_remove_trial_callbackコールバック関数を指定することができます。 コールバック関数を指定した場合には、削除毎に呼び出され、削除可否の判定をすることができます（コールバック関数についての詳細は、k2h_q_remove_ext関数の説明を参照してください）。
- K2HQueue::Dump  
  キューに関連するキーのダンプを行います。 デバッグ用のメソッドです。

#### メソッド返り値
- K2HQueue::GetCount  
  キューに蓄積されているデータ数を返します。 エラーが発生した場合には 0 を返します。
- K2HQueue::Remove  
  キューから削除した値の数を返します。エラーが発生した場合には、-1 を返します。
- 上記以外  
  成功した場合には true、失敗した場合は false を返します。

#### 注意
本クラスは操作クラスであり、クラスインスタンスを生成した時点では K2HASHデータへの操作は行いません。 実際の操作は、Push、Pop などのメソッドを実行したときに発生します。
キューに多量のデータが蓄積された状態で、K2HQueue::GetCount、K2HQueue::Readを 利用した場合、パフォーマンスはよくありません。 利用する場合は留意してください。

#### サンプル

 ```
k2hshm*    pk2hash;
    ・
    ・
    ・
  
// queue object
K2HQueue   myqueue(pk2hash, true/*FIFO*/, reinterpret_cast<const unsigned char*>("MYQUEUE_PREFIX_"), 15);  // without end of nil
// push
if(!myqueue.Push(reinterpret_cast<const unsigned char*>("test_data1"), 11) ||
   !myqueue.Push(reinterpret_cast<const unsigned char*>("test_data2"), 12) )
{
    return false
}
// pop
unsigned char* pdata   = NULL;
size_t         datalen = 0;
if(!myqueue.Pop(&pdata, datalen)){
    return false
}
free(pdata);
// remove
if(!myqueue.Remove(1)){
    return false
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HKEYQUEUE"> K2HKeyQueueクラス

#### 説明
K2HASHライブラリは、キュー（FIFO/LIFO）としての機能を提供します。
本関数群の提供するキューは、キー（Key）と値（Value）を1組として、FIFO/LIFOで蓄積（PUSH）し、読み出し（POP）できます。
本クラスは、K2HQueueの派生クラスであり、このキー（Key）と値（Value）のキューをサポートするクラスになります。
キューに関連するキー名には、特定のプレフィックスがついており、K2HQueue と同じです。

本クラスにより、キューへのデータ（キー（Key）と値（Value））の蓄積（push）と取り出し（pop）、削除を操作することができます。
本クラスのキューに、キー（Key）と値（Value）を指定して蓄積（push）を行うと、K2HASHデータ内にキー（Key）と値（Value）が作成され、かつキューにキー（Key）が蓄積されます。
キューからの取り出し（pop）では、キー（Key）と値（Value）のセットで取り出すことが出来ます。（値（Value）だけの取り出しも可能です）
キューからの取り出し（pop）を行った場合、キューからキー（Key）が削除され、かつK2HASHデータからキー（Key）と値（Value）のセットも削除されます。

本クラスにより、キー（Key）と値（Value）を K2HASHデータへ書き込みと同時に、その書き込み（更新した）順序を蓄積できます。
キューから取り出すことにより、キー（Key）と値（Value）を K2HASHデータから同時に消去（削除）ができます。

キューに蓄積されるキーと、そのキーと値はBuiltin属性を利用して暗号化、有効時間（Expire）の設定ができます。

#### メソッド
- K2HKeyQueue::K2HKeyQueue(K2HShm\* pk2h, bool is_fifo, const unsigned char\* pref, size_t preflen)
<br />
<br />
- bool K2HKeyQueue::Init(K2HShm\* pk2h, bool is_fifo, const unsigned char\* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
- bool K2HKeyQueue::Init(const unsigned char\* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
<br />
<br />
- bool K2HKeyQueue::IsEmpty(void) const
- int K2HKeyQueue::GetCount(void) const
- bool K2HKeyQueue::Read(unsigned char\*\* ppdata, size_t& datalen, int pos, const char\* encpass) const
- bool K2HKeyQueue::Read(unsigned char\*\* ppkey, size_t& keylen, unsigned char\*\* ppval, size_t& vallen, int pos, const char\* encpass) const
- bool K2HKeyQueue::Push(const unsigned char\* bydata, size_t datalen, const char\* encpass, const time_t\* expire)
- bool K2HKeyQueue::Push(const unsigned char\* pkey, size_t keylen, const unsigned char\* byval, size_t vallen, const char\* encpass, const time_t\* expire)
- bool K2HKeyQueue::Pop(unsigned char\*\* ppdata, size_t& datalen, const char\* encpass)
- bool K2HKeyQueue::Pop(unsigned char\*\* ppkey, size_t& keylen, unsigned char\*\* ppval, size_t& vallen, const char\* encpass)
- int K2HKeyQueue::Remove(int count, k2h_q_remove_trial_callback fp = NULL, void\* pExtData, const char\* encpass)
- bool K2HKeyQueue::Dump(FILE\* stream)

#### メソッド説明
- K2HKeyQueue::K2HKeyQueue
  - K2HShmオブジェクト、FIFO / LIFO を指定して K2HKeyQueueオブジェクトを生成します。 生成された時点では K2HASHデータへの操作（書き込みなど）は行いません。
- K2HKeyQueue::Init  
  - K2HShmオブジェクトを指定して K2HKeyQueueオブジェクトを初期化します。 初期化された時点では K2HASHデータへの操作（書き込みなど）は行いません。メソッドは基本クラスの K2HQueue::Init です。
- K2HKeyQueue::Init  
  - プレフィックスを指定して K2HKeyQueueオブジェクトを初期化します。 初期化された時点では K2HASHデータへの操作（書き込みなど）は行いません。メソッドは基本クラスの K2HQueue::Init です。
- K2HKeyQueue::IsEmpty  
  キューが空であるか確認します。
- K2HKeyQueue::GetCount  
  キューに蓄積されているデータ数を取得します。
- K2HKeyQueue::Read  
  キューから値（Value）をコピーします。基本クラスの K2HQueue::Pop と同じですが、返されるデータはキューに蓄積 された キー（Key）ではなく、キー（Key）の値（Value）となります。 キー（Key）と値（Value）はキューから削除されません。返された値（Value）ポインタは開放してください（注意参照）。 暗号化されている場合にはパスフレーズを指定してください。
- K2HKeyQueue::Read  
  キューからキー（Key）をコピーし、キー（Key）と値（Value）を返します。 返されるデータはキューに蓄積された キー（Key）と、キー（Key）の値（Value）となります。 キー（Key）と値（Value）はキューから削除されません。 返された キー（Key）と値（Value） ポインタは開放してください（注意参照）。 暗号化されている場合にはパスフレーズを指定してください。
- K2HKeyQueue::Push  
  キューにキー（Key）を蓄積（push）します。 メソッドは基本クラスの K2HQueue::Push です。 よって、キー（Key）は K2HASHデータへ書き込みは行われません。 書き込みを行いたい場合には後述の Pushメソッドを利用してください。
- K2HKeyQueue::Push  
  キューにキー（Key）を蓄積（push）し、キー（Key）と値（Value）をK2HASHデータに書き込みします。 暗号化する場合にはパスフレーズを指定してください。 Expire時間を指定する場合には time_t ポインタを指定してください（指定しない場合には NULL を指定してください）。
- K2HKeyQueue::Pop  
  キューから値（Value）を取り出し（pop）します。 基本クラスの K2HQueue::Pop と引数は同じですが、返されるデータはキューに蓄積されたキー（Key）ではなく、キー（Key）の値（Value）となります。 取り出した後では、キー（Key）と値（Value） はK2HASHデータからも削除されます。 返された値（Value）ポインタは開放してください。 暗号化されている場合にはパスフレーズを指定してください。
- K2HKeyQueue::Pop  
  キューからキー（Key）を取り出し（pop）、キー（Key）と値（Value） を返します。 返されるデータはキューに蓄積されたキー（Key）と、キー（Key）の値（Value）となります。 取り出した後では、キー（Key）と値（Value） はK2HASHデータからも削除されます。 返された キー（Key）と値（Value） ポインタは開放してください。 暗号化されている場合にはパスフレーズを指定してください。
- K2HKeyQueue::Remove  
  指定数分のキー（Key）をキューから削除し、そのキー（Key）と値（Value）もK2HASHデータから削除されます。 削除された キー（Key）と値（Value） は返されません。 蓄積されたキー（Key）数以上を指定した場合はキューが空となり正常終了します。 暗号化されている場合にはパスフレーズを指定してください。 引数 fp には、k2h_q_remove_trial_callbackコールバック関数を指定することができます。  コールバック関数を指定した場合には、削除ごとに呼び出され、削除可否の判定をすることができます。（コールバック関数についての詳細は、k2h_q_remove_ext関数の説明を参照 してください。）
- K2HKeyQueue::Dump  
  キューに関連するキーのダンプを行います。デバッグ用のメソッドです。メソッドは基本クラスの K2HQueue::Dump です。

#### メソッド返り値
- K2HKeyQueue::GetCount  
  キューに蓄積されているデータ数を返します。エラーが発生した場合には 0 を返します。
- K2HKeyQueue::Remove  
  キューから削除した値の数を返します。エラーが発生した場合には、-1 を返します。
- 上記以外  
  成功した場合には true、失敗した場合は false を返します。

#### 注意
本クラスは、操作クラスであり、クラスインスタンスを生成した時点では K2HASHデータへの操作は行いません。実際の操作は、Push、Pop などのメソッドを実行したときに発生します。
キューに多量のデータが蓄積された状態で、K2HKeyQueue::GetCount、K2HKeyQueue::Read を利用した場合、パフォーマンスはよくありません。 利用する場合は留意してください。

#### サンプル
 ```
k2hshm*    pk2hash;
    ・
    ・
    ・
  
// queue object
K2HKeyQueue   myqueue(pk2hash, true/*FIFO*/, reinterpret_cast<const unsigned char*>("MYQUEUE_PREFIX_"), 15);  // without end of nil
// push
if(!myqueue.Push(reinterpret_cast<const unsigned char*>("test_key1"), 10, reinterpret_cast<const unsigned char*>("test_value1"), 12) ||
   !myqueue.Push(reinterpret_cast<const unsigned char*>("test_key2"), 10, reinterpret_cast<const unsigned char*>("test_value2"), 12) )
{
    return false
}
// pop
unsigned char* pkey     = NULL;
size_t         keylen   = 0;
unsigned char* pvalue   = NULL;
size_t         valuelen = 0;
if(!myqueue.Pop(&pkey, keylen, &pvalue, valuelen)){
    return false
}
free(pkey);
free(pvalue);
// remove
if(!myqueue.Remove(1)){
    return false
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HSHM"> K2HShmクラス

#### 説明
K2HASHデータの操作・実装クラスです。 このクラスを通じて、K2HASHデータの操作を行います。
C++言語における基本的なK2HASHデータの操作は、本クラスのインスタンスを作成した後で、全ての操作を行います。インスタンスは、K2HASHファイルもしくはオンメモリをオープン（アタッチ）して生成できます。
操作が完了したら（終了時）、クローズ（デタッチ）してください。

#### クラスメソッド
- size_t K2HShm::GetSystemPageSize(void)
- int K2HShm::GetMaskBitCount(k2h_hash_t mask)
- int K2HShm::GetTransThreadPool(void)
- bool K2HShm::SetTransThreadPool(int count)
- bool K2HShm::UnsetTransThreadPool(void)

#### メソッド
- bool K2HShm::Create(const char\* file, bool isfullmapping = true, int mask_bitcnt = MIN_MASK_BITCOUNT, int cmask_bitcnt = DEFAULT_COLLISION_MASK_BITCOUNT, int max_element_cnt = DEFAULT_MAX_ELEMENT_CNT, size_t pagesize = MIN_PAGE_SIZE)
- bool K2HShm::Attach(const char\* file, bool isReadOnly, bool isCreate = true, bool isTempFile = false, bool isfullmapping = true, int mask_bitcnt = MIN_MASK_BITCOUNT, int cmask_bitcnt = DEFAULT_COLLISION_MASK_BITCOUNT, int max_element_cnt = DEFAULT_MAX_ELEMENT_CNT, size_t pagesize = MIN_PAGE_SIZE)
- bool K2HShm::AttachMem(int mask_bitcnt = MIN_MASK_BITCOUNT, int cmask_bitcnt = DEFAULT_COLLISION_MASK_BITCOUNT, int max_element_cnt = DEFAULT_MAX_ELEMENT_CNT, size_t pagesize = MIN_PAGE_SIZE)
- bool K2HShm::IsAttached(void)
- bool K2HShm::Detach(long waitms = DETACH_NO_WAIT)
<br />
<br />
- k2h_hash_t K2HShm::GetCurrentMask(void) const
- k2h_hash_t K2HShm::GetCollisionMask(void) const
- unsigned long K2HShm::GetMaxElementCount(void) const
- const char\* K2HShm::GetK2hashFilePath(void) const
<br />
<br />
- char\* K2HShm::Get(const char\* pKey, bool checkattr = true, const char\* encpass = NULL) const
- ssize_t K2HShm::Get(const unsigned char\* byKey, size_t length, unsigned char\*\* byValue, bool checkattr = true, const char\* encpass = NULL) const
- ssize_t K2HShm::Get(const unsigned char\* byKey, size_t keylen, unsigned char\*\* byValue, k2h_get_trial_callback fp, void\* pExtData, bool checkattr, const char\* encpass)
- char\* K2HShm::Get(PELEMENT pElement, int type) const
- ssize_t K2HShm::Get(PELEMENT pElement, unsigned char\*\* byData, int type) const
- strarr_t::size_type K2HShm::Get(const char\* pKey, strarr_t& strarr, bool checkattr = true, const char\* encpass = NULL) const
- strarr_t::size_type K2HShm::Get(const unsigned char\* byKey, size_t length, strarr_t& strarr, bool checkattr = true, const char\* encpass = NULL) const
- strarr_t::size_type K2HShm::Get(PELEMENT pElement, strarr_t& strarr, bool checkattr = true, const char\* encpass = NULL) const
- K2HSubKeys\* K2HShm::GetSubKeys(const char\* pKey, bool checkattr = true) const
- K2HSubKeys\* K2HShm::GetSubKeys(const unsigned char\* byKey, size_t length, bool checkattr = true) const
- K2HSubKeys\* K2HShm::GetSubKeys(PELEMENT pElement, bool checkattr = true) const
- K2HAttrs\* K2HShm::GetAttrs(const char\* pKey) const
- K2HAttrs\* K2HShm::GetAttrs(const unsigned char\* byKey, size_t length) const
- K2HAttrs\* K2HShm::GetAttrs(PELEMENT pElement) const
<br />
<br />
- bool K2HShm::Set(const char\* pKey, const char\* pValue, const char\* encpass = NULL, const time_t\* expire = NULL)
- bool K2HShm::Set(const char\* pKey, const char\* pValue, K2HSubKeys\* pSubKeys, bool isRemoveSubKeys = true, const char\* encpass = NULL, const time_t\* expire = NULL)
- bool K2HShm::Set(const char\* pKey, const char\* pValue, K2HAttrs\* pAttrs, const char\* encpass = NULL, const time_t\* expire = NULL)
- bool K2HShm::Set(const unsigned char\* byKey, size_t keylength, const unsigned char\* byValue, size_t vallength, const char\* encpass = NULL, const time_t\* expire = NULL)
- bool K2HShm::Set(const unsigned char\* byKey, size_t keylength, const unsigned char\* byValue, size_t vallength, K2HSubKeys\* pSubKeys, bool isRemoveSubKeys = true, K2HAttrs\* pAttrs = NULL, const char\* encpass = NULL, const time_t\* expire = NULL, K2hAttrOpsMan::ATTRINITTYPE attrtype = K2hAttrOpsMan::OPSMAN_MASK_NORMAL)
- bool K2HShm::AddSubkey(const char\* pKey, const char\* pSubkey, const char\* pValue, const char\* encpass = NULL, const time_t\* expire = NULL)
- bool K2HShm::AddSubkey(const char\* pKey, const char\* pSubkey, const unsigned char\* byValue, size_t vallength, const char\* encpass = NULL, const time_t\* expire = NULL)
- bool K2HShm::AddSubkey(const unsigned char\* byKey, size_t keylength, const unsigned char\* bySubkey, size_t skeylength, const unsigned char\* byValue, size_t vallength, const char\* encpass = NULL, const time_t\* expire = NULL)
- bool K2HShm::AddAttr(const char\* pKey, const char\* pattrkey, const char\* pattrval)
- bool K2HShm::AddAttr(const char\* pKey, const unsigned char\* pattrkey, size_t attrkeylen, const unsigned char\* pattrval, size_t attrvallen)
- bool K2HShm::AddAttr(const unsigned char\* byKey, size_t keylength, const unsigned char\* pattrkey, size_t attrkeylen, const unsigned char\* pattrval, size_t attrvallen)
<br />
<br />
- bool K2HShm::Remove(PELEMENT pParentElement, PELEMENT pSubElement)
- bool K2HShm::Remove(PELEMENT pElement, const char\* pSubKey)
- bool K2HShm::Remove(PELEMENT pElement, const unsigned char\* bySubKey, size_t length)
- bool K2HShm::Remove(PELEMENT pElement, bool isSubKeys = true)
- bool K2HShm::Remove(const char\* pKey, bool isSubKeys)
- bool K2HShm::Remove(const unsigned char\* byKey, size_t keylength, bool isSubKeys = true)
- bool K2HShm::Remove(const char\* pKey, const char\* pSubKey)
- bool K2HShm::Remove(const unsigned char\* byKey, size_t keylength, const unsigned char\* bySubKey, size_t sklength)
- bool K2HShm::ReplaceAll(const unsigned char\* byKey, size_t keylength, const unsigned char\* byValue, size_t vallength, const unsigned char\* bySubkeys, size_t sklength, const unsigned char\* byAttrs, size_t attrlength)
- bool K2HShm::ReplaceValue(const unsigned char\* byKey, size_t keylength, const unsigned char\* byValue, size_t vallength)
- bool K2HShm::ReplaceSubkeys(const unsigned char\* byKey, size_t keylength, const unsigned char\* bySubkeys, size_t sklength)
- bool K2HShm::ReplaceAttrs(const unsigned char\* byKey, size_t keylength, const unsigned char\* byAttrs, size_t attrlength)
<br />
<br />
- bool K2HShm::Rename(const char\* pOldKey, const char\* pNewKey)
- bool K2HShm::Rename(const unsigned char\* byOldKey, size_t oldkeylen, const unsigned char\* byNewKey, size_t newkeylen)
- bool K2HShm::Rename(const unsigned char\* byOldKey, size_t oldkeylen, const unsigned char\* byNewKey, size_t newkeylen, const unsigned char\* byAttrs, size_t attrlen)
<br />
<br />
- K2HDAccess\* K2HShm::GetDAccessObj(const char\* pKey, K2HDAccess::ACSMODE acsmode, off_t offset)
- K2HDAccess\* K2HShm::GetDAccessObj(const unsigned char\* byKey, size_t keylength, K2HDAccess::ACSMODE acsmode, off_t offset)
<br />
<br />
- bool GetElementsByHash(const k2h_hash_t starthash, const struct timespec startts, const struct timespec endts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check, k2h_hash_t\* pnexthash, PK2HBIN\* ppbindatas, size_t\* pdatacnt) const
- bool SetElementByBinArray(const PRALLEDATA prawdata, const struct timespec\* pts)
<br />
<br />
- K2HShm::iterator K2HShm::begin(const char\* pKey)
- K2HShm::iterator K2HShm::begin(const unsigned char\* byKey, size_t length)
- K2HShm::iterator K2HShm::begin(void)
- K2HShm::iterator K2HShm::end(bool isSubKey = false)
<br />
<br />
- K2HQueue\* K2HShm::GetQueueObj(bool is_fifo = true, const unsigned char\* pref = NULL, size_t preflen = 0L)
- K2HKeyQueue\* K2HShm::GetKeyQueueObj(bool is_fifo = true, const unsigned char\* pref = NULL, size_t preflen = 0L)
- bool K2HShm::IsEmptyQueue(const unsigned char\* byMark, size_t marklength) const
- int K2HShm::GetCountQueue(const unsigned char\* byMark, size_t marklength) const
- bool K2HShm::ReadQueue(const unsigned char\* byMark, size_t marklength, unsigned char\*\* ppKey, size_t& keylength, unsigned char\*\* ppValue, size_t& vallength, int pos = 0, const char\* encpass = NULL) const
- bool K2HShm::PushFifoQueue(const unsigned char\* byMark, size_t marklength, const unsigned char\* byKey, size_t keylength, const unsigned char\* byValue, size_t vallength, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs\* pAttrs = NULL, const char\* encpass = NULL, const time_t\* expire = NULL)
- bool K2HShm::PushLifoQueue(const unsigned char\* byMark, size_t marklength, const unsigned char\* byKey, size_t keylength, const unsigned char\* byValue, size_t vallength, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs\* pAttrs = NULL, const char\* encpass = NULL, const time_t\* expire = NULL)
- bool K2HShm::PopQueue(const unsigned char\* byMark, size_t marklength, unsigned char\*\* ppKey, size_t& keylength, unsigned char\*\* ppValue, size_t& vallength, K2HAttrs\*\* ppAttrs = NULL, const char\* encpass = NULL)
- int K2HShm::RemoveQueue(const unsigned char\* byMark, size_t marklength, unsigned int count, bool rmkeyval, k2h_q_remove_trial_callback fp = NULL, void\* pExtData = NULL, const char\* encpass = NULL)
<br />
<br />
- bool K2HShm::Dump(FILE\* stream = stdout, int dumpmask = DUMP_KINDEX_ARRAY)
- bool K2HShm::PrintState(FILE\* stream = stdout)
- bool K2HShm::PrintAreaInfo(void) const
- bool K2HShm::DumpQueue(FILE\* stream, const unsigned char\* byMark, size_t marklength)
- PK2HSTATE K2HShm::GetState(void) const
<br />
<br />
- bool K2HShm::EnableTransaction(const char\* filepath, const unsigned char\* pprefix = NULL, size_t prefixlen = 0, const unsigned char\* pparam = NULL, size_t paramlen = 0, const time_t\* expire = NULL) const
- bool K2HShm::DisableTransaction(void) const
<br />
<br />
- bool K2HShm::SetCommonAttribute(const bool\* is_mtime = NULL, const bool\* is_defenc = NULL, const char\* passfile = NULL, const bool\* is_history = NULL, const time_t\* expire = NULL, const strarr_t\* pluginlibs = NULL)
- bool K2HShm::AddAttrCryptPass(const char\* pass, bool is_default_encrypt = false)
- bool K2HShm::AddAttrPluginLib(const char\* path)
- bool K2HShm::CleanCommonAttribute(void)
- bool K2HShm::GetAttrVersionInfos(strarr_t& verinfos) const
- void K2HShm::GetAttrInfos(std::stringstream& ss) const
<br />
<br />
- bool K2HShm::AreaCompress(bool& isCompressed)
- bool K2HShm::SetMsyncMode(bool enable)

#### メソッド説明
- K2HShm::GetSystemPageSize  
  k2hashオブジェクトの操作する K2HASHデータ のページサイズを返します。
- K2HShm::GetMaskBitCount  
  k2hashオブジェクトの操作する K2HASHデータ の Maskの値を返します。
- K2HShm::GetTransThreadPool  
  k2hashクラスに設定されているトランザクション用スレッドプールの数を返します。
- K2HShm::SetTransThreadPool  
  k2hashクラスに設定されているトランザクション用スレッドプールの数を設定します。
- K2HShm::UnsetTransThreadPool  
  k2hashクラスに設定されているトランザクション用スレッドプールを利用しないように設定します。
- K2HShm::Create  
  K2HASHファイルの生成をします。
- K2HShm::Attach  
  K2HASHファイル、メモリにアタッチします。
- K2HShm::AttachMem  
  K2HASHメモリにアタッチします。
- K2HShm::Detach  
  アタッチしているK2HASHデータをデタッチします。 デタッチするときにトランザクション処理の完了を待つ方法を提供しています。 引数により待機ms（ミリ秒）を指定できます。 引数に 0 を指定した場合には即座にデタッチします。 -1 を指定した場合にはトランザクションが完了するまでブロックします。 正数はms（ミリ秒）を示し、指定されたms間トランザクションの完了を待ちます。
- K2HShm::GetCurrentMask  
  現在の Current Mask の値を取得します。
- K2HShm::GetCollisionMask  
  現在の衝突Mask の値を取得します。
- K2HShm::GetMaxElementCount  
  現在のエレメントの最大数を取得します。
- K2HShm::GetK2hashFilePath  
  現在オープンしている K2HASHファイルパスを取得します。
- K2HShm::Get  
  キー（Key）を指定して、値（value）やサブキー（Subkey）リストなどを取得します。 k2h_get_trial_callbackコールバック関数ポインタを指定するメソッドについては、k2h_get_value_ext関数と同等の処理を行います（コールバック関数およびこの関数の詳細な説明は、k2h_get_value_ext関数の説明を参照してください）。 対象を取得するときに属性（Attribute）の検査を実施するかどうかのフラグを指定することができます。 これは、Builtin属性などには値を復号しなければ取り出せないもの、Expire時刻の検査が必要がものなどがあるためです。Builtin属性を指定したキーの場合にはこれらの引数を適宜利用してください。
- K2HShm::GetSubKeys  
  キー（Key）を指定して、サブキー（Subkey）リストを取得します。 K2HSubKeysクラスオブジェクトポインタとして取得します。 取得した K2HSubKeysクラスオブジェクトポインタは、delete() で開放してください。 サブキーを取得するときに属性（Attribute）の検査を実施するかどうかのフラグを指定することができます。 これは、Builtin属性には Expire時刻の検査が必要がものなどがあるためです。 Builtin属性を指定したキーの場合にはフラグを適宜指定してください。
- K2HShm::GetAttrs  
  キー（Key）を指定して、設定されている属性（Attribute）を取得します。 K2HAttrsクラスオブジェクトのポインタとして取得します。 取得した K2HAttrsクラスオブジェクトポインタは、delete() で開放してください。
- K2HShm::Set  
  キー（Key）を指定して、値（value）やサブキー（Subkey）リストなどを設定します。 設定する際に、Builtin属性である暗号化、Expire時間の指定を個別に設定することも可能です（K2hAttrOpsMan::ATTRINITTYPE引数をとるタイプのメソッドがありますが、このメソッドは内部実装のためのものであり、直接利用することはほとんどありません）。
- K2HShm::AddSubkey  
  キー（Key）を指定して、サブキー（Subkey）とサブキーの値（Value）を設定します。 サブキーの生成も同時に行われます。 設定する際に、Builtin属性である暗号化、Expire時間の指定を個別に設定することも可能です。
- K2HShm::AddAttr  
  キー（Key）を指定して、属性（Attribute）を追加します。 属性名、属性値を指定します。 任意の名前、値で属性を追加できますが、Builtin属性、Plugin属性などと同じ名前を使用しないようにしてください。
- K2HShm::Remove  
  キー（Key）を指定して、値（value）やサブキー（Subkey）リスト、サブキー（Subkey）のみ、を削除します。
- K2HShm::ReplaceAll  
  キー（Key）の値（value）、サブキー（Subkey）リスト、属性（Attribute）を差し替えます。
- K2HShm::ReplaceValue  
  キー（Key）の値（value）を差し替えます。
- K2HShm::ReplaceSubkeys  
  キー（Key）のサブキー（Subkey）リストを差し替えます。
- K2HShm::ReplaceAttrs  
  キー（Key）の属性（Attribute）を差し替えます。
- K2HShm::Rename  
  キー（Key）を別名のキーにリネームします。値、サブキーリスト、属性の変更は行われず、キー名だけの変更ができます。
- K2HShm::GetDAccessObj  
  キー （Key）とモード（K2HDAccess::ACSMODE）を指定して、K2HDAccessオブジェクトを取得します。モードには、 K2HDAccess::READ_ACCESS、K2HDAccess::WRITE_ACCESS、K2HDAccess::RW_ACCESS のいずれかを指定します。
- K2HShm::GetElementsByHash  
  開始HASH値、時刻範囲を指定して、指定範囲の HASH値（マスク後の開始 HASH値、個数、最大 HASH値）に合致する最初のデータを検出し、1つのマスク後の HASH値に紐付けられたデータ（キー、値、サブキー、属性）をバイナリ列として取得します（1つのマスクされたHASH値には複数のキー＆値が存在する）。 また、 検出後は次の開始HASH値も返します。
- K2HShm::SetElementByBinArray  
  GetElementsByHash() を使って取り出した1つのバイナリデータ（キー、値、サブキー、属性）を K2HASHデータベースに設定します。 GetElementsByHash で取得した PK2HBIN配列の1つのデータ（K2HBIN）構造体のbyptrメンバーは、BALLEDATA共用体であり、そのメンバー rawdata のポインタを指定します（つまり、"&((reinterpret_cast<PBALLEDATA>(pbindatas[x].byptr))->rawdata)"を指定します）。
- K2HShm::begin  
  キー（Key）の探索開始イテレータK2HShm::iterator を取得します。
- K2HShm::end  
  キー（Key）の探索終了イテレータK2HShm::iterator を取得します。
- K2HShm::GetQueueObj  
  現在オープンしているK2HASHファイルに対する K2HQueue オブジェクトを取得します。返されたオブジェクトポインタは開放してください。
- K2HShm::GetKeyQueueObj  
  現在オープンしているK2HASHファイルに対する K2HKeyQueue オブジェクトを取得します。返されたオブジェクトポインタは開放してください。
- K2HShm::ReadQueue  
  Queueのマーカーを指定して、キューから値を読み出します。このメソッドを直接使用するのではなく、K2HQueue や K2HKeyQUeueクラスを利用してください。
- K2HShm::PushFifoQueue  
  Queueのマーカーを指定して、FIFOキューへ値を蓄積（push）します。このメソッドを直接使用するのではなく、K2HQueueやK2HKeyQUeueクラスを利用してください。
- K2HShm::PushLifoQueue  
  Queueのマーカーを指定して、LIFOキューへ値を蓄積（push）します。このメソッドを直接使用するのではなく、K2HQueueやK2HKeyQUeueクラスを利用してください。
- K2HShm::PopQueue  
  Queueのマーカーを指定して、キューから値を取り出し（pop）ます。このメソッドを直接使用するのではなく、K2HQueueやK2HKeyQUeueクラスを利用してください。
- K2HShm::RemoveQueue  
  Queueのマーカーを指定して、キューから指定個数分の値を削除します。このメソッドを直接使用するのではなく、K2HQueueやK2HKeyQUeueクラスを利用してください。（k2h_q_remove_trial_callbackコールバック関数について、およびコールバック関数の指定時の動作については、k2h_q_remove_ext関数の説明を参照してください。）
- K2HShm::Dump  
  K2HASHデータのダンプを行います。出力先の指定（デフォルトはstderr）、ダンプレベルを指定できます。 ダンプレベルには、K2HShm::DUMP_HEAD、K2HShm::DUMP_KINDEX_ARRAY、 K2HShm::DUMP_CKINDEX_ARRAY、K2HShm::DUMP_ELEMENT_LIST、 K2HShm::DUMP_PAGE_LIST が指定できます。
- K2HShm::PrintState  
  K2HASHデータの状態を出力します。出力先の指定（デフォルトは stderr）ができます。
- K2HShm::PrintAreaInfo  
  K2HASHファイルの内部エリアの情報を表示します。
- K2HShm::GetState  
  K2HASHデータの状態、設定情報などをまとめた構造体 PK2HSTATE へのポインタを返します。 返されたポインタは開放する必要があります。
- K2HShm::EnableTransaction  
  トランザクションを有効に設定します。
- K2HShm::DisableTransaction  
  トランザクションを無効に設定します。
- K2HShm::SetCommonAttribute  
  Builtin属性の更新時刻（mtime）、履歴機能（history）、Expire時間、暗号化パスフレーズリスト、暗号化デフォルト値の個別設定および、Plugin属性のライブラリロードを行うことができます。 Builtin属性で利用する機能は有効なポインタを指定してください。 利用しない場合にはNULLを指定します。
- K2HShm::AddAttrCryptPass  
  暗号化のためのパスフレーズを1つ追加ロードします。（ロード済みパスフレーズはそのままロードされたままです。）
- K2HShm::AddAttrPluginLib  
  Plugin属性のライブラリを1つロードします。
- K2HShm::CleanCommonAttribute  
  Builtin属性の設定（個別）内容を破棄し、初期化状態（Builtin属性を利用しない）にします。
- K2HShm::GetAttrVersionInfos  
  属性（Attribute）のBuiltin属性、Plugin属性のライブラリのバージョン情報を取得します。
- K2HShm::GetAttrInfos  
  現在の属性（Attribute）の設定状態を取得します。
- K2HShm::AreaCompress  
  k2hashファイルの圧縮を行います。（通常、直接利用する必要はありません。代わりに k2hcompressツールを利用してください。）
- K2HShm::SetMsyncMode  
  mmap の sync を適宜実行するか否かを指定します。デフォルトは有効となっています。（通常利用する必要はありません。）

#### メソッド返り値
個々のプロトタイプを参照してください。
bool値の返り値のプロトタイプは、成功した場合には、true を返します。失敗した場合には false を返します。

#### サンプル
 ```
K2HShm    k2hash;
if(!k2hash.Attach("/tmp/myk2hash.k2h", false, true, false, true, 8, 4, 1024, 512)){
    exit(-1);
}
if(!k2hash.Set("my key", "my value")){
    k2hash.Detach();
    exit(-1);
}
char*    pValue = k2hash.Get("my key");
if(pValue){
    printf("my key = %s\n", pValue);
    free(pValue);
}
if(!k2hash.Dump(stdout, K2HShm::DUMP_PAGE_LIST)){
    k2hash.Detach();
    exit(-1);
}
k2hash.Detach();
 ```
