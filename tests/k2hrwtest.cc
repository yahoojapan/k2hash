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
// -nofullmap			Not full mapping
// -ro					read only mode
// -del 				delete key
// -h					display help
//
typedef std::map<std::string, std::string> params_t;

static void Help(char* progname)
{
	printf("Usage: %s [options] KEY [VALUE]\n", progname ? progname : "program");
	printf("KEY                       Key string\n");
	printf("VALUE                     Value string\n");
	printf("Option  -f [file name]    File path\n");
	printf("        -g [debug level]  \"ERR\" / \"WAN\" / \"INF\"\n");
	printf("        -nofullmap        Not full mapping\n");
	printf("        -ro               read only mode\n");
	printf("        -del              delete key\n");
	printf("        -h                display help\n");
}

static bool ParameerParser(int argc, char** argv, params_t& params)
{
	params.clear();

	for(int nCnt = 1; nCnt < argc; nCnt++){		// argv[0] = progname
		if('-' != argv[nCnt][0]){
			if(params.end() == params.find("KEY")){
				params["KEY"] = argv[nCnt];
			}else if(params.end() == params.find("VAL")){
				params["VAL"] = argv[nCnt];
			}else{
				ERR_K2HPRN("Wrong parameter(%s).", argv[nCnt]);
				return false;
			}
		}
		if(0 == strcasecmp(argv[nCnt], "-f")){
			params["-f"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-g")){
			params["-g"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-nofullmap")){
			params["-nofullmap"] = "";
		}else if(0 == strcasecmp(argv[nCnt], "-ro")){
			params["-ro"] = "";
		}else if(0 == strcasecmp(argv[nCnt], "-del")){
			params["-del"] = "";
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

	if(params.end() == params.find("KEY")){
		ERR_K2HPRN("Not found \"KEY\" parameter.");
		exit(-1);
	}

	const char*	pFilePath		= NULL;
	int			MaskBitCnt		= K2HShm::DEFAULT_MASK_BITCOUNT;
	int			CMaskBitCnt		= K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT;
	int			MaxElementCnt	= K2HShm::DEFAULT_MAX_ELEMENT_CNT;
	bool		isFullMap		= true;
	bool		isReadOnly		= false;
	bool		isDelMode		= false;
	bool		isWrite			= false;

	if(params.end() != params.find("-f")){
		pFilePath = params["-f"].c_str();
	}
	if(params.end() != params.find("-nofullmap")){
		isFullMap	= false;
	}
	if(params.end() != params.find("-ro")){
		isReadOnly	= true;
	}
	if(params.end() != params.find("-del")){
		if(isReadOnly){
			ERR_K2HPRN("-del could not be specified with -ro.");
			exit(-1);
		}
		isDelMode = true;
	}
	if(params.end() != params.find("VAL")){
		if(isReadOnly){
			ERR_K2HPRN("VALUE could not be specified with -ro.");
			exit(-1);
		}
		isWrite	= true;
	}

	printf("-------------------------------------------------------\n");
	printf("   Test for Read/Write K2HASH\n");
	printf("-------------------------------------------------------\n");
	printf("File Path:                              %s\n", pFilePath ? pFilePath : "(not specified = on memory mode)");
	printf("Read only mode:                         %s\n", isReadOnly ? "true" : "false");
	printf("Extra hash function library:            %s\n", params.end() != params.find("-ext") ? params["-ext"].c_str() : "(not load)");
	printf("Hash functions version:                 %s\n", K2H_HASH_VER_FUNC());
	printf("-------------------------------------------------------\n");

	if(!k2hash.Attach(pFilePath, isReadOnly, isReadOnly ? false : true, false, isFullMap, MaskBitCnt, CMaskBitCnt, MaxElementCnt)){
		ERR_K2HPRN("Failed to load/mmap.");
		exit(-1);
	}

	if(!pFilePath){
		printf("   K2H Memory\n");
	}else{
		printf("   K2H file(%s)\n", pFilePath);
	}
	printf("-------------------------------------------------------\n");

	// Dump head
	if(!k2hash.Dump(stdout, K2HShm::DUMP_HEAD)){
		ERR_K2HPRN("Failed to head dump.");
		exit(-1);
	}

	printf("-------------------------------------------------------\n");
	printf("\n");

	// Set data
	if(!isDelMode){
		if(isWrite){
			if(!k2hash.Set(params["KEY"].c_str(), params["VAL"].c_str())){
				ERR_K2HPRN("Failed to write data(\n\t%s =>\n\t%s\n)", params["KEY"].c_str(), params["VAL"].c_str());
				exit(-1);
			}
			printf("Write data\n");
			printf("    Key string:                         %s\n", params["KEY"].c_str());
			printf("    Value string:                       %s\n", params["VAL"].c_str());
			printf("\n");
		}

		// Read data
		char*	pValue = k2hash.Get(params["KEY"].c_str());
		if(pValue){
			printf("Read data\n");
			printf("    Key string:                         %s\n", params["KEY"].c_str());
			printf("    Value string:                       %s\n", pValue ? pValue : "(null)");
			printf("\n");
		}else{
			printf("Read data --> Not found\n");
			printf("    Key string:                         %s\n", params["KEY"].c_str());
			printf("\n");
		}
		K2H_Free(pValue);

	}else{
		if(!k2hash.Remove(params["KEY"].c_str(), true)){
			ERR_K2HPRN("Failed to remove key(%s)", params["KEY"].c_str());
			exit(-1);
		}
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
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */

