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
After building **K2HASH**, you can check the operation by the following procedure.

## 1. Creating a usage environment
There are two ways to install **K2HASH** in your environment.  
One is to download and install the package of **K2HASH** from [packagecloud.io](https://packagecloud.io/).  
The other way is to build and install **K2HASH** from source code yourself.  
These methods are described below.  

### Installing packages
The **K2HASH** publishes [packages](https://packagecloud.io/app/antpickax/stable/search?q=k2hash) in [packagecloud.io - AntPickax stable repository](https://packagecloud.io/antpickax/stable) so that anyone can use it.  
The package of the **K2HASH** is released in the form of Debian package, RPM package.  
Since the installation method differs depending on your OS, please check the following procedure and install it.  

#### For Debian-based Linux distributions users, follow the steps below:
```
$ sudo apt-get update -y
$ sudo apt-get install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh | sudo bash
$ sudo apt-get install k2hash
```
To install the developer package, please install the following package.
```
$ sudo apt-get install k2hash-dev
```

#### For RPM-based Linux distributions users, follow the steps below:
```
$ sudo dnf makecache
$ sudo dnf install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.rpm.sh | sudo bash
$ sudo dnf install k2hash
```
To install the developer package, please install the following package.
```
$ sudo dnf install k2hash-devel
```

#### For ALPINE-based Linux distributions users, follow the steps below:
```
# apk update
# apk add curl
# curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.alpine.sh | sh
# apk add k2hash
```
To install the developer package, please install the following package.
```
# apk add k2hash-dev
```

#### Other OS
If you are not using the above OS, packages are not prepared and can not be installed directly.  
In this case, build from the [source code](https://github.com/yahoojapan/k2hash) described below and install it.

### Build and install from source code
For details on how to build and install **K2HASH** from [source code](https://github.com/yahoojapan/k2hash), please see [Build](https://k2hash.antpick.ax/build.html).

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

## 4. Other testing and exiting  
You can see interactive command list by "help" on prompt, and you can try to test those commands.  
If you want to quit k2hlinetool, you can input "quit" on prompt.
