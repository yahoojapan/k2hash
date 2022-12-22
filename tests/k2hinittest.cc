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
#include <map>
#include <string>

#include <k2hash.h>
#include <k2hcommon.h>
#include <k2hshm.h>
#include <k2hdbg.h>
#include <k2hashfunc.h>

using namespace std;

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
// Parse parameters
//
// -f [file name]		File path
// -g [debug level]		"ERR" "WAN" "INF"
// -mask				Mask bit count
// -cmask				CMask bit count
// -elementcnt			Max element count
// -pagesize            Page size
// -nofullmap			Not full mapping
// -ro					read only mode
// -tmp					file is temporary
// -d [Dump mode]		"KINDEX" "CKINDEX" "ELEMENT" "FULL"
// -h					display help
//
typedef std::map<std::string, std::string> params_t;

static void Help(char* progname)
{
	printf("Usage: %s [options]\n", progname ? progname : "program");
	printf("Option  -f [file name]    File path\n");
	printf("        -g [debug level]  \"ERR\" / \"WAN\" / \"INF\"\n");
	printf("        -mask             Mask bit count\n");
	printf("        -cmask            CMask bit count\n");
	printf("        -elementcnt       Max element count\n");
	printf("        -pagesize         Page size\n");
	printf("        -nofullmap        Not full mapping\n");
	printf("        -ro               read only mode\n");
	printf("        -tmp              file is temporary\n");
	printf("        -d [Dump mode]    \"HEAD\" / \"KINDEX\" / \"CKINDEX\" / \"ELEMENT\" / \"FULL\"\n");
	printf("        -h                display help\n");
}

static bool ParameerParser(int argc, char** argv, params_t& params)
{
	params.clear();

	for(int nCnt = 1; nCnt < argc; nCnt++){		// argv[0] = progname
		if('-' != argv[nCnt][0]){
			ERR_K2HPRN("Wrong parameter(%s).", argv[nCnt]);
			return false;
		}
		if(0 == strcasecmp(argv[nCnt], "-f")){
			params["-f"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-g")){
			params["-g"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-mask")){
			params["-mask"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-cmask")){
			params["-cmask"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-elementcnt")){
			params["-elementcnt"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-pagesize")){
			params["-pagesize"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-nofullmap")){
			params["-nofullmap"] = "";
		}else if(0 == strcasecmp(argv[nCnt], "-ro")){
			params["-ro"] = "";
		}else if(0 == strcasecmp(argv[nCnt], "-tmp")){
			params["-tmp"] = "";
		}else if(0 == strcasecmp(argv[nCnt], "-d")){
			params["-d"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-h")){
			params["-h"] = "";
		}
	}
	return true;
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	params_t		params;
	K2HShm			k2hash;

	if(!ParameerParser(argc, argv, params)){
		Help(argv[0]);
		exit(-1);
	}
	if(params.end() != params.find("-h")){
		Help(argv[0]);
		exit(0);
	}

	// DBG Mode
	if(params.end() != params.find("-g")){
		if(0 == strcasecmp(params["-g"].c_str(), "ERR")){
			SetK2hDbgMode(K2HDBG_ERR);
		}else if(0 == strcasecmp(params["-g"].c_str(), "WAN")){
			SetK2hDbgMode(K2HDBG_WARN);
		}else if(0 == strcasecmp(params["-g"].c_str(), "INF")){
			SetK2hDbgMode(K2HDBG_MSG);
		}else{
			ERR_K2HPRN("Wrong parameter value \"-g\" %s.", params["-g"].c_str());
			exit(-1);
		}
	}

	const char*	pFilePath		= NULL;
	int			MaskBitCnt		= K2HShm::DEFAULT_MASK_BITCOUNT;
	int			CMaskBitCnt		= K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT;
	int			MaxElementCnt	= K2HShm::DEFAULT_MAX_ELEMENT_CNT;
	size_t		PageSize		= K2HShm::MIN_PAGE_SIZE;
	bool		isReadOnly		= false;
	bool		isTempFile		= false;
	bool		isFullmap		= true;
	int			dumpmask		= K2HShm::DUMP_KINDEX_ARRAY;

	if(params.end() != params.find("-f")){
		pFilePath = params["-f"].c_str();
	}
	if(params.end() != params.find("-mask")){
		MaskBitCnt = atoi(params["-mask"].c_str());
	}
	if(params.end() != params.find("-cmask")){
		CMaskBitCnt = atoi(params["-cmask"].c_str());
	}
	if(params.end() != params.find("-elementcnt")){
		MaxElementCnt = atoi(params["-elementcnt"].c_str());
	}
	if(params.end() != params.find("-pagesize")){
		PageSize = atoi(params["-pagesize"].c_str());
		if(PageSize < static_cast<size_t>(K2HShm::MIN_PAGE_SIZE)){
			ERR_K2HPRN("Option \"-pagesize\" parameter(%zu) is under minimum pagesize(%d).", PageSize, K2HShm::MIN_PAGE_SIZE);
			exit(-1);
		}
	}
	if(params.end() != params.find("-nofullmap")){
		isFullmap	= false;
	}
	if(params.end() != params.find("-ro")){
		isReadOnly	= true;
	}
	if(params.end() != params.find("-tmp")){
		isTempFile	= true;
	}
	if(params.end() != params.find("-d")){
		if(0 == strcasecmp(params["-d"].c_str(), "HEAD")){
			dumpmask = K2HShm::DUMP_HEAD;
		}else if(0 == strcasecmp(params["-d"].c_str(), "KINDEX")){
			dumpmask = K2HShm::DUMP_KINDEX_ARRAY;
		}else if(0 == strcasecmp(params["-d"].c_str(), "CKINDEX")){
			dumpmask = K2HShm::DUMP_CKINDEX_ARRAY;
		}else if(0 == strcasecmp(params["-d"].c_str(), "ELEMENT")){
			dumpmask = K2HShm::DUMP_ELEMENT_LIST;
		}else if(0 == strcasecmp(params["-d"].c_str(), "FULL")){
			dumpmask = K2HShm::DUMP_PAGE_LIST;
		}else{
			ERR_K2HPRN("Wrong parameter value \"-d\" %s.", params["-d"].c_str());
			exit(-1);
		}
	}

	printf("-------------------------------------------------------\n");
	printf("   Test for initializing K2HASH\n");
	printf("-------------------------------------------------------\n");
	printf("File Path:                              %s\n", pFilePath ? pFilePath : "(not specified = on memory mode)");
	printf("Initial Key Index mask count:           %d\n", MaskBitCnt);
	printf("Initial Collision Key Index mask count: %d\n", CMaskBitCnt);
	printf("Initial Max element count:              %d\n", MaxElementCnt);
	printf("Initial Page size:                      %zu\n",PageSize);
	printf("Read only mode:                         %s\n", isReadOnly ? "true" : "false");
	printf("Dump mode:                              %s\n", params["-d"].c_str());
	printf("Extra hash function library:            %s\n", params.end() != params.find("-ext") ? params["-ext"].c_str() : "(not load)");
	printf("-------------------------------------------------------\n");
	printf("\n");

	if(!k2hash.Attach(pFilePath, isReadOnly, isReadOnly ? false : true, isTempFile, isFullmap, MaskBitCnt, CMaskBitCnt, MaxElementCnt, PageSize)){
		ERR_K2HPRN("Failed to load/mmap.");
		exit(-1);
	}

	if(!k2hash.Dump(stdout, dumpmask)){
		ERR_K2HPRN("Failed to dump.");
		exit(-1);
	}

	if(!k2hash.Detach()){
		WAN_K2HPRN("Failed to detach.");
	}

	printf("\n");
	printf("-------------------------------------------------------\n");
	printf("Dump finish\n");
	printf("-------------------------------------------------------\n\n");

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
