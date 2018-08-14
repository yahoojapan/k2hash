---
layout: contents
language: en-us
title: Usage
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: usageja.html
lang_opp_word: To Japanese
prev_url: feature.html
prev_string: Feature
top_url: index.html
top_string: TOP
next_url: build.html
next_string: Build
---

# Usage
After building K2HASH, you can check the operation by the following procedure.

## 1. Building and installing

## 2. Run k2hlinetool
```
$ cd k2hash/tests
$ ./k2hlinetool -m
```
_Run k2hlinetool with memory container for K2HASH_

## 3. Storing value and print it manually  
You can see interactive prompt by k2hlinetool after run it.  
After that, you input k2hlinetool command to the prompt.

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

## 4. Other testing and exiting  
You can see interactive command list by "help" on prompt, and you can try to test those commands.  
If you want to quit k2hlinetool, you can input "quit" on prompt.
