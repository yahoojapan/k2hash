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
 * CREATE:   Tue Apr 22 2014
 * REVISION:
 *
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <libgen.h>
#include <string>
#include <map>

#include <k2hash.h>
#include <k2hcommon.h>
#include <k2hshm.h>
#include <k2hutil.h>
#include <k2hashfunc.h>
#include <k2harchive.h>
#include <k2hdbg.h>

using namespace std;

//---------------------------------------------------------
// Utilities
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

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
// Parse parameters
//
// FILE                 compress k2hash file
// -replace             make and replace temporary file for compress
// -direct              compress directly with mmap
// -print               print only area information
// -mask <bit count>    bit mask count for hash
// -cmask <bit count>   collision bit mask count
// -elementcnt <count>  element count for each hash table
// -pagesize <number>   pagesize for each data
// -ext <library path>  extension library for hash function
// -g <debug level>     debugging mode: ERR(default) / WAN / INFO
//
typedef std::map<std::string, std::string> params_t;

static void Help(char* progname)
{
	PRN("");
	PRN("Usage: %s [-replace | -direct | -print] [options] FILE", progname ? programname(progname) : "program");
	PRN("");
	PRN("Option -h                   help display");
	PRN("       -replace             make and replace temporary file for compress");
	PRN("       -direct              compress directly with mmap");
	PRN("       -print               print only area information");
	PRN("       -mask <bit count>    bit mask count for hash(*1)");
	PRN("       -cmask <bit count>   collision bit mask count(*1)");
	PRN("       -elementcnt <count>  element count for each hash table(*1)");
	PRN("       -pagesize <number>   pagesize for each data");
	PRN("       -ext <library path>  extension library for hash function");
	PRN("       -g <debug level>     debugging mode: ERR(default) / WAN / INFO(*2)");
	PRN(NULL);
	PRN("(*1)These option(value) is for debugging about extending hash area.");
	PRN("    Usually, you don\'t need to specify these.");
	PRN("(*2)You can set debug level by another way which is setting environment as \"K2HDBGMODE\".");
	PRN("    \"K2HDBGMODE\" environment is took as \"SILENT\", \"ERR\", \"WAN\" or \"INFO\" value.");
	PRN("    When this process gets SIGUSR1 signal, the debug level is bumpup.");
	PRN("    The debug level is changed as \"SILENT\"->\"ERR\"->\"WAN\"->\"INFO\"->...");
	PRN("(*3)You can set debugging message log file by the environment. \"K2HDBGFILE\".");
	PRN("");
}

static bool ParameterParser(int argc, char** argv, params_t& params)
{
	params.clear();

	for(int nCnt = 1; nCnt < argc; nCnt++){		// argv[0] = progname
		if('-' != argv[nCnt][0]){
			if(params.end() == params.find("FILE")){
				params["FILE"] = argv[nCnt];
			}else{
				ERR("Wrong parameter(%s).", argv[nCnt]);
				return false;
			}
		}
		if(0 == strcasecmp(argv[nCnt], "-h")){
			params["-h"] = "";
		}else if(0 == strcasecmp(argv[nCnt], "-g")){
			params["-g"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-direct")){
			params["-direct"] = "";
		}else if(0 == strcasecmp(argv[nCnt], "-replace")){
			params["-replace"] = "";
		}else if(0 == strcasecmp(argv[nCnt], "-print")){
			params["-print"] = "";
		}else if(0 == strcasecmp(argv[nCnt], "-mask")){
			params["-mask"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-cmask")){
			params["-cmask"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-elementcnt")){
			params["-elementcnt"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-pagesize")){
			params["-pagesize"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-ext")){
			params["-ext"] = argv[++nCnt];
		}
	}
	return true;
}

static bool Confirm(const char* pMsg)
{
	char	szBuff[256];
	while(true){
		fprintf(stdout, "%s", pMsg ? pMsg : "[Y/N] ");
		if(NULL == fgets(szBuff, sizeof(szBuff), stdin)){
			return false;
		}
		if(0 == strcasecmp(szBuff, "Y\n")){
			break;
		}else if(0 == strcasecmp(szBuff, "N\n")){
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
	params_t	params;
	K2HShm		k2hash;

	if(!ParameterParser(argc, argv, params)){
		Help(argv[0]);
		exit(-1);
	}
	if(params.end() != params.find("-h")){
		Help(argv[0]);
		exit(0);
	}

	// DBG Mode
	bool	isDeepDebugMode = false;
	if(params.end() != params.find("-g")){
		if(0 == strcasecmp(params["-g"].c_str(), "ERR")){
			SetK2hDbgMode(K2HDBG_ERR);
		}else if(0 == strcasecmp(params["-g"].c_str(), "WAN")){
			SetK2hDbgMode(K2HDBG_WARN);
		}else if(0 == strcasecmp(params["-g"].c_str(), "INFO")){
			SetK2hDbgMode(K2HDBG_MSG);
			isDeepDebugMode = true;
		}else{
			ERR("Wrong parameter value \"-g\" %s.", params["-g"].c_str());
			exit(-1);
		}
	}

	// parameters
	string		strFile;
	bool		isDirect		= false;
	bool		isPrint			= false;
	int			MaskBitCnt		= 0;
	int			CMaskBitCnt		= 0;
	int			MaxElementCnt	= 0;
	size_t		PageSize		= 0UL;

	if(params.end() != params.find("FILE")){
		strFile = params["FILE"].c_str();
	}else{
		ERR("must set k2hash file.");
		exit(-1);
	}

	if(params.end() != params.find("-direct")){
		if(params.end() != params.find("-replace") || params.end() != params.find("-print")){
			ERR("parameter \"-direct\" could not set with \"-replace\" and \"-print\".");
			exit(-1);
		}
		isDirect = true;
	}else if(params.end() != params.find("-replace")){
		if(params.end() != params.find("-print")){
			ERR("parameter \"-replace\" could not set with \"-direct\" and \"-print\".");
			exit(-1);
		}
		isDirect = false;
	}else if(params.end() != params.find("-print")){
		isPrint = true;
	}else{
		ERR("must specify \"-replace\" or \"-direct\" or \"-print\" option.");
		exit(-1);
	}

	if(params.end() != params.find("-mask")){
		if(isDirect || isPrint){
			ERR("parameter \"-direct\" or \"-print\" could not set with \"-mask\".");
			exit(-1);
		}
		MaskBitCnt = atoi(params["-mask"].c_str());
	}
	if(params.end() != params.find("-cmask")){
		if(isDirect || isPrint){
			ERR("parameter \"-direct\" or \"-print\" could not set with \"-mask\".");
			exit(-1);
		}
		CMaskBitCnt = atoi(params["-cmask"].c_str());
	}
	if(params.end() != params.find("-elementcnt")){
		if(isDirect || isPrint){
			ERR("parameter \"-direct\" or \"-print\" could not set with \"-mask\".");
			exit(-1);
		}
		MaxElementCnt = atoi(params["-elementcnt"].c_str());
	}
	if(params.end() != params.find("-pagesize")){
		if(isDirect || isPrint){
			ERR("parameter \"-direct\" or \"-print\" could not set with \"-mask\".");
			exit(-1);
		}
		PageSize = atoi(params["-pagesize"].c_str());
		if(PageSize < static_cast<size_t>(K2HShm::MIN_PAGE_SIZE)){
			ERR("Option \"-pagesize\" parameter(%zu) is under minimum pagesize(%d).", PageSize, K2HShm::MIN_PAGE_SIZE);
			exit(-1);
		}
	}
	// -ext
	if(params.end() != params.find("-ext")){
		if(!K2HashDynLib::get()->Load(params["-ext"].c_str())){
			ERR("Failed to load library(%s).", params["-ext"].c_str());
			exit(-1);
		}
	}

	if(isPrint){
		// attach k2hash ( update monitor file automatically )
		if(!k2hash.Attach(strFile.c_str(), false, false, false, false)){
			ERR("Could not attach replaced file %s", strFile.c_str());
			exit(-1);
		}

		if(!k2hash.PrintAreaInfo()){
			ERR("Failed to print all k2hash area.");
			k2hash.Detach();
			exit(-1);
		}

		// detach k2hash
		if(!k2hash.Detach()){
			ERR("Failed to detach k2hash.");
		}

	}else if(isDirect){
		PRN("");
		PRN("[NOTICE] \"-direct\" is unsupported mode,");
		PRN("         SHOULD MAKE BACKUP FILE or ARCHIVE.");
		PRN("         This processing probably takes many time.");
		PRN("         If you stop this processing, maybe k2hash file is broken.");
		if(!Confirm("         Do you continue? [Y/N] ")){
			PRN("Bye...");
			exit(0);
		}
		PRN("");

		// attach k2hash ( update monitor file automatically )
		if(!k2hash.Attach(strFile.c_str(), false, false, false, false)){
			ERR("Could not attach replaced file %s", strFile.c_str());
			exit(-1);
		}

		bool	isCompressed = false;
		if(!k2hash.AreaCompress(isCompressed)){
			ERR("Failed to compress %s file directly.", strFile.c_str());
			k2hash.Detach();
			exit(-1);
		}

		// detach k2hash
		if(!k2hash.Detach()){
			ERR("Failed to detach k2hash.");
		}
		PRN("Compress %s file %s.", strFile.c_str(), isCompressed ? "succeed" : "finished but no compress");
		PRN("");

	}else{
		// replace mode
		PRN("");
		PRN("[NOTICE] This processing probably takes many time.");
		PRN("         If you stop this processing, maybe k2hash file is broken.");
		if(!Confirm("         Do you continue? [Y/N] ")){
			PRN("Bye...");
			exit(0);
		}
		PRN("");

		// attach k2hash ( read mode )
		if(!k2hash.Attach(strFile.c_str(), true, false, false, false)){
			ERR("Could not attach replaced file %s", strFile.c_str());
			exit(-1);
		}

		// if is not specified parameters, get now file's mask, cmask, elecnt, pagesize
		if(0 == MaskBitCnt){
			MaskBitCnt = K2HShm::GetMaskBitCount(k2hash.GetCurrentMask());
		}
		if(0 == CMaskBitCnt){
			CMaskBitCnt = K2HShm::GetMaskBitCount(k2hash.GetCollisionMask());
		}
		if(0 == MaxElementCnt){
			MaxElementCnt = static_cast<int>(k2hash.GetMaxElementCount());
		}
		if(0UL == PageSize){
			PageSize = k2hash.GetPageSize();
		}

		// make file paths
		string	strOrgFile = k2hash.GetK2hashFilePath();
		string	strTmpFile = strOrgFile + ".compress_tmp";
		string	strArFile = strOrgFile + ".compress_ar";

		// puts archive file from now file
		struct stat	st;
		if(0 == stat(strArFile.c_str(), &st)){
			WAN_K2HPRN("Archive File(%s) exists, so try to remove it.", strArFile.c_str());
			if(0 != unlink(strArFile.c_str())){
				ERR_K2HPRN("Archive File(%s) exists, but remove it. errno(%d)", strArFile.c_str(), errno);
				k2hash.Detach();
				exit(-1);
			}
		}
		K2HArchive	archiveobj;
		if(!archiveobj.Initialize(strArFile.c_str(), false)){
			ERR("Could not initialize archive object(%s).", strArFile.c_str());
			k2hash.Detach();
			exit(-1);
		}
		if(!archiveobj.Serialize(&k2hash, false)){
			ERR("Something error occurred during putting archive file(%s).", strArFile.c_str());
			k2hash.Detach();
			exit(-1);
		}

		// close current file.
		k2hash.Detach();

		// Create new temporary file
		K2HShm	k2hashTmp;
		if(!k2hashTmp.Create(strTmpFile.c_str(), false, MaskBitCnt, CMaskBitCnt, MaxElementCnt, PageSize)){
			ERR("Could not create new k2hash file(%s) for replacing with compress.", strTmpFile.c_str());
			exit(-1);
		}

		// load archive to new file
		if(!archiveobj.Initialize(strArFile.c_str(), false)){
			ERR("Could not initialize archive object(%s).", strArFile.c_str());
			k2hashTmp.Detach();
			exit(-1);
		}
		if(!archiveobj.Serialize(&k2hashTmp, true)){
			ERR("Something error occurred during loading archive file(%s).", strArFile.c_str());
			k2hashTmp.Detach();
			exit(-1);
		}

		// close temporary file.
		k2hashTmp.Detach();

		// replace temporary file to current file by launching tool.
		string	strCommand;
		strCommand  = "k2hreplace";
		strCommand += isDeepDebugMode ? " -d " : " ";
		strCommand += strTmpFile;
		strCommand += " ";
		strCommand += strOrgFile;
		strCommand += isDeepDebugMode ? "" : " >/dev/null 2>$1";

		int	result = system(strCommand.c_str());
		if(0 != result){
			ERR("Something error occurred during replacing k2hash file. result code is %d.", result);
			exit(result);
		}
		PRN("Succeed to replace and compress k2hash file.");
	}
	exit(0);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
