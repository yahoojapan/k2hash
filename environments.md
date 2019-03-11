---
layout: contents
language: en-us
title: Environments
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: environmentsja.html
lang_opp_word: To Japanese
prev_url: developer.html
prev_string: Developer
top_url: index.html
top_string: TOP
next_url: tools.html
next_string: Tools
---

# Environments
The K2HASH library uses following environment variables.
- K2HDBGMODE  
  Specify the level of output message.  
  The level value is silent, err, wan or info.
- K2HDBGFILE  
  Specify the file path which the output message puts.
- K2HATTR_MTIME  
  Specify ON, the updating timestamp attribute provided by built-in attribute is enabled.
- K2HATTR_HISTORY  
  Specify ON, the versioning(history) attribute provided by built-in attribute is enabled.
- K2HATTR_EXPIRE_SEC  
  Specify seconds, the expire time attribute provided by built-in attribute is enabled.
- K2HATTR_DEFENC  
  Specify ON, the function of encrypting attribute provided by built-in attribute is enabled.
- K2HATTR_ENCFILE  
  Specify the file path for encryption key when the function of encrypting attribute is enabled.
<br />
API(C/C++) as same as each environment takes precedence over this one.
