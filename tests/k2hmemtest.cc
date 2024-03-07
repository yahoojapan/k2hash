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
// Symbols
//---------------------------------------------------------
#define	DEFAULT_VALUE	"VALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUEVALUE"

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
// Parse parameters
//
// -g [debug level]		"ERR" "WAN" "INF"
// -mask				Mask bit count
// -cmask				CMask bit count
// -elementcnt			Max element count
// -pagesize			Page size
// -d [Dump mode]		"KINDEX" "CKINDEX" "ELEMENT" "FULL"
// -h					display help
//
typedef std::map<std::string, std::string> params_t;

static void Help(const char* progname)
{
	printf("Usage: %s [options] BASEKEYSTR COUNT [VALUE]\n", progname ? progname : "program");
	printf("BASEKEYSTR                Key base string\n");
	printf("COUNT                     Write key count\n");
	printf("VALUE                     Value string\n");
	printf("Option  -g [debug level]  \"ERR\" / \"WAN\" / \"INF\"\n");
	printf("        -mask             Mask bit count\n");
	printf("        -cmask            CMask bit count\n");
	printf("        -elementcnt       Max element count\n");
	printf("        -pagesize         Page size\n");
	printf("        -d [Dump mode]    \"HEAD\" / \"KINDEX\" / \"CKINDEX\" / \"ELEMENT\" / \"FULL\"\n");
	printf("        -h                display help\n");
}

static bool ParameerParser(int argc, char** argv, params_t& params)
{
	params.clear();

	for(int nCnt = 1; nCnt < argc; nCnt++){		// argv[0] = progname
		if('-' != argv[nCnt][0]){
			if(params.end() == params.find("BASEKEYSTR")){
				params["BASEKEYSTR"] = argv[nCnt];
			}else if(params.end() == params.find("COUNT")){
				params["COUNT"] = argv[nCnt];
			}else if(params.end() == params.find("VAL")){
				params["VAL"] = argv[nCnt];
			}else{
				ERR_K2HPRN("Wrong parameter(%s).", argv[nCnt]);
				return false;
			}
		}
		if(0 == strcasecmp(argv[nCnt], "-g")){
			params["-g"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-mask")){
			params["-mask"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-cmask")){
			params["-cmask"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-elementcnt")){
			params["-elementcnt"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-pagesize")){
			params["-pagesize"] = argv[++nCnt];
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

	string		strKeyBase		= "";
	long		MaxCount		= 0L;
	string		strValue		= DEFAULT_VALUE;
	int			MaskBitCnt		= K2HShm::DEFAULT_MASK_BITCOUNT;
	int			CMaskBitCnt		= K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT;
	int			MaxElementCnt	= K2HShm::DEFAULT_MAX_ELEMENT_CNT;
	size_t		PageSize		= K2HShm::MIN_PAGE_SIZE;
	int			dumpmask		= K2HShm::DUMP_KINDEX_ARRAY;

	if(params.end() == params.find("BASEKEYSTR")){
		ERR_K2HPRN("Not found \"BASEKEYSTR\" parameter.");
		exit(-1);
	}else{
		strKeyBase = params["BASEKEYSTR"].c_str();
		strKeyBase += "-";
	}
	if(params.end() == params.find("COUNT")){
		ERR_K2HPRN("Not found \"COUNT\" parameter.");
		exit(-1);
	}else{
		MaxCount = atol(params["COUNT"].c_str());
		if(0 >= MaxCount){
			ERR_K2HPRN("\"COUNT\" parameter is wrong(%ld).", MaxCount);
			exit(-1);
		}
	}
	if(params.end() != params.find("VAL")){
		strValue = params["VAL"].c_str();
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
	printf("   Test for Read/Write K2HASH\n");
	printf("-------------------------------------------------------\n");
	printf("Write key base prefix:                  %s\n", strKeyBase.c_str());
	printf("Write key count:                        %ld\n", MaxCount);
	printf("Write value:                            %s\n", strValue.c_str());
	printf("Initial Key Index mask count:           %d\n", MaskBitCnt);
	printf("Initial Collision Key Index mask count: %d\n", CMaskBitCnt);
	printf("Initial Max element count:              %d\n", MaxElementCnt);
	printf("Initial Page size:                      %zu\n",PageSize);
	printf("Dump mode:                              %s\n", params["-d"].c_str());
	printf("Extra hash function library:            %s\n", params.end() != params.find("-ext") ? params["-ext"].c_str() : "(not load)");
	printf("Hash functions version:                 %s\n", K2H_HASH_VER_FUNC());
	printf("-------------------------------------------------------\n");

	if(!k2hash.AttachMem(MaskBitCnt, CMaskBitCnt, MaxElementCnt, PageSize)){
		ERR_K2HPRN("Failed to load/mmap.");
		exit(-1);
	}

	printf("   K2H Memory\n");
	printf("-------------------------------------------------------\n");

	// Dump head
	if(!k2hash.Dump(stdout, K2HShm::DUMP_HEAD)){
		ERR_K2HPRN("Failed to head dump.");
		exit(-1);
	}

	printf("-------------------------------------------------------\n");

	// Set data
	for(long cnt = 0; cnt < MaxCount; cnt++){
		string	strkey;
		char	szBuff[64];
		sprintf(szBuff, "%ld", cnt);
		strkey = strKeyBase;
		strkey += szBuff;
		if(!k2hash.Set(strkey.c_str(), strValue.c_str())){
			ERR_K2HPRN("Failed to write data(\n\t%s =>\n\t%s\n)", strkey.c_str(), strValue.c_str());
			exit(-1);
		}
		printf("KEY: %s\t\t==> done.\n", strkey.c_str());
	}

	printf("-------------------------------------------------------\n");
	printf("   Dump after writing\n");
	printf("-------------------------------------------------------\n");

	// Dump
	if(!k2hash.Dump(stdout, dumpmask)){
		ERR_K2HPRN("Failed to dump.");
		exit(-1);
	}

	printf("-------------------------------------------------------\n");
	printf("Read/Write Finish\n");
	printf("-------------------------------------------------------\n");

	if(!k2hash.Detach()){
		WAN_K2HPRN("Failed to detach.");
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
