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

#include <k2hash.h>
#include <k2hcommon.h>
#include <k2hshm.h>
#include <k2hdbg.h>
#include <k2hashfunc.h>
#include <k2hstream.h>

using namespace std;

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

//---------------------------------------------------------
// Command handle
//---------------------------------------------------------

#define	STREAM_TEST_KEY_STR		"test_stream_key_str"
#define	STREAM_TEST_KEY_CHR		"test_stream_key_char"
#define	STREAM_TEST_KEY_INT		"test_stream_key_int"
#define	STREAM_TEST_KEY_LNG		"test_stream_key_long"
#define	STREAM_TEST_KEY_MAX		"test_stream_key_max"
#define	STREAM_TEST_KEY_SKP		"test_stream_key_seekp"
#define	STREAM_TEST_KEY_SKG		"test_stream_key_seekg"

#define	TEST_DATA_STR			"ostream_value"
#define	TEST_DATA_CHR			'Y'
#define	TEST_DATA_INT			-1
#define	TEST_DATA_LNG			-1L
#define	TEST_DATA_MAX			"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345"
#define	TEST_DATA_SEK			"seek_test_data_for_k2hstream,_this_is_test_string."
#define	TEST_DATA_SKP			"seek_test_data_for_xxxxxxxxx,_this_is_test_string."
#define	TEST_DATA_SKG			"_this_is_test_string."

static bool TestHandle(K2HShm& k2hash)
{
	PRN("-------------------------------------------------------");
	PRN("TEST : ok2hstream");
	PRN("-------------------------------------------------------");
	{
		{
			ok2hstream	strm(&k2hash, STREAM_TEST_KEY_STR);
			string		strTmp(TEST_DATA_STR);

			strm << strTmp << endl;
			strm << strTmp << ends;

			PRN("string     = \"%s\\n%s\\0\"", strTmp.c_str(), strTmp.c_str());
		}
		{
			ok2hstream	strm(&k2hash, STREAM_TEST_KEY_CHR);
			char		cTmp = TEST_DATA_CHR;

			strm << cTmp << cTmp << cTmp << cTmp;

			PRN("char       = \"%c%c%c%c\"", cTmp, cTmp, cTmp, cTmp);
		}
		{
			ok2hstream	strm(&k2hash, STREAM_TEST_KEY_INT);
            int			nTmp = TEST_DATA_INT;

			strm << nTmp << nTmp;

			PRN("int        = \"%d%d\"", nTmp, nTmp);
		}
		{
			ok2hstream	strm(&k2hash, STREAM_TEST_KEY_LNG);
            long		lTmp = -1L;

			strm << lTmp << lTmp;

			PRN("long       = \"%ld%ld\"", lTmp, lTmp);
		}
		{
			ok2hstream	strm(&k2hash, STREAM_TEST_KEY_MAX);
			string		strTmp(TEST_DATA_MAX);

			strm << strTmp << endl;
			strm << strTmp << ends;

			PRN("string max = \"%s\\n\", \"%s\\0\"", strTmp.c_str(), strTmp.c_str());
		}
		{
			ok2hstream	strm(&k2hash, STREAM_TEST_KEY_SKP);
			string		strTmp(TEST_DATA_SEK);

			strm << strTmp << ends;
			strm.seekp(19);			// head of word "k2hstream"
			strTmp = "xxxxxxxxx";
			strm << strTmp;
			PRN("seekp      = \"%s\" ---> \"%s\"", TEST_DATA_SEK, TEST_DATA_SKP);
		}
		{
			ok2hstream	strm(&k2hash, STREAM_TEST_KEY_SKG);
			string		strTmp(TEST_DATA_SEK);

			strm << strTmp << ends;
			PRN("seekg      = \"%s\"", strTmp.c_str());
		}
	}

	PRN("-------------------------------------------------------");
	PRN("TEST : ik2hstream");
	PRN("-------------------------------------------------------");
	bool	result_str = true;
	bool	result_chr = true;
	bool	result_int = true;
	bool	result_lng = true;
	bool	result_max = true;
	bool	result_skp = true;
	bool	result_skg = true;
	{
		{
			ik2hstream	strm(&k2hash, STREAM_TEST_KEY_STR);
			string		strTmp1;
			string		strTmp2;

			strm >> strTmp1;
			strm >> strTmp2;

			PRN("string     = \"%s\",\"%s\"", strTmp1.c_str(), strTmp2.c_str());
			if(0 != strcmp(strTmp1.c_str(), TEST_DATA_STR) || 0 != strcmp(strTmp2.c_str(), TEST_DATA_STR)){
				ERR("           * Error string data is wrong.");
				result_str = false;
			}
			if(!strm.eof()){
				ERR("           * Error EOF is not found.");
				result_str = false;
			}
		}
		{
			ik2hstream	strm(&k2hash, STREAM_TEST_KEY_CHR);
			char		cTmp1 = 'Z';
			char		cTmp2 = 'Z';
			char		cTmp3 = 'Z';
			char		cTmp4 = 'Z';
			char		cTmp5 = 'Z';

			strm >> cTmp1;
			strm >> cTmp2;
			strm >> cTmp3;
			strm >> cTmp4;

			PRN("char       = \"%c\", \"%c\", \"%c\", \"%c\"", cTmp1, cTmp2, cTmp3, cTmp4);
			if(cTmp1 != TEST_DATA_CHR || cTmp2 != TEST_DATA_CHR || cTmp3 != TEST_DATA_CHR || cTmp4 != TEST_DATA_CHR){
				ERR("           * Error char data is wrong.");
				result_chr = false;
			}
			strm >> cTmp5;		// ??? OK ???
			if(!strm.eof()){
				ERR("           * Error EOF is not found.");
				result_chr = false;
			}
		}
		{
			ik2hstream	strm(&k2hash, STREAM_TEST_KEY_INT);
            int			nTmp1 = 999;
            int			nTmp2 = 999;

			strm >> nTmp1;
			strm >> nTmp2;

			PRN("int        = \"%d\", \"%d\"", nTmp1, nTmp2);
			if(nTmp1 != TEST_DATA_INT || nTmp2 != TEST_DATA_INT){
				ERR("           * Error int data is wrong.");
				result_int = false;
			}
			if(!strm.eof()){
				ERR("           * Error EOF is not found.");
				result_int = false;
			}
		}
		{
			ik2hstream	strm(&k2hash, STREAM_TEST_KEY_LNG);
            long		lTmp1 = 888;
            long		lTmp2 = 888;

			strm >> lTmp1;
			strm >> lTmp2;

			PRN("long       = \"%ld\", \"%ld\"", lTmp1, lTmp2);
			if(lTmp1 != TEST_DATA_LNG || lTmp2 != TEST_DATA_LNG){
				ERR("           * Error long data is wrong.");
				result_lng = false;
			}
			if(!strm.eof()){
				ERR("           * Error EOF is not found.");
				result_lng = false;
			}
		}
		{
			ik2hstream	strm(&k2hash, STREAM_TEST_KEY_MAX);
			string		strTmp1;
			string		strTmp2;

			strm >> strTmp1;
			strm >> strTmp2;
			PRN("string max = \"%s\", \"%s\"", strTmp1.c_str(), strTmp2.c_str());
			if(0 != strcmp(strTmp1.c_str(), TEST_DATA_MAX) || 0 != strcmp(strTmp2.c_str(), TEST_DATA_MAX)){
				// strTmp length included '\0' character = 257, so use strcmp function.
				ERR("           * Error string max(256) data is wrong(compare).");
				result_max = false;
			}
			if(!strm.eof()){
				ERR("           * Error EOF is not found.");
				result_max = false;
			}
		}
		{
			ik2hstream	strm(&k2hash, STREAM_TEST_KEY_SKP);
			string		strTmp;

			strm >> strTmp;
			PRN("seekp      = \"%s\"", strTmp.c_str());
			if(0 != strcmp(strTmp.c_str(), TEST_DATA_SKP)){
				ERR("           * Error seekp data is wrong(compare).");
				result_skp = false;
			}
			if(!strm.eof()){
				ERR("           * Error EOF is not found.");
				result_skp = false;
			}
		}
		{
			ik2hstream	strm(&k2hash, STREAM_TEST_KEY_SKG);
			string		strTmp1;
			string		strTmp2;

			strm >> strTmp1;
			strm.seekg(29);			// seek to head of " this is test string."
			strm >> strTmp2;

			PRN("seekg      = \"%s\" + \"%s\"", strTmp1.c_str(), strTmp2.c_str());
			if(0 != strcmp(strTmp1.c_str(), TEST_DATA_SEK) || 0 != strcmp(strTmp2.c_str(), TEST_DATA_SKG)){
				ERR("           * Error seekp data is wrong(compare).");
				result_skg = false;
			}
			if(!strm.eof()){
				ERR("           * Error EOF is not found.");
				result_skg = false;
			}
		}
	}
	PRN("-------------------------------------------------------");
	PRN("RESULT: ");
	PRN("-------------------------------------------------------");
	PRN("String        = %s", result_str ? "OK" : "NG");
	PRN("Char          = %s", result_chr ? "OK" : "NG");
	PRN("Int           = %s", result_int ? "OK" : "NG");
	PRN("Long          = %s", result_lng ? "OK" : "NG");
	PRN("String max    = %s", result_max ? "OK" : "NG");
	PRN("Seekp         = %s", result_skp ? "OK" : "NG");
	PRN("Seekg         = %s", result_skg ? "OK" : "NG");

	return true;
}

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
// Parse parameters
//
// -f [file name]		File path
// -g [debug level]		"ERR" "WAN" "INF"
// -nofullmap			Not full mapping
// -h					display help
//
typedef std::map<std::string, std::string> params_t;

static void Help(const char* progname)
{
	printf("Usage: %s [options]\n", progname ? progname : "program");
	printf("KEY                       Key string\n");
	printf("VALUE                     Value string\n");
	printf("Option  -f [file name]    File path\n");
	printf("        -g [debug level]  \"ERR\" / \"WAN\" / \"INF\"\n");
	printf("        -nofullmap        Not full mapping\n");
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
				ERR("Wrong parameter(%s).", argv[nCnt]);
				return false;
			}
		}
		if(0 == strcasecmp(argv[nCnt], "-f")){
			params["-f"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-g")){
			params["-g"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-nofullmap")){
			params["-nofullmap"] = "";
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
			ERR("Wrong parameter value \"-g\" %s.", params["-g"].c_str());
			exit(-1);
		}
	}

	const char*	pFilePath		= NULL;
	int			MaskBitCnt		= K2HShm::DEFAULT_MASK_BITCOUNT;
	int			CMaskBitCnt		= K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT;
	int			MaxElementCnt	= K2HShm::DEFAULT_MAX_ELEMENT_CNT;
	bool		isFullMap		= true;

	if(params.end() != params.find("-f")){
		pFilePath = params["-f"].c_str();
	}
	if(params.end() != params.find("-nofullmap")){
		isFullMap	= false;
	}

	PRN("-------------------------------------------------------");
	PRN("   Test for Read/Write K2HASH");
	PRN("-------------------------------------------------------");
	PRN("File Path:                              %s", pFilePath ? pFilePath : "(not specified = on memory mode)");
	PRN("Extra hash function library:            %s", params.end() != params.find("-ext") ? params["-ext"].c_str() : "(not load)");
	PRN("Hash functions version:                 %s", K2H_HASH_VER_FUNC());
	PRN("-------------------------------------------------------");

	if(!k2hash.Attach(pFilePath, false, true, false, isFullMap, MaskBitCnt, CMaskBitCnt, MaxElementCnt)){
		ERR("Failed to load/mmap.");
		exit(-1);
	}

	if(!pFilePath){
		PRN("   K2H Memory");
	}else{
		PRN("   K2H file(%s)", pFilePath);
	}
	PRN("-------------------------------------------------------");

	// Dump head
	if(!k2hash.Dump(stdout, K2HShm::DUMP_HEAD)){
		ERR("Failed to head dump.");
		exit(-1);
	}

	PRN("-------------------------------------------------------");
	PRN("");

	//----------------------
	// Command
	//----------------------
	TestHandle(k2hash);

	if(!k2hash.Detach()){
		ERR("Failed to detach.");
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
