---
layout: contents
language: en-us
title: Tools
short_desc: K2HASH - NoSQL Key Value Store(KVS) library
lang_opp_file: toolsja.html
lang_opp_word: To Japanese
prev_url: tools.html
prev_string: Tools
top_url: index.html
top_string: TOP
next_url: 
next_string: 
---

# k2hlinetool

k2hlinetool is an interactive commandline tool for test purpose. You can run it non-interactively. Using k2hlinetool you can tune your program.

## k2hlinetool interface options

When invoking k2hlinetool, you may specify any of these options:

| Option | Description |
| --- | --- |
| -h | Print a description of all other command line options. |
| -f \<filename\> | Create *filename* by using memory mapped file. |
| -t \<filename\> | Create *filename* by using memory mapped file. This option is for temporary use only because *filename* is deleted after exit from k2hlinetool. |
| -m | Use heap memory for storage instead of file. |
| -mask \<bitness\> | Set *bitness* of initial K2HASH's MASK. The default value is 8 and the maximum value is 28. You should make this value large if you have many keys initially. This option is valid only when creating a new K2HASH file.|
| -cmask \<bitness\> | Set *bitness* of K2HASH's CMASK. The default value is 4 and recommended value is 4 or 5. This option is valid only when creating a new K2HASH file.|
| -elementcnt \<count\> | Set *count* as a maximum number of a K2HASH's element size that uses to handle hash collisions. K2HASH extends the hash table once the element is full. We recommend the size 1024 although the default size is 32. This option is valid only when creating a new K2HASH file. |
| -pagesize \<number\> | Set *number* as a pagesize of K2HASH. Each page contains data of a key, a value, a list of subkeys and reserved data for K2HASH of its own. The default pagesize is 128 bytes. You should set large size as you need. <br /> Note: 24 bytes for each page are reserved for K2HASH at least. You should tune the performance by using *k2hlinetool* if you may set smaller size than the default size. This option is valid only when creating a new K2HASH file. |
| -fullmap | Map the whole data K2HASH initializes into memory. |
| -ext \<filename\> | Load *filename* runtime library that defines external hash functions to be invoked by K2HASH. |
| -ro | Open a K2HASH file for read-only access |
| -init | Create a new K2HASH file |
| -lap | Print duration of each command execution |
| -capi | Use K2HASH C API instead of the C++ API | 
| -g \<loglevel\> | Set *loglevel* that k2hlinetool logs. Choose one out of ERR, WAN and INFO. Sending *SIGUSR1* to k2hlinetool rotates the levels. This option overrides the functionality of *K2HDBGMODE* environment variable. |
| -glog \<filepath\> | Set *filepath* to which k2hlinetool logs messages. By default, the file is stderr. This option overrides the functionality of *K2HDBGFILE* environment variable. |
| -his \<count\> | Set *count* to the maximum number of command history entries. By default, the number of history entries is 1000. <br /> Note: the history is only available in the current process. |
| -libversion | Print the K2HASH version and the credit. |
| -run \<filename\> | Execute commands in a *filename* sequentially. *filename* contains a command in each line. See the next section on commands. |

## The interpreter interface

When k2hlinetool is called with a tty with stdio, it prompts for commands. Use the "quit" command to exit from the prompt. Use the "help" command to see how to use it.

| Command | Description |
| --- | --- |
| help(h) | Print a description of all other commands. |
| quit(q)/exit | Exit. |
| info(i) [state] | Print the information on the K2HASH file. Use *state* to show the usage ratio. |
| dump(d) \<parameter\> | Print the specific information on the K2HASH file. Use "head" as *parameter* to show the header information. Use "kindex" as *parameter* to show the key index information. Use "ckindex" as *parameter* o show the collision key index information. Use "element" as *parameter* to show the element information. Use "full" as *parameter* to show the all information. |
| set(s) \<key\> \<value\> [rmsub] [pass=....] [expire=sec]| Set *key* with *value*. Use *rmsub* to remove all the subkeys with the key. With *rmsub*, K2HASH recursively removes the subkeys of the keys. Use *pass* to encrypt the value. Use *expire* to set expiration duration in units of seconds. |
| settrial(st) \<key\> [pass=....] | Print *key* with value if exists. You can update it. Otherwise, answer "n" alternatively. Use *pass* to encrypt the value. |
| setsub \<parentkey\> \<key\> \<value\> | Set *key* with *value* as a subkey of *parentkey*. |
| directset(dset) \<key\> \<value\> \<offset\>   | Set a subkey of *key* with *value*. Use *offset* where the *value* starts. The value will be filled with paddings by undefined data if *offset* position is larger than the value. |
| setf(sf) \<key\> \<offset\> \<file\> | Set *key* with values that are read from a file. Use *offset* where the value starts.  |
| fill(f) \<prefix\> \<value\> \<count\> | Create keys that starts from *prefix* with *value*. Use *count* to the number of keys. The value is same among all keys. |
| fillsub \<parentkey\> \<prefix\> \<val\> \<cnt\> | Create *parentkey*'s subkeys that starts from *prefix* with *value*. Use *count* to the number of keys. The value is same among all keys. |
| rm \<key\> [all] | Remove *key*. Use *all* to remove all the subkeys with the key. With *all*, K2HASH recursively removes the subkeys of the keys. |
| rmsub \<parentkey\> \<key\> | Remove *key* from *parentkey*. K2HASH recursively removes the subkeys of the *key*. |
| print(p) \<key\> [all] [noattrcheck] [pass=....] | Print the value of *key*. Use *all* to print value of subkeys. K2HASH recursively prints the subkeys of the keys. Use *noattrcheck* to skip decryption and to skip checking data expiration. Use *pass* to decrypt the value. |
| printattr(pa) \<key\> | Print the attribute information of *key*. |
| addattr(aa) \<key\> \<attrname\> \<attrvalue\> | Add an attribute of *attrname* with *attvalue* to *key*. |
| directprint(dp) \<key\> \<length\> \<offset\> | Print the *key* with value where starts from *offset* with *length*. |
| directsave(dsave) \<hash\> \<filepath\> | Write values that matches with *hash* to *filepath*. All keys matched with the *hash* will be saved even if their hash values are same. |
| directload(dload) \<filepath\> [unixtime] | Read data from *filepath*. Use *unixtime* to prevent from overwrite data that the key has newer timestamp than *filepath*. |
| copyfile(cf) \<key\> \<offset\> \<file\> | Create a *file* that contains *key* data starts from *offset*. |
| list(l) \<key\> | Print all keys. Use *key* to print the *key* value only. |
| stream(str) \<key\> < input \| output> | Set *key* to *output* and set value from *input* like C++ iostream class manipulation. See the following sample for reference. |
| history(his) | Print commands you used in the current interaction. |
| save \<filepath\> | Save commands you used in the current interaction to *filepath*. |
| load \<filepath\> | Execute commands in *filepath*. |
| trans(tr) \<on [filename [prefix [param]]] \| off\> [expire=sec] | Enable transaction logging to *filename* if disabled(or disable transaction logging if enabled). Set *prefix* to filter transaction log entry. Set *param* as a parameter string to pass to a transaction log handler. Set *expire* to set duration in units of seconds to be expired the transaction log data. |
| archive(ar) \<put \| load\> \<filename\> | Use *put* to create a new data *filename* from the current K2HASH file. Use *load* to load data from *filename* to the current K2HASH file. |
| queue(que) [prefix] empty | Check if a K2HASH queue is empty. Use *prefix* to set a queue name with a certain prefix. |
| queue(que) [prefix] count | Print the number of K2HASH queue. Use *prefix* to set a queue name with a certain prefix. |
| queue(que) [prefix] read \<fifo \| lifo\> \<pos\> [pass=...] | Print values in queue in *fifo* or *lifo* order. Use *prefix* to set a queue name with a certain prefix. Use *pos* to set the position of queue to start reading. Use *pass* to decrypt data. <br /> Note: *read* doesn't remove data from queue. |
| queue(que) [prefix] push \<fifo \| lifo\> \<value\> [pass=....] [expire=sec] | Add *value* to queue in *fifo* or *lifo*. Use *prefix* to set a queue name with a certain prefix. Use *pass* to encrypt data. Set *expire* to set expiration duration in units of seconds. |
| queue(que) [prefix] pop \<fifo \| lifo\> [pass=...] | Print values in queue in *fifo* or *lifo* order. *pop* removes data. Use *prefix* to set a queue name with a certain prefix. Use *pass* to decrypt data. |
| queue(que) [prefix] dump \<fifo \| lifo\> | Print values in queue in *fifo* or *lifo* order. |
| queue(que) [prefix] remove(rm) \<fifo \| lifo\> \<count\> [c] [pass=...] | Remove *count* of values in queue in *fifo* or *lifo* order. Use *prefix* to set a queue name with a certain prefix. Use *c* to prompt before every removal. Answer "y(yes)", "n(no)" and "b(break)". Use *pass* to decrypt data. |
| keyqueue(kque) [prefix] empty | Check if a K2HASH keyqueue is empty. Use *prefix* to set a queue name with a certain prefix. |
| keyqueue(kque) [prefix] count | Print the number of K2HASH keyqueue. Use *prefix* to set a queue name with a certain prefix. |
| keyqueue(kque) [prefix] read \<fifo \| lifo\> \<pos\> [pass=...] | Print values in queue in *fifo* or *lifo* order. Use *pass* to decrypt data. Use *prefix* to set a queue name with a certain prefix. Use *pos* to set the position of queue to start reading. <br /> Note: *read* doesn't remove data from queue. |
| keyqueue(kque) [prefix] push \<fifo \| lifo\> \<key\> \<value\> [pass=....] [expire=sec] | Add *key* and *value* to queue in *fifo* or *lifo*. Use *prefix* to set a queue name with a certain prefix. Use *pass* to encrypt data. Set *expire* to set expiration duration in units of seconds. |
| keyqueue(kque) [prefix] pop \<fifo \| lifo\> [pass=...] | Print values in queue in *fifo* or *lifo* order. *pop* removes data in keyqueue. Use *prefix* to set a queue name with a certain prefix. Use *pass* to decrypt data. |
| keyqueue(kque) [prefix] dump \<fifo \| lifo\> | Print values in queue in *fifo* or *lifo* order. Use *prefix* to set a queue name with a certain prefix. |
| keyqueue(kque) [prefix] remove(rm) \<fifo \| lifo\> \<count\> [c] [pass=...] | Remove data of *count* in queue in *fifo* or *lifo* order. Use *prefix* to set a queue name with a certain prefix. Use *c* to prompt before every removal. Answer "y(yes)", "n(no)" and "b(break)". Use *pass* to decrypt data. |
| builtinattr(ba) [mtime] [history] [expire=second] [enc] [pass=file path] | Set K2HASH builtin attributes. Set *mtime* to record value modification time. Set *history* to record key modification history. Set *expire* to set expiration duration in units of seconds. Set *enc* to enable encryption. Set *pass* to a file stores a pass phrase. |
| loadpluginattr(lpa) filepath | Load a *filepath* runtime library that handle attributes |
| -ext \<filename\> | Load the *library filename* runtime library that defines external HASH functions to be invoked by K2HASH. |
| addpassphrese(app) <pass> [default] | Add a *pass* to K2HASH builtin attributes. Set *default* to enable encryption with the *pass phrase*. |
| cleanallattr(caa) | Initialize attributes |
| shell |  Invoke a program defined by the SHELL environment. |
| echo \<string\>... | Print *string* .|
| sleep \<second\> | Sleep in *second* seconds. |


