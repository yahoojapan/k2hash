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

k2hlinetool is a tool for testing purpose. k2hlinetool helps you to tune your program. k2hlinetool is an interactive commandline tool although you can run it non-interactively.

## k2hlinetool interface options

k2hlinetool is an interactive commandline tool. When invoking k2hlinetool, you may specify any of these options:

| Option | Description |
| --- | --- |
| -h | Print a description of all other command line options. |
| -f <filename> | Create *filename* by using memory mapped file |
| -t <filename> | Create *filename* by using memory mapped file. This option is for temporary use only.  The file is deleted when exiting |
| -m | Use heap memory only |
| -mask <bit count> | Set the bitness of initial k2hash's MASK. The default value is 8. The maximum value is 28. You should make this value bigger if you have many keys initially. This option is only valid when creating a new k2hash file.|
| -cmask <bit count> | Set the bitness of k2hash's CMASK. The default value is 4. We recommend this value 4 or 5. This option is only valid when creating a new k2hash file.|
| -elementcnt <count> | Set the maximum number of a k2hash's element size that uses to handle hash collisions. k2hash extends the hash table once the element is full. We recommend the size 1024 although the default size is 32. This option is only valid when creating a new k2hash file. |
| -pagesize <number> | Set the page size of k2hash. The page contains data of a key, a value, a list of subkeys and reserved data for k2hash of its own. The default page size is 128 bytes. You should set larger size as you need. Note: 24 bytes for each page are reserved for k2hash at least. You should tune the performance by using k2hlinetool if you set smaller size than the default size. This option is only valid when creating a new k2hash file. |
| -fullmap | Map the whole memory area that k2hash initializes. |
| -ext <library filename> | Load the *library filename* runtime library that defines external HASH functions to be invoked by K2HASH. |
| -ro | Open a k2hash file for read-only access |
| -init | Create a new k2hash file |
| -lap | Print the duration of each command execution |
| -capi | Use the K2HASH C API instead of the C++ API | 
| -g <debug level> | Set log level that k2hlinetool logs. Choose one out of ERR, WAN and INFO. Sending *SIGUSR1* to k2hlinetool rotates the level. The functionality of *K2HDBGMODE* environment variable is override by this option. |
| -glog <file path> | Set the file to which k2hlinetool logs messages. By default, the file is stderr. The functionality of *K2HDBGFILE* environment variable is override by this option. |
| -his <count> | Set the maximum number of history entries. By default, the number of history entries is 1000. Note: the history is only available in the current process. |
| -libversion | Print the k2hash version and the credit. |
| -run <filename> | Run k2hlinetool in non-interactive mode. Execute commands in a *filename* sequentially. A command exists in a line. See the next section on valid commands.|

## The interpreter interface

When k2hlinetool is called with a tty with stdio, it prompts for commands. Use the "help" command to see how to use it. Use the "quit" command to exit from the prompt.

| help(h) | Print a description of all other commands. |
| quit(q)/exit | Exit |
| info(i) [state] | Print the information on the k2hash file. Use *state* to show the usage raito |
| dump(d) <parameter> | Print the spesific information on the k2hash file. Use *head* to show the header information. Use "kindex" to show the key index informaton. Use *ckindex* to show the collision key index information. Use *element* to show the element information. Use *full* to show the all information |
| set(s) <key> <value> [rmsub] [pass=....] [expire=sec]| | Set a key with a value. Use *rmsub* to remove all the subkeys with the key. With *rmsub*, K2HASH recursively removes the subkeys of the keys. Use *pass* to encrypt the value. Use *expire* to set the duration to be deleted the value |
| settrial(st) <key> [pass=....] | Print data of a *key* if exists and update the value. Otherwise, you should answer "n" alternatively. Use *pass* to encrpyt the value. |
| setsub <parent key> <key> <value> | Set a subkey of a key with a value |
| directset(dset) <key> <value> <offset>   | Set a subkey of a key with a value. Use *offset* to start the value position. The value will be filled with paddings by undefined data if offset position is larger than the value |
| setf(sf) <key> <offset> <file> | Set a key with a value that are read from a file |
| fill(f) <prefix> <value> <count> | Set *prefix* with a key with a value. Use *count* to the number of keys. The value is same among all keys |
| fillsub <parent> <prefix> <val> <cnt> | Set *prefix*-ed parent key with a value. Use *count* to the number of keys. The value is same among all keys |
| rm <key> [all] | Remove a *key* Use *all* to remove all the subkeys with the key. With *rmsub*, K2HASH recursively removes the subkeys of the keys |
| rmsub <parent key> <key> | Remove a *key* from *parent key* K2HASH recursively removes the subkeys of the *key* |
| print(p) <key> [all] [noattrcheck] [pass=....] | Print the value of a *key*. Use *all* to print value of subkeys. K2HASH recursively prints the subkeys of the keys. Use *pass* to decrpyt the data. Use *noattrcheck* to skip decrypiton and to skip checking data expiration |
| printattr(pa) <key> | Print the attribute information of a *key* |
| addattr(aa) <key> <attrname> <attrvalue> | Add an attribute of *attrname* with *attvalue* to *key* |
| directprint(dp) <key> <length> <offset> | Print the *key*'s value where starts from *offset* with *length* |
| directsave(dsave) <start hash> <filepath> | Write values that matches with *start hash* to *filepath*. All keys matched with the *start hash* will be saved even if their hash values are same.
| directload(dload) <file path> [unixtime] | Read data from *filepath*. Use *unixtime* to prevent from overwrite data that the key has newer timestamp than *filepath* |
| copyfile(cf) <key> <offset> <file> | Create a *file* that contains *key* data starts from *offset* with *length* |
| list(l) <key> | Prints all keys. Use *key* to print it |
| stream(str) <key> < input | output> | Set *key* to *output* and set value from *input* like C++ iostream class manipulation. See the following sample for reference |
| history(his) | Prints commands in the current interaction |
| save <filepath> | Save commands in the current interaction to *filepath* |
| load <filepath> | Execute commands in *filepath* |
| trans(tr) <on [filename [prefix [param]]] | off> [expire=sec] | Enable a transaction logging if disabled(or disable a transaction logging if enabled). Set *expire* to set duration in units of seconds to be expired the transaction logging data |
| archive(ar) <put | load> <filename> | Use *put* to create a new data *filename* from the current k2hash file. Use *load* to load data from *filename* to the current k2hash file |
| queue(que) [prefix] empty | Check if a K2HASH queue is empty |
| queue(que) [prefix] count | Print the number of K2HASH queue |
| queue(que) [prefix] read <fifo | lifo> <pos> [pass=...] | Print data with prefix in queue in *fifo* or *lifo* order. *read* doesn't remove data. Use *pass* to decrypt data |
| queue(que) [prefix] push <fifo | lifo> <value> [pass=....] [expire=sec] | Add *value* to *prefix-ed* queue in *fifo* or *lifo*. Use *pass* to encrypt data. Set *expire* to set expiration duration in units of seconds |
| queue(que) [prefix] pop <fifo | lifo> [pass=...] | Print data with *prefix* in queue in *fifo* or *lifo* order. *pop* removes data. Use *pass* to decrypt data |
| queue(que) [prefix] dump <fifo | lifo> | Print data with *prefix* in queue in *fifo* or *lifo* order |
| queue(que) [prefix] remove(rm) <fifo | lifo> <count> [c] [pass=...] | Remove data of *count* in queue in *fifo* or *lifo* order. Use *prefix* to set a queue name with a certain prefix. Use *c* to prompt before every removal. Answer "y(yes)", "n(no)" and "b(break)". Use *pass* to decrypt data. |
| keyqueue(kque) [prefix] empty | K2HASHでサポートするキューが空であるか確認します。|
| keyqueue(kque) [prefix] count | K2HASHでサポートするキューに蓄積されているデータ数を返します。|
| keyqueue(kque) [prefix] read <fifo | lifo> <pos> [pass=...] | K2HASHでサポートするキューからデータ（keyとvalue）をコピーます。prefixはキューにて使用されるキー名のプレフィック スです。FIFO/LIFOを指定してください。キューが暗号化されている場合にはパスフレーズを指定してください。|
| keyqueue(kque) [prefix] push <fifo | lifo> <key> <value> [pass=....] [expire=sec] | K2HASHでサポートするキューへデータ（key/value）を蓄積（プッシュ）します。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。指定されたkey/valueはキューだけではなく、K2HASHファイルにkey/valueとして書き込まれます。キューを暗号化する場合はパスフレーズを指定してください。またExpire時間を指定する場合には、expireを秒で指定してください。|
| keyqueue(kque) [prefix] pop <fifo | lifo> [pass=...] | K2HASHでサポートするキューからデータ（keyとvalue）を取り出し（ポップ）ます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。取り出されたkey/valueはK2HASHファイルからも削除されます。キューが暗号化されている場合にはパスフレーズを指定してください。|
| keyqueue(kque) [prefix] dump <fifo | lifo> | K2HASHでサポートするキューをダンプします。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。|
| keyqueue(kque) [prefix] remove(rm) <fifo | lifo> <count> [c] [pass=...] | K2HASHでサポートするキューから指定数（count）分のデータを削除ます。prefixはキューにて使用されるキー名のプレフィックスです。FIFO/LIFOを指定してください。削除されたデータは表示されません。キューから削除されるkey/valueはK2HASHファイルからも削除されます。最後のパラメータ"c（confirm）"をつけた場合には、削除する前に対話形式で、確認がなされます。対話形式には"y(yes)"、"n(no)"、"b(break)"で回答してください。キューが暗号化されている場合にはパスフレーズを指定してください。|
| builtinattr(ba) [mtime] [history] [expire=second] [enc] [pass=file path] | Set K2hash builtin attributes. Set *expire* to set duration in units of seconds to be deleted the value. Set *pass* to a file stores a pass phrase. Set *enc* to enable encryption |
| loadpluginattr(lpa) filepath | Load a *filepath* runtime library that handle attributes |
| -ext <library filename> | Load the *library filename* runtime library that defines external HASH functions to be invoked by K2HASH. |
| addpassphrese(app) <pass phrase> [default] | Add a *pass phrase* to K2hash builtin attributes. Set *default* to enable encryption with the *pass phrase* |
| cleanallattr(caa) | Initialize attributes |
| shell |  Invoke a program defined by the SHELL environment |
| echo <string>... | Print *string* |
| sleep <second> | Sleep in *second* seconds |


