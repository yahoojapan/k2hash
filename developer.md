---
layout: contents
language: en-us
title: Developer
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: developerja.html
lang_opp_word: To Japanese
prev_url: build.html
prev_string: Build
top_url: index.html
top_string: TOP
next_url: environments.html
next_string: Environments
---

<!-- -----------------------------------------------------------　-->
# For developer

#### [C API](#CAPI)
[Debug family(C I/F)](#DEBUG)  
[DSO load family(C I/F)](#DSO)  
[Create/Open/Close family(C I/F)](#COC)  
[Transaction archive family(C I / F)](#TRANSACTION)  
[Attribute family(C I / F)](#ATTR)  
[GET family(C I/F)](#GET)  
[SET family(C I/F)](#SET)  
[Rename family(C I/F)](#RENAME)  
[Direct binary data acquisition / setting family(C I/F)](#DIRECTBINARY)   
[Delete family(C I/F)](#DELETE)  
[Search family(C I/F)](#FIND)  
[Direct access family(C I/F)](#DIRECTACCESS)  
[Queue family(C I/F)](#QUE)  
[Queue (key & value) family(C I/F)](#KVQUE)  
[Dump and status family(C I/F)](#DUMP)  
#### [C++ API](#CPP)
[Debug family(C/C++ I/F)](#DEBUGCPP)  
[K2HashDynLib class](#K2HASHDYNLIB)  
[K2HTransDynLib class](#K2HTRANSDYNLIB)  
[K2HDAccess class](#K2HDACCESS)  
[k2hstream(ik2hstream、ok2hstream) class](#K2HSTREAM)  
[K2HArchive class](#K2HARCHIVE)  
[K2HQueue class](#K2HQUEUE)  
[K2HKeyQueue class](#K2HKEYQUEUE)  
[K2HShm class](#K2HSHM)  

<!-- -----------------------------------------------------------　-->
***

## <a name="CAPI"> C API
It is an API for C language.  
Include the following header files when developing.  


 ```
#include <k2hash/k2hash.h>
 ```

When linking please specify the following as an option.
 ```
-lk2hash
 ```

The functions for C language are explained below.

<!-- -----------------------------------------------------------　-->
***

### <a name="DEBUG"> Debug family(C I/F)
The K2HASH library can output messages to check internal operation and API operation.
This function group is a group of functions for controlling message output

#### Format
- void k2h_bump_debug_level(void)
- void k2h_set_debug_level_silent(void)
- void k2h_set_debug_level_error(void)
- void k2h_set_debug_level_warning(void)
- void k2h_set_debug_level_message(void)
- bool k2h_set_debug_file(const char* filepath)
- bool k2h_unset_debug_file(void)
- bool k2h_load_debug_env(void)
- bool k2h_set_bumpup_debug_signal_user1(void)

#### Description
- k2h_bump_debug_level  
  Bump up message output level as non-output -> error -> warning -> information -> non-output.
- k2h_set_debug_level_silent  
  Set the message output level to no output.
- k2h_set_debug_level_error  
  Set the message output level to error.
- k2h_set_debug_level_warning  
  Set the message output level above the warning.
- k2h_set_debug_level_message  
  Set the message output level above the information.
- k2h_set_debug_file  
  Specify the file to output message. If it is not set, it is output to stderr.
- k2h_unset_debug_file  
  Unset to output messages to stderr.
- k2h_load_debug_env  
  Read the environment variables K2HDBGMODE, K2HDBGFILE and set the message output and output destination according to the value.
- k2h_set_bumpup_debug_signal_user1  
  Set the signal handler SIGUSR1. When it is set up, it will bump up the message output level each time it receives SIGUSR1.

#### Return Values
K2h_set_debug_file, k2h_unset_debug_file, k2h_load_debug_env, k2h_set_bumpup_debug_signal_user1 will return true on success. If it fails, it returns false.

#### Note
For the environment variables K2HDBGMODE and K2HDBGFILE, see the [Environments](environments.html) in the manual.

#### Examples
 ```
k2h_set_bumpup_debug_signal_user1();
k2h_set_debug_file("/var/log/k2hash/error.log");
k2h_set_debug_level_message();
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="DSO"> DSO load family(C I/F) 
The K2HASH library can read the internal HASH function and the function for transaction processing as an external shared library (DSO module).
This function group loads and unloads these shared libraries.

#### Format
- bool k2h_load_hash_library(const char* libpath)
- bool k2h_unload_hash_library(void)
- bool k2h_load_transaction_library(const char* libpath)
- bool k2h_unload_transaction_library(void)

#### Description
- k2h_load_hash_library  
  Load the DSO module of the HASH function used in the K2HASH library. Specify the file path.
- k2h_unload_hash_library  
  Unload the loaded HASH function (DSO module).
- k2h_load_transaction_library  
  Load DSO module of transaction plugin. Specify the file path.
- k2h_unload_transaction_library  
  Unload the loaded transaction plug-in (DSO module).

#### Return Values
Returns true if it succeeds. If it fails, it returns false.

#### Note
K2HASH has a built-in HASH function (FNV-1A) in advance, and a transaction plug-in (Bultin transaction plug-in) that performs simple processing.
When loading the DSO module with this function group, it takes precedence over the HASH function and transaction plug-in installed inside them and is replaced.

#### Examples
 ```
if(!k2h_load_hash_library("/usr/lib64/myhashfunc.so")){
    return false;
}
if(!k2h_load_transaction_library("/usr/lib64/mytransfunc.so")){
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

### <a name="COC"> Create/Open/Close family(C I/F)
It is a group of functions that initialize, open (attach), close (detach) K2HASH (file or on memory).

#### Format
- bool k2h_create(const char* filepath, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open(const char* filepath, bool readonly, bool removefile, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open_rw(const char* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open_ro(const char* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open_tempfile(const char* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- k2h_h k2h_open_mem(int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
- bool k2h_close(k2h_h handle)
- bool k2h_close_wait(k2h_h handle, long waitms)

#### Description
- k2h_create  
  Initialize (generate) the K2HASH file.
- k2h_open  
  Attach to the K2HASH file. When filepath is set to NULL, it operates as on-memory.
- k2h_open_rw  
  Attach to the K2HASH file with Read / Write attribute. Wrapper function of k2h_open function.
- k2h_open_ro  
  Attach to the K2HASH file with Read attribute only. Wrapper function of k2h_open function.
- k2h_open_tempfile  
  Attach to the K2HASH file with Read / Write attribute. K2HASH file is opened as a temporary file and deleted when k2h_close is called. Wrapper function of k2h_open function.
- k2h_open_mem  
  Attach K2HASH as on-memory. Wrapper function of k2h_open function.
- k2h_close  
  Close the K2HASH handle.
- k2h_close_wait  
  Close the K2HASH handle. You can specify milliseconds as an argument and wait for transaction processing to complete. You can specify whether the transaction completes completely, waits for specified ms, or exits immediately (equivalent to k2h_close).

#### Parameters
- filepath  
  Specify the path of the K2HASH file.
- maskbitcnt  
  Specify the initial MASK bit count number. Specify 8 or more. When opening an existing K2HASH file, the value set in the file takes precedence and this value is ignored.
- cmaskbitcnt  
  CMASK bit count number is specified. We recommend 4 or more. When opening an existing K2HASH file, the value set in the file takes precedence and this value is ignored.
- maxelementcnt  
  Sets the upper limit of the number of duplicated elements (elements) at the time of HASH value conflict. We recommend 32 or more. When opening an existing K2HASH file, the value set in the file takes precedence and this value is ignored.
- pagesize  
  This is the block size at the time of data storage and is set as the page size. Specify 128 or more. When opening an existing K2HASH file, the value set in the file takes precedence and this value is ignored.
- readonly  
  Attach the file with only the read attribute.
- removefile  
 K2h_close is called, and the file is deleted when the file is completely detached (including other processes). Specify this when you want to use the file as temporary temporary (ex: I want to use it as cache KVS shared by multiprocess).
- fullmap  
  Specify true for memory mapping of the whole area of the attached file. If false, only the management area of K2HASH is memory mapped, and areas such as data are not memory mapped.
   For on-memory type, fullmap = true is always the same state, even if false is specified for this value, it is ignored.
- handle  
  Specify the K2HASH handle returned from the k2h_open family of functions.
- waitms  
  Specify the number of milliseconds to wait for transaction processing to complete. If 0 is specified, the operation is equivalent to k2h_close. If a positive number is specified, the processing of the transaction is terminated by that number of ms. If it does not complete within the specified msec, close it without waiting for completion. If -1 is specified, it is blocked until the transaction is completed.

#### Return Values
- k2h_create / k2h_close
  Returns true if it succeeds. If it fails, it returns false.
- k2h_open  / k2h_open_rw / k2h_open_ro / k2h_open_tempfile / k2h_open_mem
  If it succeeds, it returns a valid K2HASH handle. If it fails, K2H_INVALID_HANDLE (0) is returned.

#### Note 
The K2HASH handle returned from each function is a handle that you specify to manipulate the K2HASH file (or on memory) with other API functions.
Be sure to detach (close) the attached (open) K2HASH handle.
MASK, CMASK, and elementcnt are values that determine the structure (HASH table) of the data tree managed internally by the K2HASH library. We recommend that you try out the attached k2hlintool tool, try out the effect of that value, and set the value.
When attaching an already existing K2HASH file, these values are ignored and the set value is used.

#### Examples
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

### <a name="TRANSACTION"> Transaction archive family(C I / F)
Functions related to transaction processing provided by the K2HASH library.

#### Format
- bool k2h_transaction(k2h_h handle, bool enable, const char* transfile)
- bool k2h_enable_transaction(k2h_h handle, const char* transfile)
- bool k2h_disable_transaction(k2h_h handle)
- bool k2h_transaction_prefix(k2h_h handle, bool enable, const char* transfile, const unsigned char* pprefix, size_t prefixlen)
- bool k2h_transaction_param(k2h_h handle, bool enable, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen)
- bool k2h_transaction_param_we(k2h_h handle, bool enable, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen, const time_t* expire)
- bool k2h_enable_transaction_prefix(k2h_h handle, const char* transfile, const unsigned char* pprefix, size_t prefixlen)
- bool k2h_enable_transaction_param(k2h_h handle, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen)
- bool k2h_enable_transaction_param_we(k2h_h handle, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen, const time_t* expire)
- int k2h_get_transaction_archive_fd(k2h_h handle)
- int k2h_get_transaction_thread_pool(void)
- bool k2h_set_transaction_thread_pool(int count)
- bool k2h_unset_transaction_thread_pool(void)
- bool k2h_load_archive(k2h_h handle, const char* filepath, bool errskip)
- bool k2h_put_archive(k2h_h handle, const char* filepath, bool errskip)

#### Description
- k2h_transaction  
  Enable or disable transaction function.
- k2h_enable_transaction  
  Activate the transaction function. A wrapper function for k2h_transaction.
- k2h_disable_transaction  
  Disable the transaction function. A wrapper function for k2h_transaction.
- k2h_transaction_prefix  
  Enable / disable transaction function. This function does not use the default key prefix used for internal queues for transactions, and can be specified by yourself.
- k2h_transaction_param  
  Enable or disable transaction function. This function does not use the default key prefix used for internal queues for transactions, and can be specified by yourself. It also allows you to pass your own parameters to transaction plugins.
- k2h_transaction_param_we  
  It is equivalent to k2h_transaction_param. In transaction processing, Expire time can be specified in the transaction data in advance.
- k2h_enable_transaction_prefix  
  Activate the transaction function. A wrapper function for k2h_transaction_ prefix.
- k2h_enable_transaction_param  
  Activate the transaction function. A wrapper function for k2h_transaction_param.
- k2h_enable_transaction_param_we  
  It is equivalent to k2h_enable_transaction_param. In transaction processing, Expire time can be specified in the transaction data in advance.
- k2h_get_transaction_archive_fd  
  It is a function that acquires the file descriptor if the transaction is valid and the file path to record the transaction is set.
- k2h_get_transaction_thread_pool  
  Get number of multithreader worker pool for transaction processing. This pool number is common to all K2HASH library instances (even if attached to multiple K2HASH files (or on memory), it becomes the number of common pool as library).
- k2h_set_transaction_thread_pool  
  Set number of multithreader worker pool for transaction processing.
- k2h_unset_transaction_thread_pool  
  Avoid using multithreading for transaction processing. The number of pools will be 0.
- k2h_load_archive  
  Load K2HASH data from the archive file. The K2HASH data before loading remains unchanged, and it is overwritten with the archive data.
  The archive file can specify the following files.
 - Files output by k2h_put_archive
 - Transaction file output by k2h_transaction system function (default format file not replacing transaction related function with its own one)
 - Transaction or archive file created by k2hlinetool tool
- k2h_put_archive  
  Archive (serialize) all data of the K2HASH file (or on memory) and output it to a file.

#### Parameters
- handle  
  Specify the K2HASH handle returned from the k2h_open family of functions.
- enable  
  Specify whether the transaction function is enabled (true) or disabled (false).
- transfile  
  Specify the file path to record the transaction. You can specify NULL if you do not want to use the file.
- pprefix  
  Specify the prefix of the key (created in the K2HASH data) for queuing the transaction.
- prefixlen  
  Specify the buffer length of pprefix when specifying the prefix of the key (created in the K2HASH data) for queuing the transaction.
- pparam  
  Specify your own parameters to pass to the transaction plug-in.
- paramlen  
  Specify the pparam buffer length when specifying your own parameters to be passed to the transaction plug-in. (The maximum length of buffer that pparam can specify is up to 32 Kbytes.)
- expire  
  Specify the expiration time (Expire) of the transaction data by a pointer to time_t. If Expire time is not specified, specify NULL.
- count  
  Specify the number of multithreaded workers pooled for transaction processing. If 0 is specified, it is set not to process by multithreading.
- filepath  
  Specify the file path of the archive file.
- errskip  
  Specify whether to continue as it is when an error occurs during output or loading to the archive file. Specify "true" to continue after an error occurs. Otherwise specify false.

#### Return Values
- k2h_get_transaction_archive_fd  
  If the transaction is valid and the file path to record the transaction is set, return that file descriptor. If it is invalid, unset, error etc., it returns -1.
- k2h_get_transaction_thread_pool  
  Returns the number of multithreader worker pool for transaction processing.
- Functions other than the above  
  Returns true if it succeeds. If it fails, it returns false.

#### Note
- Transaction processing of the K2HASH library is realized by using a queue (see K2HQueue) internally. This queue requires a key name and default key prefix is used if you do not want to change it. If you do not use the default key prefix and you want to use a different key prefix, you can change it with the k2h_transaction_ prefix, k2h_enable_transaction_ prefix functions.
- The files that can be loaded with k2h_load_archive are the files output by the Bultin transaction plugin of the K2HASH library, or the files output by k2h_put_archive.
- If K2HASH data exists before calling the k2h_load_archive function, the contents of the archive file are overwritten in the K2HASH data.
- K2h_transaction, k2h_enable_transaction, k2h_transaction_ prefix, k2h_enable_transaction_ prefix allow NULL for the transfile argument (output file path of the transaction). However, you can only accept NULLs if you load your own transaction plugin and that transaction plugin does not require transaction files. The Bultin transaction plugin of the K2HASH library requires a transaction file.
- When processing transactions in multithreading, you can specify the pool number of worker threads. The number of pools is the value commonly set for instances of the K2HASH library. Even if you attach to multiple K2HASH files (or on memory), this pool number will be used as an instance.

#### Examples
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

### <a name="ATTR"> Attribute family(C I / F)
A set of functions related to attributes (Attribute) provided by the K2HASH library.

#### Format
- bool k2h_set_common_attr(k2h_h handle, const bool* is_mtime, const bool* is_defenc, const char* passfile, const bool* is_history, const time_t* expire)
- bool k2h_clean_common_attr(k2h_h handle)
- bool k2h_add_attr_plugin_library(k2h_h handle, const char* libpath)
- bool k2h_add_attr_crypt_pass(k2h_h handle, const char* pass, bool is_default_encrypt)
- bool k2h_print_attr_version(k2h_h handle, FILE* stream)
- bool k2h_print_attr_information(k2h_h handle, FILE* stream)

#### Description
- k2h_set_common_attr  
  Configure the attributes (Builtin attribute) incorporated in the K2HASH library. Set arguments according to the type of Builtin attribute (update time stamp, history function, expire function, encryption function, passphrase file specification).
- k2h_clean_common_attr  
  Initialize (clear) the setting of the Builtin attribute.
- k2h_add_attr_plugin_library  
  The K2HASH library can load and incorporate a shared library (DSO module) of its own attribute. This is called Plugin attribute.
  This function loads DSO module with Plugin attribute.
- k2h_add_attr_crypt_pass  
  Register encryption passphrase for Builtin attribute. Also, enable or disable the encryption function. (In case of passphrase being registered in invalid case, decryption will be done only.)
- k2h_print_attr_version  
  It displays (get) version of Builtin attribute and Plugin attribute.
- k2h_print_attr_information  
  It displays (get) the information of Builtin attribute and Plugin attribute.

#### Parameters
- handle  
  Specify the K2HASH handle returned from the k2h_open family of functions.
- is_mtime  
  Updating the Builtin attribute To specify whether to enable or disable the time stamp function, pass a pointer to the bool value. To change from the currently set state, specify NULL. (The Builtin attribute also reads settings from environment variables, so if you do not want to change that state, specify NULL.)
- is_defenc  
  To specify whether to enable or disable the encryption function of the Builtin attribute, pass a pointer to the bool value. To change from the currently set state, specify NULL. (The Builtin attribute also reads settings from environment variables, so if you do not want to change that state, specify NULL.)
- passfile  
  Specify the passphrase file used for encrypting and decrypting the Builtin attribute. If not specified, specify NULL. (The Builtin attribute also reads settings from environment variables, so if you do not want to change that state, specify NULL.)
- is_history  
  To specify validity or invalidity of the history function of Builtin attribute, pass a pointer to bool value. To change from the currently set state, specify NULL. (The Builtin attribute also reads settings from environment variables, so if you do not want to change that state, specify NULL.)
- expire  
  To specify the expiration date (seconds) used by the Expire function of the Builtin attribute, pass a pointer to the time_t value. To change from the currently set state, specify NULL. (The Builtin attribute also reads settings from environment variables, so if you do not want to change that state, specify NULL.)

#### Return Values
Returns true if it succeeds. If it fails, it returns false.

#### Note
The Builtin attribute is set with the k2h_set_common_attr function. The K2HASH library can also read the setting value of the Builtin attribute from the environment variable.
First, at the time of initialization of the K2HASH library, read the set value of the attribute from the environment variable. After that, the setting of the Builtin attribute is overwritten by a call such as the k2h_set_common_attr function.
Therefore, as for the value which does not change from the initial value (including setting of environment variable), initial value can be maintained by specifying NULL. )

#### Examples
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

### <a name="GET"> GET family(C I/F)
This is a function group that reads data from K2HASH file (or on memory).

#### Format
- bool k2h_get_value(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength)
- unsigned char* k2h_get_direct_value(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength)
- bool k2h_get_str_value(k2h_h handle, const char* pkey, char** ppval)
- char* k2h_get_str_direct_value(k2h_h handle, const char* pkey)
<br />
<br />
- bool k2h_get_value_wp(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, const char* pass)
- unsigned char* k2h_get_direct_value_wp(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, const char* pass)
- bool k2h_get_str_value_wp(k2h_h handle, const char* pkey, char** ppval, const char* pass)
- char* k2h_get_str_direct_value_wp(k2h_h handle, const char* pkey, const char* pass)
<br />
<br />
- bool k2h_get_value_np(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength)
- unsigned char* k2h_get_direct_value_np(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength)
- bool k2h_get_str_value_np(k2h_h handle, const char* pkey, char** ppval)
- char* k2h_get_str_direct_value_np(k2h_h handle, const char* pkey)
<br />
<br />
- bool k2h_get_value_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData)
- unsigned char* k2h_get_direct_value_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData)
- bool k2h_get_str_value_ext(k2h_h handle, const char* pkey, char** ppval, k2h_get_trial_callback fp, void* pExtData)
- char* k2h_get_str_direct_value_ext(k2h_h handle, const char* pkey, k2h_get_trial_callback fp, void* pExtData)
<br />
<br />
- bool k2h_get_value_wp_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData, const char* pass)
- unsigned char* k2h_get_direct_value_wp_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData, const char* pass)
- bool k2h_get_str_value_wp_ext(k2h_h handle, const char* pkey, char** ppval, k2h_get_trial_callback fp, void* pExtData, const char* pass)
- char* k2h_get_str_direct_value_wp_ext(k2h_h handle, const char* pkey, k2h_get_trial_callback fp, void* pExtData, const char* pass)
<br />
<br />
- bool k2h_get_value_np_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData)
- unsigned char* k2h_get_direct_value_np_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData)
- bool k2h_get_str_value_np_ext(k2h_h handle, const char* pkey, char** ppval, k2h_get_trial_callback fp, void* pExtData)
- char* k2h_get_str_direct_value_np_ext(k2h_h handle, const char* pkey, k2h_get_trial_callback fp, void* pExtData)
<br />
<br />
- bool k2h_get_subkeys(k2h_h handle, const unsigned char* pkey, size_t keylength, PK2HKEYPCK* ppskeypck, int* pskeypckcnt)
- PK2HKEYPCK k2h_get_direct_subkeys(k2h_h handle, const unsigned char* pkey, size_t keylength, int* pskeypckcnt)
- int k2h_get_str_subkeys(k2h_h handle, const char* pkey, char*** ppskeyarray)
- char** k2h_get_str_direct_subkeys(k2h_h handle, const char* pkey)
<br />
<br />
- bool k2h_get_subkeys_np(k2h_h handle, const unsigned char* pkey, size_t keylength, PK2HKEYPCK* ppskeypck, int* pskeypckcnt)
- PK2HKEYPCK k2h_get_direct_subkeys_np(k2h_h handle, const unsigned char* pkey, size_t keylength, int* pskeypckcnt)
- int k2h_get_str_subkeys_np(k2h_h handle, const char* pkey, char*** ppskeyarray)
- char** k2h_get_str_direct_subkeys_np(k2h_h handle, const char* pkey)
<br />
<br />
- bool k2h_free_keypack(PK2HKEYPCK pkeys, int keycnt)
- bool k2h_free_keyarray(char** pkeys)
<br />
<br />
- bool k2h_get_attrs(k2h_h handle, const unsigned char* pkey, size_t keylength, PK2HATTRPCK* ppattrspck, int* pattrspckcnt)
- PK2HATTRPCK k2h_get_direct_attrs(k2h_h handle, const unsigned char* pkey, size_t keylength, int* pattrspckcnt)
- PK2HATTRPCK k2h_get_str_direct_attrs(k2h_h handle, const char* pkey, int* pattrspckcnt)
<br />
<br />
- bool k2h_free_attrpack(PK2HATTRPCK pattrs, int attrcnt)

#### Description
- k2h_get_value  
  Fetch the value set for the key. (If encrypted, passphrase is registered, if there is a matching passphrase, decrypted value can be obtained.If value of decryption failed, value can not be retrieved)
- k2h_get_direct_value  
  Fetch the value set for the key. Return Value  with return value. (The same as k2h_get_value for decryption)
- k2h_get_str_value  
  Fetch the value set for the key. It is a function assuming that both keys and values are character strings. If it is a binary string, please do not use this function. (The same as k2h_get_value for decryption)
- k2h_get_str_direct_value  
  Fetch the value set for the key. Return value is Value. It is a function assuming that both keys and values are character strings. If it is a binary string, please do not use this function. (Same as k2h_get_value for decryption)
- k2h_get_value_wp  
  K2h_get_value Equivalent, you can specify a passphrase for decryption.
- k2h_get_direct_value_wp  
  K2h_get_direct_value Equivalent, you can specify a passphrase for decryption.
- k2h_get_str_value_wp  
  K2h_get_str_value Equivalent and can specify passphrase for decryption.
- k2h_get_str_direct_value_wp  
  K2h_get_str_direct_value Equivalent, you can specify a passphrase for decryption.
- k2h_get_value_np  
  It is equivalent to k2h_get_value and retrieves a value that will not be decrypted even if encrypted.
- k2h_get_direct_value_np  
  K2h_get_direct_value Equivalent and retrieve values ​​that will not be declared even if encrypted.
- k2h_get_str_value_np  
  It is equivalent to k2h_get_str_value and fetches a value that will not be decrypted even if encrypted.
- k2h_get_str_direct_value_np
  K2h_get_str_direct_value Equivalent and retrieve values ​​that will not be declared even if encrypted.
- k2h_get_value_ext  
  Fetch the value  set for the key . However, the callback function (described below) is called with the argument pExtData and is called. You can change (overwrite) the value  at the time of reading with this callback function. If the key  does not exist in the K2HASH data, the callback function is called and the value  can be set. By doing this, it is possible to give the timing to set the initial value when reading the unset key . (Same as k2h_get_value for decryption)
- k2h_get_direct_value_ext  
  It is equivalent to k2h_get_value_ext and directly returns Value.
- k2h_get_str_value_ext  
  It is equivalent to k2h_get_value_ext and treats Value as a string.
- k2h_get_str_direct_value_ext  
  It is equivalent to k2h_get_str_value_ext and directly returns Value as a string.
- k2h_get_value_wp_ext  
  K2h_get_value_ext Equivalent, you can specify a passphrase for decryption.
- k2h_get_direct_value_wp_ext  
  K2h_get_direct_value_ext Equivalent and can specify passphrase for decryption.
- k2h_get_str_value_wp_ext  
  K2h_get_str_value_ext Equivalent, you can specify a passphrase for decryption.
- k2h_get_str_direct_value_wp_ext  
  K2h_get_str_direct_value_ext Equivalent, you can specify a passphrase for decryption.
- k2h_get_value_np_ext  
  K2h_get_value_ext It is equivalent and retrieves a value that will not be decrypted even if encrypted.
- k2h_get_direct_value_np_ext  
  K2h_get_direct_value_ext It is equivalent and retrieves a value that will not be decrypted even if encrypted.
- k2h_get_str_value_np_ext  
  K2h_get_str_value_ext It is equivalent and retrieves a value that will not be decrypted even if encrypted.
- k2h_get_str_direct_value_np_ext  
  K2h_get_str_direct_value_ext It is equivalent and retrieves a value that will not be decrypted even if encrypted.
- k2h_get_subkeys  
  Retrieve the subkey list set for the key.
- k2h_get_direct_subkeys  
  Retrieve the subkey list set for the key. The return value is a subkey list.
- k2h_get_str_subkeys  
  Retrieve the subkey list set for the key. It is a function assuming that both keys and subkeys are character strings. If it is a binary string, please do not use this function.    
- k2h_get_str_direct_subkeys  
  Retrieve the subkey list set for the key. The return value is a subkey list. It is a function assuming that the key is a character string. It is a function assuming that both keys and subkeys are character strings. If it is a binary string, please do not use this function.
- k2h_get_subkeys_np
  It is equivalent to k2h_get_subkeys and retrieves values ​​that will not be decrypted even if encrypted.
- k2h_get_direct_subkeys_np
  It is equivalent to k2h_get_direct_subkeys and fetches a value that will not be decrypted even if encrypted.
- k2h_get_str_subkeys_np
  It is equivalent to k2h_get_str_subkeys and retrieves values ​​that will not be decrypted even if encrypted.
- k2h_get_str_direct_subkeys_np
  It is equivalent to k2h_get_str_direct_subkeys and retrieves a value that will not be decrypted even if encrypted.
- k2h_free_keypack
  Free up the area indicated by the K2HKEYPCK pointer returned by the k2h_get_subkeys function.
- k2h_free_keyarray
  Free up the area indicated by the character array (char *) returned by the k2h_get_str_subkeys function.
- k2h_get_attrs
  Retrieves the attribute set for the key.
- k2h_get_direct_attrs
  Retrieves the attribute set for the key. It returns the attribute by return value.
- k2h_get_str_direct_attrs
  Retrieves the attribute name attribute set in the key. It returns the attribute by return value. The attribute name is assumed to be a character string.
- k2h_free_attrpack
  Free up the area indicated by K2HATTRPCK pointer returned by k2h_get_direct_attrs etc.

#### Parameters
- k2h_get_value
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specify a pointer to the binary string of the key .  - keylength
   Specify the length of the key .
 - ppval  
   Specify a pointer to save the binary string of the fetched value . Please free the area set (returned) by this pointer with the free() function.
 - pvallength  
   Specify a pointer that returns the length of the binary string of the retrieved value .
 - pass  
   Specify passphrase.   
- k2h_get_direct_value
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specify a pointer to the binary string of the key .
 - keylength  
   Specify the length of the key .
 - pvallength  
   Specify a pointer that returns the length of the binary string of the retrieved value .
 - pass  
   Specify passphrase.
- k2h_get_str_value
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a character string pointer.
 - ppval  
   Specify a pointer to save the character string of the retrieved value . Please free the area set (returned) by this pointer with the free() function.
 - pass  
   Specify passphrase.
- k2h_get_str_direct_value
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a character string pointer.
 - pass  
   Specify passphrase.
- k2h_get_value*_ext
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specify a pointer to the binary string of the key .
 - keylength  
   Specify the length of the key .
 - ppval  
   Specify a pointer to save the binary string of the fetched value . Please free the area set (returned) by this pointer with the free() function.
 - pvallength  
   Specify a pointer that returns the length of the binary string of the retrieved value .
 - fp  
   Specify a pointer to the callback function (k2h_get_trial_callback).
 - pExtData  
   Specify an arbitrary pointer to be passed to the callback function (k2h_get_trial_callback) (NULL possible).
 - pass  
   Specify passphrase.   
- k2h_get_direct_value*_ext
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specify a pointer to the binary string of the key .
 - keylength  
   Specify the length of the key .
 - pvallength  
   Specify a pointer that returns the length of the binary string of the retrieved value .
 - fp  
   Specify a pointer to the callback function (k2h_get_trial_callback).
 - pExtData  
   Specify an arbitrary pointer to be passed to the callback function (k2h_get_trial_callback) (allow NULL specification).
 - pass  
   Specify passphrase.
- k2h_get_str_value*_ext
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Key  Specifies a character string pointer.
 - ppval  
   Specify a pointer to save the character string of the retrieved value . Please free() the area set (returned) by this pointer with free().
 - fp  
   Specify a pointer to the callback function (k2h_get_trial_callback).
 - pExtData  
   Specify an arbitrary pointer to be passed to the callback function (k2h_get_trial_callback) (allow NULL specification).
 - pass  
   Specify passphrase.
- k2h_get_str_direct_value*_ext  
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Key  Specifies a character string pointer.
 - fp  
   Specify a pointer to the callback function (k2h_get_trial_callback).
 - pExtData  
   Specify an arbitrary pointer to be passed to the callback function (k2h_get_trial_callback) (allow NULL specification).
 - pass  
   Specify passphrase.
- k2h_get_subkeys
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specify a pointer to the binary string of the key .
 - keylength  
   Specify the length of the key .
 - ppskeypck  
   Specify a pointer to save the K2HKEYPCK structure array of the retrieved subkey (Subkey) list. Free the area set (returned) by this pointer with the k2h_free_keypack() function.
 - pskeypckcnt  
   Specify a pointer that returns the number of K2HKEYPCK structure arrays in the retrieved subkey (Subkey) list.
- k2h_get_direct_subkeys
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specify a pointer to the binary string of the key .
 - keylength  
   Specify the length of the key .
 - pskeypckcnt  
   Specify a pointer that returns the number of K2HKEYPCK structure arrays in the retrieved subkey (Subkey) list.
- k2h_get_str_subkeys
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Key  Specifies a character string pointer.
 - ppskeyarray  
   Specify a pointer to save the string array (NULL termination) of the extracted subkey (Subkey) list. Free up the area set (returned) by this pointer using k2h_free_keyarray() function. The string array has a termination of NULL.
- k2h_get_str_direct_subkeys
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Key  Specifies a character string pointer.
- k2h_free_keypack 	
 - pkeys  
   Specify a pointer to the K2HKEYPCK structure array of the subkey (list) returned from k2h_get_subkeys, k2h_get_direct_subkeys.
 - keycnt  
   Specifies the number of K2HKEYPCK structure arrays in the specified subkey (Subkey) list.
- k2h_free_keyarray
 - pkeys  
   K2h_get_str_subkeys, k2h_get_str_subkeys Specifies a pointer to the string array of the subkey (Subkey) list returned from.
- k2h_get_attrs
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specify a pointer to the binary string of the key .
 - keylength  
    Specify the length of the key .
 - ppattrspck  
   Specify a pointer to save the K2HATTRPCK structure array of the extracted attribute. Free up the area set (returned) by this pointer using the k2h_free_attrpack() function.
 - pattrspckcnt  
   Specify a pointer to return the number of K2HATTRPCK structure array of extracted attribute.
- k2h_get_direct_attrs
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specify a pointer to the binary string of the key .
 - keylength  
   Specify the length of the key .
 - pattrspckcnt
   Specify a pointer to return the number of K2HATTRPCK structure array of extracted attribute.
- k2h_get_str_direct_attrs
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Key  Specifies a character string pointer.
 - pattrspckcnt  
   Specify a pointer to return the number of K2HATTRPCK structure array of extracted attribute.
- k2h_free_attrpack  
 - pattrs  
   Specify the K2HATTRPCK pointer of the attribute information returned from k2h_get_direct_attrs, k2h_get_str_direct_attrs.
 - attrcnt  
   Specify the number of attributes included in the K2HATTRPCK pointer of the attribute information returned from k2h_get_direct_attrs, k2h_get_str_direct_attrs.

#### Return Values
- k2h_get_value
  Returns true if it succeeds. If it fails, it returns false.
- k2h_get_str_value  
  Returns true if it succeeds. If it fails, it returns false.
- k2h_get_value*_ext  
  Returns true if it succeeds. If it fails, it returns false.
- k2h_get_str_value*_ext  
  Returns true if it succeeds. If it fails, it returns false.
- k2h_get_subkeys  
  Returns true if it succeeds. If it fails, it returns false.
- k2h_get_attrs  
  Returns true if it succeeds. If it fails, it returns false.
- k2h_free_keypack  
  Returns true if it succeeds. If it fails, it returns false.
- k2h_free_keyarray  
  Returns true if it succeeds. If it fails, it returns false.
- k2h_free_attrpack  
  Returns true if it succeeds. If it fails, it returns false.
- k2h_get_direct_value  
  If it succeeds, it returns a binary column pointer of the value to be retrieved. If it fails, it returns NULL. Please release the pointer returned on success with the free() function.
- k2h_get_direct_value*_ext  
  If it succeeds, it returns a binary column pointer of the value to be retrieved. If it fails, it returns NULL. Please release the pointer returned on success with the free() function.
- k2h_get_str_direct_value  
  If it succeeds, it returns a binary column pointer of the value to be retrieved. If it fails, it returns NULL. Please release the pointer returned on success with the free() function.
- k2h_get_str_direct_value*_ext  
  If it succeeds, it returns a binary column pointer of the value to be retrieved. If it fails, it returns NULL. Please release the pointer returned on success with the free() function.
- k2h_get_str_subkeys  
  If it succeeds, it returns the number of elements stored in the K2HKEYPCK structure pointer of the subkey　list to be retrieved. If it fails, it returns -1.
- k2h_get_direct_subkeys  
  If it succeeds, it returns the K2HKEYPCK structure pointer of the subkey list to be retrieved. If it fails, it returns NULL. Please release the pointer returned on success with the k2h_free_keypack() function.
- k2h_get_str_direct_subkeys  
  If it succeeds, it returns the string array pointer of the sub key list to be retrieved. If it fails, it returns NULL. Please release the pointer returned on success with the k2h_free_keyarray() function.
- k2h_get_direct_attrs  
  If it succeeds, it returns the K2HATTRPCK structure pointer of the attribute to be retrieved. If it fails, it returns NULL. Please release the pointer returned on success with the k2h_free_attrpack() function.
- k2h_get_str_direct_attrs  
  If it succeeds, it returns the K2HATTRPCK structure pointer of the attribute to be retrieved. If it fails, it returns NULL. Please release the pointer returned on success with the k2h_free_attrpack() function.

### About the k2h_get_trial_callback callback function
K2h_get_value * _ext, k2h_get_direct_value * _ext, k2h_get_str_value * _ext, k2h_get_str_direct_value * _ext function specifies the callback function (k2h_get_trial_callback)
The callback function of k2h_get_trial_callback is called back when its key is read, its value as an argument.
The called callback function can change (overwrite) the value.
Using this callback function gives the timing to change the value for a particular key.
The callback function is called even if the key does not exist in the K2HASH data.
For example, if the key is not set, you can return the initial value. This allows you to get the timing to set the initial value in the reading of an unset key.  

The callback function has the following prototype.
 ```
typedef K2HGETCBRES (*k2h_get_trial_callback)(const unsigned char* byKey, size_t keylen, const unsigned char* byValue, size_t vallen, unsigned char** ppNewValue, size_t* pnewvallen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData)
 ```
<br />
<br /> 
The callback function is called with the following arguments.
- byKey  
  This is a pointer to the binary string of the key specified when calling * _ext function.
- keylen  
  This indicates the binary column length of byKey.
- byValue  
  If there is a value for the key, it is a pointer to the binary string of that value. If the key is not set, NULL is set.
- vallen  
  This indicates the binary column length of byValue.
- ppNewValue  
  When overwriting a value with a callback function (or setting a new value with no key set), allocate a new value to this buffer and set a pointer for that binary.
- pnewvallen  
  * Set the binary string length when ppNewValue is set.
- pattrs  
  The attribute set for the key is passed with a pointer of K2HATTRPCK. * Set the binary string length when ppNewValue is set.
- attrscnt  
  The number of attributes contained in pattrs is passed.
- pExtData  
  The value of pExtData specified when calling the * _ext function is set. 

 
The callback function returns the following values.
- K2HGETCB_RES_ERROR  
  Returns when an error occurs in the callback function.
- K2HGETCB_RES_NOTHING  
  It returns this value if no new value is set (returns the current value).
- K2HGETCB_RES_OVERWRITE  
  This value is returned when setting a new value and returning the new value.
 
A sample callback function is shown below.
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

#### Note
Please use the special function to release the area, in the extracted value, sub key list, attribute area.  
The types of each structure are shown below.

- K2HKEYPCK structure
 ```
typedef struct k2h_key_pack{
  unsigned char* pkey;
  size_t   length;
}K2HKEYPCK, *PK2HKEYPCK;
 ```

- K2HATTRPCK structure
 ```
typedef struct k2h_attr_pack{
    unsigned char*    pkey;
    size_t            keylength;
    unsigned char*    pval;
    size_t            vallength;
}K2HATTRPCK, *PK2HATTRPCK;
 ```

#### Examples
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

### <a name="SET"> SET family(C I/F)
This is a group of functions to write data to K2HASH file (or on memory).

#### Format
- bool k2h_set_all(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength, const PK2HKEYPCK pskeypck, int skeypckcnt)
- bool k2h_set_str_all(k2h_h handle, const char* pkey, const char* pval, const char** pskeyarray)
- bool k2h_set_value(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength)
- bool k2h_set_str_value(k2h_h handle, const char* pkey, const char* pval)
- bool k2h_set_all_wa(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength, const PK2HKEYPCK pskeypck, int skeypckcnt, const char* pass, const time_t* expire)
- bool k2h_set_str_all_wa(k2h_h handle, const char* pkey, const char* pval, const char** pskeyarray, const char* pass, const time_t* expire)
- bool k2h_set_value_wa(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength, const char* pass, const time_t* expire)
- bool k2h_set_str_value_wa(k2h_h handle, const char* pkey, const char* pval, const char* pass, const time_t* expire)
<br />
<br />
- bool k2h_set_subkeys(k2h_h handle, const unsigned char* pkey, size_t keylength, const PK2HKEYPCK pskeypck, int skeypckcnt)
- bool k2h_set_str_subkeys(k2h_h handle, const char* pkey, const char** pskeyarray)
- bool k2h_add_subkey(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* psubkey, size_t skeylength, const unsigned char* pval, size_t vallength)
- bool k2h_add_str_subkey(k2h_h handle, const char* pkey, const char* psubkey, const char* pval)
- bool k2h_add_subkey_wa(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* psubkey, size_t skeylength, const unsigned char* pval, size_t vallength, const char* pass, const time_t* expire)
- bool k2h_add_str_subkey_wa(k2h_h handle, const char* pkey, const char* psubkey, const char* pval, const char* pass, const time_t* expire)
<br />
<br />
- bool k2h_add_attr(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pattrkey, size_t attrkeylength, const unsigned char* pattrval, size_t attrvallength)
- bool k2h_add_str_attr(k2h_h handle, const char* pkey, const char* pattrkey, const char* pattrval)

#### Description
- k2h_set_all  
  Set the value and subkey list for the key. If the key exists, it will be overwritten.
- k2h_set_str_all  
  Set the value and subkey list for the key. It is assumed that all of the keys, values, and subkey lists are strings. Please do not use it except for character strings. If the key exists, it will be overwritten.
- k2h_set_value  
  Set a value for the key. If the key exists, only the value is overwritten and the subkey list remains intact.
- k2h_set_str_value  
  Set a value for the key. It is assumed that both keys and values ​​are character strings. Please do not use it except for character strings. If the key exists, only the value is overwritten and the subkey list remains intact.
- k2h_set_all_wa  
  It is equivalent to k2h_set_all, you can also specify passphrase and Expire time.
- k2h_set_str_all_wa  
  It is equivalent to k2h_set_str_all, you can also specify passphrase and Expire time.
- k2h_set_value_wa  
  It is equivalent to k2h_set_value, you can specify passphrase and Expire time.
- k2h_set_str_value_wa  
  It is equivalent to k2h_set_str_value, you can also specify passphrase and Expire time.
- k2h_set_subkeys  
  Set the subkey list for the key. If the key exists, only the subkey list is overwritten and the value remains intact.
- k2h_set_str_subkeys  
  Set the subkey list for the key. It is assumed that both keys and sub key lists are character strings. Please do not use it except for character strings. If the key exists, only the subkey list is overwritten and the value remains intact.
- k2h_add_subkey  
  Add a subkey to the key. The subkey itself is also newly registered with the value. If a subkey already exists, it will be overwritten.
- k2h_add_str_subkey  
  Add a subkey (Subkey) to the key . It is assumed that all of the keys, subkeys, and values ​​(of the subkey) are strings. Please do not use it except for character strings. The subkey itself is also newly registered with the value . If a subkey already exists, it will be overwritten.
- k2h_add_subkey_wa  
  It is equivalent to k2h_add_subkey and you can also specify passphrase and Expire time for subkey.
- k2h_add_str_subkey_wa  
  It is equivalent to k2h_add_str_subkey, you can also specify passphrase and Expire time for subkey.
- k2h_add_attr  
  Add attributes (attribute name, attribute value) to key .
- k2h_add_str_attr  
  Add attributes (attribute name, attribute value) to key . It is assumed that all of the key, attribute name, and attribute value are character strings. Please do not use it except for character strings.
  
#### Parameters
- k2h_set_all
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
 - pval  
   Specify a pointer to a binary string of values.
 - vallength  
   Specify the length of the value.
 - pskeypck  
   Specify a K2HKEYPCK structure pointer indicating the sub key list.
 - skeypckcnt  
   Specify the number of subkey lists.
 - pass  
   Specify passphrase.
 - expire  
   Specify Expire time. (If not specified, specify NULL If the Expire function is enabled in Builtin attribute, it will be set with that value.)
- k2h_set_str_all
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - pval  
   Specify a pointer to a binary string of values.
 - pskeyarray  
   Specify a character array pointer indicating the sub key list.
 - pass  
   Specify passphrase.
 - expire	
   Specify Expire time. (If not specified, specify NULL If the Expire function is enabled in Builtin attribute, it will be set with that value.)
- k2h_set_value
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
 - pval  
   Specify a pointer to a binary string of values.
 - vallength  
   Specify the length of the value.
 - pass  
   Specify passphrase.
 - expire  
   Specify Expire time. (If not specified, specify NULL If the Expire function is enabled in Builtin attribute, it will be set with that value.)
- k2h_set_str_value
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - pval  
   Specify a pointer to a binary string of values.
 - pass  
   Specify passphrase.
 - expire  
   Specify Expire time. (If not specified, specify NULL If the Expire function is enabled in Builtin attribute, it will be set with that value.)
- k2h_set_subkeys
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
 - pskeypck  
   Specify a K2HKEYPCK structure pointer indicating the sub key list.
 - skeypckcnt  
   Specify the number of subkey lists.
- k2h_set_str_subkeys
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - pskeyarray  
   Specify a character array pointer indicating the sub key list. 
- k2h_add_subkey
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
 - psubkey  
   Specify a pointer to the character string of the subkey to be added.
 - skeylength  
   Specify the length of the subkey to be added.
 - pval  
   Specify a pointer to a binary string of values.
 - vallength  
   Specify the length of the value.
 - pass  
   Specify passphrase.
 - expire  
   Specify Expire time. (If not specified, specify NULL If the Expire function is enabled in Builtin attribute, it will be set with that value.)
- k2h_add_str_subkey
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - psubkey  
   Specify a pointer to the character string of the subkey to be added.
 - pval  
   Specify a pointer to a binary string of values.
 - pass  
   Specify passphrase.
 - expire  
   Specify Expire time. (If not specified, specify NULL If the Expire function is enabled in Builtin attribute, it will be set with that value.)
- k2h_add_attr
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
 - pattrkey  
   Specify a pointer to the binary column of the attribute name of the attribute to be added.
 - attrkeylength  
   Specify the length of the attribute name.
 - pattrval  
   Specify a pointer to the binary column of the attribute value of the attribute to be added.
 - attrvallength  
   Specify the length of the attribute value.
- k2h_add_str_attr
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - pattrkey  
   Specify a pointer to the character string of the attribute name of the attribute to be added.
 - pattrval  
   Specify a pointer to the character string of the attribute value of the attribute to be added.

#### Return Values 
Returns true if it succeeds. If it fails, it returns false.

#### Note
The K2HKEYPCK structure has the following structure.
- K2HKEYPCK structure  
 ```
typedef struct k2h_key_pack{
  unsigned char* pkey;
  size_t         length;
}K2HKEYPCK, *PK2HKEYPCK;
 ```

#### Examples
 ```
if(!k2h_set_str_value(k2handle, "mykey", "myval")){
    return false;
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="RENAME"> Rename family(C I/F)
This is a function group that changes the key name of data of K2HASH file (or on memory).

#### Format
- bool k2h_rename(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pnewkey, size_t newkeylength)
- bool k2h_rename_str(k2h_h handle, const char* pkey, const char* pnewkey)

#### Description
- k2h_rename  
  Change the key to a new key name. The subkey and attributes are inherited as is, only the key name is changed.
- k2h_rename_str  
  It is equivalent to k2h_rename. It is assumed that the key is a character string. Please do not use it except for character strings.

#### Parameters
- k2h_rename
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
 - pnewkey  
   Specify a pointer to the binary column of the changed key.
 - newkeylength  
   Specify the key length after change.
- k2h_rename_str
 - handle  
   Specify the K2HASH handle returned from the k2h_open family function.
 - pkey  
   Specify a pointer to the character string of the key.
 - pnewkey  
   Specify a pointer to the character string of the changed key.

#### Return Values 
Returns true if it succeeds. If it fails, it returns false.

#### Examples
 ```
if(!k2h_rename_str(k2handle, "mykey", "newmykey")){
    return false;
} 
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="DIRECTBINARY"> Direct binary data acquisition / setting family(C I/F)
It is a group of functions that directly acquires multiple data of "K2HASH file" (or on memory) by specifying a range such as HASH value.
This function group is a special group of functions used for copying (backup and replication).

#### Format
- bool k2h_get_elements_by_hashbool k2h_get_elements_by_hash(k2h_h handle, const k2h_hash_t starthash, const struct timespec startts, const struct timespec endts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check, k2h_hash_t* pnexthash, PK2HBIN* ppbindatas, size_t* pdatacnt)
- bool k2h_set_element_by_binary(k2h_h handle, const PK2HBIN pbindatas, const struct timespec* pts)
- void free_k2hbin(PK2HBIN pk2hbin)
- void free_k2hbins(PK2HBIN pk2hbin, size_t count)

#### Description
- k2h_get_elements_by_hash  
  Specify the start HASH value and time range, detect the first data that matches the specified range HASH value (masked start HASH value, number, maximum HASH value), tie it to the one masked HASH value (Key, value, subkey, attribute) as a binary string (there is more than one key & value for one masked HASH value). After detection, the next start HASH value is also returned.
- k2h_set_element_by_binary  
  Set one binary data (key, value, subkey, attribute) retrieved using k2h_get_elements_by_hash in the K2HASH database.
- free_k2hbin  
  Release the K2HBIN structure.
- free_k2hbins  
  Release the K2HBIN structure array.

#### Parameters
- k2h_get_elements_by_hash
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - starthash  
   Specify the HASH value to start the search. (0 to maximum value 64 bits)
 - startts / endts  
   It is a value that specifies the range of the last update time of "K2HASH data" to be searched and it indicates the minimum last update time (In order for this value to work effectively, the update time is set as the Builtin attribute in K2HASH It is necessary).  
 - target_hash  
   Indicates the start value of the HASH value to be searched. However, this value is compared against the targeted HASH value rounded to the value indicated by target_max_hash (the remainder divided by target_max_hash).
 - target_max_hash  
   It is a value for rounding the HASH value of the data using this value (the HASH value to be searched uses the remainder obtained by dividing the HASH value of the data by the target_max_hash).
 - old_hash  
   If the HASH value to be searched matches this value (and the range using target_hash_range), it compares with the startts. If this value is unnecessary (do not compare startts), set -1 (= 0xFFFFFFFFFFFFFFFF).
 - old_max_hash  
   It is a value to round out the old_hash's HASH value using this value (old_hash's HASH value uses the remainder obtained by dividing the data's HASH value by old_hash). If you do not use old_hash, set it to -1 (= 0xFFFFFFFFFFFFFFFF).
 - target_hash_range  
   Indicates the number of target HASH values from the start value (target_hash, old_hash) of the HASH value to be searched.
 - is_expire_check  
   Set to true to check the expiration date of data that matches the HASH value of the search target. (It will not work if the update time attribute is not valid as Builtin attribute.)
 - pnexthash  
   After the search, it returns the HASH value at which the next search is started (If the search result does not exist anymore, it returns 0).
 - ppbindatas  
   Returns the detected results as a K2HBIN structure array (binary data (key, value, subkey, attribute) is detected and returns NULL when not detected.
 - pdatacnt  
   It returns the number of data of the binary data string (ppbindatas) in the detected data (0 is returned when not detected).
- k2h_set_element_by_binary
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pbindatas  
   Specify one binary data detected by k2h_get_elements_by_hash (the specified value is set to K2HASH as is).
 - pts  
   If the same key as the specified binary data exists, if it is newer than this specified time, specify a value for not setting (NULL can not be specified, so always set it from the current time Please also specify the value of the future).
- free_k2hbin
 - pk2hbin  
   Specify the K2HBIN structure pointer to be released.
- free_k2hbins
 - pk2hbin  
   Specify a pointer to the K2HBIN structure array to release.
 - count  
   Specify the number of K2HBIN structure array to release.

#### Note
- The HASH value of the search target is as follows.  
If starthash = 100, target_hash = 2, target_max_hash = 10, target_hash_range = 2 on the assumption that data is distributed in K2HASH in the range of 0x0 to 0xFFFFFFFFFFFFFFF, the search starts from the HASH value = 100 . And the HASH value is searched in order of 102, 103, 112, 113, ... .
- The k2h_get_elements_by_hash function is mainly used to implement data merging in cases where cluster configuration is used together with communication middleware such as chmpx. Therefore, values such as old_hash assume the HASH value before auto merge.

#### K2HBIN structure
 ```
typedef struct k2hash_binary_data{
    unsigned char*    byptr;
    size_t            length;
}K2HBIN, *PK2HBIN;
 ```

#### Return Values 
Returns true if it succeeds. If it fails, it returns false.

#### Examples
 ```
if(!k2h_get_elements_by_hash(k2hash, hashval, startts, endts, target_hash, target_max_hash, target_hash_range, &nexthash, &pbindatas, &datacnt)){
     return false;
}
 
if(!k2h_set_element_by_binary(k2hash, &pbindatas[cnt], &ts)){
     return false;
}
 ```

 - notes  
This function I / F is an API used to copy K2HASH data, etc., and notes is required for handling.

<!-- -----------------------------------------------------------　-->
***

### <a name="DELETE"> Delete family(C I/F)
This is a function group for deleting data from K2HASH file (or on memory).

#### Format
- bool k2h_remove_all(k2h_h handle, const unsigned char* pkey, size_t keylength)
- bool k2h_remove_str_all(k2h_h handle, const char* pkey)
- bool k2h_remove(k2h_h handle, const unsigned char* pkey, size_t keylength)
- bool k2h_remove_str(k2h_h handle, const char* pkey)
- bool k2h_remove_subkey(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* psubkey, size_t skeylength)
- bool k2h_remove_str_subkey(k2h_h handle, const char* pkey, const char* psubkey)

#### Description
- k2h_remove_all  
  Delete the key. Delete all subkeys registered in the key as well.
- k2h_remove_str_all  
  Delete the key. It is assumed that the key is a character string. Please do not use it except for character strings. Delete all subkeys registered in the key as well.
- k2h_remove  
  Delete the key. The subkey registered in the key is not deleted and it remains as it is.
- k2h_remove_str  
  Delete the key. It is assumed that the key is a character string. Please do not use it except for character strings. The subkey registered in the key is not deleted and it remains as it is.
- k2h_remove_subkey  
  Delete the subkey registered in the key from the subkey list and delete the subkey itself.
- k2h_remove_str_subkey  
  Delete the subkey registered in the key from the subkey list and delete the subkey itself. It is assumed that both keys and subkeys are character strings. Please do not use it except for character strings.
  
#### Parameters
- k2h_remove_all
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
- k2h_remove_str_all
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
- k2h_remove
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
- k2h_remove_str
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
- k2h_remove_subkey
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
 - psubkey  
   Specify a pointer to the binary string of the subkey (Subkey) to be deleted.
 - skeylength  
   Specify the length of the subkey to be deleted.
- k2h_remove_str_subkey
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - psubkey
   Specify a pointer to the character string of the subkey to be deleted.

#### Return Values 
Returns true if it succeeds. If it fails, it returns false.

#### Note
K2h_remove_subkey, k2h_remove_str_subkey will fail if the key does not have the specified subkey.

#### Examples
 ```
if(!k2h_remove_str_all(k2handle, "mykey")){
    return false;
}
 ```

<!-- -----------------------------------------------------------　-->
***

### <a name="FIND"> Search family(C I/F)
This is a group of functions to search for data from K2HASH file (or on memory).

#### Format
- k2h_find_h k2h_find_first(k2h_h handle)
- k2h_find_h k2h_find_first_subkey(k2h_h handle, const unsigned char* pkey, size_t keylength)
- k2h_find_h k2h_find_first_str_subkey(k2h_h handle, const char* pkey)
- k2h_find_h k2h_find_next(k2h_find_h findhandle)
- bool k2h_find_free(k2h_find_h findhandle)
- bool k2h_find_get_key(k2h_find_h findhandle, unsigned char** ppkey, size_t* pkeylength)
- char* k2h_find_get_str_key(k2h_find_h findhandle)
- bool k2h_find_get_value(k2h_find_h findhandle, unsigned char** ppval, size_t* pvallength)
- char* k2h_find_get_direct_value(k2h_find_h findhandle)
- bool k2h_find_get_subkeys(k2h_find_h findhandle, PK2HKEYPCK* ppskeypck, int* pskeypckcnt)
- PK2HKEYPCK k2h_find_get_direct_subkeys(k2h_find_h findhandle, int* pskeypckcnt)
- int k2h_find_get_str_subkeys(k2h_find_h findhandle, char*** ppskeyarray)
- char** k2h_find_get_str_direct_subkeys(k2h_find_h findhandle)

#### Description
- k2h_find_first  
  Get the handle for key search of all K2HASH data. Please release the acquired handle with the k2h_find_free() function.
- k2h_find_first_subkey  
  Get the handle for subkey search registered in the key .
- k2h_find_first_str_subkey  
  Get the handle for subkey search registered in the key . It is assumed that the key is a character string. Please do not use it except for character strings.
- k2h_find_next  
  It performs a search and returns a search handle indicating the next.
- k2h_find_free  
  Release the search handle.
- k2h_find_get_key  
  Get the key of the K2HASH data indicated by the search handle. Release the acquired key with the free() function.
- k2h_find_get_str_key  
  Get the key of the K2HASH data indicated by the search handle as a character string. Please do not use when key is not character string. Release the acquired key with the free() function.
- k2h_find_get_value  
  Gets the value of K2HASH data indicated by the search handle. Please release the acquired value with the free() function.
- k2h_find_get_direct_value  
  Acquires the value of K2HASH data indicated by the search handle as a character string. Do not use when the value is not a character string. Please release the acquired value with the free() function.
- k2h_find_get_subkeys  
  Get the subkey list of K2HASH data indicated by the search handle. Please release the acquired subkey list with the k2h_free_keypack() function.
- k2h_find_get_direct_subkeys  
  Get the subkey list of K2HASH data indicated by the search handle. Please release the acquired subkey list with the k2h_free_keypack() function.
- k2h_find_get_str_subkeys  
  Get the subkey list of K2HASH data indicated by the search handle. Please do not use when the subkey is not a character string. Please release the acquired subkey list with the k2h_free_keyarray() function.
- k2h_find_get_str_direct_subkeys  
  Get the subkey list of K2HASH data indicated by the search handle. Please do not use when the subkey is not a character string. Please release the acquired subkey list with the k2h_free_keyarray() function.
 
#### Parameters
- k2h_find_first
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
- k2h_find_first_subkey
 - handle
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specifies a pointer to the binary column of the key.
 - keylength  
   Specify the length of the key.
- k2h_find_first_str_subkey
 - handle  
   Specify the K2HASH handle returned from the k2h_open family of functions.
 - pkey  
   Specify a pointer to the character string of the key.
- k2h_find_next
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.
- k2h_find_free
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.
- k2h_find_get_key
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.
 - ppkey  
   Specify a pointer to save the binary string of the extracted key. Please free the area set (returned) by this pointer with the free() function.
 - pkeylength  
   Specify a pointer to return the length of the binary string of the retrieved key.
- k2h_find_get_str_key
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.
- k2h_find_get_value
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.
 - ppval  
   Specify a pointer to save the binary string of the fetched value. Please free the area set (returned) by this pointer with the free() function.
 - pvallength  
   Specify a pointer to return the length of the binary string of the fetched value.
- k2h_find_get_direct_value
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.
- k2h_find_get_subkeys
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.
 - ppskeypck  
   Specify a pointer to save the K2HKEYPCK structure array of the retrieved subkey (Subkey) list. Free the area set (returned) by this pointer with the k2h_free_keypack() function.
 - pskeypckcnt  
   Specify a pointer to return the number of K2HKEYPCK structure array of the extracted subkey list.
- k2h_find_get_direct_subkeys
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.
 - pskeypckcnt  
   Specify a pointer to return the number of K2HKEYPCK structure array of the extracted subkey list.
- k2h_find_get_str_subkeys
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.
 - ppskeyarray  
   Specify a pointer to save the string array (NULL termination) of the extracted subkey list. Free up the area set (returned) by this pointer with the k2h_free_keyarray() function. The string array has a termination of NULL.
- k2h_find_get_str_direct_subkeys
 - findhandle  
   Specify the search handle returned from the k2h_find_first function.

#### Return Values
- k2h_find_first / k2h_find_first_subkey / k2h_find_first_str_subkey / k2h_find_next  
  If it succeeds, specify the search handle. If it fails, K2H_INVALID_HANDLE (0) is returned. Release the acquired search handle with the k2h_find_free() function.
- k2h_find_free / k2h_find_get_key / k2h_find_get_value / k2h_find_get_subkeys  
  Returns true if it succeeds. If it fails, it returns false.
- k2h_find_get_str_key / k2h_find_get_direct_value  
  If it succeeds, it returns the character string pointer of the key. If it fails, it returns NULL. Please release the returned string pointer with the free() function.
- k2h_find_get_direct_subkeys  
  If it succeeds, it returns a pointer to the K2HKEYPCK structure array of the subkey list. If it fails, it returns NULL. Please release the returned pointer with the k2h_free_keypack() function.
- k2h_find_get_str_subkeys  
  If it succeeds, it returns the number of array indicated by the pointer ppskeyarray of the subkey list. If it fails, it returns -1.
- k2h_find_get_str_direct_subkeys  
  If successful, it returns a pointer to the K2HKEYPCK structure array of the subkey list. If it fails, it returns NULL. Please release the returned pointer with the k2h_free_keyarray() function.

#### Note
For the period holding a valid search handle (k2h_find_h) (the period until release by k2h_find_free), a lock for reading is set for the K2HASH data pointed to by the search handle.
During the period when the lock is set, reading and writing of that key will be blocked.
Therefore, we recommend not keeping this explorer handle for a long time.
In particular, you should avoid caching the search handle and keep it for long periods within the program.

#### Examples
 ```
// Full dump
for(k2h_find_h fhandle = k2h_find_first(k2handle); K2H_INVALID_HANDLE != fhandle; fhandle = k2h_find_next(fhandle)){
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

### <a name="DIRECTACCESS"> Direct access family(C I/F)
A function group that directly accesses data of "K2HASH file" (or on memory).
It is mainly used for reading and writing to data with a large size value. Parts of the value can be overwritten or read by specifying the offset.

#### Format
- k2h_da_h k2h_da_handle(k2h_h handle, const unsigned char* pkey, size_t keylength, K2HDAMODE mode)
- k2h_da_h k2h_da_handle_read(k2h_h handle, const unsigned char* pkey, size_t keylength)
- k2h_da_h k2h_da_handle_write(k2h_h handle, const unsigned char* pkey, size_t keylength)
- k2h_da_h k2h_da_handle_rw(k2h_h handle, const unsigned char* pkey, size_t keylength)
- k2h_da_h k2h_da_str_handle(k2h_h handle, const char* pkey, K2HDAMODE mode)
- k2h_da_h k2h_da_str_handle_read(k2h_h handle, const char* pkey)
- k2h_da_h k2h_da_str_handle_write(k2h_h handle, const char* pkey)
- k2h_da_h k2h_da_str_handle_rw(k2h_h handle, const char* pkey)
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
- bool k2h_da_get_value(k2h_da_h dahandle, unsigned char** ppval, size_t* pvallength)
- bool k2h_da_get_value_offset(k2h_da_h dahandle, unsigned char** ppval, size_t* pvallength, off_t offset)
- bool k2h_da_get_value_to_file(k2h_da_h dahandle, int fd, size_t* pvallength)
- unsigned char* k2h_da_read(k2h_da_h dahandle, size_t* pvallength)
- unsigned char* k2h_da_read_offset(k2h_da_h dahandle, size_t* pvallength, off_t offset)
- char* k2h_da_read_str(k2h_da_h dahandle)
<br />
<br />
- bool k2h_da_set_value(k2h_da_h dahandle, const unsigned char* pval, size_t vallength)
- bool k2h_da_set_value_offset(k2h_da_h dahandle, const unsigned char* pval, size_t vallength, off_t offset)
- bool k2h_da_set_value_from_file(k2h_da_h dahandle, int fd, size_t* pvallength)
- bool k2h_da_set_value_str(k2h_da_h dahandle, const char* pval)

#### Description
- k2h_da_handle  
  Specify Key and obtain the handle (k2h_da_h) for direct access to the value of that Key
- k2h_da_handle_read  
  Get a read-only handle (k2h_da_h).
- k2h_da_handle_write  
  Get a write-only handle (k2h_da_h).
- k2h_da_handle_rw  
  Get a readable / writable handle (k2h_da_h).
- k2h_da_str_handle  
  Key Specify the character string and obtain the handle (k2h_da_h)
- k2h_da_str_handle_read  
  Key Specifies the character string and read-only handle (k2h_da_h).
- k2h_da_str_handle_write  
  Get the character string specification and write-only handle (k2h_da_h).
- k2h_da_str_handle_rw  
  Key Specify the character string and obtain readable / writable handle (k2h_da_h).
- k2h_da_free  
  Release the handle (k2h_da_h).
- k2h_da_get_length  
  Get the length of the value .
- k2h_da_get_buf_size  
  Get the internal buffer size for reading / writing data from arbitrary file.
- k2h_da_set_buf_size  
  Sets the internal buffer size for reading / writing data from arbitrary file.
- k2h_da_get_offset  
  Get the start offset for direct reading and writing.
- k2h_da_get_read_offset  
  Get the start offset for direct reading.
- k2h_da_get_write_offset  
  Get the start offset for direct writing.
- k2h_da_set_offset  
  Sets the start offset for direct reading and writing.
- k2h_da_set_read_offset  
  Sets the start offset for direct reading.
- k2h_da_set_write_offset  
  Set the start offset for direct writing.
- k2h_da_get_value  
  Read the value from the set offset.
- k2h_da_get_value_offset  
  Specify the offset and read the value.
- k2h_da_get_value_to_file  
  It reads the value from the set offset and writes it in the specified file.
- k2h_da_read  
  It reads the value from the set offset and returns it as a return value.
- k2h_da_read_offset  
  It reads the value by specifying the offset and returns it as the return value.
- k2h_da_read_str  
  It reads the value from the set offset and returns it as a string value as a return value.
- k2h_da_set_value  
  Write the value from the set offset.
- k2h_da_set_value_offset  
  Specify the offset and write the value.
- k2h_da_set_value_from_file  
  Writes the value read from the specified file to the value from the set offset.
- k2h_da_set_value_str  
  Write a character string value from the set offset.
  

#### Parameters
- k2h_da_handle
 - handle  
   Specify k2h_h handle.
 - pkey  
   Specify the Key value.
 - keylength  
   Specify the length of the Key value.
 - mode  
   Specify the mode of k2h_da_h handle to be acquired. The mode is K2H_DA_READ (read only), K2H_DA_WRITE (write only), K2H_DA_RW (for both reading and writing).
- k2h_da_handle_read 
 - handle  
   Specify k2h_h handle.
 - pkey  
   Specify the Key value.
 - keylength  
   Specify the length of the Key value.
- k2h_da_handle_write
 - handle  
   Specify k2h_h handle.
 - pkey  
   Specify the Key value.
 - keylength  
   Specify the length of the Key value.
- k2h_da_handle_rw
 - handle  
   Specify k2h_h handle.
 - pkey  
   Specify the Key value.
 - keylength  
   Specify the length of the Key value.
- k2h_da_str_handle
 - handle  
   Specify k2h_h handle.
 - pkey  
   Specify the Key value.
 - mode  
   Specify the mode of k2h_da_h handle to be acquired. The mode is K2H_DA_READ (read only), K2H_DA_WRITE (write only), K2H_DA_RW (for both reading and writing).
- k2h_da_str_handle_read
 - handle  
   Specify k2h_h handle.
 - pkey  
   Specify the Key value.
- k2h_da_str_handle_write
 - handle  
   Specify k2h_h handle.
 - pkey  
   Specify the Key value.
- k2h_da_str_handle_rw
 - handle  
   Specify k2h_h handle.
 - pkey  
   Specify the Key value.
- k2h_da_free
 - handle  
   Specify k2h_h handle.
- k2h_da_get_length
 - handle  
   Specify k2h_h handle.
- k2h_da_get_buf_size	
 - handle  
   Specify k2h_h handle.
- k2h_da_set_buf_size	
 - handle  
   Specify k2h_h handle.
 - bufsize  
   Specify the internal buffer size.
- k2h_da_get_offset
 - handle  
   Specify k2h_h handle.
 - is_read  
   Specify true to retrieve the offset for reading.
- k2h_da_get_read_offset
 - handle  
   Specify k2h_h handle.
- k2h_da_get_write_offset
 - handle  
   Specify k2h_h handle.
- k2h_da_set_offset
 - handle  
   Specify k2h_h handle.
 - offset  
   Specify the offset value to be set.
 - is_read  
   Specify true to retrieve the offset for reading.
- k2h_da_set_read_offset
 - handle  
   Specify k2h_h handle.
 - offset  
   Specify the offset value to be set.
- k2h_da_set_write_offset
 - handle  
   Specify k2h_h handle.
 - offset  
   Specify the offset value to be set.
- k2h_da_get_value
 - dahandle  
   Specify k2h_h handle.
 - ppval  
   Specify a pointer buffer to store the read value. A pointer to the successfully dynamically allocated memory is set. Free up this pointer with the free() function.
 - pvallength  
   Specify the reading length. If it succeeds, the number of bytes read is set.
- k2h_da_get_value_offset
 - dahandle  
   Specify k2h_h handle.
 - ppval  
   Specify a pointer buffer to store the read value. A pointer to the successfully dynamically allocated memory is set. Free up this pointer with the free() function.
 - pvallength  
   Specify the reading length. If it succeeds, the number of bytes read is set.
 - offset  
   Specify the offset of reading start.
- k2h_da_get_value_to_file
 - dahandle  
   Specify k2h_h handle.
 - fd  
   Specify the file descriptor to output the read value.
 - pvallength  
   Specify the reading length. If it succeeds, the number of bytes read is set.
- k2h_da_read
 - dahandle  
   Specify k2h_h handle.
 - pvallength  
   Specify the reading length. If it succeeds, the number of bytes read is set.
- k2h_da_read_offset
 - dahandle  
   Specify k2h_h handle.
 - pvallength  
   Specify the reading length. If it succeeds, the number of bytes read is set.
 - offset  
   Specify the offset of reading start.
- k2h_da_read_str
 - dahandle  
   Specify k2h_h handle.
- k2h_da_set_value
 - dahandle  
   Specify k2h_h handle.
 - pval  
   Specify a pointer to the data to be written.
 - vallength  
   Specify the length of the data to be written.
- k2h_da_set_value_offset
 - dahandle  
   Specify k2h_h handle.
 - pval  
   Specify a pointer to the data to be written.
 - vallength  
   Specify the length of the data to be written.
 - offset  
   Specify the start offset of writing start.
- k2h_da_set_value_from_file
 - dahandle  
   Specify k2h_h handle.
 - fd  
   Specify the file descriptor of the data to be written.
 - pvallength  
   Specify the data length to be written. If it succeeds, it returns the number of written bytes.
- k2h_da_set_value_str	
 - dahandle  
   Specify k2h_h handle.
 - pval  
   Specify a pointer to the character string to be written.

#### Return Values
- k2h_da_handle / k2h_da_handle_read / k2h_da_handle_write / k2h_da_handle_rw / k2h_da_str_handle / k2h_da_str_handle_read / k2h_da_str_handle_write / k2h_da_str_handle_rw  
  Returns the direct access handle (k2h_da_h). On failure, K2H_INVALID_HANDLE (0) is returned.
- k2h_da_free / k2h_da_set_buf_size / k2h_da_set_offset / k2h_da_set_read_offset / k2h_da_set_write_offset / k2h_da_get_value / k2h_da_get_value_offset / k2h_da_get_value_to_file / k2h_da_set_value / k2h_da_set_value_offset / k2h_da_set_value_from_file / k2h_da_set_value_str  
  Returns true on success, false on failure.
- k2h_da_get_length  
  Returns the length of the value. If an error occurs, -1 is returned.
- k2h_da_get_buf_size  
  Returns the length of the internal buffer. If an error occurs, -1 is returned.
- k2h_da_get_offset  
  Returns the offset of the currently set read or write position. If an error occurs, -1 is returned.
- k2h_da_get_read_offset  
  Returns the offset of the currently set reading position. If an error occurs, -1 is returned.
- k2h_da_get_write_offset  
  Returns the offset of the currently set writing position. If an error occurs, -1 is returned.
- k2h_da_read  / k2h_da_read_offset  
  It returns the read value in the memory allocated area. It returns NULL on error. Please release the returned pointer with the free() function.
- k2h_da_read_str  
  It returns the read value as a memory allocated area (character string pointer). It returns NULL on error. Please release the returned pointer with the free() function.

#### Note
- If you acquire the direct access handle (k2h_da_h), changes to that key  will be locked.
  Especially when acquiring a handle for writing, it is locked for writing and reading is also blocked.
  It is recommended to avoid acquiring the handle in unnecessary write mode, and immediately release the handle (k2h_da_h) after use.
- When writing to an area (from the offset) that does not exist before writing, the value in the area before the offset becomes undefined.
- For this encrypted key, this function group can not be used.
  Since the value operated by this function is the data before the decryption, encrypted data will be destroyed, so please notes.
  Also, when creating a new one, you can not encrypt it.
  Since direct access can not be performed for encryption key, please use other function (SET function).

#### Examples
 ```
// get handle
k2h_da_h    dahandle;
if(K2H_INVALID_HANDLE == (dahandle = k2h_da_str_handle_write(k2handle, "mykey"))){
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

### <a name="QUE"> Queue family(C I/F)
The K2HASH library provides functions as a queue (FIFO / LIFO).
The queue can push the value as FIFO / LIFO and pop.
This function group is a function group related to the queue.

#### Format
- k2h_q_h k2h_q_handle(k2h_h handle, bool is_fifo)
- k2h_q_h k2h_q_handle_prefix(k2h_h handle, bool is_fifo, const unsigned char* pref, size_t preflen)
- k2h_q_h k2h_q_handle_str_prefix(k2h_h handle, bool is_fifo, const char* pref)
- bool k2h_q_free(k2h_q_h qhandle)
- bool k2h_q_empty(k2h_q_h qhandle)
- int k2h_q_count(k2h_q_h qhandle)
- bool k2h_q_read(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, int pos)
- bool k2h_q_str_read(k2h_q_h qhandle, char** ppdata, int pos)
- bool k2h_q_read_wp(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, int pos, const char* encpass)
- bool k2h_q_str_read_wp(k2h_q_h qhandle, char** ppdata, int pos, const char* encpass)
- bool k2h_q_push(k2h_q_h qhandle, const unsigned char* byval, size_t vallen)
- bool k2h_q_str_push(k2h_q_h qhandle, const char* pval)
- bool k2h_q_push_wa(k2h_q_h qhandle, const unsigned char* bydata, size_t datalen, const PK2HATTRPCK pattrspck, int attrspckcnt, const char* encpass, const time_t* expire)
- bool k2h_q_str_push_wa(k2h_q_h qhandle, const char* pdata, const PK2HATTRPCK pattrspck, int attrspckcnt, const char* encpass, const time_t* expire)
- bool k2h_q_pop(k2h_q_h qhandle, unsigned char** ppval, size_t* pvallen)
- bool k2h_q_str_pop(k2h_q_h qhandle, char** ppval)
- bool k2h_q_pop_wa(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, PK2HATTRPCK* ppattrspck, int* pattrspckcnt, const char* encpass)
- bool k2h_q_str_pop_wa(k2h_q_h qhandle, char** ppdata, PK2HATTRPCK* ppattrspck, int* pattrspckcnt, const char* encpass)
- bool k2h_q_pop_wp(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, const char* encpass)
- bool k2h_q_str_pop_wp(k2h_q_h qhandle, char** ppdata, const char* encpass)
- bool k2h_q_remove(k2h_q_h qhandle, int count)
- bool k2h_q_remove_wp(k2h_q_h qhandle, int count, const char* encpass)
- int k2h_q_remove_ext(k2h_q_h qhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata)
- int k2h_q_remove_wp_ext(k2h_q_h qhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata, const char* encpass)
- bool k2h_q_dump(k2h_q_h qhandle, FILE* stream)

#### Description
- k2h_q_handle  
  Get handle to queue of FIFO or LIFO.
- k2h_q_handle_prefix  
  Get handle to queue of FIFO or LIFO. You can specify a prefix for the internal key name used in the queue at retrieval time.
- k2h_q_handle_str_prefix  
  Wrapper function to k2h_q_handle_ prefix. You can specify the prefix as a string.
- k2h_q_free  
  Discard the handle to the queue.
- k2h_q_empty  
  Check if the queue is empty.
- k2h_q_count  
  Get the number of data accumulated in the queue (see notes).
- k2h_q_read  
  Copy the value from the queue. The value will remain in the queue. Please release the pointer to the acquired value (see notes).
- k2h_q_str_read  
  Wrapper function to k2h_q_read. You can get the value as a string. Please release the pointer to the acquired value (see notes).
- k2h_q_read_wp  
  It is equivalent to k2h_q_read. You can specify a passphrase for the decryption to read the encrypted queue.
- k2h_q_str_read_wp  
  It is equivalent to k2h_q_str_read. You can specify a passphrase for the decryption to read the encrypted queue.
- k2h_q_push  
  Push the value into the queue. Push to queue only.
- k2h_q_str_push  
  Wrapper function to k2h_q_push. You can specify a value as a string.
- k2h_q_push_wa  
  It is equivalent to k2h_q_push. The Builtin attribute can be specified with the K2HATTRPCK pointer. You can also specify passphrase and Expire time.  
  If the K2HATTRPCK pointer, passphrase, Expire time is specified, the encryption and Expire time specified by the K2HATTRPCK pointer are overwritten with the specified pass phrase, Expire time.  
  The K2HATTRPCK pointer is supposed to be used mainly when pushing the data fetched from the queue into the queue by transaction processing or the like.
  This will allow you to maintain the Expire time registered in the queue for the first time and expire the retransmission of transaction data.
- k2h_q_str_push_wa  
  It is equivalent to k2h_q_str_push. The argument of Builtin attribute is equivalent to k2h_q_push_wa.
- k2h_q_pop  
  Pop values from the queue. The pop value is deleted from the queue. It deletes only from the queue. Please release the pointer to the acquired value.
- k2h_q_str_pop  
  Wrapper function to k2h_q_pop. You can get the value as a string. Please release the pointer to the acquired value.
- k2h_q_pop_wa  
  It is equivalent to k2h_q_pop. You can decrypted by specifying a passphrase. You can also retrieve the Builtin attribute set when retrieving data from the queue.
  This retrieved Builtin attribute can be operated as an Expire from the time when the transaction was first registered by handover to the argument of k2h_q_push_wa when re-registering in the queue.
- k2h_q_str_pop_wa  
  It is equivalent to k2h_q_str_pop. Passphrase and Builtin attribute are the same as k2h_q_pop_wa.
- k2h_q_pop_wp  
  It is equivalent to k2h_q_pop. You can decrypted by specifying a passphrase.
- k2h_q_str_pop_wp  
  It is equivalent to k2h_q_str_pop. It can be decrypted by being a passphrase.
- k2h_q_remove  
  Delete the specified number of values accumulated in the queue. The deleted value is not returned. Even if you specify more than the accumulated number, it will not result in an error, the contents of the queue will be empty and it will end normally.
- k2h_q_remove_wp  
  It is equivalent to k2h_q_remove. You can specify a passphrase for encrypted queues.
- k2h_q_remove_ext  
  Like k2h_q_remove, deletes the specified number of values accumulated in the queue. For this function, specify a callback function (described later). Each time the value is deleted from the queue, the callback function is called and it is possible to independently judge the value deletion.
- k2h_q_remove_wp_ext  
  It is equivalent to k2h_q_remove_ext. You can specify a passphrase for encrypted queues.
- k2h_q_dump  
  It is a function for queue debugging and dumps inside the queue.

#### Parameters
- k2h_q_handle	
 - handle  
   Specify K2HASH handle.
 - is_fifo  
   Specify the type of queue to be acquired (FIFO or LIFO).
- k2h_q_handle_prefix	
 - handle  
   Specify K2HASH handle.
 - is_fifo  
   Specify the type of queue to be acquired (FIFO or LIFO).
 - pref  
   Specify the prefix of the internal key name to be used as a queue (binary column).
 - preflen  
   Specify the buffer length of pref.
- k2h_q_handle_str_prefix
 - handle  
   Specify K2HASH handle.
 - is_fifo  
   Specify the type of queue to be acquired (FIFO or LIFO).
 - pref  
   Specify the prefix of the internal key name to be used as a queue (it is a character string and is \0 terminated).
- k2h_q_free
 - qhandle  
   Specify the handle of the queue.
- k2h_q_empty
 - qhandle  
   Specify the handle of the queue.
- k2h_q_count
 - qhandle   
   Specify the handle of the queue.
- k2h_q_read
 - qhandle  
   Specify the handle of the queue.
 - ppdata  
   Specify a pointer to a buffer that stores the value to be copied from the queue (binary string). Please release the returned pointer.
 - pdatalen  
   The buffer length of ppdata is returned.
 - pos  
   Specify the position from the top of the queue of data to be copied (leading is 0).
 - encpass  
   Specify passphrase.
- k2h_q_str_read
 - qhandle  
   Specify the handle of the queue.
 - ppdata  
   Specify a pointer to a buffer that stores the value to be copied from the queue (character string). Please release the returned pointer.
 - pos  
   Specify the position from the top of the queue of data to be copied (leading is 0).
 - encpass  
   Specify passphrase.
- k2h_q_push	
 - qhandle  
   Specify the handle of the queue.
 - byval  
   Specify the value to push into the queue (binary column).
 - vallen  
   Specify the buffer length of byval.
 - pattrspck  
   Specify the K2HATTRPCK pointer for Builtin attribute and Plugin attribute. This attribute information is mainly intended to specify the pointer returned by k2h_q_pop_wa.
 - attrspckcnt  
   Specify the number of attributes of Builtin attribute and Plugin attribute information.
 - encpass  
   Specify passphrase.
 - expire  
   Specify Expire time. If not specified, specify NULL.
- k2h_q_str_push
 - qhandle  
   Specify the handle of the queue.
 - pval  
   Specify the value to push to the queue. (It is a character string, it is \0 terminal)
 - pattrspck  
   Specify the K2HATTRPCK pointer of the Builtin attribute and Plugin attribute information. This attribute information is mainly intended to specify the pointer returned by k2h_q_str_pop_wp.
 - attrspckcnt  
   Specify the number of attributes of Builtin attribute and Plugin attribute information.
 - encpass  
   Specify passphrase.
 - expire  
   Specify Expire time. If not specified, specify NULL.
- k2h_q_pop	
 - qhandle	
   Specify the handle of the queue.
 - ppval	
   Specify a pointer to the buffer that stores the popped value from the queue (binary string). Please release the returned pointer.
 - pvallen	
   The buffer length of *pval is returned.
 - ppattrspck	
   Specify a pointer to retrieve the Builtin attribute and Plugin attribute information set in the queue.
 - pattrspckcnt	
   Specify a pointer to return the number of Builtin attribute and Plugin attribute information set in the queue and retrieved.
 - encpass	
   Specify passphrase.
- k2h_q_str_pop
 - qhandle  
   Specify the handle of the queue.
 - ppval  
   Specify a pointer to the buffer that stores the popped value from the queue (it is a character string and is \ 0 terminated). Please release the returned pointer.
 - ppattrspck  
   Specify a pointer to retrieve the Builtin attribute and Plugin attribute information set in the queue.
 - pattrspckcnt  
   Specify a pointer to return the number of Builtin attribute and Plugin attribute information set in the queue and retrieved.
 - encpass  
   Specify passphrase.
- k2h_q_remove
 - qhandle  
   Specify the handle of the queue.
 - count  	
   Specify the number of values you want to delete from the queue.
 - encpass  
   Specify passphrase.
- k2h_q_remove*_ext
 - qhandle  
   Specify the handle of the queue.
 - count  
   Specify the number of values you want to delete from the queue.
 - fp  
   Specify a pointer to the callback function.
 - pextdata  
   Specify arbitrary data to be passed to the callback function (NULL possible).
 - encpass  
   Specify passphrase.
- k2h_q_dump
 - qhandle  
   Specify the handle of the queue.
 - stream    
   Specify the output destination for debugging. If NULL is specified, it is output to stdout.

#### Return Values
- k2h_q_handle / k2h_q_handle_prefix / k2h_q_handle_str_prefix  
  If it ends normally, it returns the handle of the queue. In case of error, K2H_INVALID_HANDLE (0) is returned.
- k2h_q_count  
  Returns the number of data accumulated in the queue. If an error occurs, 0 is returned.
- k2h_q_remove*_ext  
  Returns the number of deleted queue values. On error, -1 is returned.
- Other than those above  
  Returns true if it is normally completed, and false on error.

#### K2h_q_remove_trial_callback callback function
The k2h_q_remove_ext function specifies the callback function (k2h_q_remove_trial_callback) as an argument.
The callback function is called each time the value of the queue is deleted, and it can judge whether or not to delete the value from the queue.
With this callback function, deletion can be executed under arbitrary conditions in deletion of the value accumulated in the queue.
<br />
<br />
This callback function has the following prototype.
 ```
typedef K2HQRMCBRES (*k2h_q_remove_trial_callback)(const unsigned char* bydata, size_t datalen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData);
 ```
The k2h_q_remove_ext function calls the specified callback function for each queue value deletion process.
<br />
<br />
The callback function is called with the following arguments.
- bydata  
   A pointer to the binary string to the data accumulated in the queue to be deleted is specified.
- datalen  
   The binary column length of bydata is set.
- pattrs  
   The Builtin attribute and Plugin attribute set in the queue to be deleted are passed with a pointer of K2HATTRPCK.
- attrscnt  
   The number of Builtin and Plugin attributes of pattrs is passed.
- pExtData  
   The pextdata pointer specified when calling the k2h_q_remove_ext function is set.
<br />
<br />
The callback function returns the following values.
- K2HQRMCB_RES_ERROR  
  Returns when an error occurs in the callback function.
- K2HQRMCB_RES_CON_RM  
  Remove the value you are deleting from the queue and request inspection of the next value. (Unless interrupted, the count argument will be checked when calling the k2h_q_remove_ext function.)
- K2HQRMCB_RES_CON_NOTRM  
  Cancel (do not delete) the deletion from the queue of the value to be deleted and request the inspection of the next value. (Unless interrupted, the count argument will be checked when calling the k2h_q_remove_ext function.)
- K2HQRMCB_RES_FIN_RM  
  Deletes the value to be deleted from the queue and does not check the next value and requests interruption of processing. (We will interrupt the check for the count argument when calling the k2h_q_remove_ext function.)
- K2HQRMCB_RES_FIN_NOTRM  
  Stop deletion from the queue of the value to be deleted (do not delete), do not inspect the next value, request processing interruption. (We will interrupt the check for the count argument when calling the k2h_q_remove_ext function.)
<br />
<br />
A sample callback function is shown below.

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

#### Note
- A function such as k2h_q_handle returns a handle to the queue, but at the time the handle is returned, the key creation (if not present) of the K2HASH internal queue and the attach operation to the key etc are not done.
  Using this handle, when you perform accumulation (push) or eject (pop) operations, key creation of the internal queue or access to the key occurs.
- Please release the pointer to the value returned from the k2h_q_pop type function on the caller side.
  Performance is not good when calling the k2h_q_count function and specifying the pos argument (indicating the data position) to be specified in the k2h_q_read, k2h_q_str_read family of functions, etc. when a large number of queues are accumulated. Please pay attention when using it.
- Transaction processing of the K2HASH library is implemented in the queue internally.

#### Examples

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

### <a name="KVQUE"> Queue (key & value) family(C I/F)
The K2HASH library provides functions as a queue (FIFO / LIFO).
The queues provided by this function group can be PUSH and POP in FIFO / LIFO, with one key and one value.
This function group is a group of functions related to this key and value queue.

#### Format
- k2h_keyq_h k2h_keyq_handle(k2h_h handle, bool is_fifo)
- k2h_keyq_h k2h_keyq_handle_prefix(k2h_h handle, bool is_fifo, const unsigned char* pref, size_t preflen)
- k2h_keyq_h k2h_keyq_handle_str_prefix(k2h_h handle, bool is_fifo, const char* pref)
- bool k2h_keyq_free(k2h_keyq_h keyqhandle)
- bool k2h_keyq_empty(k2h_keyq_h keyqhandle)
- int k2h_keyq_count(k2h_keyq_h keyqhandle)
- bool k2h_keyq_read(k2h_keyq_h keyqhandle, unsigned char** ppdata, size_t* pdatalen, int pos)
- bool k2h_keyq_read_keyval(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen, int pos)
- bool k2h_keyq_read_wp(k2h_keyq_h keyqhandle, unsigned char** ppdata, size_t* pdatalen, int pos, const char* encpass)
- bool k2h_keyq_read_keyval_wp(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen, int pos, const char* encpass)
- bool k2h_keyq_str_read(k2h_keyq_h keyqhandle, char** ppdata, int pos)
- bool k2h_keyq_str_read_keyval(k2h_keyq_h keyqhandle, char** ppkey, char** ppval, int pos)
- bool k2h_keyq_str_read_wp(k2h_keyq_h keyqhandle, char** ppdata, int pos, const char* encpass)
- bool k2h_keyq_str_read_keyval_wp(k2h_keyq_h keyqhandle, char** ppkey, char** ppval, int pos, const char* encpass)
- bool k2h_keyq_push(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen)
- bool k2h_keyq_push_keyval(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen, const unsigned char* byval, size_t vallen)
- bool k2h_keyq_push_wa(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen, const char* encpass, const time_t* expire)
- bool k2h_keyq_push_keyval_wa(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen, const unsigned char* byval, size_t vallen, const char* encpass, const time_t* expire)
- bool k2h_keyq_str_push(k2h_keyq_h keyqhandle, const char* pkey)
- bool k2h_keyq_str_push_keyval(k2h_keyq_h keyqhandle, const char* pkey, const char* pval)
- bool k2h_keyq_str_push_wa(k2h_keyq_h keyqhandle, const char* pkey, const char* encpass, const time_t* expire)
- bool k2h_keyq_str_push_keyval_wa(k2h_keyq_h keyqhandle, const char* pkey, const char* pval, const char* encpass, const time_t* expire)
- bool k2h_keyq_pop(k2h_keyq_h keyqhandle, unsigned char** ppval, size_t* pvallen)
- bool k2h_keyq_pop_keyval(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen)
- bool k2h_keyq_pop_wp(k2h_keyq_h keyqhandle, unsigned char** ppval, size_t* pvallen, const char* encpass)
- bool k2h_keyq_pop_keyval_wp(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen, const char* encpass)
- bool k2h_keyq_str_pop(k2h_keyq_h keyqhandle, char** ppval)
- bool k2h_keyq_str_pop_keyval(k2h_keyq_h keyqhandle, char** ppkey, char** ppval)
- bool k2h_keyq_str_pop_wp(k2h_keyq_h keyqhandle, char** ppval, const char* encpass)
- bool k2h_keyq_str_pop_keyval_wp(k2h_keyq_h keyqhandle, char** ppkey, char** ppval, const char* encpass)
- bool k2h_keyq_remove(k2h_keyq_h keyqhandle, int count)
- bool k2h_keyq_remove_wp(k2h_keyq_h keyqhandle, int count, const char* encpass)
- int k2h_keyq_remove_ext(k2h_q_h keyqhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata)
- int k2h_keyq_remove_wp_ext(k2h_keyq_h keyqhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata, const char* encpass)
- bool k2h_keyq_dump(k2h_keyq_h keyqhandle, FILE* stream)

#### Description
- k2h_keyq_handle  
  Get handle to queue of FIFO or LIFO. (This queue is a handle to a queue that can perform a series of operations as a set of keys & values.)
- k2h_keyq_handle_prefix  
  Get handle to queue of FIFO or LIFO. You can specify a prefix for the internal key name used in the queue at retrieval time. (This queue is a handle to a queue that can perform a series of operations as a set of keys & values.)
- k2h_keyq_handle_str_prefix  
  Wrapper function to k2h_keyq_handle_ prefix. You can specify the prefix as a string. (This queue is a handle to a queue that can perform a series of operations as a set of keys & values.)
- k2h_keyq_free  
  Discard the handle to the queue.
- k2h_keyq_empty  
  Make sure the queue is empty.
- k2h_keyq_count  
  Get the number of data accumulated in the queue (see notes).
- k2h_keyq_read  
  Copies the key stored in the queue and returns its value. The retrieved key is not deleted. Please release the pointer to the acquired key (see notes).
- k2h_keyq_read_keyval  
  Copies the key stored in the queue and returns the value associated with that key and key. The retrieved key is not deleted. Please release the pointer to the acquired key and value (see notes).
- k2h_keyq_read_wp  
  It is equivalent to k2h_keyq_read. Specify passphrase and read the encrypted queue.
- k2h_keyq_read_keyval_wp  
  It is equivalent to k2h_keyq_read_keyval. Specify passphrase and read the encrypted queue.
- k2h_keyq_str_read  
  Wrapper function to k2h_keyq_read. You can get the key as a string. Please release the pointer to the acquired key (see notes).
- k2h_keyq_str_read_keyval  
  Wrapper function to k2h_keyq_read_keyval. Key & value can be obtained as a string. Please release the pointer to the acquired key & value (see notes).
- k2h_keyq_str_read_wp  
  It is equivalent to k2h_keyq_str_read. Specify passphrase and read the encrypted queue.
- k2h_keyq_str_read_keyval_wp  
  It is equivalent to k2h_keyq_str_read_keyval. Specify passphrase and read the encrypted queue.
- k2h_keyq_push  
  Push the key to the queue. Push to queue only.
- k2h_keyq_push_keyval  
  Write the key & value to K2HASH and push that key to the queue. It also pushes to the queue as a key & value write and a series of operations.
- k2h_keyq_push_wa  
  It is equivalent to k2h_keyq_push. You can push by specifying passphrase and Expire time.
- k2h_keyq_push_keyval_wa  
  It is equivalent to k2h_keyq_push_keyval. You can push by specifying passphrase and Expire time.
- k2h_keyq_str_push  
  Wrapper function to k2h_keyq_push. You can specify the key as a string.
- k2h_keyq_str_push_keyval  
  Wrapper function to k2h_keyq_push_keyval. You can specify keys and values as strings.
- k2h_keyq_str_push_wa  
  It is equivalent to k2h_keyq_str_push. You can push by specifying passphrase and Expire time.
- k2h_keyq_str_push_keyval_wa  
  It is equivalent to k2h_keyq_str_push_keyval. You can push by specifying passphrase and Expire time.
- k2h_keyq_pop  
  Pops the key accumulated in the queue, retrieves the value written in that key, and returns the value. The retrieved key & value is also deleted from K2HASH. Please release the pointer to the acquired key.
- k2h_keyq_pop_keyval  
  Pops the key accumulated in the queue, retrieves the value written in that key, and returns the key & value together. The retrieved key & value is also deleted from K2HASH. Please release the pointer to the acquired key & value.
- k2h_keyq_pop_wp  
  It is equivalent to k2h_keyq_pop. Specify passphrase to retrieve encrypted queue.
- k2h_keyq_pop_keyval_wp  
  It is equivalent to k2h_keyq_pop_keyval. Specify passphrase to retrieve encrypted queue.
- k2h_keyq_str_pop  
  Wrapper function to k2h_keyq_pop. You can get the key as a string. Please release the pointer to the acquired key.
- k2h_keyq_str_pop_keyval  
  Wrapper function to k2h_keyq_pop_keyval. Key & value can be obtained as a string. Please release the pointer to the acquired key & value.
- k2h_keyq_str_pop_wp  
  It is equivalent to k2h_keyq_str_pop. Specify passphrase to retrieve encrypted queue.
- k2h_keyq_str_pop_keyval_wp  
  It is equivalent to k2h_keyq_str_pop_keyval. Specify passphrase to retrieve encrypted queue.
- k2h_keyq_remove  
  Delete the specified number of keys accumulated in the queue. Values associated with deleted keys and keys are also deleted from K2HASH. Deleted keys & values are not returned. Even if you specify more than the accumulated number, it will not result in an error, the contents of the queue will be empty and it will end normally.
- k2h_keyq_remove_wp  
  It is equivalent to k2h_keyq_remove. Specify passphrase to delete encrypted queue.
- k2h_keyq_remove_ext  
  As in k2h_keyq_remove, deletes the specified number of values accumulated in the queue. For this function, specify a callback function. Each time the value is deleted from the queue, the callback function is called and it is possible to independently judge the value deletion. (For the k2h_q_remove_trial_callback callback function and the operation of this function, see the description of the k2h_q_remove_ext function.)
- k2h_keyq_remove_wp_ext  
  It is equivalent to k2h_keyq_remove_ext. Specify passphrase to delete encrypted queue.
- k2h_keyq_dump  
  It is a function for queue debugging and dumps inside the queue.

#### Parameters
- k2h_keyq_handle	
 - handle  
   Specify K2HASH handle.
 - is_fifo  
   Specify the type of queue to be acquired (FIFO or LIFO).
- k2h_keyq_handle_prefix
 - handle  
   Specify K2HASH handle.
 - is_fifo  
   Specify the type of queue to be acquired (FIFO or LIFO).
 - pref  
   Specify the prefix of the internal key name to be used as a queue. (Binary string)
 - preflen  
   Specify the buffer length of pref.
- k2h_keyq_handle_str_prefix
 - handle  
   Specify K2HASH handle.
 - is_fifo  
   Specify the type of queue to be acquired (FIFO or LIFO).
 - pref  
   Specify the prefix of the internal key name to be used as a queue. (It is a character string, it is \0 terminal)
- k2h_keyq_free
 - keyqhandle  
   Specify the handle of the queue (for key & value).
- k2h_keyq_empty
 - keyqhandle  
   Specify the handle of the queue (for key & value).
- k2h_keyq_count
 - keyqhandle  
   Specify the handle of the queue (for key & value).
- k2h_keyq_read
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - ppdata  
   Specify a pointer to receive the value (binary string) associated with the key copied from the queue. Please release the pointer.
 - pdatalen  
   Return buffer length of ppdata.
 - pos  
   Specify the position from the top of the queue of data to be copied. (The beginning is 0)
 - encpass  
   Specify passphrase.
- k2h_keyq_read_keyval
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - ppkey  
   Specify a pointer to receive the value (binary string) associated with the key copied from the queue. Please release the pointer.
 - pkeylen  
   Return the buffer length of ppkey.
 - ppval  
   Specify a pointer to receive the value (binary string) associated with the key copied from the queue. Please release the pointer.
 - pvallen  
   Returns the buffer length of ppval.
 - pos  
   Specify the position from the top of the queue of data to be copied. (The beginning is 0)
 - encpass  
   Specify passphrase.
- k2h_keyq_str_read
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - ppdata  
   Specify a pointer to receive a value (character string, \0 terminal) associated with the key copied from the queue. Please release the pointer.
 - pos  
   Specify the position from the top of the queue of data to be copied. (The beginning is 0)
 - encpass  
   Specify passphrase.
- k2h_keyq_str_read_keyval
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - ppkey  
   Specify a pointer to receive a value (character string, \0 terminal) associated with the key copied from the queue. Please release the pointer.
 - ppval  
   Specify a pointer to receive a value (character string, \0 terminal) associated with the key copied from the queue. Please release the pointer.
 - pos  
   Specify the position from the top of the queue of data to be copied. (The beginning is 0)
 - encpass  
   Specify passphrase.
- k2h_keyq_push
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - bykey  
   Specify the key to push to the queue. (Binary string)
 - keylen  
   Specify the buffer length of bykey.
 - encpass  
   Specify passphrase.
 - expire  
   If you specify Expire time (in seconds) specifies a pointer to a time_t. If not specified, specify NULL.
- k2h_keyq_push_keyval
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - bykey  
   Specify the key to push in the queue. (Binary string)
 - keylen  
   Specify the buffer length of bykey.
 - byval  
   Specify a value associated with the key to push to the queue. (Binary string)
 - vallen  
   Specify the buffer length of byval.
 - encpass  
   Specify passphrase.
 - expire  
   If you specify Expire time (in seconds) specifies a pointer to a time_t. If not specified, specify NULL.
- k2h_keyq_str_push
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - pkey  
   Specify the key to push in the queue. (It is a character string, it is \0 terminal)
 - encpass 
   Specify passphrase.
 - expire  
   If you specify Expire time (in seconds) specifies a pointer to a time_t. If not specified, specify NULL.
- k2h_keyq_str_push_keyval
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - pkey  
   Specify the key to push to the queue. (It is a character string, it is \0 terminal)
 - pval  
   Specify a value associated with the key to push to the queue. (It is a character string, it is \0 terminal)
 - encpass  
   Specify passphrase.
 - expire  
   If you specify Expire time (in seconds) specifies a pointer to a time_t. If not specified, specify NULL.
- k2h_keyq_pop
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - ppval  
   Specify a pointer to receive the value (binary string) associated with the key popped from the queue. Please release the pointer.
 - pvallen  
   Returns the buffer length of ppval.
 - encpass  
   Specify passphrase.
- k2h_keyq_pop_keyval
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - ppkey  
   Specify a pointer to receive the popped key (binary string) from the queue. Please release the pointer.
 - pkeylen  
   Return the buffer length of ppkey.
 - ppval  
   Specify a pointer to receive a value (character string, \0 terminated) that is associated with the key popped from the queue. Please release the pointer.
 - pvallen  
   Returns the buffer length of ppval.
 - encpass  
   Specify passphrase.
- k2h_keyq_str_pop
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - ppval  
   Specify a pointer to receive a value (character string, \0 terminated) that is associated with the key popped from the queue. Please release the pointer.
 - encpass  
   Specify passphrase.
- k2h_keyq_str_pop_keyval
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - ppkey  
   Specify a pointer to receive a key popped from the queue (which is a character string, \ 0 is terminated). Please release the pointer.
 - ppval  
   Specify a pointer to receive a value (character string, \0 terminated) that is associated with the key popped from the queue. Please release the pointer.
 - encpass  
   Specify passphrase.
- k2h_keyq_remove
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - count  
   Specify the number of keys & values you want to delete from the queue.
 - encpass  
   Specify passphrase.
- k2h_keyq_remove*_ext
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - count  
   Specify the number of keys & values you want to delete from the queue.
 - fp  
   Specify a pointer to k2h_q_remove_trial_callback callback function.
 - pextdata  
   Specify an arbitrary pointer to pass to "k2h_q_remove_trial_callback callback function". (NULL allowed)
 - encpass  
   Specify passphrase.
- k2h_keyq_dump
 - keyqhandle  
   Specify the handle of the queue (for key & value).
 - stream  
   Specify the output destination for debugging. If NULL is specified, it is output to stdout.

##### Return Values
- k2h_keyq_handle / k2h_keyq_handle_prefix / k2h_keyq_handle_str_prefix  
  If it ends normally, it returns the handle of the queue. In case of error, K2H_INVALID_HANDLE (0) is returned.
- k2h_keyq_count  
  Returns the number of data accumulated in the queue. If an error occurs, 0 is returned.
- k2h_keyq_remove*_ext  
  Returns the number of deleted queue values. On error, -1 is returned.
- Other than those above  
  Returns true if it is normally completed, and false on error.

#### Note
- This function group is almost the same as [Queue family(C I / F)](#QUE).  
  The difference from [Queue family(CI / F)](#QUE) is that it handles keys and values as a set. Simultaneously with the push and pop operation to the queue, the set of keys and values are also deleted from K2HASH I will.  
- By using this function group, you can store keys and values in K2HASH and queue at the same time, and you can manage the expiration dates of the keys and values using the queue.
- In deleting, you can also delete the key and value body stored in K2HASH by deleting (pop) from the queue.
- When using this function group in the queue, the key and value are stored in K2HASH as a set, so you can access the key as KVS, such as accessing values for normal keys.
- Similar to [Queue family(CI / F)](#QUE), functions such as handle return handles to the queue, but when the handle is returned, the key creation of the K2HASH internal queue (if it does not exist) and , Attach operation to the key etc is not done.
   Using these handles, when you push or pop the operation, key creation of the internal queue is done, or key access occurs.
- Please release the pointer to the value returned from the function of the k2h_keyq_pop type on the caller side. 
- Performance is not good when calling the k2h_keyq_count function and specifying the pos argument (indicating the data position) to be specified for the k2h_keyq_read type function backward when the queue is accumulated in large quantities. Please take care when using it.
- When using the encryption and Expire function of the Builtin attribute, the accumulated queue and the key and value of that queue are both encrypted and the Expire time is set.
   In order to read the queue, passphrase for accumulation is necessary, and it becomes impossible to read out after Expire time elapses.

#### Examples
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

### <a name="DUMP"> Dump and status family(C I/F)
This is a set of debugging functions for dumping K2HASH data.

#### Format
- bool k2h_dump_head(k2h_h handle, FILE* stream)
- bool k2h_dump_keytable(k2h_h handle, FILE* stream)
- bool k2h_dump_full_keytable(k2h_h handle, FILE* stream)
- bool k2h_dump_elementtable(k2h_h handle, FILE* stream)
- bool k2h_dump_full(k2h_h handle, FILE* stream)
- bool k2h_print_state(k2h_h handle, FILE* stream)
- void k2h_print_version(FILE* stream)
- PK2HSTATE k2h_get_state(k2h_h handle)

#### Description
- k2h_dump_head  
  Dump header information of K2HASH data. Dump output is done on stream. If stream is NULL, it is output to stderr.
- k2h_dump_keytable  
  Dump the header and HASH table information of K2HASH data. Dump output is done on stream. If stream is NULL, it is output to stderr.
- k2h_dump_full_keytable  
  Dump the header, HASH table, sub HASH table information of K2HASH data. Dump output is done on stream. If stream is NULL, it is output to stderr.
- k2h_dump_elementtable  
  Dump the header, HASH table, sub HASH table, element information of K2HASH data. Dump output is done on stream. If stream is NULL, it is output to stderr.
- k2h_dump_full  
  Dump the full (header, HASH table, sub HASH table, element , individual data) information of K2HASH data. Dump output is done on stream. If stream is NULL, it is output to stderr.
- k2h_print_state  
  The status (usage status) of K2HASH data is output. Output is done on stream. If stream is NULL, it is output to stderr.
- k2h_print_version  
  Display version and credit of K2HASH library.
- k2h_get_state  
  Returns the structure K2HSTATE that summarizes the header information and status (usage status) of K2HASH data.
  
#### Parameters
- handle  
   Specify the K2HASH handle returned from the k2h_open family function.
- stream  
   Specify the destination FILE pointer. When NULL is specified, it is output to stderr.

#### Return Values
- Other than k2h_get_state  
   Returns true if it succeeds. If it fails, it returns false.
- k2h_get_state  
   If it succeeds, it returns the allocated PK2HSTATE pointer(this pointer needs to be released). If it fails, it returns NULL.

#### Note
- To understand the dump result, you need to know the structure of K2HASH file (or on memory).  
  For the dump result, data of K2HASH file (or on memory) (including HASH table) is output.
- Because full dump dumps all valid K2HASH data, please pay attention to the capacity of output file and processing time for output.

#### Examples

 ```
k2h_print_state(k2handle, NULL);
k2h_dump_full(k2handle, NULL);
 ```

<!-- -----------------------------------------------------------　-->
***

## <a name="CPP"> C++ API
This is the I/F of the C ++ language API for using the K2HASH library.
<br />
<br />
Include the following header file at development time.
 ```
#include <k2hash/k2hash.h>
#include <k2hash/k2hshm.h>
 ```
<br />
When linking please specify the following as an option.
 ```
-lk2hash
 ```
<br />
The functions for the C ++ language are explained below.

### <a name="DEBUGCPP"> Debug family(C/C++ I/F)
The K2HASH library can output messages to check internal operation and API operation.
This function group is a group of functions for controlling message output.

#### Format
- K2hDbgMode SetK2hDbgMode(K2hDbgMode mode)
- K2hDbgMode BumpupK2hDbgMode(void)
- K2hDbgMode GetK2hDbgMode(void)
- bool LoadK2hDbgEnv(void)
- bool SetK2hDbgFile(const char* filepath)
- bool UnsetK2hDbgFile(void)
- bool SetSignalUser1(void)

#### Description
- SetK2hDbgMode  
  Set the message output level for debugging. The default is non-output.
- BumpupK2hDbgMode  
  Bump up the message output level for debugging. The default is non-output, and each time this function is called, it switches between non-output -> error -> warning -> message -> non-output.
- GetK2hDbgMode  
  Get the current message output level for debugging.
- LoadK2hDbgEnv  
  Re-read the message output level for debugging and the environment variable (K2HDBGMODE, K2HDBGFILE) related to the output file, and reset the environment variable if it is set.
- SetK2hDbgFile  
  Set the message output file for debugging. By default (when environment variable K2HDBGFILE is not specified), stderr is set.
- UnsetK2hDbgFile  
  When a message output file for debugging is set, it returns to the default stderr.
- SetSignalUser1  
  By setting SIGUSR1 signal, set the signal handler so that you can bump up the message output level for debugging.

#### Parameters
- mode  
  Set the value of K2hDbgMode that specifies the message output level. The currently settable values are the following four types.
  - Non-output  
    K2hDbgMode::K2HDBG_SILENT
  - Error  
    K2hDbgMode::K2HDBG_ERR
  - Warning  
    K2hDbgMode::K2HDBG_WARN
  - Message(Information)  
    K2hDbgMode::K2HDBG_MSG
- filepath  
  Specify the file path of the message output file.

#### Return Values
- SetK2hDbgMode  
  Returns the message output level for debugging which was set immediately before.
- BumpupK2hDbgMode  
  Returns the message output level for debugging which was set immediately before.
- GetK2hDbgMode  
  Returns the currently set message output level for debugging.
- LoadK2hDbgEnv / SetK2hDbgFile / UnsetK2hDbgFile / SetSignalUser1  
  Returns true if it succeeds. If it fails, it returns false.

#### Note
- The message output level for debugging and the environment variables (K2HDBGMODE, K2HDBGFILE) related to the output file are set when the K2HASH library is loaded, so it is not necessary to set them again by LoadK2hDbgEnv() function.
- Please note that the setting of SIGUSR1 signal handler by SetSignalUser1() function may inconvenience operation depending on the program to be implemented.  
  The K2HASH library sets up signal handlers, handles signals, but does not call previously set handlers.   
  Also, please check on the implementing side if you want to check the operation when multiply SIGUSR1 signal is received in multithread.  

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HASHDYNLIB"> K2HashDynLib class

#### Description
The K2HASH library can read the HASH function used internally as an external shared library (DSO module).
This class is a management class that loads and unloads this shared library.
This class object is a singleton which exists only in the K2HASH library.
To use the method of this class, please obtain the singleton's K2HashDynLib object pointer and use it.

#### HASH function prototype
The DSO module must contain the following three functions.

 ```
// First hash value returns, the value is used Key Index
k2h_hash_t k2h_hash(const void* ptr, size_t length);
 
// Second hash value returns, the value is used the Element in collision keys.
k2h_hash_t k2h_second_hash(const void* ptr, size_t length);
 
// Hash function(library) version string, the value is stamped into SHM file.
// This retuned value length must be under 32 byte.
const char* k2h_hash_version(void);
 ```
For details, see k2hashfunc.h.

#### Method
- static K2HashDynLib* K2HashDynLib::get(void)
- bool K2HashDynLib::Load(const char* path)
- bool K2HashDynLib::Unload(void)

#### Method Description
- K2HashDynLib::get  
  It is a class method and returns the only K2HashDynLib object pointer of the singleton K2HASH library. Please call the method of this class using the object returned by this method.
- K2HashDynLib::Load  
  Load the DSO module of the HASH function.
- K2HashDynLib::Unload  
  If there is a DSO module of the loaded HASH function, it unloads it.

#### Method return value
- K2HashDynLib::get  
  K2HASH library which is singleton It returns only K2HashDynLib object pointer. This function will not fail.
- K2HashDynLib::Load  
  Returns true if it succeeds. If it fails, it returns false.
- K2HashDynLib::Unload  
  Returns true if it succeeds. If it fails, it returns false.
  
#### Note
Please handle singleton handling.

#### Examples
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

### <a name="K2HTRANSDYNLIB"> K2HTransDynLib class

#### Description
The K2HASH library can read internal transaction processing as an external shared library (DSO module).
This class is a management class that loads and unloads this shared library.
This class object is a singleton which exists only in the K2HASH library.
To use the method of this class, please acquire the singleton's K2HTransDynLib object pointer and use it.

#### Transaction system function prototype
The DSO module must contain the following three functions.

 ```
// transaction callback function
bool k2h_trans(k2h_h handle, PBCOM pBinCom);
 
// Transaction function(library) version string.
const char* k2h_trans_version(void);
 
// transaction control function
bool k2h_trans_cntl(k2h_h handle, PTRANSOPT pOpt);
 ```

For details, see k2htransfunc.h.

#### Method
- static K2HTransDynLib* K2HTransDynLib::get(void)
- bool K2HTransDynLib::Load(const char* path)
- bool K2HTransDynLib::Unload(void)

#### Method Description
- K2HTransDynLib::get  
  It is a class method and returns the only K2HTransDynLib object pointer of the singleton K2HASH library.
  Please call the method of this class using the object returned by this method.
- K2HTransDynLib::Load  
  Load DSO module of transaction type function.
- K2HTransDynLib::Unload  
  If there is a DSO module of a loaded transactional function, it unloads it.

#### Method return value
- K2HTransDynLib::get  
  It returns the only K2HTransDynLib object pointer of the singleton K2HASH library. This function will not fail.
- K2HTransDynLib::Load  
  Returns true if it succeeds. If it fails, it returns false.
- K2HTransDynLib::Unload  
  Returns true if it succeeds. If it fails, it returns false.

#### Note 
Please handle singleton handling.

#### Examples
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

### <a name="K2HDACCESS"> K2HDAccess class

#### Description
It is a class that directly accesses data of K2HASH file (or on memory).
From the k2hshm class, you can retrieve and use objects of this class by specifying the key.
It is mainly used for reading and writing to data with a large size value. Parts of the value can be overwritten or read by specifying the offset.

#### Method
- K2HDAccess::K2HDAccess(K2HShm* pk2hshm, K2HDAccess::ACSMODE access_mode)
<br />
<br />
- bool K2HDAccess::IsInitialize(void) const
- bool K2HDAccess::Open(const char* pKey)
- bool K2HDAccess::Open(const unsigned char* byKey, size_t keylength)
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
- bool K2HDAccess::Write(const char* pValue)
- bool K2HDAccess::Write(const unsigned char* byValue, size_t vallength)
- bool K2HDAccess::Write(int fd, size_t& wlength)
- bool K2HDAccess::Read(char** ppValue)
- bool K2HDAccess::Read(unsigned char** byValue, size_t& vallength)
- bool K2HDAccess::Read(int fd, size_t& rlength)

#### Method Description
- K2HDAccess::K2HDAccess  
  It is a constructor. Specify a pointer to the k2hshm object and the mode to be accessed (do not directly create this class but use the method of the k2hshm class to get the K2HDAccess object).
- K2HDAccess::IsInitialize  
  Confirmation of initialization
- K2HDAccess::Open  
  Specify the key and open it for K2HDAccess (Do not directly generate this class, please obtain the K2HDAccess object using the method of the k2hshm class.)
- K2HDAccess::Close  
  Close the key. You can unlock the key by closing it, so please call as necessary.
- K2HDAccess :: SetOffset  
   Set the offset of the read / write position.
- K2HDAccess :: SetWriteOffset  
   Sets the offset of the writing position.
- K2HDAccess :: SetReadOffset  
   Sets the offset of the read position.
- K2HDAccess :: GetWriteOffset  
   Get the offset of the writing position.
- K2HDAccess :: GetReadOffset  
   Get the offset of the read position.
- K2HDAccess :: GetSize  
   Gets the length of the value.
- K2HDAccess :: SetFioSize  
   Specify the size of the buffer to be used when reading from a file or outputting to a file (default is 400 Kbyte).
- K2HDAccess :: GetFioSize  
   Get the size of the buffer used when reading from a file or outputting to a file.
- K2HDAccess :: Write  
   Write to the value.
- K2HDAccess :: Read  
   Read the value.

#### Method return value
- bool  
  Returns true if it succeeds, false if it fails.
- off_t  
  If it succeeds, it returns the write / read position as an offset. If it fails, it returns -1.
- size_t  
  Returns the size of the buffer.

#### Note
When direct access is used, it is not affected by attributes such as encryption. This means, for example, direct access to encrypted data, it will destroy the data itself, so do not use it for the key that sets the attribute.

#### Examples
 ```
k2hshm*    pk2hash;
    ・
    ・
    ・
 
// attach write object
K2HDAccess*    pAccess;
if(NULL == (pAccess = pk2hash->GetDAccessObj("mykey", K2HDAccess::WRITE_ACCESS, 0))){
    return false
}
 
// write
if(!pAccess->Write("my test data")){
    delete pAccess;
    return false;
}
delete pAccess;
 
// attach read object
if(NULL == (pAccess = pk2hash->GetDAccessObj("mykey", K2HDAccess::READ_ACCESS, 0))){
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

### <a name="K2HSTREAM"> k2hstream(ik2hstream、ok2hstream) class

#### Description
Class for handling access to data of "K2HASH file (or on memory)" as iostream.  
We implement direct access (reading and writing) to values as iostream derived classes.  
You can specify k2hshm class and key, initialize the stream class (k2hstream, ik2hstream, ok2hstream) and use it as an iostream.  
It is equivalent to std :: stringstream and you can use seekpos.  
Please refer to std :: stringstream for detailed explanation.

#### Base class
<table>
<tr><td>k2hstream </td><td>std::basic_stream</td></tr>
<tr><td>ik2hstream</td><td>std::basic_istream</td></tr>
<tr><td>ok2hstream</td><td>std::basic_ostream</td></tr>
</table>

#### Examples
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

#### Note
At the time this class was created (precisely when the key was opened through this class), locking will occur on the key.
Read lock in ik2hstream class, and write lock in ok2hstream and k2hstream classes are applied.
In order to avoid unnecessary locked state, this class should be discarded or closed when use is completed.

<!-- -----------------------------------------------------------　-->
***

### <a name="K2HARCHIVE"> K2HArchive class

#### Description
Class for archiving K2HASH data.
The K2HASH data managed by the K2HShm class is archived by the K2HArchive class.

#### Method
- K2HArchive::K2HArchive(const char* pFile = NULL, bool iserrskip = false)
- bool K2HArchive::Initialize(const char* pFile, bool iserrskip)
- bool K2HArchive::Serialize(K2HShm* pShm, bool isLoad) const

#### Method Description
- K2HArchive::K2HArchive  
  It is a constructor. You can specify the archive file and the processing flag (flag indicating whether or not to continue processing when an error occurs) when an error occurs.
- K2HArchive::Initialize  
  Initialization method. You can specify the archive file and the processing flag (flag indicating whether or not to continue processing when an error occurs) when an error occurs.
- K2HArchive::Serialize  
  Serialized (archived) execution method. Specify the K2HShm class pointer you want to archive. Output and load flags are specified.

#### Method return value
Returns true if it succeeds. If it fails, it returns false.



#### Examples
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

### <a name="K2HQUEUE"> K2HQueue class

#### Description
The K2HASH library provides functions as a queue (FIFO / LIFO).  
The queue can push and pop values as FIFO / LIFO.  
This class is a class for manipulating queues.  
With this class you can manipulate pushing, popping, and deleting data into the queue.  
<br />
The queue provided by the K2HASH library is implemented with keys and values, so a specific prefix is given to the internal key names stored in the queue.
If prefix is not specified, "\0K2HQUEUE_PREFIX_" (note that the first byte is '\0'(0x00)) is used as the default.
You can use the Builtin attribute to specify queue encryption and valid(Expire) time.

#### Method
- K2HQueue::K2HQueue(K2HShm* pk2h, bool is_fifo, const unsigned char* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
<br />
<br />
- bool K2HQueue::Init(K2HShm* pk2h, bool is_fifo, const unsigned char* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
- bool K2HQueue::Init(const unsigned char* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
<br />
<br />
- bool K2HQueue::IsEmpty(void) const
- int K2HQueue::GetCount(void) const
- bool K2HQueue::Read(unsigned char** ppdata, size_t& datalen, int pos, const char* encpass) const
- bool K2HQueue::Push(const unsigned char* bydata, size_t datalen, K2HAttrs* pAttrs, const char* encpass, const time_t* expire)
- bool K2HQueue::Pop(unsigned char** ppdata, size_t& datalen, K2HAttrs** ppAttrs, const char* encpass)
- int K2HQueue::Remove(int count, k2h_q_remove_trial_callback fp = NULL, void* pExtData, const char* encpass)
- bool K2HQueue::Dump(FILE* stream)

#### Method Description
- K2HQueue::K2HQueue  
  Create a queue object by specifying the K2HShm object, FIFO / LIFO. At the time of generation, operation (writing etc.) to K2HASH data is not performed. You can specify the mask of the Builtin attribute as an argument (usually this argument is never used).
- K2HQueue::Init  
  initialize the queue object by specifying the K2HShm object. Operation (writing etc.) to K2HASH data is not performed at the time of initialization. You can specify the mask of the Builtin attribute as an argument (usually this argument is never used).
- K2HQueue::Init  
  Initialize the queue object with a prefix. Operation (writing etc.) to K2HASH data is not performed at the time of initialization. You can specify the mask of the Builtin attribute as an argument (usually this argument is never used).
- K2HQueue::IsEmpty  
  Make sure the queue is empty.
- K2HQueue::GetCount  
  Returns the number of data accumulated in the queue (see notes).
- K2HQueue::Read  
  Copy the data from the queue. Data is not deleted from the queue. Please release the pointer of returned data (see notes). For an encrypted queue, please specify passphrase.
- K2HQueue::Push  
  Push the data to the queue. You can also specify passphrase and Expire time when accumulating. You can also specify an object pointer to an attribute information class that can be used when accumulating the popped queue again.
- K2HQueue::Pop  
  Pop the data from the queue. Please release the pointer of returned data. For an encrypted queue, please specify passphrase. You can get the object pointer to the attribute information class if necessary (this value can be passed to Push if accumulating again).
- K2HQueue::Remove  
  Delete the specified number of data from the queue. Deleted data will not be returned. If you specify more than the number of accumulated data, the queue becomes empty and it ends normally. For an encrypted queue, please specify passphrase.
  For the argument fp, you can specify the k2h_q_remove_trial_callback callback function. When callback function is specified, it is called for each deletion and it is possible to judge whether it can be deleted (For details about the callback function, see the description of k2h_q_remove_ext function).
- K2HQueue::Dump  
  We will dump the keys related to the queue. It is a method for debugging.

#### Method return value
- K2HQueue::GetCount  
  Returns the number of data accumulated in the queue. If an error occurs, 0 is returned.
- K2HQueue::Remove  
  Returns the number of values deleted from the queue. If an error occurs, -1 is returned.
- Other than those above  
  Returns true if it succeeds, false if it fails.

#### Note
This class is an operation class, and it does not operate on K2HASH data at the time of creating a class instance. The actual operation occurs when a method such as Push, Pop, etc. is executed.
Performance is not good when using K2HQueue :: GetCount, K2HQueue :: Read with a lot of data accumulated in the queue. Please take care when using it.

#### Examples

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

### <a name="K2HKEYQUEUE"> K2HKeyQueue class

#### Description
The K2HASH library provides functions as a queue (FIFO / LIFO).
Queues provided by this function group can be pushed and popped with FIFO / LIFO with a key and values as one pair.
This class is a derived class of K2HQueue, it will be a class supporting this key and value queue.
The key name associated with the queue has a specific prefix and is the same as K2HQueue.
<br />
With this class you can manipulate push, pop, and delete data (keys and values) into the queue.
If you push by specifying the key and value in the queue of this class, the key and value are created in the K2HASH data and the key is accumulated in the queue.
In the pop from the queue, you can retrieve it with a set of keys and values. (It is also possible to retrieve only the value)
If you pop from the queue, the key is deleted from the queue and the set of keys and values is also deleted from the K2HASH data.
<br />
With this class, you can accumulate the written (updated) order of keys and values simultaneously with writing to K2HASH data.
By extracting from the queue, keys and values can be erased simultaneously from K2HASH data.
<br />
Encryption and expiration time (Expire) can be set using the Builtin attribute of the keys accumulated in the queue and their keys and values.
<br />

#### Method
- K2HKeyQueue::K2HKeyQueue(K2HShm* pk2h, bool is_fifo, const unsigned char* pref, size_t preflen)
<br />
<br />
- bool K2HKeyQueue::Init(K2HShm* pk2h, bool is_fifo, const unsigned char* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
- bool K2HKeyQueue::Init(const unsigned char* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
<br />
<br />
- bool K2HKeyQueue::IsEmpty(void) const
- int K2HKeyQueue::GetCount(void) const
- bool K2HKeyQueue::Read(unsigned char** ppdata, size_t& datalen, int pos, const char* encpass) const
- bool K2HKeyQueue::Read(unsigned char** ppkey, size_t& keylen, unsigned char** ppval, size_t& vallen, int pos, const char* encpass) const
- bool K2HKeyQueue::Push(const unsigned char* bydata, size_t datalen, const char* encpass, const time_t* expire)
- bool K2HKeyQueue::Push(const unsigned char* pkey, size_t keylen, const unsigned char* byval, size_t vallen, const char* encpass, const time_t* expire)
- bool K2HKeyQueue::Pop(unsigned char** ppdata, size_t& datalen, const char* encpass)
- bool K2HKeyQueue::Pop(unsigned char** ppkey, size_t& keylen, unsigned char** ppval, size_t& vallen, const char* encpass)
- int K2HKeyQueue::Remove(int count, k2h_q_remove_trial_callback fp = NULL, void* pExtData, const char* encpass)
- bool K2HKeyQueue::Dump(FILE* stream)

#### Method Description
- K2HKeyQueue::K2HKeyQueue
  - Create K2HKeyQueue object with K2HShm object and FIFO / LIFO specified. At the time of generation, operation (writing etc.) to K2HASH data is not performed.
- K2HKeyQueue::Init  
  - Initialize the K2HKeyQueue object with K2HShm object specified. Operation (writing etc.) to K2HASH data is not performed at the time of initialization. The method is the base class K2HQueue::Init.
- K2HKeyQueue::Init  
  - Initialize the K2HKeyQueue object with the prefix. Operation (writing etc.) to K2HASH data is not performed at the time of initialization. The method is the base class K2HQueue::Init.
- K2HKeyQueue::IsEmpty  
  Make sure the queue is empty.
- K2HKeyQueue::GetCount  
  Get the number of data accumulated in the queue.
- K2HKeyQueue::Read  
  Copy the value from the queue. It is the same as the base class K2HQueue :: Pop, but the returned data is not the key accumulated in the queue but the value of the key. Keys and values are not deleted from the queue. Please release the pointer of the returned value (see notes). If it is encrypted, please specify passphrase.
- K2HKeyQueue::Read  
  Copies the key from the queue and returns the key and value. The returned data is the value of the key accumulated in the queue and the key. Keys and values are not deleted from the queue. Please release the returned key and value pointer (see notes). If it is encrypted, please specify passphrase.
- K2HKeyQueue::Push  
  Push the key to the queue. The method is the base class K2HQueue :: Push. Therefore, the key is not written to the K2HASH data. If you want to write, please use the push method below.
- K2HKeyQueue::Push  
  Push the key to the queue and write the key and value to the K2HASH data. When encrypting, please specify passphrase. To specify the Expire time, specify the time_t pointer (specify NULL if not specified).
- K2HKeyQueue::Pop  
  Pop values from the queue. The argument is the same as the base class K2HQueue :: Pop, but the returned data is not the key accumulated in the queue but the value of the key. After taking out, the key and value are also deleted from the K2HASH data. Please release the pointer of the returned value. If it is encrypted, please specify passphrase.
- K2HKeyQueue::Pop  
  Pops the key from the queue and returns the key and value. The returned data is the key and value stored in the queue. After taking out, the key and value are deleted from the K2HASH data. Please release the returned key and value pointer. If it is encrypted, please specify passphrase.
- K2HKeyQueue::Remove  
  The specified number of keys are deleted from the queue, and those keys and values are also deleted from the K2HASH data. Deleted keys and values are not returned. If more than the number of accumulated keys is specified, the queue becomes empty and it ends normally. If it is encrypted, please specify passphrase. For the argument fp, you can specify the k2h_q_remove_trial_callback callback function. If you specify a callback function, it is called for each deletion, and it is possible to judge whether it can be deleted. (For details on the callback function, see the description of the k2h_q_remove_ext function.)
- K2HKeyQueue::Dump  
  We will dump the keys related to the queue. It is a method for debugging. The method is the base class K2HQueue::Dump.

#### Method return value
- K2HKeyQueue::GetCount  
  Returns the number of data accumulated in the queue. If an error occurs, 0 is returned.
- K2HKeyQueue::Remove  
  Returns the number of values deleted from the queue. If an error occurs, -1 is returned.
- Other than those above  
  Returns true if it succeeds, false if it fails.

#### Note
This class is an operation class, and it does not operate on K2HASH data at the time of creating a class instance. The actual operation occurs when a method such as Push, Pop, etc. is executed.
Performance is not good when using K2HKeyQueue::GetCount, K2HKeyQueue::Read with a large amount of data accumulated in the queue. Please take care when using it.

#### Examples
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

### <a name="K2HSHM"> K2HShm class

#### Description
K2HASH Data manipulation and implementation class. Through this class, we will manipulate the K2HASH data.
Basic K2HASH data manipulation in the C ++ language performs all operations after creating instances of this class. An instance can be created by opening (attaching) the K2HASH file or on memory.
When the operation is completed (at the time of termination) close (detach) please.

#### Class method
- size_t K2HShm::GetSystemPageSize(void)
- int K2HShm::GetMaskBitCount(k2h_hash_t mask)
- int K2HShm::GetTransThreadPool(void)
- bool K2HShm::SetTransThreadPool(int count)
- bool K2HShm::UnsetTransThreadPool(void)

#### Method
- bool K2HShm::Create(const char* file, bool isfullmapping = true, int mask_bitcnt = MIN_MASK_BITCOUNT, int cmask_bitcnt = DEFAULT_COLLISION_MASK_BITCOUNT, int max_element_cnt = DEFAULT_MAX_ELEMENT_CNT, size_t pagesize = MIN_PAGE_SIZE)
- bool K2HShm::Attach(const char* file, bool isReadOnly, bool isCreate = true, bool isTempFile = false, bool isfullmapping = true, int mask_bitcnt = MIN_MASK_BITCOUNT, int cmask_bitcnt = DEFAULT_COLLISION_MASK_BITCOUNT, int max_element_cnt = DEFAULT_MAX_ELEMENT_CNT, size_t pagesize = MIN_PAGE_SIZE)
- bool K2HShm::AttachMem(int mask_bitcnt = MIN_MASK_BITCOUNT, int cmask_bitcnt = DEFAULT_COLLISION_MASK_BITCOUNT, int max_element_cnt = DEFAULT_MAX_ELEMENT_CNT, size_t pagesize = MIN_PAGE_SIZE)
- bool K2HShm::IsAttached(void)
- bool K2HShm::Detach(long waitms = DETACH_NO_WAIT)
<br />
<br />
- k2h_hash_t K2HShm::GetCurrentMask(void) const
- k2h_hash_t K2HShm::GetCollisionMask(void) const
- unsigned long K2HShm::GetMaxElementCount(void) const
- const char* K2HShm::GetK2hashFilePath(void) const
<br />
<br />
- char* K2HShm::Get(const char* pKey, bool checkattr = true, const char* encpass = NULL) const
- ssize_t K2HShm::Get(const unsigned char* byKey, size_t length, unsigned char** byValue, bool checkattr = true, const char* encpass = NULL) const
- ssize_t K2HShm::Get(const unsigned char* byKey, size_t keylen, unsigned char** byValue, k2h_get_trial_callback fp, void* pExtData, bool checkattr, const char* encpass)
- char* K2HShm::Get(PELEMENT pElement, int type) const
- ssize_t K2HShm::Get(PELEMENT pElement, unsigned char** byData, int type) const
- strarr_t::size_type K2HShm::Get(const char* pKey, strarr_t& strarr, bool checkattr = true, const char* encpass = NULL) const
- strarr_t::size_type K2HShm::Get(const unsigned char* byKey, size_t length, strarr_t& strarr, bool checkattr = true, const char* encpass = NULL) const
- strarr_t::size_type K2HShm::Get(PELEMENT pElement, strarr_t& strarr, bool checkattr = true, const char* encpass = NULL) const
- K2HSubKeys* K2HShm::GetSubKeys(const char* pKey, bool checkattr = true) const
- K2HSubKeys* K2HShm::GetSubKeys(const unsigned char* byKey, size_t length, bool checkattr = true) const
- K2HSubKeys* K2HShm::GetSubKeys(PELEMENT pElement, bool checkattr = true) const
- K2HAttrs* K2HShm::GetAttrs(const char* pKey) const
- K2HAttrs* K2HShm::GetAttrs(const unsigned char* byKey, size_t length) const
- K2HAttrs* K2HShm::GetAttrs(PELEMENT pElement) const
<br />
<br />
- bool K2HShm::Set(const char* pKey, const char* pValue, const char* encpass = NULL, const time_t* expire = NULL)
- bool K2HShm::Set(const char* pKey, const char* pValue, K2HSubKeys* pSubKeys, bool isRemoveSubKeys = true, const char* encpass = NULL, const time_t* expire = NULL)
- bool K2HShm::Set(const char* pKey, const char* pValue, K2HAttrs* pAttrs, const char* encpass = NULL, const time_t* expire = NULL)
- bool K2HShm::Set(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, const char* encpass = NULL, const time_t* expire = NULL)
- bool K2HShm::Set(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, K2HSubKeys* pSubKeys, bool isRemoveSubKeys = true, K2HAttrs* pAttrs = NULL, const char* encpass = NULL, const time_t* expire = NULL, K2hAttrOpsMan::ATTRINITTYPE attrtype = K2hAttrOpsMan::OPSMAN_MASK_NORMAL)
- bool K2HShm::AddSubkey(const char* pKey, const char* pSubkey, const char* pValue, const char* encpass = NULL, const time_t* expire = NULL)
- bool K2HShm::AddSubkey(const char* pKey, const char* pSubkey, const unsigned char* byValue, size_t vallength, const char* encpass = NULL, const time_t* expire = NULL)
- bool K2HShm::AddSubkey(const unsigned char* byKey, size_t keylength, const unsigned char* bySubkey, size_t skeylength, const unsigned char* byValue, size_t vallength, const char* encpass = NULL, const time_t* expire = NULL)
- bool K2HShm::AddAttr(const char* pKey, const char* pattrkey, const char* pattrval)
- bool K2HShm::AddAttr(const char* pKey, const unsigned char* pattrkey, size_t attrkeylen, const unsigned char* pattrval, size_t attrvallen)
- bool K2HShm::AddAttr(const unsigned char* byKey, size_t keylength, const unsigned char* pattrkey, size_t attrkeylen, const unsigned char* pattrval, size_t attrvallen)
<br />
<br />
- bool K2HShm::Remove(PELEMENT pParentElement, PELEMENT pSubElement)
- bool K2HShm::Remove(PELEMENT pElement, const char* pSubKey)
- bool K2HShm::Remove(PELEMENT pElement, const unsigned char* bySubKey, size_t length)
- bool K2HShm::Remove(PELEMENT pElement, bool isSubKeys = true)
- bool K2HShm::Remove(const char* pKey, bool isSubKeys)
- bool K2HShm::Remove(const unsigned char* byKey, size_t keylength, bool isSubKeys = true)
- bool K2HShm::Remove(const char* pKey, const char* pSubKey)
- bool K2HShm::Remove(const unsigned char* byKey, size_t keylength, const unsigned char* bySubKey, size_t sklength)
- bool K2HShm::ReplaceAll(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, const unsigned char* bySubkeys, size_t sklength, const unsigned char* byAttrs, size_t attrlength)
- bool K2HShm::ReplaceValue(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength)
- bool K2HShm::ReplaceSubkeys(const unsigned char* byKey, size_t keylength, const unsigned char* bySubkeys, size_t sklength)
- bool K2HShm::ReplaceAttrs(const unsigned char* byKey, size_t keylength, const unsigned char* byAttrs, size_t attrlength)
<br />
<br />
- bool K2HShm::Rename(const char* pOldKey, const char* pNewKey)
- bool K2HShm::Rename(const unsigned char* byOldKey, size_t oldkeylen, const unsigned char* byNewKey, size_t newkeylen)
- bool K2HShm::Rename(const unsigned char* byOldKey, size_t oldkeylen, const unsigned char* byNewKey, size_t newkeylen, const unsigned char* byAttrs, size_t attrlen)
<br />
<br />
- K2HDAccess* K2HShm::GetDAccessObj(const char* pKey, K2HDAccess::ACSMODE acsmode, off_t offset)
- K2HDAccess* K2HShm::GetDAccessObj(const unsigned char* byKey, size_t keylength, K2HDAccess::ACSMODE acsmode, off_t offset)
<br />
<br />
- bool GetElementsByHash(const k2h_hash_t starthash, const struct timespec startts, const struct timespec endts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check, k2h_hash_t* pnexthash, PK2HBIN* ppbindatas, size_t* pdatacnt) const
- bool SetElementByBinArray(const PRALLEDATA prawdata, const struct timespec* pts)
<br />
<br />
- K2HShm::iterator K2HShm::begin(const char* pKey)
- K2HShm::iterator K2HShm::begin(const unsigned char* byKey, size_t length)
- K2HShm::iterator K2HShm::begin(void)
- K2HShm::iterator K2HShm::end(bool isSubKey = false)
<br />
<br />
- K2HQueue* K2HShm::GetQueueObj(bool is_fifo = true, const unsigned char* pref = NULL, size_t preflen = 0L)
- K2HKeyQueue* K2HShm::GetKeyQueueObj(bool is_fifo = true, const unsigned char* pref = NULL, size_t preflen = 0L)
- bool K2HShm::IsEmptyQueue(const unsigned char* byMark, size_t marklength) const
- int K2HShm::GetCountQueue(const unsigned char* byMark, size_t marklength) const
- bool K2HShm::ReadQueue(const unsigned char* byMark, size_t marklength, unsigned char** ppKey, size_t& keylength, unsigned char** ppValue, size_t& vallength, int pos = 0, const char* encpass = NULL) const
- bool K2HShm::PushFifoQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs* pAttrs = NULL, const char* encpass = NULL, const time_t* expire = NULL)
- bool K2HShm::PushLifoQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs* pAttrs = NULL, const char* encpass = NULL, const time_t* expire = NULL)
- bool K2HShm::PopQueue(const unsigned char* byMark, size_t marklength, unsigned char** ppKey, size_t& keylength, unsigned char** ppValue, size_t& vallength, K2HAttrs** ppAttrs = NULL, const char* encpass = NULL)
- int K2HShm::RemoveQueue(const unsigned char* byMark, size_t marklength, unsigned int count, bool rmkeyval, k2h_q_remove_trial_callback fp = NULL, void* pExtData = NULL, const char* encpass = NULL)
<br />
<br />
- bool K2HShm::Dump(FILE* stream = stdout, int dumpmask = DUMP_KINDEX_ARRAY)
- bool K2HShm::PrintState(FILE* stream = stdout)
- bool K2HShm::PrintAreaInfo(void) const
- bool K2HShm::DumpQueue(FILE* stream, const unsigned char* byMark, size_t marklength)
- PK2HSTATE K2HShm::GetState(void) const
<br />
<br />
- bool K2HShm::EnableTransaction(const char* filepath, const unsigned char* pprefix = NULL, size_t prefixlen = 0, const unsigned char* pparam = NULL, size_t paramlen = 0, const time_t* expire = NULL) const
- bool K2HShm::DisableTransaction(void) const
<br />
<br />
- bool K2HShm::SetCommonAttribute(const bool* is_mtime = NULL, const bool* is_defenc = NULL, const char* passfile = NULL, const bool* is_history = NULL, const time_t* expire = NULL, const strarr_t* pluginlibs = NULL)
- bool K2HShm::AddAttrCryptPass(const char* pass, bool is_default_encrypt = false)
- bool K2HShm::AddAttrPluginLib(const char* path)
- bool K2HShm::CleanCommonAttribute(void)
- bool K2HShm::GetAttrVersionInfos(strarr_t& verinfos) const
- void K2HShm::GetAttrInfos(std::stringstream& ss) const
<br />
<br />
- bool K2HShm::AreaCompress(bool& isCompressed)
- bool K2HShm::SetMsyncMode(bool enable)

#### Method Description
- K2HShm::GetSystemPageSize  
  Returns the page size of K2HASH data operated by the k2hash object.
- K2HShm::GetMaskBitCount  
  Returns the value of Mask of K2HASH data operated by k2hash object.
- K2HShm::GetTransThreadPool  
  Returns the number of transaction thread pools set in the k2hash class.
- K2HShm::SetTransThreadPool  
  Set the number of transaction thread pools set in k2hash class.
- K2HShm::UnsetTransThreadPool  
  Set not to use the thread pool for transaction set in k2hash class.
- K2HShm::Create  
  Generate K2HASH file.
- K2HShm :: Attach  
  Attach K2HASH file to memory.
- K2HShm :: AttachMem  
  Attach to K2HASH memory.
- K2HShm::Detach  
  Detach attaching K2HASH data. It provides a way to wait for completion of transaction processing when detaching. Wait ms (millisecond) can be specified by argument. If 0 is specified as an argument, detaching is done immediately. If -1 is specified, it will block until the transaction is completed. A positive number indicates ms (milliseconds) and waits for completion of the transaction for the specified ms.
- K2HShm::GetCurrentMask  
  Gets the value of the current Current Mask.
- K2HShm::GetCollisionMask  
  Get the value of the current conflict Mask.
- K2HShm::GetMaxElementCount  
  Get the maximum number of elements currently.
- K2HShm::GetK2hashFilePath  
   Get the K2HASH file path currently open.
- K2HShm::Get  
  Specify the key and get the value, subkey list, etc. K2h_get_trial_callback For methods that specify a callback function pointer, it performs the same processing as the k2h_get_value_ext function. (Refer to the description of the k2h_get_value_ext function for a detailed explanation of the callback function and this function.)
You can specify a flag on whether to check Attribute when acquiring the target key. This is due to the Builtin attribute etc. which can not be retrieved unless the value is declared, and the expire time must be checked. In the case of a key for which the Builtin attribute is specified, use these arguments as appropriate.
- K2HShm::GetSubKeys  
  Specify the key and get the subkey list. Get as K2HSubKeys class object pointer. Release the acquired K2HSubKeys class object pointer with delete(). You can specify a flag on whether to check Attribute when subkey is acquired. This is because the Builtin attribute requires inspection of the Expire time. In case of key with Builtin attribute specified, please specify flag as appropriate.
- K2HShm::GetAttrs  
  Specify the key and acquire the set attribute. K2HAttrs Get as a pointer to a class object. Release the acquired K2HAttrs class object pointer with delete().
- K2HShm::Set  
  Specify the key and set the value, subkey list and so on. When setting, it is possible to individually set the Builtin attribute encryption and Expire time specification (there is a type method of taking the K2hAttrOpsMan::ATTRINITTYPE argument, but this method is for internal implementation There is little to use directly.)
- K2HShm::AddSubkey  
  Specify the key and set the value of subkey and subkey. Subkey generation is also done at the same time. When setting up, it is also possible to set the Builtin attribute encryption and Expire time individually.
- K2HShm::AddAttr  
  Specify the key and add the attribute. Attribute name and attribute value are specified. Attributes can be added with arbitrary names and values, but please do not use the same name as Builtin attribute, Plugin attribute etc.
- K2HShm::Remove  
  Specify the key and delete only the value, subkey list, subkey only.
- K2HShm::ReplaceAll  
  Replace the key value, subkey list, and attributes.
- K2HShm::ReplaceValue  
  Replace the value of the key.
- K2HShm::ReplaceSubkeys  
  Replace the key's subkey list.
- K2HShm::ReplaceAttrs  
  Replace the attributes of the key.
- K2HShm::Rename  
  Rename the key to a key of another name. Values, subkey lists and attributes are not changed, only the key name can be changed.
- K2HShm::GetDAccessObj  
  Specify the key and mode (K2HDAccess::ACSMODE) to obtain the K2HDAccess object. For mode, specify either K2HDAccess::READ_ACCESS, K2HDAccess::WRITE_ACCESS, or K2HDAccess::RW_ACCESS.
- K2HShm::GetElementsByHash  
  Specify the start HASH value and time range, detect the first data that matches the specified range HASH value (masked start HASH value, number, maximum HASH value), tie it to the one masked HASH value (Key, value, subkey, attribute) as a binary string (there are multiple keys & values for one masked HASH value). After detection, it also returns the next start HASH value.
- K2HShm::SetElementByBinArray  
  Set one binary data (key, value, subkey, attribute) retrieved using GetElementsByHash() in the K2HASH database. The byptr member of one data (K2HBIN) structure of the PK2HBIN array obtained by GetElementsByHash is a BALLEDATA union and specifies a pointer to its member rawdata (that is, "&((reinterpret_cast <PBALLEDATA>(pbindatas[x].byptr))->rawdata) ".
- K2HShm::begin  
  Get key search start iterator K2HShm :: iterator.
- K2HShm::end  
  Find key Finish Iterator Get K2HShm :: iterator.
- K2HShm::GetQueueObj  
  Get the K2HQueue object for the currently open K2HASH file. Please release the returned object pointer.
- K2HShm::GetKeyQueueObj  
  Gets the K2HKeyQueue object for the currently open K2HASH file. Please release the returned object pointer.
- K2HShm::ReadQueue  
  Specify the Queue marker and read the value from the queue. Instead of using this method directly, please use the K2HQueue or K2HKeyQUeue class.
- K2HShm::PushFifoQueue  
  Specify the Queue marker and push the value to the FIFO queue. Instead of using this method directly, please use the K2HQueue or K2HKeyQUeue class.
- K2HShm::PushLifoQueue  
  Specify the Queue marker and push the value to the LIFO queue. Instead of using this method directly, please use the K2HQueue or K2HKeyQUeue class.
- K2HShm::PopQueue  
  Specify Queue's marker and pop values ​​from the queue. Instead of using this method directly, please use the K2HQueue or K2HKeyQUeue class.
- K2HShm::RemoveQueue  
  Specify the Queue marker and delete the specified number of values from the queue. Instead of using this method directly, please use the K2HQueue or K2HKeyQUeue class. (For the k2h_q_remove_trial_callback callback function and the operation when specifying the callback function, see the description of the k2h_q_remove_ext function.)
- K2HShm::Dump  
  Dump K2HASH data. You can specify the output destination (default is stderr) and the dump level. For the dump level, you can specify K2HShm::DUMP_HEAD, K2HShm::DUMP_KINDEX_ARRAY, K2HShm::DUMP_CKINDEX_ARRAY, K2HShm::DUMP_ELEMENT_LIST, K2HShm::DUMP_PAGE_LIST.
- K2HShm::PrintState  
  K2HASH Data status is output. You can specify the output destination (default is stderr).
- K2HShm::PrintAreaInfo  
  K2HASH Displays the information of the internal area of the file.
- K2HShm::GetState  
  K2HASH Returns a pointer to the structure PK2HSTATE which summarizes the data status, setting information, etc. The returned pointer must be released.
- K2HShm::EnableTransaction  
  Activate the transaction.
- K2HShm::DisableTransaction  
  Set transaction invalid.
- K2HShm::SetCommonAttribute  
  You can set the Builtin attribute update time (mtime), history function (history), Expire time, encrypted passphrase list, individual setting of encryption default value, and library loading of Plugin attribute. Please specify a valid pointer for the function used in the Builtin attribute. If you do not use it, specify NULL.
- K2HShm::AddAttrCryptPass  
  Load an additional passphrase for encryption. (The loaded passphrase is still loaded as it is.)
- K2HShm::AddAttrPluginLib  
  Load one library with Plugin attribute.
- K2HShm::CleanCommonAttribute  
  Setting of Builtin attribute (Individual) Discard the contents and set it to the initialization state (not using the Builtin attribute).
- K2HShm::GetAttrVersionInfos  
  Get the version information of the library of Attribute's Builtin attribute and Plugin attribute.
- K2HShm::GetAttrInfos  
  Gets the setting status of the current attribute.
- K2HShm::AreaCompress  
  Compresses the k2hash file. (Usually, you do not need to use it directly, please use the k2hcompress tool instead.)
- K2HShm::SetMsyncMode  
  Specify whether to execute mmap's sync appropriately or not. The default is enabled. (You do not usually need to use it.)

#### Method return value
See individual prototypes.  
The prototype of the return value of the bool value returns true if it succeeds. If it fails, it returns false.

#### Examples
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
