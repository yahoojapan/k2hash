/*
 * K2HASH
 *
 * Copyright 2015 Yahoo Japan Corporation.
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
 * CREATE:   Mon Apr 20 2015
 * REVISION:
 *
 */

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#include <libgen.h>

#include <map>
#include <string>

#include <k2hash.h>
#include <k2hcommon.h>
#include <k2hshm.h>
#include <k2hdbg.h>

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	KEY_FORMAT				"KEY-%016X"
#define	KEY_FORMAT_UNIQ			"KEY-%016X-%016X"
#define	KEY_BUFF_LENGTH			48							// max 37 character + null + etc.
#define	BENCH_TMP_FILE_FORM		"/tmp/k2hbench-%d.dat"
#define	BENCH_FILE_LENGTH		256
#define	BENCH_TYPE_RO			1
#define	BENCH_TYPE_WO			2
#define	BENCH_TYPE_RW			(BENCH_TYPE_RO | BENCH_TYPE_WO)

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
//
// For option parser
//
typedef struct opt_param{
	std::string		rawstring;
	bool			is_number;
	int				num_value;
}OPTPARAM, *POPTPARAM;

typedef std::map<std::string, OPTPARAM>		optparams_t;

//
// All option
//
typedef struct bench_opts{
	bool			is_help;
	K2hDbgMode		dbglevel;

	// k2hash file/memory type
	char			szfile[BENCH_FILE_LENGTH];	// If empty, means memory mode
	bool			is_temp_mode;
	bool			is_read_only;
	bool			is_fullmap;

	// parameters to initialize
	int				maskcnt;
	int				cmaskcnt;
	int				elementcnt;
	size_t			pagesize;

	// bench mark options
	int				bench_type;
	int				proccnt;
	int				threadcnt;
	int				loopcnt;
	int				datacnt;
	size_t			dlength;
	bool			is_start_no_data;
}BOPTS, *PBOPTS;

//
// For child process(thread)
//
typedef struct child_control{
	int				procid;				// process id for child
	int				threadid;			// thread id(gettid)
	pthread_t		pthreadid;			// pthread id(pthread_create)
	bool			is_ready;			// 
	bool			is_exit;			// 
	struct timespec	ts;					// result time
	int				errorcnt;			// error count
}CHLDCNTL, *PCHLDCNTL;

//
// For parent(main) process, common data
//
typedef struct exec_control{
	int				procid;				// = parent id
	BOPTS			opt;
	bool			is_suspend;
	bool			is_wait_doing;
	bool			is_exit;
}EXECCNTL, *PEXECCNTL;

//
// For thread function
//
typedef struct thread_param{
	K2HShm*		pk2hshm;
	PEXECCNTL	pexeccntl;
	PCHLDCNTL	pmycntl;
}THPARAM, *PTHPARAM;

//---------------------------------------------------------
// Macros
//---------------------------------------------------------
static inline void PRN(const char* format, ...)
{
	if(format){
		va_list ap;
		va_start(ap, format);
		vfprintf(stdout, format, ap); 
		va_end(ap);
	}
	fprintf(stdout, "\n");
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
}

static inline char* programname(char* prgpath)
{
	if(!prgpath){
		return NULL;
	}
	char*	pprgname = basename(prgpath);
	if(0 == strncmp(pprgname, "lt-", strlen("lt-"))){
		pprgname = &pprgname[strlen("lt-")];
	}
	return pprgname;
}

static inline void init_bench_opts(BOPTS& benchopts)
{
	memset(&benchopts.szfile[0], 0, BENCH_FILE_LENGTH);

	benchopts.is_help				= false;
	benchopts.dbglevel				= K2HDBG_SILENT;
	benchopts.is_temp_mode			= false;
	benchopts.is_read_only			= false;
	benchopts.is_fullmap			= true;

	benchopts.maskcnt				= K2HShm::DEFAULT_MASK_BITCOUNT;
	benchopts.cmaskcnt				= K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT;
	benchopts.elementcnt			= K2HShm::DEFAULT_MAX_ELEMENT_CNT;
	benchopts.pagesize				= K2HShm::MIN_PAGE_SIZE;

	benchopts.bench_type			= BENCH_TYPE_RW;
	benchopts.proccnt				= 1;
	benchopts.threadcnt				= 1;
	benchopts.loopcnt				= 1;
	benchopts.datacnt				= 1;
	benchopts.dlength				= 128;
	benchopts.is_start_no_data		= false;
}

static inline void copy_bench_opts(BOPTS& dest, const BOPTS& src)
{
	memcpy(&dest.szfile[0], &src.szfile[0], BENCH_FILE_LENGTH);
	dest.is_help			= src.is_help;
	dest.dbglevel			= src.dbglevel;
	dest.is_temp_mode		= src.is_temp_mode;
	dest.is_read_only		= src.is_read_only;
	dest.is_fullmap			= src.is_fullmap;

	dest.maskcnt			= src.maskcnt;
	dest.cmaskcnt			= src.cmaskcnt;
	dest.elementcnt			= src.elementcnt;
	dest.pagesize			= src.pagesize;

	dest.bench_type			= src.bench_type;
	dest.proccnt			= src.proccnt;
	dest.threadcnt			= src.threadcnt;
	dest.loopcnt			= src.loopcnt;
	dest.datacnt			= src.datacnt;
	dest.dlength			= src.dlength;
	dest.is_start_no_data	= src.is_start_no_data;
}

static inline void get_nomotonic_time(struct timespec& ts)
{
	if(-1 == clock_gettime(CLOCK_MONOTONIC, &ts)){
		ERR("Could not get CLOCK_MONOTONIC timespec, but continue...(errno=%d)", errno);
		ts.tv_sec	= 0;
		ts.tv_nsec	= 0;
	}
}

static inline void get_nomotonic_time(struct timespec& diffts, const struct timespec& startts)
{
	struct timespec	endts;
	if(-1 == clock_gettime(CLOCK_MONOTONIC, &endts)){
		ERR("Could not get CLOCK_MONOTONIC timespec, but continue...(errno=%d)", errno);
		endts.tv_sec	= 0;
		endts.tv_nsec	= 0;
	}

	if(endts.tv_nsec < startts.tv_nsec){
		if(0 < endts.tv_sec){
			endts.tv_sec--;
			endts.tv_nsec += 1000 * 1000 * 1000;
		}else{
			ERR("Start time > End time, so result is ZERO.");
			diffts.tv_sec	= 0;
			diffts.tv_nsec	= 0;
			return;
		}
	}
	if(endts.tv_sec < startts.tv_sec){
		ERR("Start time > End time, so result is ZERO.");
		diffts.tv_sec	= 0;
		diffts.tv_nsec	= 0;
		return;
	}
	diffts.tv_nsec	= endts.tv_nsec - startts.tv_nsec;
	diffts.tv_sec	= endts.tv_sec - startts.tv_sec;
}

//---------------------------------------------------------
// Utility Class
//---------------------------------------------------------
//
// For test data which is random value by each thread.
//
class TestData
{
	private:
		static const char	errData = 0x00;
		char*				pData;
		size_t				length;

	public:
		explicit TestData(size_t datalen = 1) : pData(NULL), length(datalen)
		{
			if(1 <= datalen){
				length = datalen;
				if(NULL == (pData = reinterpret_cast<char*>(calloc(length, sizeof(char))))){
					ERR("Could not allocate memory.");
					length = 0;
				}else{
					// data is repeating from 0x20 - 0x7e character.
					memset(pData, (static_cast<char>(gettid() % static_cast<int>('}' - ' ')) + ' '), length - 1);
				}
			}else{
				length = 0;
			}
		}
		virtual ~TestData()
		{
			K2H_Free(pData);
		}
		const char* Data(void) const { return (0 == length ? &TestData::errData : pData); }
		size_t Length(void) const { return (0 == length ? 1 : length); }
};

const char	TestData::errData;

//---------------------------------------------------------
// Utility Functions
//---------------------------------------------------------
static void Help(char* progname)
{
	PRN(NULL);
	PRN("Usage: %s [ -f filename | -t filename | -m | -h ] [options]", progname ? programname(progname) : "program");
	PRN(NULL);
	PRN("K2Hash type: choice one of them");
	PRN("       -h                   help display");
	PRN("       -f <filename>        mode by filename for permanent hash file(always create new file)");
	PRN("       -t <filename>        mode by filename for temporary hash file");
	PRN("       -m                   mode for only memory");
	PRN(NULL);
	PRN("Bench mark options:");
	PRN("       -type [ro|wo|rw]     bench type as READ ONLY/WRITE ONLY/READ WRITE(default rw).");
	PRN("       -proc [count]        process count(default 1).");
	PRN("       -thread [count]      thread count(default 1).");
	PRN("       -loop [loop count]   Loop count(default 1) for each thread, set over 0.");
	PRN("       -dcount [count]      data count(default 1). -type is wo/rw and this value is 0 means all uniq key");
	PRN("       -dlength [byte]      data length(default 128 byte).");
	PRN("       -start_nodata        not initialize data before starting.");
	PRN(NULL);
	PRN("K2Hash create options:");
	PRN("       -ro                  read only mode(only permanent k2hash file)");
	PRN("       -nofullmap           not full mapping");
	PRN("       -mask <bit count>    bit mask count for hash when creating a file/memory");
	PRN("       -cmask <bit count>   collision bit mask count when creating a file/memory");
	PRN("       -elementcnt <count>  element count for each hash table when creating a file/memory");
	PRN("       -pagesize <number>   pagesize for each data when creating a file/memory");
	PRN(NULL);
	PRN("debug option:");
	PRN("       -g <debug level>     debugging mode: ERR(default) / WAN / INFO(*1)");
	PRN(NULL);
	PRN("(*1)You can set debug level by another way which is setting environment as \"K2HDBGMODE\".");
	PRN("    \"K2HDBGMODE\" environment is took as \"SILENT\", \"ERR\", \"WAN\" or \"INFO\" value.");
	PRN("    When this process gets SIGUSR1 signal, the debug level is bumpup.");
	PRN("    The debug level is changed as \"SILENT\"->\"ERR\"->\"WAN\"->\"INFO\"->...");
	PRN(NULL);
}

static void OptionParser(int argc, char** argv, optparams_t& optparams)
{
	optparams.clear();
	for(int cnt = 1; cnt < argc && argv && argv[cnt]; cnt++){
		OPTPARAM	param;
		param.rawstring = "";
		param.is_number = false;
		param.num_value = 0;

		// get option name
		char*	popt = argv[cnt];
		if(ISEMPTYSTR(popt)){
			continue;		// skip
		}
		if('-' != *popt){
			ERR("%s option is not started with \"-\".", popt);
			continue;
		}

		// check option parameter
		if((cnt + 1) < argc && argv[cnt + 1]){
			char*	pparam = argv[cnt + 1];
			if(!ISEMPTYSTR(pparam) && '-' != *pparam){
				// found param
				param.rawstring = pparam;

				// check number
				param.is_number = true;
				for(char* ptmp = pparam; *ptmp; ++ptmp){
					if(0 == isdigit(*ptmp)){
						param.is_number = false;
						break;
					}
				}
				// cppcheck-suppress unmatchedSuppression
				// cppcheck-suppress knownConditionTrueFalse
				if(param.is_number){
					param.num_value = atoi(pparam);
				}
				++cnt;
			}
		}
		optparams[string(popt)] = param;
	}
}

//
// Parse and set options and check another option combination
//
static bool SetBenchOptions(int argc, char** argv, BOPTS& benchopts)
{
	// parse
	optparams_t	optparams;
	OptionParser(argc, argv, optparams);

	// -h
	if(optparams.end() != optparams.find("-h")){
		benchopts.is_help = true;
		return true;					// exit ASSAP
	}
	benchopts.is_help = false;

	// -g
	if(optparams.end() != optparams.find("-g")){
		if(0 == optparams["-g"].rawstring.length()){
			ERR("\"-g\" option needs file path parameter.");
			return false;
		}
		if(0 == strcasecmp(optparams["-g"].rawstring.c_str(), "SILENT")){
			benchopts.dbglevel	= K2HDBG_SILENT;
		}else if(0 == strcasecmp(optparams["-g"].rawstring.c_str(), "ERR")){
			benchopts.dbglevel	= K2HDBG_ERR;
		}else if(0 == strcasecmp(optparams["-g"].rawstring.c_str(), "WAN") || 0 == strcasecmp(optparams["-g"].rawstring.c_str(), "WARN")){
			benchopts.dbglevel	= K2HDBG_WARN;
		}else if(0 == strcasecmp(optparams["-g"].rawstring.c_str(), "MSG") || 0 == strcasecmp(optparams["-g"].rawstring.c_str(), "INFO") || 0 == strcasecmp(optparams["-g"].rawstring.c_str(), "INF")){
			benchopts.dbglevel	= K2HDBG_MSG;
		}else{
			ERR("\"-g\" option parameter(%s) is unknown type.", optparams["-g"].rawstring.c_str());
			return false;
		}
	}else{
		benchopts.dbglevel	= K2HDBG_SILENT;
	}

	// -f, -t, -m
	bool	is_set_file_type = false;
	if(optparams.end() != optparams.find("-f")){
		if(0 == optparams["-f"].rawstring.length()){
			ERR("\"-f\" option needs file path parameter.");
			return false;
		}
		if(BENCH_FILE_LENGTH <= optparams["-f"].rawstring.length()){
			ERR("\"-f\" option parameter(%s) is too long, file path is %d maximum.", optparams["-f"].rawstring.c_str(), BENCH_FILE_LENGTH);
			return false;
		}
		strcpy(&benchopts.szfile[0], optparams["-f"].rawstring.c_str());
		benchopts.is_temp_mode	= false;
		is_set_file_type		= true;
	}
	if(optparams.end() != optparams.find("-t")){
		if(is_set_file_type){
			ERR("\"-t\" option could not be specified with \"-f\" and \"-m\" option.");
			return false;
		}
		if(0 == optparams["-t"].rawstring.length()){
			ERR("\"-t\" option needs file path parameter.");
			return false;
		}
		if(BENCH_FILE_LENGTH <= optparams["-t"].rawstring.length()){
			ERR("\"-t\" option parameter(%s) is too long, file path is %d maximum.", optparams["-t"].rawstring.c_str(), BENCH_FILE_LENGTH);
			return false;
		}
		strcpy(&benchopts.szfile[0], optparams["-t"].rawstring.c_str());
		benchopts.is_temp_mode	= true;
		is_set_file_type		= true;
	}
	if(optparams.end() != optparams.find("-m")){
		if(is_set_file_type){
			ERR("\"-m\" option could not be specified with \"-f\" and \"-t\" option.");
			return false;
		}
		if(0 != optparams["-m"].rawstring.length()){
			ERR("\"-m\" option does not need parameter(%s).", optparams["-m"].rawstring.c_str());
			return false;
		}
		memset(&benchopts.szfile[0], 0, BENCH_FILE_LENGTH);
		benchopts.is_temp_mode	= false;
		is_set_file_type		= true;
	}
	if(!is_set_file_type){
		ERR("Specify \"-m\" or \"-f\" or \"-t\" option.");
		return false;
	}

	// -ro, -nofullmap
	if(optparams.end() != optparams.find("-ro")){
		if(0 != optparams["-ro"].rawstring.length()){
			ERR("\"-ro\" option does not need parameter(%s).", optparams["-ro"].rawstring.c_str());
			return false;
		}
		if('\0' == benchopts.szfile[0] || benchopts.is_temp_mode){
			ERR("\"-ro\" option could not set with \"-m\" or \"-t\" option.");
			return false;
		}
		benchopts.is_read_only = true;
	}else{
		benchopts.is_read_only = false;
	}
	if(optparams.end() != optparams.find("-nofullmap")){
		if(0 != optparams["-nofullmap"].rawstring.length()){
			ERR("\"-nofullmap\" option does not need parameter(%s).", optparams["-nofullmap"].rawstring.c_str());
			return false;
		}
		if('\0' == benchopts.szfile[0] || benchopts.is_temp_mode){
			ERR("\"-nofullmap\" option could not set with \"-m\" or \"-t\" option.");
			return false;
		}
		benchopts.is_fullmap = false;
	}else{
		benchopts.is_fullmap = true;
	}

	// -mask, -cmask, -elementcnt, -pagesize
	if(optparams.end() != optparams.find("-mask")){
		if(0 == optparams["-mask"].rawstring.length()){
			ERR("\"-mask\" option needs parameter.");
			return false;
		}
		if(!optparams["-mask"].is_number){
			ERR("\"-mask\" option parameter(%s) must be decimal string.", optparams["-mask"].rawstring.c_str());
			return false;
		}
		benchopts.maskcnt = optparams["-mask"].num_value;
	}else{
		benchopts.maskcnt = K2HShm::DEFAULT_MASK_BITCOUNT;
	}
	if(optparams.end() != optparams.find("-cmask")){
		if(0 == optparams["-cmask"].rawstring.length()){
			ERR("\"-cmask\" option needs parameter.");
			return false;
		}
		if(!optparams["-cmask"].is_number){
			ERR("\"-cmask\" option parameter(%s) must be decimal string.", optparams["-cmask"].rawstring.c_str());
			return false;
		}
		benchopts.cmaskcnt = optparams["-cmask"].num_value;
	}else{
		benchopts.cmaskcnt = K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT;
	}
	if(optparams.end() != optparams.find("-elementcnt")){
		if(0 == optparams["-elementcnt"].rawstring.length()){
			ERR("\"-elementcnt\" option needs parameter.");
			return false;
		}
		if(!optparams["-elementcnt"].is_number){
			ERR("\"-elementcnt\" option parameter(%s) must be decimal string.", optparams["-elementcnt"].rawstring.c_str());
			return false;
		}
		benchopts.elementcnt = optparams["-elementcnt"].num_value;
	}else{
		benchopts.elementcnt = K2HShm::DEFAULT_MAX_ELEMENT_CNT;
	}
	if(optparams.end() != optparams.find("-pagesize")){
		if(0 == optparams["-pagesize"].rawstring.length()){
			ERR("\"-pagesize\" option needs parameter.");
			return false;
		}
		if(!optparams["-pagesize"].is_number){
			ERR("\"-pagesize\" option parameter(%s) must be decimal string.", optparams["-pagesize"].rawstring.c_str());
			return false;
		}
		benchopts.pagesize = optparams["-pagesize"].num_value;
	}else{
		benchopts.pagesize = K2HShm::MIN_PAGE_SIZE;
	}

	// -type, -proc, -thread, -loop, -dcount
	if(optparams.end() != optparams.find("-type")){
		if(0 == optparams["-type"].rawstring.length()){
			ERR("\"-type\" option needs parameter.");
			return false;
		}
		if(optparams["-type"].rawstring == "ro"){
			benchopts.bench_type = BENCH_TYPE_RO;
		}else if(optparams["-type"].rawstring == "wo"){
			benchopts.bench_type = BENCH_TYPE_WO;
		}else if(optparams["-type"].rawstring == "rw"){
			benchopts.bench_type = BENCH_TYPE_RW;
		}else{
			ERR("\"-type\" option parameter(%s) must be \"ro\" or \"wo\"or \"rw\".", optparams["-type"].rawstring.c_str());
			return false;
		}
	}else{
		benchopts.bench_type = BENCH_TYPE_RW;
	}
	if(optparams.end() != optparams.find("-proc")){
		if(0 == optparams["-proc"].rawstring.length()){
			ERR("\"-proc\" option needs parameter.");
			return false;
		}
		if(!optparams["-proc"].is_number){
			ERR("\"-proc\" option parameter(%s) must be decimal string.", optparams["-proc"].rawstring.c_str());
			return false;
		}
		if(('\0' == benchopts.szfile[0] || benchopts.is_temp_mode) && 1 < optparams["-proc"].num_value){
			ERR("\"-proc\" option parameter(%d) could not set with \"-t\" or \"-m\" option, must be \"-proc 1\" or not set.", optparams["-proc"].num_value);
			return false;
		}
		benchopts.proccnt = optparams["-proc"].num_value;
	}else{
		benchopts.proccnt = 1;
	}
	if(optparams.end() != optparams.find("-thread")){
		if(0 == optparams["-thread"].rawstring.length()){
			ERR("\"-thread\" option needs parameter.");
			return false;
		}
		if(!optparams["-thread"].is_number){
			ERR("\"-thread\" option parameter(%s) must be decimal string.", optparams["-thread"].rawstring.c_str());
			return false;
		}
		benchopts.threadcnt = optparams["-thread"].num_value;
	}else{
		benchopts.threadcnt = 1;
	}
	if(optparams.end() != optparams.find("-loop")){
		if(0 == optparams["-loop"].rawstring.length()){
			ERR("\"-loop\" option needs parameter.");
			return false;
		}
		if(!optparams["-loop"].is_number){
			ERR("\"-loop\" option parameter(%s) must be decimal string.", optparams["-loop"].rawstring.c_str());
			return false;
		}
		benchopts.loopcnt = optparams["-loop"].num_value;
	}else{
		benchopts.loopcnt = 1;
	}
	if(optparams.end() != optparams.find("-dcount")){
		if(0 == optparams["-dcount"].rawstring.length()){
			ERR("\"-dcount\" option needs parameter.");
			return false;
		}
		if(optparams["-dcount"].rawstring == "0"){
			if(0 == (benchopts.bench_type & BENCH_TYPE_WO)){
				ERR("\"-dcount\" option parameter(%s) is allowed only \"-type\" option \"wo\" or \"rw\".", optparams["-dcount"].rawstring.c_str());
				return false;
			}
			benchopts.datacnt = 0;
		}else{
			if(!optparams["-dcount"].is_number){
				ERR("\"-dcount\" option parameter(%s) must be decimal string.", optparams["-dcount"].rawstring.c_str());
				return false;
			}
			benchopts.datacnt = optparams["-dcount"].num_value;
		}
	}else{
		benchopts.datacnt = 1;
	}
	if(optparams.end() != optparams.find("-dlength")){
		if(0 == optparams["-dlength"].rawstring.length()){
			ERR("\"-dlength\" option needs parameter.");
			return false;
		}
		if(!optparams["-dlength"].is_number){
			ERR("\"-dlength\" option parameter(%s) must be decimal string.", optparams["-dlength"].rawstring.c_str());
			return false;
		}
		if(0 >= optparams["-dlength"].num_value){
			ERR("\"-dlength\" option parameter(%s) must be over 0.", optparams["-dlength"].rawstring.c_str());
			return false;
		}
		benchopts.dlength = static_cast<size_t>(optparams["-dlength"].num_value);
	}else{
		benchopts.dlength = 128;
	}
	if(optparams.end() != optparams.find("-start_nodata")){
		if(0 != optparams["-start_nodata"].rawstring.length()){
			ERR("\"-start_nodata\" option does not need parameter(%s).", optparams["-start_nodata"].rawstring.c_str());
			return false;
		}
		if(BENCH_TYPE_RO == benchopts.bench_type){
			ERR("\"-start_nodata\" option does not set with \"-type ro\".");
			return false;
		}
		benchopts.is_start_no_data = true;
	}else{
		if(0 == benchopts.datacnt){
			PRN("[INFO] Specified \"-type\" option is \"wo\" or \"rw\", and \"-dcount\" is \"0\", so start with no data as same as \"-start_nodata\" option specified.");
			benchopts.is_start_no_data = true;
		}else{
			benchopts.is_start_no_data = false;
		}
	}
	return true;
}

static K2HShm* InitializeK2HashFile(BOPTS& benchopts)
{
	K2HShm*	pk2hshm = new K2HShm();

	// build file(memory)
	bool	result;
	if('\0' == benchopts.szfile[0]){
		result = pk2hshm->AttachMem(benchopts.maskcnt, benchopts.cmaskcnt, benchopts.elementcnt, benchopts.pagesize);
	}else{
		result = pk2hshm->Create(&benchopts.szfile[0], benchopts.is_fullmap, benchopts.maskcnt, benchopts.cmaskcnt, benchopts.elementcnt, benchopts.pagesize);
	}
	if(!result){
		ERR("Failed to create(initialize) k2hash file(memory).");
		K2H_Delete(pk2hshm);
		return NULL;
	}

	// initialize data
	if(!benchopts.is_start_no_data){
		TestData	data(benchopts.dlength);	// initial data
		char		szKey[KEY_BUFF_LENGTH];
		for(int cnt = 0; cnt < benchopts.datacnt; ++cnt){
			sprintf(szKey, KEY_FORMAT, cnt);
			if(!pk2hshm->Set(szKey, data.Data())){
				ERR("Failed to initialize data to k2hash file(memory).");
				K2H_Delete(pk2hshm);
				return NULL;
			}
		}
	}
	return pk2hshm;
}

static int OpenBenchFile(const string& filepath, size_t& totalsize, bool is_create)
{
	int	fd;

	if(is_create){
		// remove exist file
		struct stat	st;
		if(0 == stat(filepath.c_str(), &st)){
			// file exists
			if(0 != unlink(filepath.c_str())){
				ERR("Could not remove exist file(%s), errno=%d", filepath.c_str(), errno);
				return -1;
			}
		}

		// create file
		if(-1 == (fd = open(filepath.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
			ERR("Could not make file(%s), errno=%d", filepath.c_str(), errno);
			return -1;
		}

		// truncate & clean up
		if(0 != ftruncate(fd, totalsize)){
			ERR("Could not truncate file(%s) to %zu, errno = %d", filepath.c_str(), totalsize, errno);
			K2H_CLOSE(fd);
			return -1;
		}
		unsigned char	szBuff[1024];
		memset(szBuff, 0, sizeof(szBuff));
		for(ssize_t wrote = 0, onewrote = 0; static_cast<size_t>(wrote) < totalsize; wrote += onewrote){
			onewrote = min(static_cast<size_t>(sizeof(unsigned char) * 1024), (totalsize - static_cast<size_t>(wrote)));
			if(-1 == k2h_pwrite(fd, szBuff, onewrote, wrote)){
				ERR("Failed to write initializing file(%s), errno = %d", filepath.c_str(), errno);
				K2H_CLOSE(fd);
				return -1;
			}
		}
	}else{
		struct stat	st;
		if(0 != stat(filepath.c_str(), &st)){
			// file does not exist
			ERR("Could not find file(%s)", filepath.c_str());
			return -1;
		}
		if(-1 == (fd = open(filepath.c_str(), O_RDWR))){
			ERR("Could not open file(%s), errno = %d", filepath.c_str(), errno);
			return -1;
		}
		totalsize = static_cast<size_t>(st.st_size);
	}
	return fd;
}

static void* MmapBenchFile(bool is_create, int proccnt, int threadcnt, string& filepath, int& fd, PEXECCNTL& pexeccntl, PCHLDCNTL& pchldcntl, size_t& totalsize)
{
	// file path
	if(is_create){
		char	szBuff[32];
		sprintf(szBuff, BENCH_TMP_FILE_FORM, gettid());
		filepath	= szBuff;
		totalsize	= sizeof(EXECCNTL) + (sizeof(CHLDCNTL) * proccnt * threadcnt);
	}else{
		totalsize	= 0;
	}

	// open
	if(-1 == (fd = OpenBenchFile(filepath, totalsize, is_create))){
		ERR("Could not open(initialize) file(%s)", filepath.c_str());
		return NULL;
	}

	// mmap
	void*	pShm;
	if(MAP_FAILED == (pShm = mmap(NULL, totalsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0))){
		ERR("Could not mmap to file(%s), errno = %d", filepath.c_str(), errno);
		K2H_CLOSE(fd);
		return NULL;
	}

	// set pointer
	pexeccntl = reinterpret_cast<PEXECCNTL>(pShm);
	pchldcntl = reinterpret_cast<PCHLDCNTL>(reinterpret_cast<off_t>(pShm) + sizeof(EXECCNTL));

	return pShm;
}

static void PrintResult(const PEXECCNTL pexeccntl, const PCHLDCNTL pchldcntl, const struct timespec& realtime)
{
	if(!pexeccntl || !pchldcntl){
		ERR("Parameters are wrong.");
		return;
	}

	// calc total
	struct timespec	addtotalts	= {0, 0};
	int				totalerr	= 0;
	for(int cnt = 0; cnt < pexeccntl->opt.proccnt * pexeccntl->opt.threadcnt; ++cnt){
		totalerr			+= pchldcntl[cnt].errorcnt;
		addtotalts.tv_sec	+= pchldcntl[cnt].ts.tv_sec;
		addtotalts.tv_nsec	+= pchldcntl[cnt].ts.tv_nsec;
		if((1000 * 1000 * 1000) < addtotalts.tv_nsec){
			addtotalts.tv_nsec	-= 1000 * 1000 * 1000;
			addtotalts.tv_sec	+= 1;
		}
	}
	// additional measurement total time
	long	addtotalfns	= (addtotalts.tv_sec * 1000 * 1000 * 1000) + addtotalts.tv_nsec;
	long	addtotalsec	= static_cast<long>(addtotalts.tv_sec);
	long	addtotalms	= (addtotalfns / (1000 * 1000)) % 1000;
	long	addtotalus	= (addtotalfns / 1000) % 1000;
	long	addtotalns	= addtotalfns % 1000;

	// additional measurement average time
	long	addavrgfns	= addtotalfns / static_cast<long>(pexeccntl->opt.proccnt * pexeccntl->opt.threadcnt * (pexeccntl->opt.loopcnt * (BENCH_TYPE_RW == pexeccntl->opt.bench_type ? 2 : 1)));		// Loop count
	long	addavrgsec	= addavrgfns / (1000 * 1000 * 1000);
	long	addavrgms	= (addavrgfns / (1000 * 1000)) % 1000;
	long	addavrgus	= (addavrgfns / 1000) % 1000;
	long	addavrgns	= addavrgfns % 1000;

	// additional measurement total time
	long	realtotalfns= (realtime.tv_sec * 1000 * 1000 * 1000) + realtime.tv_nsec;
	long	realtotalsec= static_cast<long>(realtime.tv_sec);
	long	realtotalms	= (realtotalfns / (1000 * 1000)) % 1000;
	long	realtotalus	= (realtotalfns / 1000) % 1000;
	long	realtotalns	= realtotalfns % 1000;

	// additional measurement average time
	long	realavrgfns	= realtotalfns / static_cast<long>(pexeccntl->opt.proccnt * pexeccntl->opt.threadcnt * (pexeccntl->opt.loopcnt * (BENCH_TYPE_RW == pexeccntl->opt.bench_type ? 2 : 1)));	// Loop count
	long	realavrgsec	= realavrgfns / (1000 * 1000 * 1000);
	long	realavrgms	= (realavrgfns / (1000 * 1000)) % 1000;
	long	realavrgus	= (realavrgfns / 1000) % 1000;
	long	realavrgns	= realavrgfns % 1000;

	PRN("===========================================================");
	PRN("K2Hash bench mark");
	PRN("-----------------------------------------------------------");
	PRN("K2Hash type                   %s",			pexeccntl->opt.is_temp_mode ? "Temporary file" : ('\0' == pexeccntl->opt.szfile[0]) ? "Memory(no file)" : "Permanent file");
	PRN("File path                     %s",			pexeccntl->opt.is_temp_mode ? "tmpfile on system" : ('\0' == pexeccntl->opt.szfile[0]) ? "no file" : pexeccntl->opt.szfile);
	PRN("Attach type                   %s",			pexeccntl->opt.is_read_only ? "Read only" : "Read/Write");
	PRN("Mapping type                  %s",			pexeccntl->opt.is_fullmap ? "Full mapping" : "Partial mapping");
	PRN("Data structure");
	PRN("  Mask bit count              %d bit",		pexeccntl->opt.maskcnt);
	PRN("  Collision Mask bit count    %d bit",		pexeccntl->opt.cmaskcnt);
	PRN("  Element count               %d",			pexeccntl->opt.elementcnt);
	PRN("  Page size                   %zu byte",	pexeccntl->opt.pagesize);
	PRN("-----------------------------------------------------------");
	PRN("Access type for test          %s",			BENCH_TYPE_RO == pexeccntl->opt.bench_type ? "Read only" : BENCH_TYPE_WO == pexeccntl->opt.bench_type ? "Write only" : "Read/Write");
	PRN("Key length                    %s",			0 == pexeccntl->opt.datacnt ? "38 byte(\"KEY-000000000000000-000000000000000\")" : "21 byte(\"KEY-000000000000000\")");
	PRN("Value length                  %zu",		pexeccntl->opt.dlength);
	PRN("Initial data count            %d",			pexeccntl->opt.is_start_no_data ? 0 : pexeccntl->opt.datacnt);
	PRN("Total process(thread) count   %d",			pexeccntl->opt.proccnt * pexeccntl->opt.threadcnt);
	PRN("  Process count               %d",			pexeccntl->opt.proccnt);
	PRN("  Thread count                %d",			pexeccntl->opt.threadcnt);
	PRN("Total access count            %d",			pexeccntl->opt.proccnt * pexeccntl->opt.threadcnt * (pexeccntl->opt.loopcnt * (BENCH_TYPE_RW == pexeccntl->opt.bench_type ? 2 : 1)));
	PRN("  Loop count(each proc)       %d",			pexeccntl->opt.loopcnt);
	PRN("  Count of each loop          %d%s",		pexeccntl->opt.loopcnt * (BENCH_TYPE_RW == pexeccntl->opt.bench_type ? 2 : 1), BENCH_TYPE_RW == pexeccntl->opt.bench_type ? " (read and write access by each loop)" : " (one read/write access by each loop)");
	PRN("-----------------------------------------------------------");
	PRN("Total error count             %ld",			totalerr);
	PRN("Total time(addition thread)   %03lds %03ldms %03ldus %03ldns (%ldns)", addtotalsec, addtotalms, addtotalus, addtotalns, addtotalfns);
	PRN("Average time(addition thread) %03lds %03ldms %03ldus %03ldns (%ldns)", addavrgsec, addavrgms, addavrgus, addavrgns, addavrgfns);
	PRN("Total time(real)              %03lds %03ldms %03ldus %03ldns (%ldns)", realtotalsec, realtotalms, realtotalus, realtotalns, realtotalfns);
	PRN("Average time(real)            %03lds %03ldms %03ldus %03ldns (%ldns)", realavrgsec, realavrgms, realavrgus, realavrgns, realavrgfns);
	PRN("-----------------------------------------------------------");
}

//---------------------------------------------------------
// Child process
//---------------------------------------------------------
static void* RunThread(void* param)
{
	PTHPARAM	pThParam = reinterpret_cast<PTHPARAM>(param);
	if(!pThParam || !pThParam->pmycntl){
		ERR("Parameter for thread is wrong.");
		pthread_exit(NULL);
	}

	pThParam->pmycntl->threadid = gettid();
	if(!pThParam->pk2hshm || !pThParam->pexeccntl){
		ERR("Parameter for thread is wrong.");
		pThParam->pmycntl->is_ready = true;
		pThParam->pmycntl->is_exit = true;
		pthread_exit(NULL);
	}

	// wait for suspend flag off
	while(pThParam->pexeccntl->is_suspend && !pThParam->pexeccntl->is_exit){
		struct timespec	sleeptime = {0L, 10 * 1000 * 1000};	// 10ms
		nanosleep(&sleeptime, NULL);
	}
	if(pThParam->pexeccntl->is_exit){
		ERR("Exit thread ASSAP.");
		pThParam->pmycntl->is_ready	= true;
		pThParam->pmycntl->is_exit	= true;
		pthread_exit(NULL);
	}

	//---------------------------
	// Initialize internal datas.
	//---------------------------
	TestData	value(pThParam->pexeccntl->opt.dlength);									// initial value
	bool		is_uniq_key	= (0 == pThParam->pexeccntl->opt.datacnt);
	tid_t		tid_val		= gettid();
	char		szKey[KEY_BUFF_LENGTH];
	int			keynum;
	if(is_uniq_key){
		keynum = 0;
	}else{
		keynum = static_cast<int>(random()) % pThParam->pexeccntl->opt.datacnt;				// start key number by random.
	}

	// set datas and ready flag
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress redundantAssignment
	pThParam->pmycntl->threadid = gettid();
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress redundantAssignment
	pThParam->pmycntl->is_ready = true;

	// wait for start
	while(pThParam->pexeccntl->is_wait_doing && !pThParam->pexeccntl->is_exit){
		struct timespec	sleeptime = {0L, 1 * 1000 * 1000};	// 1ms
		nanosleep(&sleeptime, NULL);
	}
	if(pThParam->pexeccntl->is_exit){
		ERR("Exit thread ASSAP.");
		pThParam->pmycntl->is_exit = true;
		pthread_exit(NULL);
	}

	// set start timespec
	struct timespec	start;
	get_nomotonic_time(start);

	//---------------------------
	// Loop: Do read/write to k2hash
	//---------------------------
	for(int cnt = 0; cnt < pThParam->pexeccntl->opt.loopcnt; ++cnt){
		// make key
		if(is_uniq_key){
			sprintf(szKey, KEY_FORMAT_UNIQ, tid_val, keynum++);
		}else{
			sprintf(szKey, KEY_FORMAT, keynum);
			if(pThParam->pexeccntl->opt.datacnt <= ++keynum){
				keynum = 0;
			}
		}

		// write key
		if(0 != (pThParam->pexeccntl->opt.bench_type & BENCH_TYPE_WO)){
			if(!pThParam->pk2hshm->Set(szKey, value.Data())){
				// error
				pThParam->pmycntl->errorcnt++;
			}
		}

		// read key
		if(0 != (pThParam->pexeccntl->opt.bench_type & BENCH_TYPE_RO)){
			char*	pval;
			if(NULL == (pval = pThParam->pk2hshm->Get(szKey))){
				// error
				pThParam->pmycntl->errorcnt++;
			}else{
				K2H_Free(pval);
			}
		}
	}

	// set timespec and exit flag
	get_nomotonic_time(pThParam->pmycntl->ts, start);

	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress redundantAssignment
	pThParam->pmycntl->is_exit = true;

	pthread_exit(NULL);
	return NULL;
}

static int RunChild(string cntlfile)
{
	pid_t	pid = getpid();

	// mmap(attach) control file
	int			cntlfd		= -1;
	void*		pShmBase	= NULL;
	PEXECCNTL	pexeccntl	= NULL;
	PCHLDCNTL	pchldcntl	= NULL;
	size_t		totalsize	= 0;
	if(NULL == (pShmBase = MmapBenchFile(false, 0, 0, cntlfile, cntlfd, pexeccntl, pchldcntl, totalsize))){
		ERR("Could not mmap to file(%s), errno = %d", cntlfile.c_str(), errno);
		return EXIT_FAILURE;
	}

	// wait for suspend flag off
	while(pexeccntl->is_suspend && !pexeccntl->is_exit){
		struct timespec	sleeptime = {0L, 10 * 1000 * 1000};	// 10ms
		nanosleep(&sleeptime, NULL);
	}
	if(pexeccntl->is_exit){
		ERR("Exit process ASSAP.");
		munmap(pShmBase, totalsize);
		K2H_CLOSE(cntlfd);
		return EXIT_FAILURE;
	}

	// search my start pos of PCHLDCNTL.
	PCHLDCNTL	pmycntl = NULL;
	for(int cnt = 0; cnt < pexeccntl->opt.proccnt * pexeccntl->opt.threadcnt; cnt += pexeccntl->opt.threadcnt){
		if(pchldcntl[cnt].procid == pid){
			pmycntl = &pchldcntl[cnt];
			break;
		}
	}
	if(!pmycntl){
		ERR("Could not find my procid in PCHLDCNTL.");
		munmap(pShmBase, totalsize);
		K2H_CLOSE(cntlfd);
		return EXIT_FAILURE;
	}

	// Attach k2hash file
	K2HShm*	pk2hshm = new K2HShm();
	if(!pk2hshm->Attach(&pexeccntl->opt.szfile[0], pexeccntl->opt.is_read_only, false, pexeccntl->opt.is_temp_mode, pexeccntl->opt.is_fullmap, pexeccntl->opt.maskcnt, pexeccntl->opt.cmaskcnt, pexeccntl->opt.elementcnt, pexeccntl->opt.pagesize)){
		ERR("Could not attach k2hash file(%s).", pexeccntl->opt.szfile);
		munmap(pShmBase, totalsize);
		K2H_CLOSE(cntlfd);
		return EXIT_FAILURE;
	}

	// create threads
	PTHPARAM	pthparam = new THPARAM[pexeccntl->opt.threadcnt];
	for(int cnt = 0; cnt < pexeccntl->opt.threadcnt; ++cnt){
		// initialize
		pmycntl[cnt].procid		= pid;
		pmycntl[cnt].threadid	= 0;
		pmycntl[cnt].pthreadid	= 0;
		pmycntl[cnt].is_ready	= false;
		pmycntl[cnt].is_exit	= false;		// *1
		pmycntl[cnt].ts.tv_sec	= 0;
		pmycntl[cnt].ts.tv_nsec	= 0;
		pmycntl[cnt].errorcnt	= 0;

		// create thread
		pthparam[cnt].pk2hshm	= pk2hshm;
		pthparam[cnt].pexeccntl	= pexeccntl;
		pthparam[cnt].pmycntl	= &pmycntl[cnt];
		if(0 != pthread_create(&(pmycntl[cnt].pthreadid), NULL, RunThread, &(pthparam[cnt]))){
			ERR("Could not create thread.");
			pmycntl[cnt].is_exit = true;		// *1
			break;
		}
	}

	// wait all thread exit
	for(int cnt = 0; cnt < pexeccntl->opt.threadcnt; ++cnt){
		if(0 == pmycntl[cnt].pthreadid){
			continue;
		}
		void*		pretval = NULL;
		int			result;
		if(0 != (result = pthread_join(pmycntl[cnt].pthreadid, &pretval))){
			ERR("Failed to wait thread exit. return code(error) = %d", result);
			continue;
		}
	}
	K2H_Delete(pthparam);

	// exit
	return EXIT_SUCCESS;
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	BOPTS	benchopts;
	init_bench_opts(benchopts);

	if(!SetBenchOptions(argc, argv, benchopts)){
		// option(parameter) error
		exit(EXIT_FAILURE);
	}

	// help
	if(benchopts.is_help){
		Help(argv[0]);
		exit(EXIT_SUCCESS);
	}

	// debug mode
	SetK2hDbgMode(benchopts.dbglevel);

	// Initialize control file
	string		cntlfile;
	int			cntlfd		= -1;
	void*		pShmBase	= NULL;
	PEXECCNTL	pexeccntl	= NULL;
	PCHLDCNTL	pchldcntl	= NULL;
	size_t		totalsize	= 0;
	if(NULL == (pShmBase = MmapBenchFile(true, benchopts.proccnt, benchopts.threadcnt, cntlfile, cntlfd, pexeccntl, pchldcntl, totalsize))){
		ERR("Could not mmap to file(%s), errno = %d", cntlfile.c_str(), errno);
		exit(EXIT_SUCCESS);
	}
	pexeccntl->procid		= getpid();
	pexeccntl->is_suspend	= true;
	pexeccntl->is_wait_doing= true;
	pexeccntl->is_exit		= false;
	copy_bench_opts(pexeccntl->opt, benchopts);
	for(int cnt = 0; cnt < (benchopts.proccnt * benchopts.threadcnt); ++cnt){
		pchldcntl[cnt].procid		= 0;
		pchldcntl[cnt].threadid		= 0;
		pchldcntl[cnt].pthreadid	= 0;
		pchldcntl[cnt].is_ready		= false;
		pchldcntl[cnt].is_exit		= true;
		pchldcntl[cnt].ts.tv_sec	= 0;
		pchldcntl[cnt].ts.tv_nsec	= 0;
		pchldcntl[cnt].errorcnt		= 0;
	}

	// Initialize data(k2hash)
	K2HShm*	pk2hshm;
	if(NULL == (pk2hshm = InitializeK2HashFile(benchopts))){
		munmap(pShmBase, totalsize);
		K2H_CLOSE(cntlfd);
		exit(EXIT_FAILURE);
	}

	// timespec for real time measurement
	struct timespec	realstart;
	struct timespec	realtime;

	// Run child process/threads
	if(1 < benchopts.proccnt){
		// multi process bench(with multi thread)

		// detach k2hash file
		pk2hshm->Detach();
		K2H_Delete(pk2hshm);

		// create child process
		int	childcnt = 0;
		for(childcnt = 0; childcnt < benchopts.proccnt; ++childcnt){
			// initialize
			for(int pcnt = 0; pcnt < benchopts.threadcnt; ++pcnt){
				pchldcntl[childcnt * benchopts.threadcnt + pcnt].procid		= 0;
				pchldcntl[childcnt * benchopts.threadcnt + pcnt].threadid	= 0;
				pchldcntl[childcnt * benchopts.threadcnt + pcnt].pthreadid	= 0;
				pchldcntl[childcnt * benchopts.threadcnt + pcnt].is_ready	= false;
				pchldcntl[childcnt * benchopts.threadcnt + pcnt].is_exit	= false;	// *1
				pchldcntl[childcnt * benchopts.threadcnt + pcnt].ts.tv_sec	= 0;
				pchldcntl[childcnt * benchopts.threadcnt + pcnt].ts.tv_nsec	= 0;
				pchldcntl[childcnt * benchopts.threadcnt + pcnt].errorcnt	= 0;
			}

			pid_t	pid = fork();
			if(-1 == pid){
				ERR("Could not fork child process, errno = %d", errno);
				pexeccntl->is_exit = true;

				for(int pcnt = 0; pcnt < benchopts.threadcnt; ++pcnt){
					pchldcntl[childcnt * benchopts.threadcnt + pcnt].is_exit = true;	// *1
				}
				break;
			}
			if(0 == pid){
				// child process
				int	result = RunChild(cntlfile);
				exit(result);

			}else{
				// parent process
				for(int pcnt = 0; pcnt < benchopts.threadcnt; ++pcnt){
					pchldcntl[childcnt * benchopts.threadcnt + pcnt].procid = pid;
				}
			}
		}

		// start children initializing
		pexeccntl->is_suspend	= false;

		// wait all children ready
		if(!pexeccntl->is_exit){
			// wait for all children is ready
			for(bool is_all_initialized = false; !is_all_initialized; ){
				is_all_initialized = true;
				for(int cnt = 0; cnt < (benchopts.proccnt * benchopts.threadcnt); ++cnt){
					if(!pchldcntl[cnt].is_exit && !pchldcntl[cnt].is_ready){
						is_all_initialized = false;
						break;
					}
				}
				if(!is_all_initialized){
					struct timespec	sleeptime = {0L, 10 * 1000 * 1000};	// 10ms
					nanosleep(&sleeptime, NULL);
				}
			}
		}

		// set start time point
		get_nomotonic_time(realstart);

		// start(no blocking) doing
		pexeccntl->is_wait_doing = false;

		// wait all process exit
		pid_t	exitpid;
		int		status = 0;
		while(0 < (exitpid = waitpid(-1, &status, 0))){
			--childcnt;
			if(0 >= childcnt){
				break;
			}
		}

		// set measurement timespec
		get_nomotonic_time(realtime, realstart);

	}else{
		// single process bench

		// create threads
		PTHPARAM	pthparam = new THPARAM[pexeccntl->opt.threadcnt];
		for(int cnt = 0; cnt < pexeccntl->opt.threadcnt; ++cnt){
			// initialize
			pchldcntl[cnt].procid		= getpid();
			pchldcntl[cnt].threadid		= 0;
			pchldcntl[cnt].pthreadid	= 0;
			pchldcntl[cnt].is_ready		= false;
			pchldcntl[cnt].is_exit		= false;	// *1
			pchldcntl[cnt].ts.tv_sec	= 0;
			pchldcntl[cnt].ts.tv_nsec	= 0;
			pchldcntl[cnt].errorcnt		= 0;

			// create thread
			pthparam[cnt].pk2hshm	= pk2hshm;
			pthparam[cnt].pexeccntl	= pexeccntl;
			pthparam[cnt].pmycntl	= &pchldcntl[cnt];
			if(0 != pthread_create(&(pchldcntl[cnt].pthreadid), NULL, RunThread, &(pthparam[cnt]))){
				ERR("Could not create thread.");
				pexeccntl->is_exit		= true;
				pchldcntl[cnt].is_exit	= true;		// *1
				break;
			}
		}

		// start children initializing
		pexeccntl->is_suspend	= false;

		// wait all children ready
		// cppcheck-suppress unmatchedSuppression
		// cppcheck-suppress knownConditionTrueFalse
		if(!pexeccntl->is_exit){
			// wait for all children is ready
			for(bool is_all_initialized = false; !is_all_initialized; ){
				is_all_initialized = true;
				for(int cnt = 0; cnt < benchopts.threadcnt; ++cnt){
					if(!pchldcntl[cnt].is_exit && !pchldcntl[cnt].is_ready){
						is_all_initialized = false;
						break;
					}
				}
				if(!is_all_initialized){
					struct timespec	sleeptime = {0L, 10 * 1000 * 1000};	// 10ms
					nanosleep(&sleeptime, NULL);
				}
			}
		}

		// set start time point
		get_nomotonic_time(realstart);

		// start(no blocking) doing
		pexeccntl->is_wait_doing = false;

		// wait all thread exit
		for(int cnt = 0; cnt < pexeccntl->opt.threadcnt; ++cnt){
			if(0 == pchldcntl[cnt].pthreadid){
				continue;
			}
			void*		pretval = NULL;
			int			result;
			if(0 != (result = pthread_join(pchldcntl[cnt].pthreadid, &pretval))){
				ERR("Failed to wait thread exit. return code(error) = %d", result);
				continue;
			}
		}

		// set measurement timespec
		get_nomotonic_time(realtime, realstart);

		K2H_Delete(pthparam);

		// detach k2hash file
		pk2hshm->Detach();
		K2H_Delete(pk2hshm);
	}

	// display result
	PrintResult(pexeccntl, pchldcntl, realtime);

	// cleanup
	if('\0' != benchopts.szfile[0]){
		unlink(benchopts.szfile);
	}
	munmap(pShmBase, totalsize);
	K2H_CLOSE(cntlfd);
	unlink(cntlfile.c_str());

	exit(EXIT_SUCCESS);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
