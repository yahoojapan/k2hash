---
layout: contents
language: en-us
title: Feature
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: featureja.html
lang_opp_word: To Japanese
prev_url: home.html
prev_string: Overview
top_url: index.html
top_string: TOP
next_url: usage.html
next_string: Usage
---

# Feature

## Flexible installation
We provide suitable K2HASH installation for your OS. If you use Ubuntu, CentOS, Fedora or Debian, you can easily install it from [packagecloud.io](https://packagecloud.io/antpickax/stable). Even if you use none of them, you can use it by [Build](https://k2hash.antpick.ax/build.html) by yourself.

## Sub Key
The most characteristic of K2HASH is Sub-Key, K2HASH can not only store a value corresponding to a key, but also link another key as a child to the key.  
In other words, K2HASH can create a parent-child relationship between Key(the key) and Sub-Key(another key).  
By this function, K2HASH can provide simple operation for building hierarchical key tree.  
And some other features can be realized with this function.

## Data Container Type
k2HASH can create and use a file and memory as a data container.  
If it is used memory type, K2HASH can access and operate data with high performance.  
By using file type, k2HASH can store lots of key and large datas as a persisted data container.  
(If you select the mode to map the whole file to memory, K2HASH can access data with high performance.)

### Memory Mapping Type
When K2HASH stores and accesses to file type container, there is two type memory mapping for a file to memory.  
The first type maps the entire file to memory, the other maps only index area to memory.  
The first type is very fast to access data, the second type can have large data area.

## High Performance
K2HASH can access(read/write) data at high performance.  
k2HASH works faster even if the file is mapped only index area to memory.

## Support Binary Data
K2HASH allows binary data type for a key and a value, and those are allowed variable length.  
Except for the upper limit of the system, K2HASH has no limit on data length.  
In the future, it will not be affected by restrictions on the system as well.

## Automatically Container Size
K2HASH increases data container size automatically if it is needed.  
In other words, if the key count or total data size is increased, K2HASH adds entity index and data area automatically.  
Then you can create small K2HASH data at start, and scale out it automatically.

### Page Size For Data
K2HASH uses equally divided pages for data(key and value).  
Therefore if the data only increases, K2HASH is less susceptible to fragmentation.

## Multi-Processes and Multi-Threads
K2HASH library can be linked from the program which is for multi processes program and multi threads program.  
You can use it without being conscious of whether your program is multi processes or multi threads, and you call do only calling its API.  
K2HASH internally exclusively controls when accessing data, thus you can access data safely at anytime.

## Encrypting Value
K2HASH can encrypt a value for each key.  
There are two way of encryption keys used for encryption.  
One of way is common encryption keys for all data, the other way is each encryption key for each data.  
You can use common encryption keys when accessing data, and you can specify a different encryption key for API calls.

## Data Versioning
K2HASH can save old data(key-value) as history(versioning) at updating the data.  
You can use old datas as history.

## Attributes
K2HASH can set additional attribute value for each key like a value.  
There is two type for attribute, one of type is built-in attribute, the other is customized attribute which is provided by user's shared library.

### Built-in Attributes
- mtime  
MTIME attribute is timestamp at updating the data.
- expire  
This attribute is expire time for the data.  
You can not access the data after the expiration date has passed.  
For example, if you use this attribute for your cached data, the data is expired automatically.
- encryption  
This attribute is set MD5 value of encryption key to the data if the data is encrypted.

If you need to use customized attributes, you can make the shared library which implements K2HASH attribute's I/F, and you can register it to K2HASH library at initializing.

## Transaction
You can do your own processing at updating the data(key-value and etc), this processing can be implemented as K2HASH transaction plugin.  
The K2HASH transaction plugin is interface by shared library, this runs in process space of your program which is linked K2HASH library.  
The K2HASH transaction plugin can work as synchronous or asynchronous.

K2HASH has a built-in transaction processing and can work this processing if there is not additional plugins.  
You can use this built-in transaction processing for the function of archiving data.

In addition, K2HASH products provide [K2HTPDTOR](https://k2htpdtor.antpick.ax/) which is one of standard K2HASH transaction plugin.  
K2HTPDTOR is primarily used to transfer transaction data to other servers by [CHMPX](https://chmpx.antpick.ax/) and make copies of K2HASH data on the other server.

## Archive
K2HASH has built-in transaction plugin, this plugin puts archiving data which is transaction data to a file.  
For example, you can save archiving data as a file, and you can load it to another K2HASH container to restore.

## Queue
K2HASH provides FIFO/LIFO Queue derived from Sub-Key function.  
The element in Queue is set value or key-value.  
You can push value(or key-value) as a element to Queue, and pop it by FILE or LIFO.

## Tools
K2HASH provides some command line tools for utility and debugging.  
You can use these tools for operating and debugging datas in K2HASH.
