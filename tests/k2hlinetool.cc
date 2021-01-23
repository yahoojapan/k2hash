/*
 * K2HASH
 *
 * Copyright 2013 Yahoo Japan Corporation.
 *
 * K2HASH is key-valuew store base libraries.
 * K2HASH is made for the purpose of the construction of
 * original KVS system and the offer of the library.
 * The characteristic is this KVS library which Key can
 * layer. And can support multi-processing and multi-thread,
 * and is provided safely as available KVS.
 *
 * For the full copyright and license information, please view
 * the license file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Fri Nor 31 2013
 * REVISION:
 *
 */

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdarg.h>
#include <errno.h>
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <libgen.h>

#include <k2hash.h>
#include <k2hcommon.h>
#include <k2hshm.h>
#include <k2hdbg.h>
#include <k2hashfunc.h>
#include <k2htransfunc.h>
#include <k2htrans.h>
#include <k2harchive.h>
#include <k2hstream.h>
#include <k2hqueue.h>

using namespace std;

//---------------------------------------------------------
// Macros
//---------------------------------------------------------
#if	0
#define	PRN(...)		fprintf(stdout, __VA_ARGS__); \
						fprintf(stdout, "\n");

#define ERR(...)		fprintf(stderr, "[ERR] "); \
						PRN(__VA_ARGS__);
#else

static inline void PRN(const char* format, ...)
{
	if(format){
		va_list ap;
		va_start(ap, format);
		vfprintf(stdout, format, ap);
		va_end(ap);
	}
	fprintf(stdout, "\n");
	fflush(stdout);
}

static inline void ERR(const char* format, ...)
{
	fprintf(stderr, "[ERR] ");
	if(format){
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
}

#endif

//---------------------------------------------------------
// Global(static)
//---------------------------------------------------------
static bool isModeCAPI = false;

//---------------------------------------------------------
// Typedefs
//---------------------------------------------------------
typedef std::vector<std::string> strlist_t;
typedef std::vector<std::string> params_t;
typedef std::map<std::string, params_t> option_t;

typedef struct option_type{
	const char*	option;
	const char*	norm_option;
	int			min_param_cnt;
	int			max_param_cnt;
}OPTTYPE, *POPTTYPE;

typedef	const struct option_type	*CPOPTTYPE;
typedef	const char					*const_pchar;

//---------------------------------------------------------
// Class LapTime
//---------------------------------------------------------
class LapTime
{
	private:
		static bool	isEnable;

	private:
		static bool Set(bool enable);

		struct timeval	start;

	public:
		static bool Toggle(void) { return Set(!LapTime::isEnable); }
		static bool Enable(void) { return Set(true); }
		static bool Disable(void) { return Set(false); }

		LapTime();
		virtual ~LapTime();
};

bool LapTime::isEnable = false;

bool LapTime::Set(bool enable)
{
	bool	old = LapTime::isEnable;
	LapTime::isEnable = enable;
	return old;
}

LapTime::LapTime()
{
	memset(&start, 0, sizeof(struct timeval));
	gettimeofday(&start, NULL);
}

LapTime::~LapTime()
{
	if(LapTime::isEnable){
		struct timeval	end;
		struct timeval	lap;

		memset(&end, 0, sizeof(struct timeval));
		gettimeofday(&end, NULL);

		memset(&lap, 0, sizeof(struct timeval));
		timersub(&end, &start, &lap);

		time_t	hour, min, sec, msec, usec;

		sec	 = lap.tv_sec % 60;
		min	 = (lap.tv_sec / 60) % 60;
		hour = (lap.tv_sec / 60) / 60;
		msec = lap.tv_usec / 1000;
		usec = lap.tv_usec % 1000;

		PRN(NULL);
		PRN("Lap time: %jdh %jdm %jds %jdms %jdus(%jds %jdus)\n",
			static_cast<intmax_t>(hour), static_cast<intmax_t>(min), static_cast<intmax_t>(sec),static_cast<intmax_t>(msec), static_cast<intmax_t>(usec),
			static_cast<intmax_t>(lap.tv_sec), static_cast<intmax_t>(lap.tv_usec));
	}
}

//---------------------------------------------------------
// Class ConsoleInput
//---------------------------------------------------------
class ConsoleInput
{
	protected:
		static const int	DEFAULT_HISTORY_MAX	= 500;

		size_t			history_max;
		string			prompt;
		strarr_t		history;
		ssize_t			history_pos;
		string			input;
		size_t			input_pos;	// == cursor pos
		struct termios	tty_backup;
		bool			is_set_terminal;
		int				last_errno;

	protected:
		bool SetTerminal(void);
		bool UnsetTerminal(void);
		bool ReadByte(char& cInput);
		void ClearInput(void);
		void ClearLine(void);

	public:
		size_t SetMax(size_t max);
		bool SetPrompt(const char* pprompt);

		ConsoleInput();
		virtual ~ConsoleInput();

		bool Clean(void);
		bool GetCommand(void);
		bool PutHistory(const char* pCommand);
		bool RemoveLastHistory(void);
		int LastErrno(void) const { return last_errno; }
		const string& str(void) const { return input; }
		const char* c_str(void) const { return input.c_str(); }
		const strarr_t& GetAllHistory(void) const { return history; }
};

//---------------------------------------------------------
// Class ConsoleInput::Methods
//---------------------------------------------------------
ConsoleInput::ConsoleInput() : history_max(DEFAULT_HISTORY_MAX), prompt("PROMPT> "), history_pos(-1L), input(""), input_pos(0UL), is_set_terminal(false), last_errno(0)
{
}

ConsoleInput::~ConsoleInput()
{
	UnsetTerminal();
	Clean();
}

bool ConsoleInput::Clean(void)
{
	history.clear();
	prompt.clear();
	input.clear();
	return true;
}

size_t ConsoleInput::SetMax(size_t max)
{
	size_t	old = history_max;
	if(0 != max){
		history_max = max;
	}
	return old;
}

bool ConsoleInput::SetPrompt(const char* pprompt)
{
	if(ISEMPTYSTR(pprompt)){
		return false;
	}
	prompt = pprompt;
	return true;
}

bool ConsoleInput::SetTerminal(void)
{
	if(is_set_terminal){
		// already set
		return true;
	}

	struct termios tty_change;

	// backup
	tcgetattr(0, &tty_backup);
	tty_change				= tty_backup;
	tty_change.c_lflag		&= ~(ECHO | ICANON);
	tty_change.c_cc[VMIN]	= 0;
	tty_change.c_cc[VTIME]	= 1;

	// set
	tcsetattr(0, TCSAFLUSH, &tty_change);
	is_set_terminal = true;

	return true;
}

bool ConsoleInput::UnsetTerminal(void)
{
	if(!is_set_terminal){
		// already unset
		return true;
	}

	// unset
	tcsetattr(0, TCSAFLUSH, &tty_backup);
	is_set_terminal = false;

	return true;
}

//
// If error occurred, return 0x00
//
bool ConsoleInput::ReadByte(char& cInput)
{
	cInput = '\0';
	if(-1 == read(0, &cInput, sizeof(char))){
		last_errno = errno;
		return false;
	}
	last_errno = 0;
	return true;
}

void ConsoleInput::ClearInput(void)
{
	history_pos	= -1L;
	input_pos	= 0UL;
	last_errno	= 0;
	input.erase();
}

void ConsoleInput::ClearLine(void)
{
	for(size_t Count = 0; Count < input_pos; Count++){		// cursor to head
		putchar('\x08');
	}
	for(size_t Count = 0; Count < input.length(); Count++){	// clear by space
		putchar(' ');
	}
	for(size_t Count = 0; Count < input.length(); Count++){	// rewind cursor to head
		putchar('\x08');
	}
	fflush(stdout);
}

//
// [Input key value]
//	0x1b 0x5b 0x41			Up
//	0x1b 0x5b 0x42			Down
//	0x1b 0x5b 0x43			Right
//	0x1b 0x5b 0x44			Left
//	0x7f					Delete
//	0x08					backSpace
//	0x01					CTRL-A
//	0x05					CTRL-E
//	0x1b 0x5b 0x31 0x7e		HOME
//	0x1b 0x5b 0x34 0x7e		END
//
bool ConsoleInput::GetCommand(void)
{
	ClearInput();
	SetTerminal();

	// prompt
	printf("%s", ConsoleInput::prompt.c_str());
	fflush(stdout);

	char	input_char;
	while(true){
		// read one character
		if(!ReadByte(input_char)){
			if(EINTR == last_errno){
				last_errno = 0;
				continue;
			}
			break;
		}
		if('\n' == input_char){
			// finish input one line
			putchar('\n');
			fflush(stdout);
			PutHistory(input.c_str());
			break;

		}else if('\x1b' == input_char){
			// escape character --> next byte read
			if(!ReadByte(input_char)){
				break;
			}
			if('\x5b' == input_char){
				// read more character
				if(!ReadByte(input_char)){
					break;
				}
				if('\x41' == input_char){
					// Up key
					if(0 != history_pos && 0 < history.size()){
						ClearLine();	// line clear

						if(-1L == history_pos){
							history_pos = static_cast<ssize_t>(history.size() - 1UL);
						}else if(0 != history_pos){
							history_pos--;
						}
						input = history[history_pos];

						for(input_pos = 0UL; input_pos < input.length(); input_pos++){
							putchar(input[input_pos]);
						}
						fflush(stdout);
					}

				}else if('\x42' == input_char){
					// Down key
					if(-1L != history_pos && static_cast<size_t>(history_pos) < history.size()){
						ClearLine();	// line clear

						if(history.size() <= static_cast<size_t>(history_pos) + 1UL){
							history_pos = -1L;
							input.erase();
							input_pos = 0UL;
						}else{
							history_pos++;
							input = history[history_pos];
							input_pos = input.length();

							for(input_pos = 0UL; input_pos < input.length(); input_pos++){
								putchar(input[input_pos]);
							}
							fflush(stdout);
						}
					}

				}else if('\x43' == input_char){
					// Right key
					if(input_pos < input.length()){
						putchar(input[input_pos]);
						fflush(stdout);
						input_pos++;
					}

				}else if('\x44' == input_char){
					// Left key
					if(0 < input_pos){
						input_pos--;
						putchar('\x08');
						fflush(stdout);
					}

				}else if('\x31' == input_char){
					// read more character
					if(!ReadByte(input_char)){
						break;
					}
					if('\x7e' == input_char){
						// Home key
						for(size_t Count = 0; Count < input_pos; Count++){
							putchar('\x08');
						}
						input_pos = 0UL;
						fflush(stdout);
					}

				}else if('\x34' == input_char){
					// read more character
					if(!ReadByte(input_char)){
						break;
					}
					if('\x7e' == input_char){
						// End key
						for(size_t Count = input_pos; Count < input.length(); Count++){
							putchar(input[Count]);
						}
						input_pos = input.length();
						fflush(stdout);
					}

				}else if('\x33' == input_char){
					// read more character
					if(!ReadByte(input_char)){
						break;
					}
					if('\x7e' == input_char){
						// BackSpace key on OSX
						if(0 < input_pos){
							input.erase((input_pos - 1), 1);
							input_pos--;
							putchar('\x08');
							for(size_t Count = input_pos; Count < input.length(); Count++){
								putchar(input[Count]);
							}
							putchar(' ');
							for(size_t Count = input_pos; Count < input.length(); Count++){
								putchar('\x08');
							}
							putchar('\x08');
							fflush(stdout);
						}
					}
				}
			}

		}else if('\x7f' == input_char){
			// Delete
			if(0 < input.length()){
				input.erase(input_pos, 1);

				for(size_t Count = input_pos; Count < input.length(); Count++){
					putchar(input[Count]);
				}
				putchar(' ');
				for(size_t Count = input_pos; Count < input.length(); Count++){
					putchar('\x08');
				}
				putchar('\x08');
				fflush(stdout);
			}

		}else if('\x08' == input_char){
			// BackSpace
			if(0 < input_pos){
				input.erase((input_pos - 1), 1);
				input_pos--;
				putchar('\x08');
				for(size_t Count = input_pos; Count < input.length(); Count++){
					putchar(input[Count]);
				}
				putchar(' ');
				for(size_t Count = input_pos; Count < input.length(); Count++){
					putchar('\x08');
				}
				putchar('\x08');
				fflush(stdout);
			}

		}else if('\x01' == input_char){
			// ctrl-A
			for(size_t Count = 0; Count < input_pos; Count++){
				putchar('\x08');
			}
			input_pos = 0;
			fflush(stdout);

		}else if('\x05' == input_char){
			// ctrl-E
			for(size_t Count = input_pos; Count < input.length(); Count++){
				putchar(input[Count]);
			}
			input_pos = input.length();
			fflush(stdout);

		}else if(isprint(input_char)){
			// normal character
			input.insert(input_pos, 1, input_char);
			for(size_t Count = input_pos; Count < input.length(); Count++){
				putchar(input[Count]);
			}
			input_pos++;
			for(size_t Count = input_pos; Count < input.length(); Count++){
				putchar('\x08');
			}
			fflush(stdout);
		}
	}
	UnsetTerminal();

	if(0 != last_errno){
		return false;
	}
	return true;
}

bool ConsoleInput::PutHistory(const char* pCommand)
{
	if(ISEMPTYSTR(pCommand)){
		return false;
	}
	history.push_back(string(pCommand));
	if(ConsoleInput::history_max < history.size()){
		history.erase(history.begin());
	}
	return true;
}

bool ConsoleInput::RemoveLastHistory(void)
{
	if(0 < history.size()){
		history.pop_back();
	}
	return true;
}

//---------------------------------------------------------
// Help
//---------------------------------------------------------
//
// -h                   help display
// -f <filename>        mode by filename for permanent hash file
// -t <filename>        mode by filename for temporary hash file
// -m                   mode for only memory
// -mask <bit count>    bit mask count for hash(*)
// -cmask <bit count>   collision bit mask count(*)
// -elementcnt <count>  element count for each hash table(*)
// -pagesize <number>   pagesize for each data
// -fullmap             full mapping
// -ext <library path>  extension library for hash function
// -ro                  read only mode(only permanent hash file)
// -init                only initialize permanent hash file
// -lap                 print lap time after line command
// -capi                use C API for calling internal library
// -g <debug level>     debugging mode: ERR(default) / WAN / INFO
// -glog <file path>    output file for debugging message(default stderr)
// -his <count>         set history count(default 500)
// -libversion          display k2hash library version
// -run <file path>     run command(history) file.
//
static void Help(const char* progname)
{
	PRN(NULL);
	PRN("Usage: %s [options] [ -f filename | -t filename | -m | -h ]", progname ? progname : "program");
	PRN(NULL);
	PRN("Option -h                   help display");
	PRN("       -f <filename>        mode by filename for permanent hash file");
	PRN("       -t <filename>        mode by filename for temporary hash file");
	PRN("       -m                   mode for only memory");
	PRN("       -mask <bit count>    bit mask count for hash(*1)");
	PRN("       -cmask <bit count>   collision bit mask count(*1)");
	PRN("       -elementcnt <count>  element count for each hash table(*1)");
	PRN("       -pagesize <number>   pagesize for each data");
	PRN("       -fullmap             full mapping");
	PRN("       -ext <library path>  extension library for hash function");
	PRN("       -trlib <lib path>    extension transaction library");
	PRN("       -ro                  read only mode(only permanent hash file)");
	PRN("       -init                only initialize permanent hash file");
	PRN("       -lap                 print lap time after line command");
	PRN("       -capi                use C API for calling internal library");
	PRN("       -g <debug level>     debugging mode: ERR(default) / WAN / INFO(*2)");
	PRN("       -glog <file path>    output file for debugging message(default stderr)(*3)");
	PRN("       -his <count>         set history count(default 500)");
	PRN("       -libversion          display k2hash library version");
	PRN("       -run <file path>     run command(history) file.");
	PRN(NULL);
	PRN("(*1)These option(value) is for debugging about extending hash area.");
	PRN("    Usually, you don\'t need to specify these.");
	PRN("(*2)You can set debug level by another way which is setting environment as \"K2HDBGMODE\".");
	PRN("    \"K2HDBGMODE\" environment is took as \"SILENT\", \"ERR\", \"WAN\" or \"INFO\" value.");
	PRN("    When this process gets SIGUSR1 signal, the debug level is bumpup.");
	PRN("    The debug level is changed as \"SILENT\"->\"ERR\"->\"WAN\"->\"INFO\"->...");
	PRN("(*3)You can set debugging message log file by the environment. \"K2HDBGFILE\".");
	PRN("(*4)For setting builtin attribute, you can specify following environments.");
	PRN("    \"K2HATTR_MTIME\", \"K2HATTR_HISTORY\", \"K2HATTR_EXPIRE_SEC\", \"K2HATTR_DEFENC\", \"K2HATTR_ENCFILE\"");
	PRN(NULL);
}

//
// Command: [command] [parameters...]
//
// help(h)                                  					print help
// quit(q)/exit                             					quit
// info(i) [state]                          					print k2hash file/memory information and with state
// dump(d) <parameter>                      					dump k2hash, parameter: head(default) / kindex / ckindex / element / full
// set(s) <key> <value> [rmsub] [pass=....] [expire=sec]       set key-value, if rmsub is specified, remove all subkey under key. if value is "null", it means no value.
// settrial(st) <key>											set key-value if key is not existed.
// setsub <parent key> <key> <value>        					set key-value under parent key. if value is "null", it means no value.
// directset(dset) <key> <value> <offset>   					set value from offset directly.
// setf(sf) <key> <offset> <file>           					set directly key-value from file.
// fill(f) <prefix> <value> <count>         					set key-value by prefix repeating by count
// fillsub <parent> <prefix> <val> <cnt>    					set key-value under parent key by prefix repeating by count
// rm <key> [all]                           					remove key, if all parameter is specified, remove all sub key under key
// rmsub <parent key> <key>                 					remove key under parent key
// rename(ren) <key> <new key>                                  rename key to new key name
// print(p) <key> [all] [noattrcheck] [pass=....]               print value/subkeys by key, if all parameter is specified, print nesting sub keys
// printattr(pa) <key>                                          print attribute by key.
// addattr(aa) <key> <attr name> <attr value>                   add attribute to key.
// directprint(dp) <key> <length> <offset>  					print value from offset and length directly.
// copyfile(cf) <key> <offset> <file>       					output directly key-value to file.
// list(l) <key>                            					dump existed key list.
// stream(str) <key> <input | output>       					stream test by interactive.
// history(his)                             					display all history, you can use a command line in history by "!<number>".
// save <file path>                         					save history to file.
// load <file path>                         					load and run command file.
// trans(tr) <on [filename [prefix [param]]] | off>
//												[expire=sec] 	disable/enable transaction.
// archive(ar) <put | load> <filename>      					put/load archive(transaction) file.
// queue(que) [prefix] empty									check queue is empty
// queue(que) [prefix] count									get data count in queue
// queue(que) [prefix] read <fifo | lifo> <pos>	[pass=...]		read the value from queue at position
// queue(que) [prefix] push <fifo | lifo> <value> [pass=...]
// 												[expire=sec] 	push the value to queue(fifo/lifo)
// queue(que) [prefix] pop <fifo | lifo> [pass=...]				pop the value from queue
// queue(que) [prefix] dump <fifo | lifo>						dump queue
// queue(que) [prefix] remove(rm) <fifo | lifo> <count> [c]
// 													[pass=...] 	remove count of values in queue
// keyqueue(kque) [prefix] empty								check keyqueue is empty
// keyqueue(kque) [prefix] count								get data count in keyqueue
// keyqueue(kque) [prefix] read <fifo | lifo> <pos> [pass=...]	read the value from keyqueue at position
// keyqueue(kque) [prefix] push <fifo | lifo> <key> <value>
// 										[pass=...] [expire=sec]	push the key name to queue(fifo/lifo) and key-value into k2hash
// keyqueue(kque) [prefix] pop <fifo | lifo> [pass=...]			pop the key-value from queue and remove key-value from k2hash
// keyqueue(kque) [prefix] dump <fifo | lifo>					dump queue(as same as queue command)
// keyqueue(kque) [prefix] remove(rm) <fifo | lifo> <count> [c]
// 													[pass=...]	remove count of key-name in queue and remove those from k2hash
// builtinattr(ba) [mtime] [history] [expire=second]
//                                       [enc] [pass=file path] set builtin attribute
// loadpluginattr(lpa) filepath                                 load plugin attribute library.
// addpassphrase(app) <pass phrase> [default]                   add pass phrase for crypt into builtin attribute.
// cleanallattr(caa)                                            clear all attribute setting.
// shell														exit shell(same as "!" command).
// echo <string>...												echo string
// sleep <second>												sleep seconds
//
static void LineHelp(void)
{
	PRN(NULL);
	PRN("Command: [command] [parameters...]");
	PRN(NULL);
	PRN("help(h)                                                      print help");
	PRN("quit(q)/exit                                                 quit");
	PRN("info(i) [state]                                              print k2hash file/memory information and with state");
	PRN("dump(d) <parameter>                                          dump k2hash, parameter: head(default) / kindex / ckindex / element / full");
	PRN("set(s) <key> <value> [rmsub] [pass=....] [expire=sec]        set key-value, if rmsub is specified, remove all subkey under key. if value is \"null\", it means no value.");
	PRN("settrial(st) <key> [pass=....]                               set key-value if key is not existed.");
	PRN("setsub <parent key> <key> <value>                            set key-value under parent key. if value is \"null\", it means no value.");
	PRN("directset(dset) <key> <value> <offset>                       set value from offset directly.");
	PRN("setf(sf) <key> <offset> <file>                               set directly key-value from file.");
	PRN("fill(f) <prefix> <value> <count>                             set key-value by prefix repeating by count");
	PRN("fillsub <parent> <prefix> <val> <cnt>                        set key-value under parent key by prefix repeating by count");
	PRN("rm <key> [all]                                               remove key, if all parameter is specified, remove all sub key under key");
	PRN("rmsub <parent key> <key>                                     remove key under parent key");
	PRN("rename(ren) <key> <new key>                                  rename key to new key name");
	PRN("print(p) <key> [all] [noattrcheck] [pass=....]               print value/subkeys by key, if all parameter is specified, print nesting sub keys");
	PRN("printattr(pa) <key>                                          print attribute by key.");
	PRN("addattr(aa) <key> <attr name> <attr value>                   add attribute to key.");
	PRN("directprint(dp) <key> <length> <offset>                      print value from offset and length directly.");
	PRN("directsave(dsave) <start hash> <file path>                   save element binary data to file by hash value");
	PRN("directload(dload) <file path> [unixtime]                     load element by binary data from file");
	PRN("copyfile(cf) <key> <offset> <file>                           output directly key-value to file.");
	PRN("list(l) <key>                                                dump existed key list");
	PRN("stream(str) <key> < input | output>                          stream test by interactive.");
	PRN("history(his)                                                 display all history, you can use a command line in history by \"!<number>\".");
	PRN("save <file path>                                             save history to file.");
	PRN("load <file path>                                             load and run command file.");
	PRN("trans(tr) <on [filename [prefix [param]]] | off> [expire=sec]");
	PRN("                                                             disable/enable transaction.");
	PRN("threadpool(pool) [number]                                    set/display thread pool count for transaction, 0 means no thread pool.");
	PRN("archive(ar) <put | load> <filename>                          put/load archive(transaction) file.");
	PRN("queue(que) [prefix] empty                                    check queue is empty");
	PRN("queue(que) [prefix] count                                    get data count in queue");
	PRN("queue(que) [prefix] read <fifo | lifo> <pos> [pass=...]      read the value from queue at position");
	PRN("queue(que) [prefix] push <fifo | lifo> <value> [pass=....] [expire=sec]");
	PRN("                                                             push the value to queue(fifo/lifo)");
	PRN("queue(que) [prefix] pop <fifo | lifo> [pass=...]             pop the value from queue");
	PRN("queue(que) [prefix] dump <fifo | lifo>                       dump queue");
	PRN("queue(que) [prefix] remove(rm) <fifo | lifo> <count> [c] [pass=...]");
	PRN("                                                             remove count of values in queue");
	PRN("keyqueue(kque) [prefix] empty                                check keyqueue is empty");
	PRN("keyqueue(kque) [prefix] count                                get data count in keyqueue");
	PRN("keyqueue(kque) [prefix] read <fifo | lifo> <pos> [pass=...]  read the value from keyqueue at position");
	PRN("keyqueue(kque) [prefix] push <fifo | lifo> <key> <value> [pass=....] [expire=sec]");
	PRN("                                                             push the key name to queue(fifo/lifo) and key-value into k2hash");
	PRN("keyqueue(kque) [prefix] pop <fifo | lifo> [pass=...]         pop the key-value from queue and remove key-value from k2hash");
	PRN("keyqueue(kque) [prefix] dump <fifo | lifo>                   dump queue(as same as queue command)");
	PRN("keyqueue(kque) [prefix] remove(rm) <fifo | lifo> <count> [c] [pass=...]");
	PRN("                                                             remove count of key-name in queue and remove those from k2hash");
	PRN("builtinattr(ba) [mtime] [history] [expire=second] [enc] [pass=file path]");
	PRN("                                                             set builtin attribute.");
	PRN("loadpluginattr(lpa) filepath                                 load plugin attribute library.");
	PRN("addpassphrase(app) <pass phrase> [default]                   add pass phrase for crypt into builtin attribute.");
	PRN("cleanallattr(caa)                                            clear all attribute setting.");
	PRN("shell                                                        exit shell(same as \"!\" command).");
	PRN("echo <string>...                                             echo string");
	PRN("sleep <second>                                               sleep seconds");
	PRN(NULL);
}

//---------------------------------------------------------
// Parser
//---------------------------------------------------------
const OPTTYPE ExecOptionTypes[] = {
	{"-h",				"-h",				0,	0},
	{"-f",				"-f",				1,	1},
	{"-t",				"-t",				1,	1},
	{"-m",				"-m",				0,	0},
	{"-mask",			"-mask",			1,	1},
	{"-cmask",			"-cmask",			1,	1},
	{"-elementcnt",		"-elementcnt",		1,	1},
	{"-pagesize",		"-pagesize",		1,	1},
	{"-fullmap",		"-fullmap",			0,	0},
	{"-ext",			"-ext",				1,	1},
	{"-trlib",			"-trlib",			1,	1},
	{"-ro",				"-ro",				0,	0},
	{"-init",			"-init",			0,	0},
	{"-lap",			"-lap",				0,	0},
	{"-capi",			"-capi",			0,	0},
	{"-g",				"-g",				0,	1},
	{"-glog",			"-glog",			1,	1},
	{"-his",			"-his",				1,	1},
	{"-libversion",		"-libversion",		0,	0},
	{"-run",			"-run",				1,	1},
	{NULL,				NULL,				0,	0}
};

const OPTTYPE LineOptionTypes[] = {
	{"help",			"help",				0,	0},
	{"h",				"help",				0,	0},
	{"quit",			"quit",				0,	0},
	{"q",				"quit",				0,	0},
	{"exit",			"quit",				0,	0},
	{"info",			"info",				0,	1},
	{"i",				"info",				0,	1},
	{"dump",			"dump",				0,	1},
	{"d",				"dump",				0,	1},
	{"set",				"set",				2,	4},
	{"s",				"set",				2,	4},
	{"settrial",		"settrial",			1,	2},
	{"st",				"settrial",			1,	2},
	{"setsub",			"setsub",			3,	3},
	{"directset",		"directset",		3,	3},
	{"dset",			"directset",		3,	3},
	{"setf",			"setf",				3,	3},
	{"sf",				"setf",				3,	3},
	{"fill",			"fill",				3,	3},
	{"f",				"fill",				3,	3},
	{"fillsub",			"fillsub",			4,	4},
	{"rm",				"rm",				1,	2},
	{"rmsub",			"rmsub",			2,	2},
	{"rename",			"rename",			2,	2},
	{"ren",				"rename",			2,	2},
	{"print",			"print",			1,	3},
	{"p",				"print",			1,	3},
	{"printattr",		"printattr",		1,	1},
	{"pa",				"printattr",		1,	1},
	{"addattr",			"addattr",			2,	3},
	{"aa",				"addattr",			2,	3},
	{"directprint",		"directprint",		3,	3},
	{"dp",				"directprint",		3,	3},
	{"directsave",		"directsave",		2,	2},
	{"dsave",			"directsave",		2,	2},
	{"directload",		"directload",		1,	2},
	{"dload",			"directload",		1,	2},
	{"copyfile",		"copyfile",			3,	3},
	{"cf",				"copyfile",			3,	3},
	{"list",			"list",				0,	1},
	{"l",				"list",				0,	1},
	{"stream",			"stream",			2,	2},
	{"str",				"stream",			2,	2},
	{"history",			"history",			0,	0},
	{"his",				"history",			0,	0},
	{"save",			"save",				1,	1},
	{"load",			"load",				1,	1},
	{"trans",			"trans",			1,	5},
	{"tr",				"trans",			1,	5},
	{"threadpool",		"threadpool",		0,	1},
	{"pool",			"threadpool",		0,	1},
	{"archive",			"archive",			2,	2},
	{"ar",				"archive",			2,	2},
	{"shell",			"shell",			0,	0},
	{"queue",			"queue",			1,	6},
	{"que",				"queue",			1,	6},
	{"keyqueue",		"keyqueue",			1,	7},
	{"kque",			"keyqueue",			1,	7},
	{"builtinattr",		"builtinattr",		0,	6},
	{"ba",				"builtinattr",		0,	6},
	{"loadpluginattr",	"loadpluginattr",	1,	1},
	{"lpa",				"loadpluginattr",	1,	1},
	{"addpassphrase",	"addpassphrase",	1,	2},
	{"app",				"addpassphrase",	1,	2},
	{"cleanallattr",	"cleanallattr",		0,	0},
	{"caa",				"cleanallattr",		0,	0},
	{"sh",				"shell",			0,	0},
	{"echo",			"echo",				1,	9999},
	{"sleep",			"sleep",			1,	1},
	{NULL,				NULL,				0,	0}
};

inline void CleanOptionMap(option_t& opts)
{
	for(option_t::iterator iter = opts.begin(); iter != opts.end(); opts.erase(iter++)){
		iter->second.clear();
	}
}

static bool BaseOptionParser(strlist_t& args, CPOPTTYPE pTypes, option_t& opts)
{
	if(!pTypes){
		return false;
	}
	opts.clear();

	for(size_t Count = 0; Count < args.size(); Count++){
		if(0 < args[Count].length() && '#' == args[Count].at(0)){
			// comment line
			return false;
		}
		size_t Count2;
		for(Count2 = 0; pTypes[Count2].option; Count2++){
			if(0 == strcasecmp(args[Count].c_str(), pTypes[Count2].option)){
				if(args.size() < ((Count + 1) + pTypes[Count2].min_param_cnt)){
					ERR("Option(%s) needs %d parameter.", args[Count].c_str(), pTypes[Count2].min_param_cnt);
					return false;
				}

				size_t		Count3;
				params_t	params;
				params.clear();
				for(Count3 = 0; Count3 < static_cast<size_t>(pTypes[Count2].max_param_cnt); Count3++){
					if(args.size() <= ((Count + 1) + Count3)){
						break;
					}
					params.push_back(args[(Count + 1) + Count3].c_str());
				}
				Count += Count3;
				opts[pTypes[Count2].norm_option] = params;
				break;
			}
		}
		if(!pTypes[Count2].option){
			ERR("Unknown option(%s).", args[Count].c_str());
			return false;
		}
	}
	return true;
}

static bool ExecOptionParser(int argc, char** argv, option_t& opts, string& prgname)
{
	if(0 == argc || !argv){
		return false;
	}
	prgname = basename(argv[0]);

	strlist_t	args;
	for(int nCnt = 1; nCnt < argc; nCnt++){
		args.push_back(argv[nCnt]);
	}

	opts.clear();
	return BaseOptionParser(args, ExecOptionTypes, opts);
}

static bool LineOptionParser(const char* pCommand, option_t& opts)
{
	opts.clear();

	if(!pCommand){
		return false;
	}
	if(0 == strlen(pCommand)){
		return true;
	}

	strlist_t	args;
	string		strParameter;
	bool		isMakeParameter	= false;
	bool		isQuart			= false;
	for(const_pchar pPos = pCommand; '\0' != *pPos && '\n' != *pPos; ++pPos){
		if(isMakeParameter){
			// keeping parameter
			if(isQuart){
				// pattern: "...."
				if('\"' == *pPos){
					if(0 == isspace(*(pPos + sizeof(char))) && '\0' != *(pPos + sizeof(char))){
						ERR("Quart is not matching.");
						return false;
					}
					// end of quart
					isMakeParameter	= false;
					isQuart			= false;

				}else if('\\' == *pPos && '\"' == *(pPos + sizeof(char))){
					// escaped quart
					pPos++;
					strParameter += *pPos;
				}else{
					strParameter += *pPos;
				}

			}else{
				// normal pattern
				if(0 == isspace(*pPos)){
					if('\\' == *pPos){
						continue;
					}
					strParameter += *pPos;
				}else{
					isMakeParameter = false;
				}
			}
			if(!isMakeParameter){
				// end of one parameter
				if(0 < strParameter.length()){
					args.push_back(strParameter);
					strParameter.clear();
				}
			}
		}else{
			// not keeping parameter
			if(0 == isspace(*pPos)){
				strParameter.clear();
				isMakeParameter	= true;

				if('\"' == *pPos){
					isQuart		= true;
				}else{
					isQuart		= false;

					if('\\' == *pPos){
						// found escape character
						pPos++;
						if('\0' == *pPos || '\n' == *pPos){
							break;
						}
					}
					strParameter += *pPos;
				}
			}
			// skip space
		}
	}
	// last check
	if(isMakeParameter){
		if(isQuart){
			ERR("Quart is not matching.");
			return false;
		}
		if(0 < strParameter.length()){
			args.push_back(strParameter);
			strParameter.clear();
		}
	}

	if(!BaseOptionParser(args, LineOptionTypes, opts)){
		return false;
	}
	if(1 < opts.size()){
		ERR("Too many option parameter.");
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Utilities
//---------------------------------------------------------
//
// Return: if left lines, returns true.
//
static bool ReadLine(int fd, string& line)
{
	char	szBuff;

	line.erase();
	while(true){
		ssize_t	readlength;

		szBuff = '\0';
		// read one character
		if(-1 == (readlength = read(fd, &szBuff, 1))){
			line.erase();
			return false;
		}

		// check EOF
		if(0 == readlength){
			return false;
		}

		// check character
		if('\r' == szBuff || '\0' == szBuff){
			// skip words

		}else if('\n' == szBuff){
			// skip comment line & no command line
			bool	isSpace		= true;
			bool	isComment	= false;
			for(size_t cPos = 0; isSpace && cPos < line.length(); cPos++){
				if(0 == isspace(line.at(cPos))){
					isSpace = false;

					if('#' == line.at(cPos)){
						isComment = true;
					}
					break;
				}
			}
			if(!isComment && !isSpace){
				break;
			}
			// this line is comment or empty, so read next line.
			line.erase();

		}else{
			line += szBuff;
		}
	}
	return true;
}

#define	BINARY_DUMP_BYTE_SIZE		16

static std::string BinaryDumpLineUtility(const unsigned char* value, size_t vallen)
{
	if(!value || 0 == vallen){
		return string("");
	}

	const char*	pstrvalue = reinterpret_cast<const char*>(value);
	string		stroutput;
	string		binoutput;
	char		szBuff[8];
	size_t		pos;
	for(pos = 0; pos < vallen && pos < BINARY_DUMP_BYTE_SIZE; pos++){
		if(0 != isprint(pstrvalue[pos])){
			stroutput += value[pos];
		}else{
			stroutput += static_cast<char>(0xFF);
		}

		sprintf(szBuff, "%02X ", value[pos]);
		binoutput += szBuff;
		if(8 == (pos + 1)){
			binoutput += " ";
		}
	}
	for(; pos < BINARY_DUMP_BYTE_SIZE; pos++){
		stroutput += " ";
	}
	stroutput += "    ";

	// append
	stroutput += binoutput;

	return stroutput;
}

static bool BinaryDumpUtility(const char* prefix, const unsigned char* value, size_t vallen)
{
	if(!value || 0 == vallen){
		return false;
	}

	string	strpref;
	if(ISEMPTYSTR(prefix)){
		strpref = "VALUE = ";
	}else{
		strpref = " ";
		strpref += prefix;
		strpref += " = ";
	}
	string	spacer;
	for(size_t len = strpref.size(); 0 < len; len--){
		spacer += " ";
	}

	string	output;
	string	line;
	size_t	restcnt = vallen;
	for(size_t pos = 0; pos < vallen; pos += BINARY_DUMP_BYTE_SIZE){
		if(0 == pos){
			output += strpref;
		}else{
			output += "\n";
			output += spacer;
		}
		line	= BinaryDumpLineUtility(&value[pos], restcnt);
		output += line;
		restcnt-= (BINARY_DUMP_BYTE_SIZE <= restcnt ? BINARY_DUMP_BYTE_SIZE : restcnt);
	}
	PRN("%s", output.c_str());

	return true;
}

static char* GetPrintableString(const unsigned char* byData, size_t length)
{
	if(!byData || 0 == length){
		length = 0;
	}
	char*	result;
	if(NULL == (result = reinterpret_cast<char*>(calloc(length + 1, sizeof(char))))){
		ERR("Could not allocate memory.");
		return NULL;
	}
	for(size_t pos = 0; pos < length; ++pos){
		result[pos] = isprint(byData[pos]) ? byData[pos] : (0x00 != byData[pos] ? 0xFF : (pos + 1 < length ? ' ' : 0x00));
	}
	return result;
}

//---------------------------------------------------------
// Command handling
//---------------------------------------------------------
static bool CommandStringHandle(K2HShm& k2hash, ConsoleInput& InputIF, const char* pCommand, bool& is_exit);

static bool InfoCommand(K2HShm& k2hash, params_t& params)
{
	bool	isState = false;
	if(1 < params.size()){
		ERR("Unknown parameters(%s...) for \"info\" command.", params[1].c_str());
		return true;		// for continue.
	}else if(1 == params.size()){
		if(0 == strcasecmp(params[0].c_str(), "state")){
			isState = true;
		}else{
			ERR("Unknown parameter(%s) for \"info\" command.", params[0].c_str());
			return true;	// for continue.
		}
	}

	PRN("-------------------------------------------------------");
	PRN("K2HASH HEAD INFORMATION");
	PRN("-------------------------------------------------------");
	if(isModeCAPI){
		if(!k2h_dump_head(reinterpret_cast<k2h_h>(&k2hash), NULL)){
			ERR("Something error occurred while dumping head of k2hash.");
			return true;	// for continue.
		}
	}else{
		if(!k2hash.Dump(stdout, K2HShm::DUMP_HEAD)){
			ERR("Something error occurred while dumping head of k2hash.");
			return true;	// for continue.
		}
	}

	PRN(NULL);
	PRN("-------------------------------------------------------");
	PRN("K2HASH HASH FUNCTION");
	PRN("-------------------------------------------------------");
	PRN("Hash functions version:                 %s", K2H_HASH_VER_FUNC());
	PRN("Coded function addr(k2h_hash):          %p", k2h_hash);
	PRN("Coded function addr(k2h_second_hash):   %p", k2h_second_hash);
	PRN("Coded function addr(k2h_hash_version):  %p", k2h_hash_version);
	PRN("Loaded function addr(k2h_hash):         %p", K2HashDynLib::get()->get_k2h_hash());
	PRN("Loaded function addr(k2h_second_hash):  %p", K2HashDynLib::get()->get_k2h_second_hash());
	PRN("Loaded function addr(k2h_hash_version): %p", K2HashDynLib::get()->get_k2h_hash_version());
	PRN(NULL);
	PRN("-------------------------------------------------------");
	PRN("K2HASH TRANSACTION FUNCTION");
	PRN("-------------------------------------------------------");
	PRN("Transaction functions version:          %s", K2H_TRANS_VER_FUNC());
	PRN("Coded function addr(k2h_trans):         %p", k2h_trans);
	PRN("Coded function addr(k2h_trans_version): %p", k2h_trans_version);
	PRN("Coded function addr(k2h_trans_cntl):    %p", k2h_trans_cntl);
	PRN("Loaded function addr(k2h_trans):        %p", K2HTransDynLib::get()->get_k2h_trans());
	PRN("Loaded function addr(k2h_trans_version):%p", K2HTransDynLib::get()->get_k2h_trans_version());
	PRN("Loaded function addr(k2h_trans_cntl):   %p", K2HTransDynLib::get()->get_k2h_trans_cntl());
	PRN("Transaction thread pool count:          %d", K2HShm::GetTransThreadPool());

	PRN(NULL);
	PRN("-------------------------------------------------------");
	PRN("K2HASH ATTRIBUTE LIBRARIES");
	PRN("-------------------------------------------------------");
	if(isModeCAPI){
		if(!k2h_print_attr_information(reinterpret_cast<k2h_h>(&k2hash), NULL)){
			ERR("Something error occurred while printing attribute information.");
			return true;	// for continue.
		}
	}else{
		stringstream	ss;
		k2hash.GetAttrInfos(ss);
		PRN("%s", ss.str().c_str());
	}

	if(isState){
		PRN("-------------------------------------------------------");
		PRN("K2HASH STATE");
		PRN("-------------------------------------------------------");

		if(isModeCAPI){
			if(!k2h_print_state(reinterpret_cast<k2h_h>(&k2hash), NULL)){
				ERR("Something error occurred while printing state of k2hash.");
				return true;	// for continue.
			}
		}else{
			if(!k2hash.PrintState(stdout)){
				ERR("Something error occurred while printing state of k2hash.");
				return true;	// for continue.
			}
		}
	}
	PRN("-------------------------------------------------------");

	return true;
}

static bool DumpCommand(K2HShm& k2hash, params_t& params)
{
	int		DumpMode = K2HShm::DUMP_HEAD;
	string	modestr  = "HEAD";

	if(0 != params.size()){
		if(0 == strcasecmp(params[0].c_str(), "HEAD")){
			DumpMode = K2HShm::DUMP_HEAD;
			modestr  = "HEAD";
		}else if(0 == strcasecmp(params[0].c_str(), "KINDEX")){
			DumpMode = K2HShm::DUMP_KINDEX_ARRAY;
			modestr  = "KEY INDEX";
		}else if(0 == strcasecmp(params[0].c_str(), "CKINDEX")){
			DumpMode = K2HShm::DUMP_CKINDEX_ARRAY;
			modestr  = "COLLISION KEY INDEX";
		}else if(0 == strcasecmp(params[0].c_str(), "ELEMENT")){
			DumpMode = K2HShm::DUMP_ELEMENT_LIST;
			modestr  = "ELEMENT";
		}else if(0 == strcasecmp(params[0].c_str(), "FULL")){
			DumpMode = K2HShm::DUMP_PAGE_LIST;
			modestr  = "FULL";
		}else{
			ERR("Unknown parameter(%s) for \"dump\" command.", params[0].c_str());
			return true;	// for continue.
		}
	}

	PRN("-------------------------------------------------------");
	PRN("K2HASH %s DUMP", modestr.c_str());
	PRN("-------------------------------------------------------");

	bool result;
	if(isModeCAPI){
		if(K2HShm::DUMP_HEAD == DumpMode){
			result = k2h_dump_head(reinterpret_cast<k2h_h>(&k2hash), NULL);
		}else if(K2HShm::DUMP_KINDEX_ARRAY == DumpMode){
			result = k2h_dump_keytable(reinterpret_cast<k2h_h>(&k2hash), NULL);
		}else if(K2HShm::DUMP_CKINDEX_ARRAY == DumpMode){
			result = k2h_dump_full_keytable(reinterpret_cast<k2h_h>(&k2hash), NULL);
		}else if(K2HShm::DUMP_ELEMENT_LIST == DumpMode){
			result = k2h_dump_elementtable(reinterpret_cast<k2h_h>(&k2hash), NULL);
		}else{	// K2HShm::DUMP_PAGE_LIST == DumpMode
			result = k2h_dump_full(reinterpret_cast<k2h_h>(&k2hash), NULL);
		}
	}else{
		result = k2hash.Dump(stdout, DumpMode);
	}
	if(!result){
		ERR("Something error occurred while dumping head of k2hash.");
		return true;	// for continue.
	}
	return true;
}

static bool SetCommand(K2HShm& k2hash, params_t& params)
{
	bool 	isRemoveSubkeyAll	= false;
	time_t	expire_time			= -1;
	string	PassPhrase("");
	for(size_t pos = 2; pos < params.size(); ++pos){
		if(0 == strcasecmp(params[pos].c_str(), "RMSUB")){
			isRemoveSubkeyAll = true;
		}else if(0 == strncasecmp(params[pos].c_str(), "PASS=", 5)){
			PassPhrase = params[pos].substr(5);
		}else if(0 == strncasecmp(params[pos].c_str(), "EXPIRE=", 7)){
			expire_time = static_cast<time_t>(atoi(params[pos].substr(7).c_str()));
		}else{
			ERR("Unknown parameter(%s) for set command.", params[2].c_str());
			return true;	// for continue.
		}
	}
	const char*	pValue = NULL;
	if(0 < params[1].length()){
		if(0 != strcasecmp(params[1].c_str(), "null")){
			pValue = params[1].c_str();
		}
	}

	// [NOTE]
	// It is allowed to set the expire value directly to 0.
	// This is useful if you want to create keys as placeholders and so on.
	// However, the expire value can be set 0 only when this method is
	// called directly.
	//
	if(0 == expire_time){
		PRN("[MSG] The expire value is 0. This means that this key is specifically allowed as a placeholder.");
	}

	if(isModeCAPI){
		if(isRemoveSubkeyAll){
			if(!k2h_set_str_all_wa(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str(), pValue, NULL, (PassPhrase.empty() ? NULL : PassPhrase.c_str()), (expire_time <= 0 ? NULL : &expire_time))){
				ERR("Something error occurred while writing key-value into k2hash.");
				return true;	// for continue.
			}
		}else{
			if(!k2h_set_str_value_wa(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str(), pValue, (PassPhrase.empty() ? NULL : PassPhrase.c_str()), (expire_time <= 0 ? NULL : &expire_time))){
				ERR("Something error occurred while writing key-value into k2hash.");
				return true;	// for continue.
			}
		}
	}else{
		if(!k2hash.Set(params[0].c_str(), pValue, NULL, isRemoveSubkeyAll, (PassPhrase.empty() ? NULL : PassPhrase.c_str()), (expire_time <= 0 ? NULL : &expire_time))){
			ERR("Something error occurred while writing key-value into k2hash.");
			return true;	// for continue.
		}
	}
	return true;
}

static K2HGETCBRES SetTrialCallback(const unsigned char* byKey, size_t keylen, const unsigned char* byValue, size_t vallen, unsigned char** ppNewValue, size_t* pnewvallen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData)
{
	if(!byKey || 0 == keylen || !ppNewValue || !pnewvallen){
		return K2HGETCB_RES_ERROR;
	}
	*ppNewValue	= NULL;
	*pnewvallen	= 0;

	PRN("\n");
	if(!BinaryDumpUtility("KEY        ", byKey, keylen)){
		ERR("Something error occurred during callback.");
		return K2HGETCB_RES_ERROR;
	}
	if(byValue && 0 < vallen){
		if(!BinaryDumpUtility("VALUE(NOW) ", byValue, vallen)){
			ERR("Something error occurred during callback.");
			return K2HGETCB_RES_ERROR;
		}
	}
	PRN(" ATTRIBUTE = {");
	for(int cnt = 0; cnt < attrscnt; ++cnt){
		char*	pAttrKey = GetPrintableString(pattrs[cnt].pkey, pattrs[cnt].keylength);
		if(!BinaryDumpUtility(pAttrKey, pattrs[cnt].pval, pattrs[cnt].vallength)){
			ERR("Something error occurred during callback.");
			K2H_Free(pAttrKey);
			return K2HGETCB_RES_ERROR;
		}
		K2H_Free(pAttrKey);
	}
	PRN(" }");

	K2HGETCBRES	res = K2HGETCB_RES_NOTHING;
	char		szBuff[1024];
	while(true){
		if(byValue && 0 < vallen){
			fprintf(stdout, "Do you replace value? [n/\"value\"] (n) -> ");
		}else{
			fprintf(stdout, "Do you make new key-value? [n/\"value\"] (n) -> ");
		}
		if(NULL == fgets(szBuff, sizeof(szBuff) - 1, stdin)){
			ERR("Something error occurred during callback.");
			return K2HGETCB_RES_ERROR;
		}
		if(strlen(szBuff) <= sizeof(szBuff) - 1){
			if(0 < strlen(szBuff)){
				if('\n' == szBuff[strlen(szBuff) - 1]){
					szBuff[strlen(szBuff) - 1] = '\0';
				}
			}
		}else{
			szBuff[sizeof(szBuff) - 1] = '\0';
		}
		if(0 < strlen(szBuff)){
			if(0 == strcasecmp(szBuff, "n") || 0 == strcasecmp(szBuff, "no") || 0 == strcasecmp(szBuff, "false")){
				res = K2HGETCB_RES_NOTHING;
			}else{
				res = K2HGETCB_RES_OVERWRITE;
				*ppNewValue	= reinterpret_cast<unsigned char*>(strdup(szBuff));
				*pnewvallen	= strlen(szBuff) + 1;
			}
			break;
		}
	}
	return res;
}

static bool SetTrialCommand(K2HShm& k2hash, params_t& params)
{
	if(1 != params.size() && 2 != params.size()){
		if(2 < params.size()){
			ERR("Unknown parameter(%s) for queue push command.", params[2].c_str());
		}else{
			ERR("settrial command needs more parameter.");
		}
		return true;	// for continue.
	}

	string		pass;
	const char*	ppass	= NULL;
	if(2 == params.size()){
		if(0 == strncasecmp(params[1].c_str(), "pass=", 5)){
			pass = params[1].substr(5);
			ppass = pass.c_str();
		}
	}

	char*	pValue = NULL;
	if(isModeCAPI){
		if(!k2h_get_str_value_wp_ext(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str(), &pValue, SetTrialCallback, NULL, ppass)){
			ERR("Something error occurred while writing key-value into k2hash.");
			return true;	// for continue.
		}

	}else{
		if(-1 == k2hash.Get(reinterpret_cast<const unsigned char*>(params[0].c_str()), params[0].length() + 1, reinterpret_cast<unsigned char**>(&pValue), SetTrialCallback, NULL, true, ppass)){
			ERR("Something error occurred while writing key-value into k2hash.");
			return true;	// for continue.
		}
	}
	K2H_Free(pValue);

	return true;
}

static bool SetSubkeyCommand(K2HShm& k2hash, params_t& params)
{
	const char*	pValue = NULL;
	if(0 < params[2].length()){
		if(0 != strcasecmp(params[2].c_str(), "null")){
			pValue = params[2].c_str();
		}
	}

	if(isModeCAPI){
		if(!k2h_add_str_subkey(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str(), params[1].c_str(), pValue)){
			ERR("Something error occurred while set subkey(%s) into key(%s).", params[1].c_str(), params[0].c_str());
			return true;	// for continue.
		}
	}else{
		if(!k2hash.AddSubkey(params[0].c_str(), params[1].c_str(), pValue)){
			ERR("Something error occurred while set subkey(%s) into key(%s).", params[1].c_str(), params[0].c_str());
			return true;	// for continue.
		}
	}
	return true;
}

static bool DirectSetCommand(K2HShm& k2hash, params_t& params)
{
	if(isModeCAPI){
		// get handle
		k2h_da_h	dahandle;
		if(K2H_INVALID_HANDLE == (dahandle = k2h_da_str_handle_write(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str()))){
			ERR("Could not get k2h_da_h handle.");
			return true;		// normal exit
		}

		// offset
		off_t	offset = static_cast<off_t>(atoi(params[2].c_str()));
		if(!k2h_da_set_write_offset(dahandle, offset)){
			ERR("Could not set write offset.");
			k2h_da_free(dahandle);
			return true;		// normal exit
		}

		// write
		if(!k2h_da_set_value_str(dahandle, params[1].c_str())){
			ERR("Failed writing value.");
			k2h_da_free(dahandle);
			return true;		// normal exit
		}
		k2h_da_free(dahandle);

	}else{
		// offset
		off_t	offset = static_cast<off_t>(atoi(params[2].c_str()));

		// attach object
		K2HDAccess*	pAccess;
		if(NULL == (pAccess = k2hash.GetDAccessObj(params[0].c_str(), K2HDAccess::WRITE_ACCESS, offset))){
			ERR("Could not initialize internal K2HDAccess object.");
			return true;		// normal exit
		}

		// write
		if(!pAccess->Write(params[1].c_str())){
			ERR("Failed writing %s.", params[1].c_str());
			K2H_Delete(pAccess);
			return true;		// normal exit
		}
		K2H_Delete(pAccess);
	}
	return true;
}

static bool DirectSetFileCommand(K2HShm& k2hash, params_t& params)
{
	// file open
	int	fd;
	if(-1 == (fd = open(params[2].c_str(), O_RDONLY))){
		ERR("Could not open file(%s).", params[2].c_str());
		return true;		// normal exit
	}

	// file size
	struct stat	st;
	if(-1 == fstat(fd, &st)){
		ERR_K2HPRN("Could not get stat from fd. errno = %d", errno);
		K2H_CLOSE(fd);
		return false;
	}

	if(isModeCAPI){
		// get handle
		k2h_da_h	dahandle;
		if(K2H_INVALID_HANDLE == (dahandle = k2h_da_str_handle_write(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str()))){
			ERR("Could not get k2h_da_h handle.");
			K2H_CLOSE(fd);
			return true;		// normal exit
		}

		// offset
		off_t	offset = static_cast<off_t>(atoi(params[1].c_str()));
		if(!k2h_da_set_write_offset(dahandle, offset)){
			ERR("Could not set write offset.");
			K2H_CLOSE(fd);
			k2h_da_free(dahandle);
			return true;		// normal exit
		}

		// write
		size_t	wlength = static_cast<size_t>(st.st_size);
		if(!k2h_da_set_value_from_file(dahandle, fd, &wlength)){
			ERR("Failed writing value.");
			K2H_CLOSE(fd);
			k2h_da_free(dahandle);
			return true;		// normal exit
		}
		k2h_da_free(dahandle);

	}else{
		// offset
		off_t	offset = static_cast<off_t>(atoi(params[1].c_str()));

		// attach object
		K2HDAccess*	pAccess;
		if(NULL == (pAccess = k2hash.GetDAccessObj(params[0].c_str(), K2HDAccess::WRITE_ACCESS, offset))){
			ERR("Could not initialize internal K2HDAccess object.");
			K2H_CLOSE(fd);
			return true;		// normal exit
		}

		// write
		size_t	wlength = static_cast<size_t>(st.st_size);
		if(!pAccess->Write(fd, wlength)){
			ERR("Failed writing.");
			K2H_CLOSE(fd);
			K2H_Delete(pAccess);
			return true;		// normal exit
		}
		K2H_Delete(pAccess);
	}
	K2H_CLOSE(fd);

	return true;
}

static bool FillCommand(K2HShm& k2hash, params_t& params)
{
	int		nCount = atoi(params[2].c_str());
	if(0 >= nCount){
		ERR("Count parameter(%s) is wrong.", params[2].c_str());
		return true;	// for continue.
	}

	const char*	pValue = NULL;
	if(0 < params[1].length()){
		pValue = params[1].c_str();
	}

	for(int nCnt = 0; nCnt < nCount; nCnt++){
		stringstream	ssKey;
		ssKey << params[0] << "-" << nCnt;

		string	strKey = ssKey.str();
		if(isModeCAPI){
			if(!k2h_set_str_all(reinterpret_cast<k2h_h>(&k2hash), strKey.c_str(), pValue, NULL)){
				ERR("Something error occurred while key(%s)-value into k2hash.", strKey.c_str());
				break;
			}
		}else{
			if(!k2hash.Set(strKey.c_str(), pValue)){
				ERR("Something error occurred while key(%s)-value into k2hash.", strKey.c_str());
				break;
			}
		}
	}
	return true;
}

static bool FillSubkeyCommand(K2HShm& k2hash, params_t& params)
{
	int		nCount = atoi(params[3].c_str());
	if(0 >= nCount){
		ERR("Count parameter(%s) is wrong.", params[3].c_str());
		return true;	// for continue.
	}

	const char*	pValue = NULL;
	if(0 < params[2].length()){
		pValue = params[2].c_str();
	}

	for(int nCnt = 0; nCnt < nCount; nCnt++){
		stringstream	ssKey;
		ssKey << params[1] << "-" << nCnt;

		string	strKey = ssKey.str();
		if(isModeCAPI){
			if(!k2h_add_str_subkey(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str(), strKey.c_str(), pValue)){
				ERR("Something error occurred while key(%s)-value under key(%s) into k2hash.", strKey.c_str(), params[0].c_str());
				break;
			}
		}else{
			if(!k2hash.AddSubkey(params[0].c_str(), strKey.c_str(), pValue)){
				ERR("Something error occurred while key(%s)-value under key(%s) into k2hash.", strKey.c_str(), params[0].c_str());
				break;
			}
		}
	}
	return true;
}

static bool RemoveCommand(K2HShm& k2hash, params_t& params)
{
	bool isRemoveSubkeyAll = false;
	if(1 < params.size()){
		if(0 != strcasecmp(params[1].c_str(), "ALL")){
			ERR("Unknown parameter(%s) for rm command.", params[1].c_str());
			return true;	// for continue.
		}
		isRemoveSubkeyAll = true;
	}

	bool	result;
	if(isModeCAPI){
		if(isRemoveSubkeyAll){
			result = k2h_remove_str_all(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str());
		}else{
			result = k2h_remove_str(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str());
		}
	}else{
		result = k2hash.Remove(params[0].c_str(), isRemoveSubkeyAll);
	}
	if(!result){
		ERR("Something error occurred while remove key(%s) with(%s) subkeys.", params[0].c_str(), isRemoveSubkeyAll ? "out" : "");
		return true;	// for continue.
	}
	return true;
}

static bool RemoveSubkeyCommand(K2HShm& k2hash, params_t& params)
{
	if(isModeCAPI){
		if(!k2h_remove_str_subkey(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str(), params[1].c_str())){
			ERR("Something error occurred while remove subkey(%s) under key(%s).", params[1].c_str(), params[0].c_str());
			return true;	// for continue.
		}
	}else{
		if(!k2hash.Remove(params[0].c_str(), params[1].c_str())){
			ERR("Something error occurred while remove subkey(%s) under key(%s).", params[1].c_str(), params[0].c_str());
			return true;	// for continue.
		}
	}
	return true;
}

static bool PrintKeys(K2HShm& k2hash, const char* pKey, const string& spacer, bool isSubkey, bool is_check_attr, const char* pass)
{
	string	myspacer = spacer + "  ";

	// print own key - value
	char*	pValue;
	if(isModeCAPI){
		if(is_check_attr){
			pValue = k2h_get_str_direct_value_wp(reinterpret_cast<k2h_h>(&k2hash), pKey, pass);
		}else{
			pValue = k2h_get_str_direct_value_np(reinterpret_cast<k2h_h>(&k2hash), pKey);
		}
	}else{
		pValue = k2hash.Get(pKey, is_check_attr, pass);
	}
	if(pValue){
		PRN("%s+\"%s\" => \"%s\"", myspacer.c_str(), pKey, pValue);
	}else{
		PRN("%s+\"%s\" => value is not found", myspacer.c_str(), pKey ? pKey : "(null)");
	}
	K2H_Free(pValue);

	// subkeys
	K2HSubKeys*	pSubKey = NULL;
	if(isModeCAPI){
		char**	ppSKeyArray;
		if(NULL != (ppSKeyArray = k2h_get_str_direct_subkeys(reinterpret_cast<k2h_h>(&k2hash), pKey))){
			pSubKey = new K2HSubKeys();
			for(char** pptmp = ppSKeyArray; pptmp && *pptmp; pptmp++){
				pSubKey->insert(*pptmp);
			}
			k2h_free_keyarray(ppSKeyArray);
		}
	}else{
		pSubKey = k2hash.GetSubKeys(pKey);
	}
	if(pSubKey){
		strarr_t	strarr;
		if(0 < pSubKey->StringArray(strarr)){
			// cppcheck-suppress postfixOperator
			for(strarr_t::iterator iter = strarr.begin(); iter != strarr.end(); iter++){
				if(isSubkey){
					// reentrant
					if(!PrintKeys(k2hash, iter->c_str(), myspacer, isSubkey, is_check_attr, pass)){
						// something error occurred, but nothing to do because already putting error from function.
					}
				}else{
					PRN("%s     subkey: \"%s\"", myspacer.c_str(), iter->c_str());
				}
			}
		}
		K2H_Delete(pSubKey);
	}
	return true;
}

static bool RenameCommand(K2HShm& k2hash, params_t& params)
{
	if(isModeCAPI){
		if(!k2h_rename_str(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str(), params[1].c_str())){
			ERR("Something error occurred while renaming key.");
			return true;	// for continue.
		}
	}else{
		if(!k2hash.Rename(params[0].c_str(), params[1].c_str())){
			ERR("Something error occurred while renaming key.");
			return true;	// for continue.
		}
	}
	return true;
}

static bool PrintCommand(K2HShm& k2hash, params_t& params)
{
	bool	isSubkeys		= false;
	bool	is_check_attr	= true;
	string	PassPhrase("");
	for(size_t pos = 1; pos < params.size(); ++pos){
		if(0 == strcasecmp(params[pos].c_str(), "ALL")){
			isSubkeys = true;
		}else if(0 == strcasecmp(params[pos].c_str(), "noattrcheck")){
			is_check_attr = false;
		}else if(0 == strncasecmp(params[pos].c_str(), "PASS=", 5)){
			PassPhrase = params[pos].substr(5);
		}else{
			ERR("Unknown parameter(%s) for print command.", params[pos].c_str());
			return true;	// for continue.
		}
	}

	string	spacer = "";
	if(!PrintKeys(k2hash, params[0].c_str(), spacer, isSubkeys, is_check_attr, (PassPhrase.empty() ? NULL : PassPhrase.c_str()))){
		ERR("Something error occurred while displaying subkeys.");
		return true;	// for continue.
	}
	return true;
}

static bool PrintAttrCommand(K2HShm& k2hash, params_t& params)
{
	if(1 != params.size()){
		if(1 < params.size()){
			ERR("Unknown parameter(%s) for printattr command.", params[1].c_str());
		}else{
			ERR("printattr command needs one parameters(key).");
		}
		return true;	// for continue.
	}

	if(isModeCAPI){
		int			attrspckcnt	= 0;
		PK2HATTRPCK	pattrpck;
		if(NULL != (pattrpck = k2h_get_str_direct_attrs(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str(), &attrspckcnt))){
			PRN("\"%s\" attribute = {", params[0].c_str());
			for(int cnt = 0; cnt < attrspckcnt; ++cnt){
				char*	pattrkey = GetPrintableString(pattrpck[cnt].pkey, pattrpck[cnt].keylength);
				char*	pattrval = GetPrintableString(pattrpck[cnt].pval, pattrpck[cnt].vallength);

				PRN("    \"%s\"\t=> \"%s\"", pattrkey ? pattrkey : "(null)", pattrval ? pattrval : "(null)");

				K2H_Free(pattrkey);
				K2H_Free(pattrval);
			}
			PRN("}");
			k2h_free_attrpack(pattrpck, attrspckcnt);
		}else{
			PRN("\"%s\" => attribute is not found", params[0].c_str());
		}

	}else{
		K2HAttrs*	pAttr = k2hash.GetAttrs(params[0].c_str());
		if(pAttr){
			PRN("\"%s\" attribute = {", params[0].c_str());
			for(K2HAttrs::iterator iter = pAttr->begin(); iter != pAttr->end(); ++iter){
				char*	pattrkey = GetPrintableString(iter->pkey, iter->keylength);
				char*	pattrval = GetPrintableString(iter->pval, iter->vallength);

				PRN("    \"%s\"\t=> \"%s\"", pattrkey ? pattrkey : "(null)", pattrval ? pattrval : "(null)");

				K2H_Free(pattrkey);
				K2H_Free(pattrval);
			}
			PRN("}");
			K2H_Delete(pAttr);
		}else{
			PRN("\"%s\" => attribute is not found", params[0].c_str());
		}
	}
	return true;
}

static bool AddAttrCommand(K2HShm& k2hash, params_t& params)
{
	string	key;
	string	attrkey;
	string	attrval;

	if(3 < params.size()){
		ERR("Unknown parameter(%s) for addattr command.", params[3].c_str());
		return true;	// for continue.
	}else if(2 > params.size()){
		ERR("addattr command needs two or three parameters: key/attr key(/attr value).");
		return true;	// for continue.
	}else{
		key		= params[0];
		attrkey	= params[1];
		if(3 == params.size()){
			attrval = params[2];
		}
	}

	if(isModeCAPI){
		if(!k2h_add_str_attr(reinterpret_cast<k2h_h>(&k2hash), key.c_str(), attrkey.c_str(), attrval.empty() ? NULL : attrval.c_str())){
			ERR("Something error occurred while adding attribute into key.");
			return true;	// for continue.
		}
	}else{
		if(!k2hash.AddAttr(key.c_str(), attrkey.c_str(), attrval.empty() ? NULL : attrval.c_str())){
			ERR("Something error occurred while adding attribute into key.");
			return true;	// for continue.
		}
	}
	return true;
}

static bool DirectPrintCommand(K2HShm& k2hash, params_t& params)
{
	unsigned char*	byValue		= NULL;
	size_t			vallength	= static_cast<size_t>(atoi(params[1].c_str()));

	if(isModeCAPI){
		// get handle
		k2h_da_h	dahandle;
		if(K2H_INVALID_HANDLE == (dahandle = k2h_da_str_handle_read(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str()))){
			ERR("Could not get k2h_da_h handle.");
			return true;		// normal exit
		}

		// offset
		off_t	offset = static_cast<off_t>(atoi(params[2].c_str()));
		if(!k2h_da_set_read_offset(dahandle, offset)){
			ERR("Could not set read offset.");
			k2h_da_free(dahandle);
			return true;		// normal exit
		}

		// read
		if(!k2h_da_get_value(dahandle, &byValue, &vallength) || !byValue){
			ERR("Failed reading value.");
			k2h_da_free(dahandle);
			K2H_Free(byValue);
			return true;		// normal exit
		}
		k2h_da_free(dahandle);

	}else{
		// offset
		off_t	offset = static_cast<off_t>(atoi(params[2].c_str()));

		// attach object
		K2HDAccess*	pAccess;
		if(NULL == (pAccess = k2hash.GetDAccessObj(params[0].c_str(), K2HDAccess::READ_ACCESS, offset))){
			ERR("Could not initialize internal K2HDAccess object.");
			return true;		// normal exit
		}

		// read
		if(!pAccess->Read(&byValue, vallength) || !byValue){
			ERR("Failed reading %s key.", params[0].c_str());
			K2H_Delete(pAccess);
			K2H_Free(byValue);
			return true;		// normal exit
		}
		K2H_Delete(pAccess);
	}

	// print
	char	szBinBuff[64];
	char	szChBuff[32];
	char	szTmpBuff[16];
	size_t	pos;
	for(pos = 0UL; pos < vallength; pos++){
		size_t	wpos;

		if(0 == (pos % 8)){
			sprintf(szBinBuff, "                            ");	// Base for "00 00 00 00  00 00 00 00    "
			sprintf(szChBuff, "         ");						// Base for "SSSS SSSS"
		}
		wpos = pos % 8;
		sprintf(szTmpBuff, "%02X", byValue[pos]);
		memcpy(&szBinBuff[(3 < wpos ? (wpos * 3) + 1 : wpos * 3)], szTmpBuff, 2);
		sprintf(szTmpBuff, "%c", (isprint(byValue[pos]) ? byValue[pos] : 0x00 == byValue[pos] ? ' ' : 0xFF));
		memcpy(&szChBuff[(3 < wpos ? wpos + 1 : wpos)], szTmpBuff, 1);

		if(7 == (pos % 8)){
			PRN(" %s - %s", szBinBuff, szChBuff);
		}
	}
	if(0 != (pos % 8)){
		PRN(" %s - %s", szBinBuff, szChBuff);
	}
	K2H_Free(byValue);

	return true;
}

static bool DirectSaveCommand(K2HShm& k2hash, params_t& params)
{
	k2h_hash_t	hashval				= static_cast<k2h_hash_t>(atoi(params[0].c_str()));
	struct timespec	startts 		= {0, 0};
	struct timespec endts			= {time(NULL), 0};
	k2h_hash_t	target_hash			= 0;
	k2h_hash_t	target_max_hash		= 1;
	k2h_hash_t	old_hash			= static_cast<k2h_hash_t>(-1);	// this means ignored
	k2h_hash_t	old_max_hash		= static_cast<k2h_hash_t>(-1);	// this means ignored
	long		target_hash_range	= 1;
	bool		is_expire_check		= true;							// check expire
	k2h_hash_t	nexthash			= 0;
	PK2HBIN		pbindatas			= NULL;
	size_t		datacnt				= 0;

	// get direct
	if(isModeCAPI){
		if(!k2h_get_elements_by_hash(reinterpret_cast<k2h_h>(&k2hash), hashval, startts, endts, target_hash, target_max_hash, old_hash, old_max_hash, target_hash_range, is_expire_check, &nexthash, &pbindatas, &datacnt)){
			ERR("Failed reading elements data by hashvalue(0x%" PRIx64 ").", hashval);
			return true;		// normal exit
		}
	}else{
		if(!k2hash.GetElementsByHash(hashval, startts, endts, target_hash, target_max_hash, old_hash, old_max_hash, target_hash_range, is_expire_check, &nexthash, &pbindatas, &datacnt)){
			ERR("Failed reading elements data by hashvalue(0x%" PRIx64 ").", hashval);
			return true;		// normal exit
		}
	}
	if(!pbindatas || 0 == datacnt){
		free_k2hbins(pbindatas, datacnt);
		ERR("There is no element for hashvalue(0x%" PRIx64 ").", hashval);
		return true;			// normal exit
	}

	// open file
	int	fd;
	if(-1 == (fd = open(params[1].c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644))){
		ERR("Could not open file(%s) for writing binary elements data. errno(%d)", params[1].c_str(), errno);
		free_k2hbins(pbindatas, datacnt);
		return true;			// for continue
	}

	// write to data
	//
	// Output format(string + binary)
	//
	//	COUNT=%zu\n
	//	LENGTH=%zu\n
	//	DATA=<binary data>\n
	//		.
	//		.
	//		.
	//
	off_t	fepos;
	char	szBuff[256];
	sprintf(szBuff, "COUNT=%zu\n", datacnt);
	if(-1 == (fepos = lseek(fd, 0, SEEK_END)) || -1 == k2h_pwrite(fd, szBuff, strlen(szBuff), fepos)){
		ERR("Could not write data count to file(%s)", params[1].c_str());
		free_k2hbins(pbindatas, datacnt);
		K2H_CLOSE(fd);
		return true;			// for continue
	}

	for(size_t cnt = 0; cnt < datacnt; ++cnt){
		// length
		sprintf(szBuff, "LENGTH=%zu\n", pbindatas[cnt].length);
		if(-1 == (fepos = lseek(fd, 0, SEEK_END)) || -1 == k2h_pwrite(fd, szBuff, strlen(szBuff), fepos)){
			ERR("Could not write %zu data length to file(%s)", cnt, params[1].c_str());
			free_k2hbins(pbindatas, datacnt);
			K2H_CLOSE(fd);
			return true;			// for continue
		}
		// data
		if(-1 == (fepos = lseek(fd, 0, SEEK_END)) || -1 == k2h_pwrite(fd, pbindatas[cnt].byptr, pbindatas[cnt].length, fepos)){
			ERR("Could not write %zu binary data to file(%s)", cnt, params[1].c_str());
			free_k2hbins(pbindatas, datacnt);
			K2H_CLOSE(fd);
			return true;			// for continue
		}
	}

	K2H_CLOSE(fd);

	return true;
}

static bool DirectLoadCommand(K2HShm& k2hash, params_t& params)
{
	struct timespec	ts	= {time(NULL), 0};
	if(2 <= params.size()){
		ts.tv_sec = static_cast<time_t>(atoi(params[1].c_str()));
	}

	// open file
	int	fd;
	if(-1 == (fd = open(params[0].c_str(), O_RDONLY))){
		ERR("Could not open file(%s) for reading binary elements data. errno(%d)", params[0].c_str(), errno);
		return true;			// for continue
	}

	//
	// File format(string + binary)
	//
	//	COUNT=%zu\n
	//	LENGTH=%zu\n
	//	DATA=<binary data>\n
	//		.
	//		.
	//		.
	//

	// read binary data count
	off_t	rpos = 0;
	char	szBuff[256];
	if(strlen("COUNT=") != k2h_pread(fd, szBuff, strlen("COUNT="), rpos)){
		ERR("Could not read data count key word from file(%s)", params[0].c_str());
		K2H_CLOSE(fd);
		return true;			// for continue
	}
	if(0 != strncmp(szBuff, "COUNT=", strlen("COUNT="))){
		ERR("Wrong format file(%s), the data does not start \"COUNT=\"", params[0].c_str());
		K2H_CLOSE(fd);
		return true;			// for continue
	}
	rpos += strlen("COUNT=");
	for(size_t cnt = 0; cnt < sizeof(szBuff); ++cnt, ++rpos){
		if(1 != k2h_pread(fd, &szBuff[cnt], 1, rpos)){
			ERR("Could not read data count from file(%s)", params[0].c_str());
			K2H_CLOSE(fd);
			return true;			// for continue
		}
		// terminated by '\n'
		if(0 == isdigit(szBuff[cnt])){
			szBuff[cnt] = '\0';
			++rpos;
			break;
		}
	}
	size_t	datacnt = static_cast<size_t>(atoi(szBuff));

	// read all binary data
	PK2HBIN	pbindatas;
	if(NULL == (pbindatas = reinterpret_cast<PK2HBIN>(calloc(datacnt, sizeof(K2HBIN))))){
		ERR("Could not allocate memory");
		K2H_CLOSE(fd);
		return true;			// for continue
	}
	for(size_t cnt = 0; cnt < datacnt; ++cnt){
		// read length
		if(strlen("LENGTH=") != k2h_pread(fd, szBuff, strlen("LENGTH="), rpos)){
			ERR("Could not read %zu binary data length from file(%s)", cnt, params[0].c_str());
			free_k2hbins(pbindatas, datacnt);
			K2H_CLOSE(fd);
			return true;			// for continue
		}
		if(0 != strncmp(szBuff, "LENGTH=", strlen("LENGTH="))){
			ERR("Wrong format file(%s), the %zu binary data length does not start \"LENGTH=\"", params[0].c_str(), cnt);
			free_k2hbins(pbindatas, datacnt);
			K2H_CLOSE(fd);
			return true;			// for continue
		}
		rpos += strlen("LENGTH=");
		for(size_t cnt2 = 0; cnt2 < sizeof(szBuff); ++cnt2, ++rpos){
			if(1 != k2h_pread(fd, &szBuff[cnt2], 1, rpos)){
				ERR("Could not read data count from file(%s)", params[0].c_str());
				free_k2hbins(pbindatas, datacnt);
				K2H_CLOSE(fd);
				return true;		// for continue
			}
			// terminated by '\n'
			if(0 == isdigit(szBuff[cnt2])){
				szBuff[cnt2] = '\0';
				++rpos;
				break;
			}
		}
		size_t	datalength = static_cast<size_t>(atoi(szBuff));

		// read binary
		unsigned char*	pbin;
		if(NULL == (pbin = reinterpret_cast<unsigned char*>(malloc(sizeof(unsigned char) * datalength)))){
			ERR("Could not allocate memory");
			free_k2hbins(pbindatas, datacnt);
			K2H_CLOSE(fd);
			return true;			// for continue
		}
		if(static_cast<ssize_t>(datalength) != k2h_pread(fd, pbin, datalength, rpos)){
			ERR("Could not read %zu binary data from file(%s)", cnt, params[0].c_str());
			free_k2hbins(pbindatas, datacnt);
			K2H_CLOSE(fd);
			return true;			// for continue
		}
		rpos += datalength;

		pbindatas[cnt].byptr	= pbin;
		pbindatas[cnt].length	= datalength;
	}
	K2H_CLOSE(fd);

	// Loop: set direct
	for(size_t cnt = 0; cnt < datacnt; ++cnt){
		if(isModeCAPI){
			if(!k2h_set_element_by_binary(reinterpret_cast<k2h_h>(&k2hash), &pbindatas[cnt], &ts)){
				ERR("Failed writing %zu binary data", cnt);
				free_k2hbins(pbindatas, datacnt);
				return true;		// normal exit
			}
		}else{
			if(!k2hash.SetElementByBinArray(reinterpret_cast<PRALLEDATA>(pbindatas[cnt].byptr), &ts)){
				ERR("Failed writing %zu binary data", cnt);
				free_k2hbins(pbindatas, datacnt);
				return true;		// normal exit
			}
		}
	}
	free_k2hbins(pbindatas, datacnt);

	return true;
}

static bool DirectCopyFileCommand(K2HShm& k2hash, params_t& params)
{
	// file open
	int	fd;
	if(-1 == (fd = open(params[2].c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
		ERR("Could not open file(%s).", params[2].c_str());
		return true;		// normal exit
	}
	size_t	rlength = 1024 * 1024 * 1024;
	rlength *= 1024;

	if(isModeCAPI){
		// get handle
		k2h_da_h	dahandle;
		if(K2H_INVALID_HANDLE == (dahandle = k2h_da_str_handle_read(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str()))){
			ERR("Could not get k2h_da_h handle.");
			K2H_CLOSE(fd);
			return true;		// normal exit
		}

		// offset
		off_t	offset = static_cast<off_t>(atoi(params[1].c_str()));
		if(!k2h_da_set_read_offset(dahandle, offset)){
			ERR("Could not set read offset.");
			k2h_da_free(dahandle);
			K2H_CLOSE(fd);
			return true;		// normal exit
		}

		// read
		if(!k2h_da_get_value_to_file(dahandle, fd, &rlength)){
			ERR("Failed reading.");
			k2h_da_free(dahandle);
			K2H_CLOSE(fd);
			return true;		// normal exit
		}
		k2h_da_free(dahandle);

	}else{
		// offset
		off_t	offset = static_cast<off_t>(atoi(params[1].c_str()));

		// attach object
		K2HDAccess*	pAccess;
		if(NULL == (pAccess = k2hash.GetDAccessObj(params[0].c_str(), K2HDAccess::READ_ACCESS, offset))){
			ERR("Could not initialize internal K2HDAccess object.");
			K2H_CLOSE(fd);
			return true;		// normal exit
		}

		// read
		if(!pAccess->Read(fd, rlength)){
			ERR("Failed reading.");
			K2H_CLOSE(fd);
			K2H_Delete(pAccess);
			return true;		// normal exit
		}
		K2H_Delete(pAccess);
	}
	PRN("+\"%s\" => %zu byte read and write into file(%s).", params[0].c_str(), rlength, params[2].c_str());
	K2H_CLOSE(fd);

	return true;
}

static bool ListCommand(K2HShm& k2hash, params_t& params)
{
	static string	strSpacer("");

	if(0 < params.size()){
		// list subkeys in the key
		char*	pValue;
		if(isModeCAPI){
			pValue = k2h_get_str_direct_value(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str());
		}else{
			pValue = k2hash.Get(params[0].c_str());
		}
		PRN("%s+\"%s\" => \"%s\"", strSpacer.c_str(), params[0].c_str(), pValue ? pValue : "(null)");

		string	strBupSpacer = strSpacer;
		strSpacer += "  ";
		if(isModeCAPI){
			for(k2h_find_h k2hfhandle = k2h_find_first_str_subkey(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str()); K2H_INVALID_HANDLE != k2hfhandle; k2hfhandle = k2h_find_next(k2hfhandle)){
				// [TODO]
				// Subkey should be unsigned char type, but params is based on string now.
				//
				char*	pKey = k2h_find_get_str_key(k2hfhandle);
				if(pKey){
					// reentrant
					params_t	subparams;
					subparams.clear();
					subparams.push_back(string(pKey));

					ListCommand(k2hash, subparams);
				}
				K2H_Free(pKey);
			}
		}else{
			// cppcheck-suppress postfixOperator
			for(K2HShm::iterator iter = k2hash.begin(params[0].c_str()); iter != k2hash.end(true); iter++){
				// [TODO]
				// Subkey should be unsigned char type, but params is based on string now.
				//
				char*	pKey = k2hash.Get(*iter, K2HShm::PAGEOBJ_KEY);
				if(pKey){
					// reentrant
					params_t	subparams;
					subparams.clear();
					subparams.push_back(string(pKey));

					ListCommand(k2hash, subparams);
				}
				K2H_Free(pKey);
			}
		}
		strSpacer = strBupSpacer;
		K2H_Free(pValue);

	}else{
		// list all keys
		if(isModeCAPI){
			for(k2h_find_h k2hfhandle = k2h_find_first(reinterpret_cast<k2h_h>(&k2hash)); K2H_INVALID_HANDLE != k2hfhandle; k2hfhandle = k2h_find_next(k2hfhandle)){
				unsigned char*	byKey	= NULL;
				unsigned char*	byVal	= NULL;
				size_t			KeyLen	= 0;
				size_t			ValLen	= 0;
				char*			pKey	= NULL;
				char*			pValue	= NULL;
				if(k2h_find_get_key(k2hfhandle, &byKey, &KeyLen)){
					pKey = GetPrintableString(byKey, KeyLen);
				}
				if(k2h_find_get_value(k2hfhandle, &byVal, &ValLen)){
					pValue = GetPrintableString(byVal, ValLen);
				}
				PRN("+\"%s\" => \"%s\"", pKey ? pKey : "(null)", pValue ? pValue : "(null)");

				// subkey
				K2HSubKeys*	pSubKeys = NULL;
				char**		ppSKeyArray;
				if(NULL != (ppSKeyArray = k2h_find_get_str_direct_subkeys(k2hfhandle))){
					pSubKeys = new K2HSubKeys();
					for(char** pptmp = ppSKeyArray; pptmp && *pptmp; pptmp++){
						pSubKeys->insert(*pptmp);
					}
					k2h_free_keyarray(ppSKeyArray);
				}
				if(pSubKeys){
					string	strSubkeys = "";
					// cppcheck-suppress postfixOperator
					for(K2HSubKeys::iterator iter2 = pSubKeys->begin(); iter2 != pSubKeys->end(); iter2++){
						if(iter2->pSubKey){
							if(0 < strSubkeys.length()){
								strSubkeys += ", ";
							}
							string	strTmp;
							if(0 < iter2->length && '\0' == iter2->pSubKey[iter2->length - 1]){
								strTmp = reinterpret_cast<const char*>(iter2->pSubKey);
							}else{
								strTmp = string(reinterpret_cast<const char*>(iter2->pSubKey), iter2->length);
							}
							strSubkeys += strTmp;
						}else{
							// why?
						}
					}
					PRN("         subkeys: \"%s\"", strSubkeys.c_str());
					K2H_Delete(pSubKeys);
				}
				K2H_Free(byKey);
				K2H_Free(byVal);
				K2H_Free(pKey);
				K2H_Free(pValue);
			}

		}else{
			// cppcheck-suppress postfixOperator
			for(K2HShm::iterator iter = k2hash.begin(); iter != k2hash.end(); iter++){
				unsigned char*	byKey	= NULL;
				unsigned char*	byVal	= NULL;
				ssize_t			KeyLen	= 0;
				ssize_t			ValLen	= 0;
				char*			pKey	= NULL;
				char*			pValue	= NULL;
				if(0 < (KeyLen = k2hash.Get(*iter, &byKey, K2HShm::PAGEOBJ_KEY))){
					pKey = GetPrintableString(byKey, static_cast<size_t>(KeyLen));
				}
				if(0 < (ValLen = k2hash.Get(*iter, &byVal, K2HShm::PAGEOBJ_VALUE))){
					pValue = GetPrintableString(byVal, static_cast<size_t>(ValLen));
				}
				PRN("+\"%s\" => \"%s\"", pKey ? pKey : "(null)", pValue ? pValue : "(null)");

				// subkey
				K2HSubKeys*	pSubKeys;
				if(NULL != (pSubKeys = k2hash.GetSubKeys(pKey))){
					string	strSubkeys = "";
					// cppcheck-suppress postfixOperator
					for(K2HSubKeys::iterator iter2 = pSubKeys->begin(); iter2 != pSubKeys->end(); iter2++){
						if(iter2->pSubKey){
							if(0 < strSubkeys.length()){
								strSubkeys += ", ";
							}
							string	strTmp;
							if(0 < iter2->length && '\0' == iter2->pSubKey[iter2->length - 1]){
								strTmp = reinterpret_cast<const char*>(iter2->pSubKey);
							}else{
								strTmp = string(reinterpret_cast<const char*>(iter2->pSubKey), iter2->length);
							}
							strSubkeys += strTmp;
						}else{
							// why?
						}
					}
					PRN("         subkeys: \"%s\"", strSubkeys.c_str());
					K2H_Delete(pSubKeys);
				}
				K2H_Free(byKey);
				K2H_Free(byVal);
				K2H_Free(pKey);
				K2H_Free(pValue);
			}
		}
	}
	return true;
}

static bool StreamCommand(K2HShm& k2hash, params_t& params)
{
	if(2 != params.size()){
		if(2 < params.size()){
			ERR("Unknown parameter(%s) for stream command.", params[2].c_str());
		}else{
			ERR("stream command needs two parameters(key and input/output).");
		}
		return true;	// for continue.
	}

	// check mode
	ConsoleInput	interactiveIF;
	bool			isInput = false;
	if(0 == strcasecmp(params[1].c_str(), "input")){
		isInput = true;
		interactiveIF.SetPrompt(">> ");
	}else if(0 == strcasecmp(params[1].c_str(), "output")){
		isInput = false;
		interactiveIF.SetPrompt("<< ");
	}else{
		ERR("Unknown parameter(%s) for archive command.", params[1].c_str());
		return true;	// for continue.
	}

	// do
	if(isModeCAPI){
		ERR("Not implement this function for C API.");
		return true;
	}else{
		if(isInput){
			PRN("");
			PRN("*** Input stream test : \"%s\" key *********************", params[0].c_str());
			PRN(" You can specify below value types for reading value from stream.");
			PRN("   - \"string\", \"char\", \"int\", \"long\"");
			PRN(" Specify \".\" only to exit this interactive mode.");
			PRN("********************************************************");
			PRN("");

			// open stream
			ik2hstream	strm(&k2hash, params[0].c_str());
			if(strm.eof()){
				ERR("Could not open key(%s) or empty value.", params[0].c_str());
				return true;
			}

			// loop
			while(true){
				if(strm.eof()){
					PRN("\n*** Input key(%s) is reached to eof.", params[0].c_str());
					break;
				}

				if(!interactiveIF.GetCommand()){
					ERR("Something error occurred while reading stdin: err(%d).", interactiveIF.LastErrno());
					return false;
				}
				string strOneLine = interactiveIF.c_str();

				if(strOneLine == "."){
					break;

				}else if(0 == strcasecmp(strOneLine.c_str(), "char")){
					char	data;
					strm >> data;
					printf(" char = \"%c\"\n", data);
				}else if(0 == strcasecmp(strOneLine.c_str(), "int")){
					int	data;
					strm >> data;
					printf(" int = \"%d(%x)\"\n", data, data);

				}else if(0 == strcasecmp(strOneLine.c_str(), "long")){
					long	data;
					strm >> data;
					printf(" long = \"%ld(0x%lx)\"\n", data, static_cast<unsigned long>(data));

				}else if(0 == strcasecmp(strOneLine.c_str(), "string")){
					string	data;
					strm >> data;
					printf(" string = \"%s\"\n", data.c_str());

				}else{
					ERR("Unknown value type(%s).", strOneLine.c_str());
				}
			}	// while

		}else{
			PRN("");
			PRN("*** Output stream test : \"%s\" key *********************", params[0].c_str());
			PRN(" You can specify any string for writing value to stream.");
			PRN(" The string does not terminate null character.");
			PRN(" If you need to terminate it, specify \"ends\".");
			PRN(" If you specify \"endl\", puts 0x0a.");
			PRN(" Specify \".\" only to exit this interactive mode.");
			PRN("********************************************************");
			PRN("");

			// open stream
			ok2hstream	strm(&k2hash, params[0].c_str());

			// loop
			while(true){
				if(!interactiveIF.GetCommand()){
					ERR("Something error occurred while reading stdin: err(%d).", interactiveIF.LastErrno());
					return false;
				}
				string strOneLine = interactiveIF.c_str();

				if(strOneLine == "endl"){
					strm << std::endl;
				}else if(strOneLine == "ends"){
					strm << std::ends;
				}else if(strOneLine == "."){
					break;
				}else{
					strm << strOneLine;
				}
			}	// while
		}
	}
	return true;
}

static bool HistoryCommand(const ConsoleInput& InputIF)
{
	const strarr_t&	history = InputIF.GetAllHistory();

	int	nCnt = 1;
	// cppcheck-suppress postfixOperator
	for(strarr_t::const_iterator iter = history.begin(); iter != history.end(); iter++, nCnt++){
		PRN(" %d  %s", nCnt, iter->c_str());
	}
	return true;
}

static bool SaveCommand(const ConsoleInput& InputIF, params_t& params)
{
	int	fd;
	if(-1 == (fd = open(params[0].c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644))){
		ERR("Could not open file(%s) for writing history. errno(%d)", params[0].c_str(), errno);
		return true;	// for continue
	}

	const strarr_t&	history = InputIF.GetAllHistory();
	// cppcheck-suppress postfixOperator
	for(strarr_t::const_iterator iter = history.begin(); iter != history.end(); iter++){
		// check except command for writing file
		if(	0 == strncasecmp(iter->c_str(), "his",		strlen("his"))		||
			0 == strncasecmp(iter->c_str(), "history",	strlen("history"))	||
			0 == strncasecmp(iter->c_str(), "shell",	strlen("shell"))	||
			0 == strncasecmp(iter->c_str(), "sh",		strlen("sh"))		||
			0 == strncasecmp(iter->c_str(), "!!",		strlen("!!"))		||
			0 == strncasecmp(iter->c_str(), "save",		strlen("save"))		||
			0 == strncasecmp(iter->c_str(), "load",		strlen("load"))		||
			iter->at(0) == '!' )
		{
			continue;
		}
		const char*	pHistory;
		size_t		wrote_byte;
		ssize_t		one_wrote_byte;
		for(pHistory = iter->c_str(), wrote_byte = 0, one_wrote_byte = 0L; wrote_byte < iter->length(); wrote_byte += one_wrote_byte){
			if(-1 == (one_wrote_byte = write(fd, &pHistory[wrote_byte], (iter->length() - wrote_byte)))){
				ERR("Failed writing history to file(%s). errno(%d)", params[0].c_str(), errno);
				K2H_CLOSE(fd);
				return true;	// for continue
			}
		}
		if(-1 == write(fd, "\n", 1)){
			ERR("Failed writing history to file(%s). errno(%d)", params[0].c_str(), errno);
			K2H_CLOSE(fd);
			return true;	// for continue
		}
	}
	K2H_CLOSE(fd);
	return true;
}

static bool LoadCommand(K2HShm& k2hash, ConsoleInput& InputIF, params_t& params, bool& is_exit)
{
	int	fd;
	if(-1 == (fd = open(params[0].c_str(), O_RDONLY))){
		ERR("Could not open file(%s) for reading commands. errno(%d)", params[0].c_str(), errno);
		return true;	// for continue
	}

	string	CommandLine;
	for(bool ReadResult = true; ReadResult; ){
		ReadResult = ReadLine(fd, CommandLine);
		if(0 < CommandLine.length()){
			PRN("> %s", CommandLine.c_str());

			// reentrant
			if(!CommandStringHandle(k2hash, InputIF, CommandLine.c_str(), is_exit)){
				ERR("Something error occurred at loading command file(%s) - \"%s\", so stop running.", params[0].c_str(), CommandLine.c_str());
				break;
			}
			if(is_exit){
				break;
			}
		}
	}
	K2H_CLOSE(fd);
	return true;
}

static bool ExtractExpireParameter(params_t& params, time_t& expire)
{
	for(params_t::iterator iter = params.begin(); iter != params.end(); ++iter){
		if(0 == strncasecmp(iter->c_str(), "expire=", 7)){
			string	strtmp = iter->substr(7);
			expire = static_cast<time_t>(atoi(strtmp.c_str()));
			params.erase(iter);
			if(expire <= 0){
				ERR("expire parameter must be number over 0.");
				return false;
			}
			return true;
		}
	}
	return false;
}

static bool TransCommand(K2HShm& k2hash, params_t& params)
{
	if(0 == strcasecmp(params[0].c_str(), "ON")){
		time_t		expire		= 0;
		time_t*		pexpire		= NULL;
		if(ExtractExpireParameter(params, expire)){
			pexpire = &expire;
		}

		const char*	pTransFile	= NULL;
		const char*	pPrefix		= NULL;
		const char*	pParam		= NULL;
		if(1 == params.size()){
			PRN("[MSG] trans ON command without transaction file path.");
		}else if(2 == params.size()){
			PRN("[MSG] trans ON command with transaction file path and without prefix.");
			pTransFile	= params[1].c_str();
		}else if(3 == params.size()){
			PRN("[MSG] trans ON command with transaction file path and prefix.");
			pTransFile	= params[1].c_str();
			pPrefix		= params[2].c_str();
		}else if(4 == params.size()){
			PRN("[MSG] trans ON command with transaction file path, prefix and param.");
			pTransFile	= params[1].c_str();
			pPrefix		= params[2].c_str();
			pParam		= params[3].c_str();
		}else if(4 < params.size()){
			ERR("Unknown parameter(%s) for trans ON command.", params[2].c_str());
			return true;	// for continue.
		}

		if(isModeCAPI){
			if(!k2h_enable_transaction_param_we(reinterpret_cast<k2h_h>(&k2hash), pTransFile, reinterpret_cast<const unsigned char*>(pPrefix), (pPrefix ? strlen(pPrefix) : 0), reinterpret_cast<const unsigned char*>(pParam), (pParam ? strlen(pParam) + 1 : 0), pexpire)){
				ERR("Something error occurred by setting transaction.");
				return true;	// for continue.
			}
		}else{
			if(!k2hash.EnableTransaction(pTransFile, reinterpret_cast<const unsigned char*>(pPrefix), (pPrefix ? strlen(pPrefix) : 0), reinterpret_cast<const unsigned char*>(pParam), (pParam ? strlen(pParam) + 1 : 0), pexpire)){	// not set nil for prefix
				ERR("Something error occurred by setting transaction.");
				return true;	// for continue.
			}
		}

	}else if(0 == strcasecmp(params[0].c_str(), "OFF")){
		if(1 < params.size()){
			ERR("Unknown parameter(%s) for trans OFF command.", params[1].c_str());
			return true;	// for continue.
		}

		if(isModeCAPI){
			if(!k2h_disable_transaction(reinterpret_cast<k2h_h>(&k2hash))){
				ERR("Something error occurred by setting transaction.");
				return true;	// for continue.
			}
		}else{
			if(!k2hash.DisableTransaction()){
				ERR("Something error occurred by setting transaction.");
				return true;	// for continue.
			}
		}
	}else{
		ERR("Unknown parameter(%s) for trans command.", params[0].c_str());
		return true;	// for continue.
	}

	// print transaction version
	PRN(" Transaction function version: %s", K2H_TRANS_VER_FUNC());
	PRN("");
	return true;
}

#if	0	// this is one of calling pattern C DSO I/F
static bool TransCommand(K2HShm& k2hash, params_t& params)
{
	TRANSOPT	transopt;
	if(0 == strcasecmp(params[0].c_str(), "ON")){
		time_t		expire		= 0;
		time_t*		pexpire		= NULL;
		if(ExtractExpireParameter(params, expire)){
			pexpire = &expire;
		}

		const char*	pTransFile	= NULL;
		const char*	pPrefix		= NULL;
		const char*	pParam		= NULL;
		if(1 == params.size()){
			PRN("[MSG] trans ON command without transaction file path.");
		}else if(2 == params.size()){
			PRN("[MSG] trans ON command with transaction file path and without prefix.");
			pTransFile	= params[1].c_str();
		}else if(3 == params.size()){
			PRN("[MSG] trans ON command with transaction file path and prefix.");
			pTransFile	= params[1].c_str();
			pPrefix =	params[2].c_str();
		}else if(4 == params.size()){
			PRN("[MSG] trans ON command with transaction file path, prefix and param.");
			pTransFile	= params[1].c_str();
			pPrefix		= params[2].c_str();
			pParam		= params[3].c_str();
		}else if(4 < params.size()){
			ERR("Unknown parameter(%s) for trans ON command.", params[2].c_str());
			return true;	// for continue.
		}

		if(pTransFile && (MAX_TRANSACTION_FILEPATH - 1) < strlen(pTransFile)){
			ERR("file path(%s) is too long, file path must be small as %d byte.", pTransFile, (MAX_TRANSACTION_FILEPATH - 1));
			return true;	// for continue.
		}
		if(pTransFile){
			strcpy(transopt.szFilePath, pTransFile);
		}else{
			transopt.szFilePath[0] = '\0';
		}
		transopt.isEnable = true;

		if(pPrefix && (MAX_TRANSACTION_PREFIX - 1) < strlen(pPrefix)){
			ERR("prefix(%s) is too long, file path must be small as %d byte.", pPrefix, (MAX_TRANSACTION_PREFIX - 1));
			return true;	// for continue.
		}
		if(pPrefix){
			transopt.PrefixLength	= strlen(pPrefix);						// not set nil
			memcpy(transopt.byTransPrefix, pPrefix, transopt.PrefixLength);
		}else{
			transopt.PrefixLength	= 0;
			memset(transopt.byTransPrefix, 0, MAX_TRANSACTION_PREFIX);
		}

		if(pParam && (MAX_TRANSACTION_PARAM - 1) < strlen(pParam)){
			ERR("param(%s) is too long, file path must be small as %d byte.", pParam, (MAX_TRANSACTION_PARAM - 1));
			return true;	// for continue.
		}
		if(pParam){
			transopt.ParamLength	= strlen(pParam) + 1;
			memcpy(transopt.byTransParam, pParam, transopt.ParamLength);
		}else{
			transopt.ParamLength	= 0;
			memset(transopt.byTransParam, 0, MAX_TRANSACTION_PARAM);
		}

	}else if(0 == strcasecmp(params[0].c_str(), "OFF")){
		if(1 < params.size()){
			ERR("Unknown parameter(%s) for trans OFF command.", params[1].c_str());
			return true;	// for continue.
		}
		transopt.szFilePath[0]	= '\0';
		transopt.isEnable		= false;
		transopt.PrefixLength	= 0;
		transopt.ParamLength	= 0;
		memset(transopt.byTransPrefix, 0, MAX_TRANSACTION_PREFIX);
		memset(transopt.byTransParam, 0, MAX_TRANSACTION_PARAM);

	}else{
		ERR("Unknown parameter(%s) for trans command.", params[0].c_str());
		return true;	// for continue.
	}

	// print transaction version
	PRN(" Transaction function version: %s", K2H_TRANS_VER_FUNC());
	PRN("");

	// control
	if(!K2H_TRANS_CNTL_FUNC(reinterpret_cast<k2h_h>(&k2hash), &transopt)){
		ERR("Something error occurred by setting transaction.");
		return true;	// for continue.
	}
	return true;
}
#endif

static bool ThreadPoolCommand(K2HShm& k2hash, params_t& params)
{
	if(0 == params.size()){
		int	count;
		if(isModeCAPI){
			count = k2h_get_transaction_thread_pool();
		}else{
			count = K2HShm::GetTransThreadPool();
		}
		// print transaction thread pool count
		PRN(" Transaction Thread Pool count: %d", count);
		PRN("");

	}else if(1 == params.size()){
		int		count = atoi(params[0].c_str());
		bool	result;
		if(isModeCAPI){
			result = k2h_set_transaction_thread_pool(count);
		}else{
			result = K2HShm::SetTransThreadPool(count);
		}

		if(!result){
			ERR("Something error occurred by setting transaction thread pool count.");
			return true;	// for continue.
		}

		// print
		PRN(" Success to set Transaction Thread Pool count: %d", count);
		PRN("");

	}else{
		ERR("Unknown parameter(%s) for thread pool command.", params[1].c_str());
		return true;	// for continue.
	}

	return true;
}

static bool ArchiveCommand(K2HShm& k2hash, params_t& params)
{
	bool	isLoad;
	string	filepath;
	if(2 != params.size()){
		if(2 < params.size()){
			ERR("Unknown parameter(%s) for archive command.", params[2].c_str());
		}else{
			ERR("archive command needs archive file path as 2\'nd parameter.");
		}
		return true;	// for continue.
	}
	if(0 == strcasecmp(params[0].c_str(), "put")){
		isLoad = false;
	}else if(0 == strcasecmp(params[0].c_str(), "load")){
		isLoad = true;
	}else{
		ERR("Unknown parameter(%s) for archive command.", params[0].c_str());
		return true;	// for continue.
	}
	filepath = params[1].c_str();

	// archive
	if(isModeCAPI){
		bool result;
		if(isLoad){
			result = k2h_load_archive(reinterpret_cast<k2h_h>(&k2hash), filepath.c_str(), false);
		}else{
			result = k2h_put_archive(reinterpret_cast<k2h_h>(&k2hash), filepath.c_str(), false);
		}
		if(!result){
			ERR("Something error occurred during %s archive file(%s).", isLoad ? "loading" : "putting", filepath.c_str());
			return true;	// for continue.
		}
	}else{
		K2HArchive	archiveobj;
		if(!archiveobj.Initialize(filepath.c_str(), false)){
			ERR("Something error occurred about file path(%s).", filepath.c_str());
			return true;	// for continue.
		}
		if(!archiveobj.Serialize(&k2hash, isLoad)){
			ERR("Something error occurred during %s archive file(%s).", isLoad ? "loading" : "putting", filepath.c_str());
			return true;	// for continue.
		}
	}
	return true;
}

static bool QueueEmptySubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(1 != params.size()){
		if(1 < params.size()){
			ERR("Unknown parameter(%s) for queue empty command.", params[1].c_str());
		}else{
			ERR("queue empty command needs more parameter.");
		}
		return true;	// for continue.
	}

	// Queue empty
	if(isModeCAPI){
		k2h_q_h	qhandle;
		if(prefix){
			qhandle = k2h_q_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), true, prefix);	// fifo
		}else{
			qhandle = k2h_q_handle(reinterpret_cast<k2h_h>(&k2hash), true);						// fifo
		}
		if(K2H_INVALID_HANDLE == qhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		if(k2h_q_empty(qhandle)){
			PRN(" Queue is EMPTY.");
		}else{
			PRN(" Queue is NOT empty.");
		}
		PRN("");

		if(!k2h_q_free(qhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HQueue	queue;
		if(!queue.Init(&k2hash, true, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){	// fifo
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}
		if(queue.IsEmpty()){
			PRN(" Queue is EMPTY.");
		}else{
			PRN(" Queue is NOT empty.");
		}
		PRN("");
	}
	return true;
}

static bool QueueCountSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(1 != params.size()){
		if(1 < params.size()){
			ERR("Unknown parameter(%s) for queue count command.", params[1].c_str());
		}else{
			ERR("queue count command needs more parameter.");
		}
		return true;	// for continue.
	}

	// Queue count
	if(isModeCAPI){
		k2h_q_h	qhandle;
		if(prefix){
			qhandle = k2h_q_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), true, prefix);	// fifo
		}else{
			qhandle = k2h_q_handle(reinterpret_cast<k2h_h>(&k2hash), true);						// fifo
		}
		if(K2H_INVALID_HANDLE == qhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		int	count = k2h_q_count(qhandle);
		PRN(" Data count in Queue : %d", count);
		PRN("");

		if(!k2h_q_free(qhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HQueue	queue;
		if(!queue.Init(&k2hash, true, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){	// fifo
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		int	count = queue.GetCount();
		PRN(" Data count in Queue : %d", count);
		PRN("");
	}
	return true;
}

static bool QueueReadSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(3 != params.size() && 4 != params.size()){
		if(4 < params.size()){
			ERR("Unknown parameter(%s) for queue read command.", params[4].c_str());
		}else{
			ERR("queue read command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("queue read command has unknown parameter: %s", params[1].c_str());
	}
	int	pos = atoi(params[2].c_str());

	string		strpass;
	const char*	pass = NULL;
	if(4 == params.size()){
		if(0 == strncasecmp(params[3].c_str(), "pass=", 5)){
			strpass	= params[3].substr(5);
			pass	= strpass.c_str();
		}else{
			ERR("unknown parameter %s.", params[3].c_str());
			return true;		// for continue.
		}
	}

	// Queue Read
	if(isModeCAPI){
		k2h_q_h	qhandle;
		if(prefix){
			qhandle = k2h_q_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			qhandle = k2h_q_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == qhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		unsigned char*	pval	= NULL;
		size_t			vallen	= 0;
		if(!k2h_q_read_wp(qhandle, &pval, &vallen, pos, pass)){
			ERR("Something error occurred during reading queue.");
			k2h_q_free(qhandle);
			return true;	// for continue.
		}

		if(!pval || 0 == vallen){
			ERR("There is no read queue.");
			k2h_q_free(qhandle);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("READ QUEUE", pval, vallen)){
			ERR("Something error occurred during printing read queue.");
			K2H_Free(pval);
			k2h_q_free(qhandle);
			return true;	// for continue.
		}
		K2H_Free(pval);

		if(!k2h_q_free(qhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		unsigned char*	pval	= NULL;
		size_t			vallen	= 0;
		if(!queue.Read(&pval, vallen, pos, pass)){
			ERR("Something error occurred during reading queue.");
			return true;	// for continue.
		}

		if(!pval || 0 == vallen){
			ERR("There is no read queue.");
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("READ QUEUE", pval, vallen)){
			ERR("Something error occurred during printing read queue.");
			K2H_Free(pval);
			return true;	// for continue.
		}
		K2H_Free(pval);
	}
	return true;
}

static bool QueuePushSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(3 != params.size() && 4 != params.size() && 5 != params.size()){
		if(5 < params.size()){
			ERR("Unknown parameter(%s) for queue push command.", params[5].c_str());
		}else{
			ERR("queue push command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("queue push command has unknown parameter: %s", params[1].c_str());
	}
	string	strvalue = params[2];

	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress unreadVariable
	time_t		expire	= 0;
	string		pass;
	const char*	ppass	= NULL;
	time_t*		pexpire	= NULL;
	for(size_t pos = 3; pos < params.size(); ++pos){
		if(0 == strncasecmp(params[pos].c_str(), "pass=", 5)){
			pass = params[pos].substr(5);
			ppass = pass.c_str();
		}else if(0 == strncasecmp(params[pos].c_str(), "expire=", 7)){
			string	strtmp = params[pos].substr(7);
			expire = static_cast<time_t>(atoi(strtmp.c_str()));
			if(expire <= 0){
				ERR("expire parameter must be number over 0.");
				return true;	// for continue.
			}
			pexpire = &expire;
		}else{
			ERR("unknown parameter %s.", params[pos].c_str());
			return true;		// for continue.
		}
	}

	// Queue PUSH
	if(isModeCAPI){
		k2h_q_h	qhandle;
		if(prefix){
			qhandle = k2h_q_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			qhandle = k2h_q_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == qhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		if(!k2h_q_str_push_wa(qhandle, strvalue.c_str(), NULL, 0, ppass, pexpire)){
			ERR("Something error occurred during pushing queue.");
			k2h_q_free(qhandle);
			return true;	// for continue.
		}

		if(!k2h_q_free(qhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}
		if(!queue.Push(reinterpret_cast<const unsigned char*>(strvalue.c_str()), strvalue.size() + 1, NULL, ppass, pexpire)){
			ERR("Something error occurred during pushing queue.");
			return true;	// for continue.
		}
	}
	return true;
}

static bool QueuePopSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(2 != params.size() && 3 != params.size()){
		if(3 < params.size()){
			ERR("Unknown parameter(%s) for queue pop command.", params[3].c_str());
		}else{
			ERR("queue pop command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("queue pop command has unknown parameter: %s", params[1].c_str());
	}

	string		strpass;
	const char*	pass = NULL;
	if(3 == params.size()){
		if(0 == strncasecmp(params[2].c_str(), "pass=", 5)){
			strpass	= params[2].substr(5);
			pass	= strpass.c_str();
		}else{
			ERR("unknown parameter %s.", params[2].c_str());
			return true;		// for continue.
		}
	}

	// Queue POP
	if(isModeCAPI){
		k2h_q_h	qhandle;
		if(prefix){
			qhandle = k2h_q_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			qhandle = k2h_q_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == qhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		unsigned char*	pval	= NULL;
		size_t			vallen	= 0;
		if(!k2h_q_pop_wp(qhandle, &pval, &vallen, pass)){
			ERR("Something error occurred during popping queue.");
			k2h_q_free(qhandle);
			return true;	// for continue.
		}

		if(!pval || 0 == vallen){
			ERR("There is no popped queue.");
			k2h_q_free(qhandle);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("POPPED QUEUE", pval, vallen)){
			ERR("Something error occurred during printing popped queue.");
			K2H_Free(pval);
			k2h_q_free(qhandle);
			return true;	// for continue.
		}
		K2H_Free(pval);

		if(!k2h_q_free(qhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		unsigned char*	pval	= NULL;
		size_t			vallen	= 0;
		if(!queue.Pop(&pval, vallen, NULL, pass)){
			ERR("Something error occurred during popping queue.");
			return true;	// for continue.
		}

		if(!pval || 0 == vallen){
			ERR("There is no popped queue.");
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("POPPED QUEUE", pval, vallen)){
			ERR("Something error occurred during printing popped queue.");
			K2H_Free(pval);
			return true;	// for continue.
		}
		K2H_Free(pval);
	}
	return true;
}

static bool QueueDumpSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(2 != params.size()){
		if(2 < params.size()){
			ERR("Unknown parameter(%s) for queue dump command.", params[2].c_str());
		}else{
			ERR("queue dump command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("queue dump command has unknown parameter: %s", params[1].c_str());
	}

	// Queue DUMP
	if(isModeCAPI){
		k2h_q_h	qhandle;
		if(prefix){
			qhandle = k2h_q_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			qhandle = k2h_q_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == qhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		if(!k2h_q_dump(qhandle, NULL)){
			ERR("Something error occurred during dumping queue.");
			k2h_q_free(qhandle);
			return true;	// for continue.
		}
		if(!k2h_q_free(qhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		if(!queue.Dump(stdout)){
			ERR("Something error occurred during dumping queue.");
			return true;	// for continue.
		}
	}
	return true;
}

static K2HQRMCBRES QueueRemoveCallback(const unsigned char* bydata, size_t datalen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData)
{
	if(!bydata || 0 == datalen){
		return K2HQRMCB_RES_ERROR;
	}

	PRN("\n");
	if(!BinaryDumpUtility("REMOVING QUEUED DATA(KEY)", bydata, datalen)){
		ERR("Something error occurred during callback.");
		return K2HQRMCB_RES_ERROR;
	}
	PRN("REMOVING QUEUED ATTRIBUTE = {");
	for(int cnt = 0; cnt < attrscnt; ++cnt){
		char*	pAttrKey = GetPrintableString(pattrs[cnt].pkey, pattrs[cnt].keylength);
		if(!BinaryDumpUtility(pAttrKey, pattrs[cnt].pval, pattrs[cnt].vallength)){
			ERR("Something error occurred during callback.");
			K2H_Free(pAttrKey);
			return K2HQRMCB_RES_ERROR;
		}
		K2H_Free(pAttrKey);
	}
	PRN("}");

	bool	is_remove	= false;
	bool	is_break	= false;
	char	szBuff[1024];
	while(true){
		fprintf(stdout, "Do you remove this queued data? [y/n/b] (y) -> ");
		if(NULL == fgets(szBuff, sizeof(szBuff) - 1, stdin)){
			ERR("Something error occurred during callback.");
			return K2HQRMCB_RES_ERROR;
		}
		if(strlen(szBuff) <= sizeof(szBuff) - 1){
			if(0 < strlen(szBuff)){
				if('\n' == szBuff[strlen(szBuff) - 1]){
					szBuff[strlen(szBuff) - 1] = '\0';
				}
			}
		}else{
			szBuff[sizeof(szBuff) - 1] = '\0';
		}

		if(0 == strcasecmp(szBuff, "y") || 0 == strcasecmp(szBuff, "yes") || 0 == strcasecmp(szBuff, "true")){
			is_remove = true;
			break;
		}else if(0 == strcasecmp(szBuff, "n") || 0 == strcasecmp(szBuff, "no") || 0 == strcasecmp(szBuff, "false")){
			is_remove = false;
			break;
		}else if(0 == strcasecmp(szBuff, "b") || 0 == strcasecmp(szBuff, "break") || 0 == strcasecmp(szBuff, "stop")){
			is_break = true;
			break;
		}
	}

	K2HQRMCBRES	res = K2HQRMCB_RES_ERROR;
	if(is_remove){
		if(is_break){
			res = K2HQRMCB_RES_FIN_RM;
		}else{
			res = K2HQRMCB_RES_CON_RM;
		}
	}else{
		if(is_break){
			res = K2HQRMCB_RES_FIN_NOTRM;
		}else{
			res = K2HQRMCB_RES_CON_NOTRM;
		}
	}
	return res;
}

static bool QueueRemoveSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(3 != params.size() && 4 != params.size() && 5 != params.size()){
		if(5 < params.size()){
			ERR("Unknown parameter(%s) for queue remove command.", params[5].c_str());
		}else{
			ERR("queue remove command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("queue remove command has unknown parameter: %s", params[1].c_str());
	}

	int	count = atoi(params[2].c_str());
	if(count <= 0){
		ERR("queue remove count is ignore.");
		return true;	// for continue.
	}

	string		pass;
	const char*	ppass		= NULL;
	bool		is_confirm	= false;
	for(size_t cnt = 3; cnt < params.size(); ++cnt){
		if(0 == strncasecmp(params[cnt].c_str(), "pass=", 5)){
			pass = params[cnt].substr(5);
			ppass = pass.c_str();
		}else if(0 == strcasecmp(params[cnt].c_str(), "c") || 0 == strcasecmp(params[cnt].c_str(), "confirm")){
			is_confirm = true;
		}else{
			ERR("unknown parameter %s.", params[cnt].c_str());
			return true;		// for continue.
		}
	}

	// Queue REMOVE
	if(isModeCAPI){
		k2h_q_h	qhandle;
		if(prefix){
			qhandle = k2h_q_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			qhandle = k2h_q_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == qhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		if(is_confirm){
			if(!k2h_q_remove_wp_ext(qhandle, count, QueueRemoveCallback, NULL, ppass)){
				ERR("Something error occurred during removing queue.");
				k2h_q_free(qhandle);
				return true;	// for continue.
			}
		}else{
			if(!k2h_q_remove_wp(qhandle, count, ppass)){
				ERR("Something error occurred during removing queue.");
				k2h_q_free(qhandle);
				return true;	// for continue.
			}
		}

		if(!k2h_q_free(qhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}
		if(is_confirm){
			if(-1 == queue.Remove(count, QueueRemoveCallback, NULL, ppass)){
				ERR("Something error occurred during removing queue.");
				return true;	// for continue.
			}
		}else{
			if(-1 == queue.Remove(count, NULL, NULL, ppass)){
				ERR("Something error occurred during removing queue.");
				return true;	// for continue.
			}
		}
	}
	return true;
}

static bool QueueCommand(K2HShm& k2hash, params_t& params)
{
	bool	bResult = false;

	// check sub command
	if(0 == strcasecmp(params[0].c_str(), "empty")){
		bResult = QueueEmptySubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "count")){
		bResult = QueueCountSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "read")){
		bResult = QueueReadSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "push")){
		bResult = QueuePushSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "pop")){
		bResult = QueuePopSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "dump")){
		bResult = QueueDumpSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "remove") || 0 == strcasecmp(params[0].c_str(), "rm")){
		bResult = QueueRemoveSubCommand(k2hash, NULL, params);

	}else if(2 <= params.size()){
		// case of first parameter is prefix
		string	strprefix = params.front();
		params.erase(params.begin());

		if(0 == strcasecmp(params[0].c_str(), "empty")){
			bResult = QueueEmptySubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "count")){
			bResult = QueueCountSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "read")){
			bResult = QueueReadSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "push")){
			bResult = QueuePushSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "pop")){
			bResult = QueuePopSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "dump")){
			bResult = QueueDumpSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "remove") || 0 == strcasecmp(params[0].c_str(), "rm")){
			bResult = QueueRemoveSubCommand(k2hash, strprefix.c_str(), params);

		}else{
			ERR("queue command needs sub command parameter.");
			bResult = true;
		}
	}else{
		ERR("queue command needs sub command parameter.");
		bResult = true;
	}
	return bResult;
}

static bool KeyQueueEmptySubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(1 != params.size()){
		if(1 < params.size()){
			ERR("Unknown parameter(%s) for key queue empty command.", params[1].c_str());
		}else{
			ERR("key queue empty command needs more parameter.");
		}
		return true;	// for continue.
	}

	// KeyQueue empty
	if(isModeCAPI){
		k2h_keyq_h	keyqhandle;
		if(prefix){
			keyqhandle = k2h_keyq_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), true, prefix);	// fifo
		}else{
			keyqhandle = k2h_keyq_handle(reinterpret_cast<k2h_h>(&k2hash), true);						// fifo
		}
		if(K2H_INVALID_HANDLE == keyqhandle){
			ERR("Something error occurred during initializing key queue.");
			return true;	// for continue.
		}

		if(k2h_keyq_empty(keyqhandle)){
			PRN(" Key Queue is EMPTY.");
		}else{
			PRN(" Key Queue is NOT empty.");
		}
		PRN("");

		if(!k2h_keyq_free(keyqhandle)){
			ERR("Something error occurred during closing key queue.");
			return true;	// for continue.
		}

	}else{
		K2HKeyQueue	queue;
		if(!queue.Init(&k2hash, true, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){	// fifo
			ERR("Something error occurred during initializing key queue.");
			return true;	// for continue.
		}
		if(queue.IsEmpty()){
			PRN(" Key Queue is EMPTY.");
		}else{
			PRN(" Key Queue is NOT empty.");
		}
		PRN("");
	}
	return true;
}

static bool KeyQueueCountSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(1 != params.size()){
		if(1 < params.size()){
			ERR("Unknown parameter(%s) for key queue count command.", params[1].c_str());
		}else{
			ERR("key queue count command needs more parameter.");
		}
		return true;	// for continue.
	}

	// KeyQueue count
	if(isModeCAPI){
		k2h_keyq_h	keyqhandle;
		if(prefix){
			keyqhandle = k2h_keyq_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), true, prefix);	// fifo
		}else{
			keyqhandle = k2h_keyq_handle(reinterpret_cast<k2h_h>(&k2hash), true);						// fifo
		}
		if(K2H_INVALID_HANDLE == keyqhandle){
			ERR("Something error occurred during initializing key queue.");
			return true;	// for continue.
		}

		int	count = k2h_keyq_count(keyqhandle);
		PRN(" Data count in Key Queue : %d", count);
		PRN("");

		if(!k2h_keyq_free(keyqhandle)){
			ERR("Something error occurred during closing key queue.");
			return true;	// for continue.
		}

	}else{
		K2HKeyQueue	queue;
		if(!queue.Init(&k2hash, true, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){	// fifo
			ERR("Something error occurred during initializing key queue.");
			return true;	// for continue.
		}

		int	count = queue.GetCount();
		PRN(" Data count in Key Queue : %d", count);
		PRN("");
	}
	return true;
}

static bool KeyQueueReadSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(3 != params.size() && 4 != params.size()){
		if(4 < params.size()){
			ERR("Unknown parameter(%s) for queue read command.", params[4].c_str());
		}else{
			ERR("queue read command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("queue read command has unknown parameter: %s", params[1].c_str());
	}
	int	pos = atoi(params[2].c_str());

	string		strpass;
	const char*	pass = NULL;
	if(4 == params.size()){
		if(0 == strncasecmp(params[3].c_str(), "pass=", 5)){
			strpass	= params[3].substr(5);
			pass	= strpass.c_str();
		}else{
			ERR("unknown parameter %s.", params[3].c_str());
			return true;		// for continue.
		}
	}

	// KeyQueue Read
	if(isModeCAPI){
		k2h_keyq_h	keyqhandle;
		if(prefix){
			keyqhandle = k2h_keyq_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			keyqhandle = k2h_keyq_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == keyqhandle){
			ERR("Something error occurred during initializing key queue.");
			return true;	// for continue.
		}

		unsigned char*	pkey	= NULL;
		size_t			keylen	= 0;
		unsigned char*	pval	= NULL;
		size_t			vallen	= 0;
		if(!k2h_keyq_read_keyval_wp(keyqhandle, &pkey, &keylen, &pval, &vallen, pos, pass)){
			ERR("Something error occurred during reading key queue.");
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}

		if(!pkey || 0 == keylen){
			ERR("There is no read queue.");
			K2H_Free(pkey);
			K2H_Free(pval);
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("READ QUEUE(KEY)  ", pkey, keylen)){
			ERR("Something error occurred during printing read key queue.");
			K2H_Free(pkey);
			K2H_Free(pval);
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}
		K2H_Free(pkey);

		if(!pval || 0 == vallen){
			ERR("There is no value.");
			K2H_Free(pval);
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("READ QUEUE(VALUE)", pval, vallen)){
			ERR("Something error occurred during printing read key queue.");
			K2H_Free(pval);
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}
		K2H_Free(pval);

		if(!k2h_keyq_free(keyqhandle)){
			ERR("Something error occurred during closing key queue.");
			return true;	// for continue.
		}

	}else{
		K2HKeyQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing key queue.");
			return true;	// for continue.
		}

		unsigned char*	pkey	= NULL;
		size_t			keylen	= 0;
		unsigned char*	pval	= NULL;
		size_t			vallen	= 0;
		if(!queue.Read(&pkey, keylen, &pval, vallen, pos, pass)){
			ERR("Something error occurred during reading key queue.");
			return true;	// for continue.
		}

		if(!pkey || 0 == keylen){
			ERR("There is no read queue.");
			K2H_Free(pkey);
			K2H_Free(pval);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("READ QUEUE(KEY)  ", pkey, keylen)){
			ERR("Something error occurred during printing read key queue.");
			K2H_Free(pkey);
			K2H_Free(pval);
			return true;	// for continue.
		}
		K2H_Free(pkey);

		if(!pval || 0 == vallen){
			ERR("There is no read queue.");
			K2H_Free(pval);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("READ QUEUE(VALUE)", pval, vallen)){
			ERR("Something error occurred during printing read key queue.");
			K2H_Free(pval);
			return true;	// for continue.
		}
		K2H_Free(pval);
	}
	return true;
}

static bool KeyQueuePushSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(4 != params.size() && 5 != params.size() && 6 != params.size()){
		if(6 < params.size()){
			ERR("Unknown parameter(%s) for queue push command.", params[6].c_str());
		}else{
			ERR("queue push command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("queue push command has unknown parameter: %s", params[1].c_str());
	}
	string	strkey	= params[2];
	string	strvalue= params[3];

	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress unreadVariable
	time_t		expire	= 0;
	string		pass;
	const char*	ppass	= NULL;
	time_t*		pexpire	= NULL;
	for(size_t pos = 4; pos < params.size(); ++pos){
		if(0 == strncasecmp(params[pos].c_str(), "pass=", 5)){
			pass = params[pos].substr(5);
			ppass = pass.c_str();
		}else if(0 == strncasecmp(params[pos].c_str(), "expire=", 7)){
			string	strtmp = params[pos].substr(7);
			expire = static_cast<time_t>(atoi(strtmp.c_str()));
			if(expire <= 0){
				ERR("expire parameter must be number over 0.");
				return true;	// for continue.
			}
			pexpire = &expire;
		}else{
			ERR("unknown parameter %s.", params[pos].c_str());
			return true;		// for continue.
		}
	}

	// Queue PUSH
	if(isModeCAPI){
		k2h_keyq_h	keyqhandle;
		if(prefix){
			keyqhandle = k2h_keyq_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			keyqhandle = k2h_keyq_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == keyqhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		if(!k2h_keyq_str_push_keyval_wa(keyqhandle, strkey.c_str(), strvalue.c_str(), ppass, pexpire)){
			ERR("Something error occurred during pushing queue.");
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}

		if(!k2h_keyq_free(keyqhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HKeyQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}
		if(!queue.Push(reinterpret_cast<const unsigned char*>(strkey.c_str()), strkey.size() + 1, reinterpret_cast<const unsigned char*>(strvalue.c_str()), strvalue.size() + 1, ppass, pexpire)){
			ERR("Something error occurred during pushing queue.");
			return true;	// for continue.
		}
	}
	return true;
}

static bool KeyQueuePopSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(2 != params.size() && 3 != params.size()){
		if(3 < params.size()){
			ERR("Unknown parameter(%s) for queue pop command.", params[3].c_str());
		}else{
			ERR("queue pop command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("queue pop command has unknown parameter: %s", params[1].c_str());
	}

	string		strpass;
	const char*	pass = NULL;
	if(3 == params.size()){
		if(0 == strncasecmp(params[2].c_str(), "pass=", 5)){
			strpass	= params[2].substr(5);
			pass	= strpass.c_str();
		}else{
			ERR("unknown parameter %s.", params[2].c_str());
			return true;		// for continue.
		}
	}

	// Queue POP
	if(isModeCAPI){
		k2h_keyq_h	keyqhandle;
		if(prefix){
			keyqhandle = k2h_keyq_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			keyqhandle = k2h_keyq_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == keyqhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		unsigned char*	pkey	= NULL;
		size_t			keylen	= 0;
		unsigned char*	pval	= NULL;
		size_t			vallen	= 0;
		if(!k2h_keyq_pop_keyval_wp(keyqhandle, &pkey, &keylen, &pval, &vallen, pass)){
			ERR("Something error occurred during popping queue.");
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}

		if(!pkey || 0 == keylen){
			ERR("There is no popped queue.");
			K2H_Free(pkey);
			K2H_Free(pval);
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("POPPED QUEUE(KEY)  ", pkey, keylen)){
			ERR("Something error occurred during printing popped queue.");
			K2H_Free(pkey);
			K2H_Free(pval);
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}
		K2H_Free(pkey);

		if(!pval || 0 == vallen){
			ERR("There is no value.");
			K2H_Free(pval);
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("POPPED QUEUE(VALUE)", pval, vallen)){
			ERR("Something error occurred during printing popped queue.");
			K2H_Free(pval);
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}
		K2H_Free(pval);

		if(!k2h_keyq_free(keyqhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HKeyQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		unsigned char*	pkey	= NULL;
		size_t			keylen	= 0;
		unsigned char*	pval	= NULL;
		size_t			vallen	= 0;
		if(!queue.Pop(&pkey, keylen, &pval, vallen, pass)){
			ERR("Something error occurred during popping queue.");
			return true;	// for continue.
		}

		if(!pkey || 0 == keylen){
			ERR("There is no popped queue.");
			K2H_Free(pkey);
			K2H_Free(pval);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("POPPED QUEUE(KEY)  ", pkey, keylen)){
			ERR("Something error occurred during printing popped queue.");
			K2H_Free(pkey);
			K2H_Free(pval);
			return true;	// for continue.
		}
		K2H_Free(pkey);

		if(!pval || 0 == vallen){
			ERR("There is no popped queue.");
			K2H_Free(pval);
			return true;	// for continue.
		}

		if(!BinaryDumpUtility("POPPED QUEUE(VALUE)", pval, vallen)){
			ERR("Something error occurred during printing popped queue.");
			K2H_Free(pval);
			return true;	// for continue.
		}
		K2H_Free(pval);
	}
	return true;
}

static bool KeyQueueDumpSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(2 != params.size()){
		if(2 < params.size()){
			ERR("Unknown parameter(%s) for queue dump command.", params[2].c_str());
		}else{
			ERR("queue dump command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("queue dump command has unknown parameter: %s", params[1].c_str());
	}

	// Queue DUMP
	if(isModeCAPI){
		k2h_keyq_h	keyqhandle;
		if(prefix){
			keyqhandle = k2h_keyq_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			keyqhandle = k2h_keyq_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == keyqhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		if(!k2h_keyq_dump(keyqhandle, NULL)){
			ERR("Something error occurred during dumping queue.");
			k2h_keyq_free(keyqhandle);
			return true;	// for continue.
		}
		if(!k2h_keyq_free(keyqhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HKeyQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		if(!queue.Dump(stdout)){
			ERR("Something error occurred during dumping queue.");
			return true;	// for continue.
		}
	}
	return true;
}

static bool KeyQueueRemoveSubCommand(K2HShm& k2hash, const char* prefix, params_t& params)
{
	if(4 != params.size() && 5 != params.size()){
		if(5 < params.size()){
			ERR("Unknown parameter(%s) for queue remove command.", params[5].c_str());
		}else{
			ERR("keyqueue remove command needs more parameter.");
		}
		return true;	// for continue.
	}

	bool	is_fifo = false;
	if(0 == strcasecmp(params[1].c_str(), "fifo")){
		is_fifo = true;
	}else if(0 == strcasecmp(params[1].c_str(), "lifo")){
		is_fifo = false;
	}else{
		ERR("keyqueue remove command has unknown parameter: %s", params[1].c_str());
	}

	int	count = atoi(params[2].c_str());
	if(count <= 0){
		ERR("queue remove count is ignore.");
		return true;	// for continue.
	}

	string		pass;
	const char*	ppass		= NULL;
	bool		is_confirm	= false;
	for(size_t cnt = 3; cnt < params.size(); ++cnt){
		if(0 == strncasecmp(params[cnt].c_str(), "pass=", 5)){
			pass = params[cnt].substr(5);
			ppass = pass.c_str();
		}else if(0 == strcasecmp(params[cnt].c_str(), "c") || 0 == strcasecmp(params[cnt].c_str(), "confirm")){
			is_confirm = true;
		}else{
			ERR("unknown parameter %s.", params[cnt].c_str());
			return true;		// for continue.
		}
	}

	// Queue REMOVE
	if(isModeCAPI){
		k2h_keyq_h	keyqhandle;
		if(prefix){
			keyqhandle = k2h_keyq_handle_str_prefix(reinterpret_cast<k2h_h>(&k2hash), is_fifo, prefix);
		}else{
			keyqhandle = k2h_keyq_handle(reinterpret_cast<k2h_h>(&k2hash), is_fifo);
		}
		if(K2H_INVALID_HANDLE == keyqhandle){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}

		if(is_confirm){
			if(-1 == k2h_keyq_remove_wp_ext(keyqhandle, count, QueueRemoveCallback, NULL, ppass)){
				ERR("Something error occurred during removing queue.");
				k2h_keyq_free(keyqhandle);
				return true;	// for continue.
			}
		}else{
			if(!k2h_keyq_remove_wp(keyqhandle, count, ppass)){
				ERR("Something error occurred during removing queue.");
				k2h_keyq_free(keyqhandle);
				return true;	// for continue.
			}
		}

		if(!k2h_keyq_free(keyqhandle)){
			ERR("Something error occurred during closing queue.");
			return true;	// for continue.
		}

	}else{
		K2HKeyQueue	queue;
		if(!queue.Init(&k2hash, is_fifo, reinterpret_cast<const unsigned char*>(prefix), (prefix ? strlen(prefix) : 0))){
			ERR("Something error occurred during initializing queue.");
			return true;	// for continue.
		}
		if(is_confirm){
			if(-1 == queue.Remove(count, QueueRemoveCallback, NULL, ppass)){
				ERR("Something error occurred during removing queue.");
				return true;	// for continue.
			}
		}else{
			if(-1 == queue.Remove(count, NULL, NULL, ppass)){
				ERR("Something error occurred during removing queue.");
				return true;	// for continue.
			}
		}
	}
	return true;
}

static bool KeyQueueCommand(K2HShm& k2hash, params_t& params)
{
	bool	bResult = false;

	// check sub command
	if(0 == strcasecmp(params[0].c_str(), "empty")){
		bResult = KeyQueueEmptySubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "count")){
		bResult = KeyQueueCountSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "read")){
		bResult = KeyQueueReadSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "push")){
		bResult = KeyQueuePushSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "pop")){
		bResult = KeyQueuePopSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "dump")){
		bResult = KeyQueueDumpSubCommand(k2hash, NULL, params);

	}else if(0 == strcasecmp(params[0].c_str(), "remove") || 0 == strcasecmp(params[0].c_str(), "rm")){
		bResult = KeyQueueRemoveSubCommand(k2hash, NULL, params);

	}else if(2 <= params.size()){
		// case of first parameter is prefix
		string	strprefix = params.front();
		params.erase(params.begin());

		if(0 == strcasecmp(params[0].c_str(), "empty")){
			bResult = KeyQueueEmptySubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "count")){
			bResult = KeyQueueCountSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "read")){
			bResult = KeyQueueReadSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "push")){
			bResult = KeyQueuePushSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "pop")){
			bResult = KeyQueuePopSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "dump")){
			bResult = KeyQueueDumpSubCommand(k2hash, strprefix.c_str(), params);

		}else if(0 == strcasecmp(params[0].c_str(), "remove") || 0 == strcasecmp(params[0].c_str(), "rm")){
			bResult = KeyQueueRemoveSubCommand(k2hash, strprefix.c_str(), params);

		}else{
			ERR("keyqueue command needs sub command parameter.");
			bResult = true;
		}
	}else{
		ERR("keyqueue command needs sub command parameter.");
		bResult = true;
	}
	return bResult;
}

static bool BultinAttrCommand(K2HShm& k2hash, params_t& params)
{
	bool*		pis_mtime	= NULL;
	bool*		pis_defenc	= NULL;
	const char*	ppassfile	= NULL;
	bool*		pis_history	= NULL;
	time_t*		pexpire		= NULL;

	bool		is_mtime	= true;
	bool		is_defenc	= true;
	string		passfile;
	bool		is_history	= true;

	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress unreadVariable
	time_t		expire		= 0;

	for(params_t::iterator iter = params.begin(); iter != params.end(); ++iter){
		if(0 == strcasecmp(iter->c_str(), "mtime")){
			pis_mtime = &is_mtime;

		}else if(0 == strcasecmp(iter->c_str(), "history")){
			pis_history = &is_history;

		}else if(0 == strcasecmp(iter->c_str(), "enc")){
			pis_defenc = &is_defenc;

		}else if(0 == strncasecmp(iter->c_str(), "expire=", 7)){
			string	strtmp = iter->substr(7);
			expire = static_cast<time_t>(atoi(strtmp.c_str()));
			if(expire <= 0){
				ERR("expire parameter must be number over 0.");
				return true;	// for continue.
			}
			pexpire = &expire;

		}else if(0 == strncasecmp(iter->c_str(), "pass=", 5)){
			passfile	= iter->substr(5);
			ppassfile	= passfile.c_str();
		}else{
			ERR("unknown parameter %s.", iter->c_str());
			return true;		// for continue.
		}
	}
	if(pis_defenc && !ppassfile){
		ERR("Must specify \"pass\" parameter when specifying \"enc\" parameter.");
		return true;		// for continue.
	}

	// set
	if(isModeCAPI){
		if(!k2h_set_common_attr(reinterpret_cast<k2h_h>(&k2hash), pis_mtime, pis_defenc, ppassfile, pis_history, pexpire)){
			ERR("Something error occurred during setting builtin attribute.");
		}
	}else{
		if(!k2hash.SetCommonAttribute(pis_mtime, pis_defenc, ppassfile, pis_history, pexpire, NULL)){
			ERR("Something error occurred during setting builtin attribute.");
		}
	}
	return true;
}

static bool LoadPluginAttrCommand(K2HShm& k2hash, params_t& params)
{
	if(1 != params.size()){
		ERR("loadpluginattr command need one parameter.");
		return true;		// for continue.
	}

	// set
	if(isModeCAPI){
		if(!k2h_add_attr_plugin_library(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str())){
			ERR("Something error occurred during loading attribute library.");
		}
	}else{
		if(!k2hash.AddAttrPluginLib(params[0].c_str())){
			ERR("Something error occurred during loading attribute library.");
		}
	}
	return true;
}

static bool AddPassPhraseCommand(K2HShm& k2hash, params_t& params)
{
	if(1 > params.size()){
		ERR("addpassphrase command need one parameter.");
		return true;		// for continue.
	}
	bool	IsDefault = false;
	if(1 < params.size()){
		if(0 == strcasecmp(params[1].c_str(), "default")){
			IsDefault = true;
		}else{
			ERR("unknown parameter %s.", params[1].c_str());
			return true;		// for continue.
		}
	}
	// set
	if(isModeCAPI){
		if(!k2h_add_attr_crypt_pass(reinterpret_cast<k2h_h>(&k2hash), params[0].c_str(), IsDefault)){
			ERR("Something error occurred during adding pass phrase for crypt.");
		}
	}else{
		if(!k2hash.AddAttrCryptPass(params[0].c_str(), IsDefault)){
			ERR("Something error occurred during adding pass phrase for crypt.");
		}
	}
	return true;
}

static bool CleanAllAttrCommand(K2HShm& k2hash, params_t& params)
{
	if(1 <= params.size()){
		ERR("unknown parameter %s.", params[0].c_str());
		return true;		// for continue.
	}
	// set
	if(isModeCAPI){
		if(!k2h_clean_common_attr(reinterpret_cast<k2h_h>(&k2hash))){
			ERR("Something error occurred during cleaning all attribute.");
		}
	}else{
		if(!k2hash.CleanCommonAttribute()){
			ERR("Something error occurred during cleaning all attribute.");
		}
	}
	return true;
}

static bool ShellCommand(void)
{
	static const char*	pDefaultShell = "/bin/sh";

	if(0 == system(NULL)){
		ERR("Could not execute shell.");
		return true;	// for continue
	}

	const char*	pEnvShell = getenv("SHELL");
	if(!pEnvShell){
		pEnvShell = pDefaultShell;
	}
	if(-1 == system(pEnvShell)){
		ERR("Something error occurred by executing shell(%s).", pEnvShell);
		return true;	// for continue
	}
	return true;
}

static bool EchoCommand(params_t& params)
{
	string	strDisp("");
	for(size_t cnt = 0; cnt < params.size(); ++cnt){
		if(0 < cnt){
			strDisp += ' ';
		}
		strDisp += params[cnt];
	}
	if(!strDisp.empty()){
		PRN("%s", strDisp.c_str());
	}
	return true;
}

static bool SleepCommand(params_t& params)
{
	if(1 != params.size()){
		if(1 < params.size()){
			ERR("unknown parameter %s.", params[1].c_str());
		}else{
			ERR("sleep command needs parameter.");
		}
		return true;		// for continue.
	}
	unsigned int	sec = static_cast<unsigned int>(atoi(params[0].c_str()));
	sleep(sec);
	return true;
}

static bool CommandStringHandle(K2HShm& k2hash, ConsoleInput& InputIF, const char* pCommand, bool& is_exit)
{
	is_exit = false;

	if(ISEMPTYSTR(pCommand)){
		return true;
	}

	option_t	opts;
	if(!LineOptionParser(pCommand, opts)){
		return true;	// for continue.
	}
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress stlSize
	if(0 == opts.size()){
		return true;
	}

	// Command switch
	if(opts.end() != opts.find("help") || opts.end() != opts.find("h")){
		LineHelp();
	}else if(opts.end() != opts.find("quit") || opts.end() != opts.find("q") || opts.end() != opts.find("exit")){
		PRN("Quit & Detach.");

		// normal exit
		is_exit = true;
	}else if(opts.end() != opts.find("info") || opts.end() != opts.find("i")){
		LapTime	laptime;
		if(!InfoCommand(k2hash, opts["info"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("dump") || opts.end() != opts.find("d")){
		LapTime	laptime;
		if(!DumpCommand(k2hash, opts["dump"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("set") || opts.end() != opts.find("s")){
		LapTime	laptime;
		if(!SetCommand(k2hash, opts["set"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("settrial") || opts.end() != opts.find("st")){
		LapTime	laptime;
		if(!SetTrialCommand(k2hash, opts["settrial"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("setsub")){
		LapTime	laptime;
		if(!SetSubkeyCommand(k2hash, opts["setsub"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("directset") || opts.end() != opts.find("dset")){
		LapTime	laptime;
		if(!DirectSetCommand(k2hash, opts["directset"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("setf") || opts.end() != opts.find("sf")){
		LapTime	laptime;
		if(!DirectSetFileCommand(k2hash, opts["setf"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("fill") || opts.end() != opts.find("f")){
		LapTime	laptime;
		if(!FillCommand(k2hash, opts["fill"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("fillsub")){
		LapTime	laptime;
		if(!FillSubkeyCommand(k2hash, opts["fillsub"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("rm")){
		LapTime	laptime;
		if(!RemoveCommand(k2hash, opts["rm"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("rmsub")){
		LapTime	laptime;
		if(!RemoveSubkeyCommand(k2hash, opts["rmsub"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("rename")){
		LapTime	laptime;
		if(!RenameCommand(k2hash, opts["rename"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("print") || opts.end() != opts.find("p")){
		LapTime	laptime;
		if(!PrintCommand(k2hash, opts["print"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("printattr")){
		LapTime	laptime;
		if(!PrintAttrCommand(k2hash, opts["printattr"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("addattr")){
		LapTime	laptime;
		if(!AddAttrCommand(k2hash, opts["addattr"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("directprint") || opts.end() != opts.find("dp")){
		LapTime	laptime;
		if(!DirectPrintCommand(k2hash, opts["directprint"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("directsave") || opts.end() != opts.find("dsave")){
		LapTime	laptime;
		if(!DirectSaveCommand(k2hash, opts["directsave"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("directload") || opts.end() != opts.find("dload")){
		LapTime	laptime;
		if(!DirectLoadCommand(k2hash, opts["directload"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("copyfile") || opts.end() != opts.find("cf")){
		LapTime	laptime;
		if(!DirectCopyFileCommand(k2hash, opts["copyfile"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("list") || opts.end() != opts.find("l")){
		LapTime	laptime;
		if(!ListCommand(k2hash, opts["list"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("stream") || opts.end() != opts.find("str")){
		LapTime	laptime;
		if(!StreamCommand(k2hash, opts["stream"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("history") || opts.end() != opts.find("his")){
		if(!HistoryCommand(InputIF)){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("save")){
		if(!SaveCommand(InputIF, opts["save"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("load")){
		if(!LoadCommand(k2hash, InputIF, opts["load"], is_exit)){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("trans")){
		if(!TransCommand(k2hash, opts["trans"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("threadpool")){
		if(!ThreadPoolCommand(k2hash, opts["threadpool"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("archive")){
		if(!ArchiveCommand(k2hash, opts["archive"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("queue")){
		if(!QueueCommand(k2hash, opts["queue"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("keyqueue")){
		if(!KeyQueueCommand(k2hash, opts["keyqueue"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("builtinattr")){
		if(!BultinAttrCommand(k2hash, opts["builtinattr"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("loadpluginattr")){
		if(!LoadPluginAttrCommand(k2hash, opts["loadpluginattr"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("addpassphrase")){
		if(!AddPassPhraseCommand(k2hash, opts["addpassphrase"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("cleanallattr")){
		if(!CleanAllAttrCommand(k2hash, opts["cleanallattr"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("shell") || opts.end() != opts.find("sh")){
		if(!ShellCommand()){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("echo")){
		if(!EchoCommand(opts["echo"])){
			CleanOptionMap(opts);
			return false;
		}
	}else if(opts.end() != opts.find("sleep")){
		if(!SleepCommand(opts["sleep"])){
			CleanOptionMap(opts);
			return false;
		}
	}else{
		ERR("Unknown command. see \"help\".");
	}
	CleanOptionMap(opts);

	return true;
}

static bool ExecHistoryCommand(K2HShm& k2hash, ConsoleInput& InputIF, ssize_t history_pos, bool& is_exit)
{
	const strarr_t&	history = InputIF.GetAllHistory();

	if(-1L == history_pos && 0 < history.size()){
		history_pos = static_cast<ssize_t>(history.size() - 1UL);
	}else if(0 < history_pos && static_cast<size_t>(history_pos) < history.size()){	// last history is "!..."
		history_pos--;
	}else{
		ERR("No history number(%zd) is existed.", history_pos);
		return true;	// for continue.
	}
	InputIF.RemoveLastHistory();						// remove last(this) command from history
	InputIF.PutHistory(history[history_pos].c_str());	// and push this command(replace history)

	// execute
	PRN(" %s", history[history_pos].c_str());
	bool	result	= CommandStringHandle(k2hash, InputIF, history[history_pos].c_str(), is_exit);

	return result;
}

static bool CommandHandle(K2HShm& k2hash, ConsoleInput& InputIF)
{
	if(!InputIF.GetCommand()){
		ERR("Something error occurred while reading stdin: err(%d).", InputIF.LastErrno());
		return false;
	}
	const string	strLine = InputIF.c_str();
	bool			is_exit = false;
	if(0 < strLine.length() && '!' == strLine[0]){
		// special character("!") command
		const char*	pSpecialCommand = strLine.c_str();
		pSpecialCommand++;

		if('\0' == *pSpecialCommand){
			// exit shell
			InputIF.RemoveLastHistory();	// remove last(this) command from history
			InputIF.PutHistory("shell");	// and push "shell" command(replace history)
			if(!ShellCommand()){
				return false;
			}
		}else{
			// execute history
			ssize_t	history_pos;
			if(1 == strlen(pSpecialCommand) && '!' == *pSpecialCommand){
				// "!!"
				history_pos = -1L;
			}else{
				history_pos = static_cast<ssize_t>(atoi(pSpecialCommand));
			}
			if(!ExecHistoryCommand(k2hash, InputIF, history_pos, is_exit) || is_exit){
				return false;
			}
		}
	}else{
		if(!CommandStringHandle(k2hash, InputIF, strLine.c_str(), is_exit) || is_exit){
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	option_t	opts;
	string		prgname;
	K2HShm		k2hash;
	k2h_h		k2hhandle = 0;
	K2HShm*		pk2hash = &k2hash;

	//----------------------
	// Console: default
	//----------------------
	ConsoleInput	InputIF;
	InputIF.SetMax(1000);				// command history 1000
	InputIF.SetPrompt("K2HTOOL> ");		// Prompt

	if(!ExecOptionParser(argc, argv, opts, prgname)){
		Help(prgname.c_str());
		exit(EXIT_FAILURE);
	}
	if(opts.end() != opts.find("-h")){
		Help(prgname.c_str());
		exit(EXIT_SUCCESS);
	}
	if(opts.end() != opts.find("-libversion")){
		k2h_print_version(stdout);
		exit(EXIT_SUCCESS);
	}

	//----------------------
	// Options
	//----------------------
	string	FilePath;
	bool	isTempFile		= false;
	bool	isMemory		= false;
	bool	isFullMap		= false;
	bool	isReadOnly		= false;
	bool	isInitOnly		= false;
	bool	isExtlib		= false;
	bool	isTrlib			= false;
	int		MaskBitCnt		= K2HShm::DEFAULT_MASK_BITCOUNT;
	int		CMaskBitCnt		= K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT;
	int		MaxElementCnt	= K2HShm::DEFAULT_MAX_ELEMENT_CNT;
	size_t	PageSize		= K2HShm::MIN_PAGE_SIZE;

	// API Mode
	if(opts.end() != opts.find("-capi")){
		isModeCAPI	= true;
		pk2hash		= NULL;
	}

	// -f / -t / -m / -fullmap
	if(opts.end() != opts.find("-f")){
		if(opts.end() != opts.find("-m") || opts.end() != opts.find("-t")){
			ERR("Option \"-f\" can not be specified with \"-t\" and \"-m\" option.");
			exit(EXIT_FAILURE);
		}
		FilePath = opts["-f"][0];
	}else if(opts.end() != opts.find("-t")){
		if(opts.end() != opts.find("-m")){
			ERR("Option \"-t\" can not be specified with \"-f\" and \"-m\" option.");
			exit(EXIT_FAILURE);
		}
		FilePath	= opts["-t"][0];
		isTempFile	= true;
		isFullMap	= true;
	}else if(opts.end() != opts.find("-m")){
		isTempFile	= false;
		isMemory	= true;
		isFullMap	= true;
	}else{
		ERR("Must specify \"-f\" or \"-t\", \"-m\" option.");
		exit(EXIT_FAILURE);
	}
	if(opts.end() != opts.find("-fullmap")){
		isFullMap = true;
	}

	// -ro / -init / -lap / -his
	if(opts.end() != opts.find("-ro")){
		if(isMemory){
			ERR("Option \"-ro\" can not be specified with \"-m\", must use only with \"-f\" or \"-t\" option.");
			exit(EXIT_FAILURE);
		}
		isReadOnly = true;
	}
	if(opts.end() != opts.find("-init")){
		if(isTempFile || isMemory){
			ERR("Option \"-init\" can not be specified with \"-t\" and \"-m\", must use only with \"-f\" option.");
			exit(EXIT_FAILURE);
		}
		if(isReadOnly){
			ERR("Option \"-init\" can not be specified with \"-ro\" option.");
			exit(EXIT_FAILURE);
		}
		isInitOnly	= true;
	}
	if(opts.end() != opts.find("-lap")){
		LapTime::Enable();
	}
	if(opts.end() != opts.find("-his")){
		int	hiscount = atoi(opts["-his"][0].c_str());
		InputIF.SetMax(static_cast<size_t>(hiscount));
	}

	// -mask / -cmask / -elementcnt / -pagesize / -ro
	if(opts.end() != opts.find("-mask")){
		MaskBitCnt = atoi(opts["-mask"][0].c_str());
	}
	if(opts.end() != opts.find("-cmask")){
		CMaskBitCnt = atoi(opts["-cmask"][0].c_str());
	}
	if(opts.end() != opts.find("-elementcnt")){
		MaxElementCnt = atoi(opts["-elementcnt"][0].c_str());
	}
	if(opts.end() != opts.find("-pagesize")){
		PageSize = atoi(opts["-pagesize"][0].c_str());
		if(PageSize < static_cast<size_t>(K2HShm::MIN_PAGE_SIZE)){
			ERR("Option \"-pagesize\" parameter(%zu) is under minimum pagesize(%d).", PageSize, K2HShm::MIN_PAGE_SIZE);
			exit(EXIT_FAILURE);
		}
	}
	if(opts.end() != opts.find("-ro")){
		isReadOnly	= true;
	}

	// -ext
	if(opts.end() != opts.find("-ext")){
		bool result;
		if(isModeCAPI){
			result = k2h_load_hash_library(opts["-ext"][0].c_str());
		}else{
			result = K2HashDynLib::get()->Load(opts["-ext"][0].c_str());
		}
		if(!result){
			ERR("Failed to load library(%s).", opts["-ext"][0].c_str());
			exit(EXIT_FAILURE);
		}
		isExtlib = true;
	}

	// -trlib
	if(opts.end() != opts.find("-trlib")){
		bool result;
		if(isModeCAPI){
			result = k2h_load_transaction_library(opts["-trlib"][0].c_str());
		}else{
			result = K2HTransDynLib::get()->Load(opts["-trlib"][0].c_str());
		}
		if(!result){
			ERR("Failed to load transaction library(%s).", opts["-trlib"][0].c_str());
			exit(EXIT_FAILURE);
		}
		isTrlib = true;
	}

	// -g / -glog / SIGUSR1
	if(opts.end() != opts.find("-g")){
		if(0 == opts["-g"].size()){
			// nothing to do.
			// default is silent
		}else if(0 == strcasecmp(opts["-g"][0].c_str(), "ERR")){
			if(isModeCAPI){
				k2h_set_debug_level_error();
			}else{
				::SetK2hDbgMode(K2HDBG_ERR);
			}
		}else if(0 == strcasecmp(opts["-g"][0].c_str(), "WAN")){
			if(isModeCAPI){
				k2h_set_debug_level_warning();
			}else{
				::SetK2hDbgMode(K2HDBG_WARN);
			}
		}else if(0 == strcasecmp(opts["-g"][0].c_str(), "INFO")){
			if(isModeCAPI){
				k2h_set_debug_level_message();
			}else{
				::SetK2hDbgMode(K2HDBG_MSG);
			}
		}else{
			ERR("Unknown parameter(%s) value for \"-g\" option.", opts["-g"][0].c_str());
			exit(EXIT_FAILURE);
		}
	}
	{
		bool result;
		if(isModeCAPI){
			result = k2h_set_bumpup_debug_signal_user1();
		}else{
			result = SetSignalUser1();
		}
		if(!result){
			ERR("Failed to set SIGUSR1 handler for bumpup debug level.");
			exit(EXIT_FAILURE);
		}
	}
	if(opts.end() != opts.find("-glog")){
		bool result;
		if(isModeCAPI){
			result = k2h_set_debug_file(opts["-glog"][0].c_str());
		}else{
			result = SetK2hDbgFile(opts["-glog"][0].c_str());
		}
		if(!result){
			ERR("Failed to set debug message log file(%s).", opts["-glog"][0].c_str());
			exit(EXIT_FAILURE);
		}
	}

	// -run option
	string	CommandFile("");
	if(opts.end() != opts.find("-run")){
		if(0 == opts["-run"].size()){
			ERR("Option \"-run\" needs parameter as command file path.");
			exit(EXIT_FAILURE);
		}else if(1 < opts["-run"].size()){
			ERR("Unknown parameter(%s) value for \"-run\" option.", opts["-run"][1].c_str());
			exit(EXIT_FAILURE);
		}
		// check file
		struct stat	st;
		if(0 != stat(opts["-run"][0].c_str(), &st)){
			ERR("Parameter command file path(%s) for option \"-run\" does not exist(errno=%d).", opts["-run"][0].c_str(), errno);
			exit(EXIT_FAILURE);
		}
		CommandFile = opts["-run"][0];
	}

	// Display
	PRN("-------------------------------------------------------");
	PRN(" K2HASH TOOL");
	PRN("-------------------------------------------------------");
	if(!isTempFile && !isMemory){
		if(isReadOnly){
			PRN("Permanent file(read only attached):     %s", FilePath.c_str());
		}else{
			PRN("Permanent file:                         %s", FilePath.c_str());
		}
	}else if(isTempFile){
		PRN("Temporary file:                         %s", FilePath.c_str());
	}else{	// isMemory
		PRN("On memory mode");
	}
	PRN("Attached parameters:");
	PRN("    Full are mapping:                   %s", isFullMap ? "true" : "false");
	PRN("    Key Index mask count:               %d", MaskBitCnt);
	PRN("    Collision Key Index mask count:     %d", CMaskBitCnt);
	PRN("    Max element count:                  %d", MaxElementCnt);
	if(isExtlib){
		PRN("Extra hash function library:            %s", opts["-ext"][0].c_str());
	}
	if(isTrlib){
		PRN("Extra Transaction  function library:    %s", opts["-trlib"][0].c_str());
	}
	PRN("-------------------------------------------------------");
	PRN(NULL);

	// cleanup for valgrind
	CleanOptionMap(opts);

	//----------------------
	// Attach K2H
	//----------------------
	if(isInitOnly){
		if(isModeCAPI){
			if(!k2h_create(FilePath.c_str(), MaskBitCnt, CMaskBitCnt, MaxElementCnt, PageSize)){
				ERR("Could not initialize permanent file(%s).", FilePath.c_str());
				exit(EXIT_FAILURE);
			}
		}else{
			if(!k2hash.Create(FilePath.c_str(), isFullMap, MaskBitCnt, CMaskBitCnt, MaxElementCnt, PageSize)){
				ERR("Could not initialize permanent file(%s).", FilePath.c_str());
				exit(EXIT_FAILURE);
			}
		}
		PRN("Permanent file(%s) is completely initialized.", FilePath.c_str());
		PRN(NULL);
	}else{
		if(isModeCAPI){
			struct stat	st;
			if(!isMemory && !isTempFile && 0 != stat(FilePath.c_str(), &st)){
				if(!k2h_create(FilePath.c_str(), MaskBitCnt, CMaskBitCnt, MaxElementCnt, PageSize)){
					ERR("File(%s) does not exist, so create it, but could not initialize permanent file.", FilePath.c_str());
					exit(EXIT_FAILURE);
				}
			}
			if(K2H_INVALID_HANDLE == (k2hhandle = k2h_open(isMemory ? NULL : FilePath.c_str(), isReadOnly, isTempFile, isFullMap, MaskBitCnt, CMaskBitCnt, MaxElementCnt, PageSize))){
				ERR("Failed to attach %s.", isMemory ? "memory" : "file");
				exit(EXIT_FAILURE);
			}
			pk2hash = reinterpret_cast<K2HShm*>(k2hhandle);
		}else{
			if(!k2hash.Attach(isMemory ? NULL : FilePath.c_str(), isReadOnly, isReadOnly ? false : true, isTempFile, isFullMap, MaskBitCnt, CMaskBitCnt, MaxElementCnt, PageSize)){
				ERR("Failed to attach %s.", isMemory ? "memory" : "file");
				exit(EXIT_FAILURE);
			}
		}
	}

	//----------------------
	// Main Loop
	//----------------------
	if(!isInitOnly){
		do{
			// check command file at starting
			if(!CommandFile.empty()){
				string	LoadCommandLine("load ");
				LoadCommandLine += CommandFile;

				bool	is_exit = false;
				if(!CommandStringHandle(k2hash, InputIF, LoadCommandLine.c_str(), is_exit) || is_exit){
					break;
				}
				CommandFile.clear();
			}
			// start interactive until error occurred.

		}while(CommandHandle(*pk2hash, InputIF));
	}
	InputIF.Clean();

	//----------------------
	// Detach K2H
	//----------------------
	if(isModeCAPI){
		k2h_unload_hash_library();
		if(k2hhandle && !k2h_close(k2hhandle)){
			ERR("Failed to detach %s.", isMemory ? "memory" : "file");
			exit(EXIT_FAILURE);
		}
	}else{
		K2HashDynLib::get()->Unload();
		if(!k2hash.Detach()){
			ERR("Failed to detach %s.", isMemory ? "memory" : "file");
			exit(EXIT_FAILURE);
		}
	}
	// for valgrind
	k2hash.CleanCommonAttribute();

	//----------------------
	// Others
	//----------------------
	if(isModeCAPI){
		k2h_unset_debug_file();
	}else{
		UnsetK2hDbgFile();
	}

	exit(EXIT_SUCCESS);
}

//
// Local variables:
// coding: utf-8
// tab-width: 4
// c-basic-offset: 4
// indent-tabs-mode: t
// End:
// vim600: ts=4 fdm=marker fenc=utf-8
// vim<600: ts=4 fenc=utf-8
