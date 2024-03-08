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
#include <k2htransfunc.h>

using namespace std;

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
// Parse parameters
//
// -g [debug level]		"ERR" "WAN" "INF"
// -putfile [filepath]  transaction file name(default: testtrans.log)
// -hashext [library]	load library for hash function
// -transext [library]	load library for transaction function
// -h					display help
//
typedef std::map<std::string, std::string> params_t;

static void Help(const char* progname)
{
	printf("Usage: %s [options]\n", progname ? progname : "program");
	printf("Option  -g [debug level]     \"ERR\" / \"WAN\" / \"INF\"\n");
	printf("        -putfile [filepath]  transaction file name(default: testtrans.log)\n");
	printf("        -hashext [library]   load library for hash function\n");
	printf("        -transext [library]  load library for transaction function\n");
	printf("        -h                   display help\n");
	printf("\n");
}

static bool ParameerParser(int argc, char** argv, params_t& params)
{
	params.clear();

	for(int nCnt = 1; nCnt < argc; nCnt++){		// argv[0] = progname
		if('-' != argv[nCnt][0]){
			ERR_K2HPRN("Wrong parameter(%s).", argv[nCnt]);
			return false;
		}
		if(0 == strcasecmp(argv[nCnt], "-g")){
			params["-g"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-putfile")){
			params["-putfile"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-hashext")){
			params["-hashext"] = argv[++nCnt];
		}else if(0 == strcasecmp(argv[nCnt], "-transext")){
			params["-transext"] = argv[++nCnt];
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

	// transaction file
	string	output = "testtrans.log";
	if(params.end() != params.find("-putfile")){
		output = params["-putfile"].c_str();
	}

	// extra library
	if(params.end() != params.find("-hashext")){
		if(!K2HashDynLib::get()->Load(params["-hashext"].c_str())){
			ERR_K2HPRN("Failed to load hash library.");
			exit(-1);
		}
	}
	if(params.end() != params.find("-transext")){
		if(!K2HTransDynLib::get()->Load(params["-transext"].c_str())){
			ERR_K2HPRN("Failed to load transaction library.");
			exit(-1);
		}
	}

	// Test by on memory
	if(!k2hash.AttachMem(K2HShm::DEFAULT_MASK_BITCOUNT, K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT, K2HShm::DEFAULT_MAX_ELEMENT_CNT, K2HShm::MIN_PAGE_SIZE)){
		ERR_K2HPRN("Failed to load/mmap.");
		exit(-1);
	}

	// print libraries
	printf("\n");
	printf("-------------------------------------------------------\n");
	printf("K2HASH HASH FUNCTION\n");
	printf("-------------------------------------------------------\n");
	printf("Hash functions version:                 %s\n", K2H_HASH_VER_FUNC());
	printf("Coded function addr(k2h_hash):          %p\n", k2h_hash);
	printf("Coded function addr(k2h_second_hash):   %p\n", k2h_second_hash);
	printf("Coded function addr(k2h_hash_version):  %p\n", k2h_hash_version);
	printf("Loaded function addr(k2h_hash):         %p\n", K2HashDynLib::get()->get_k2h_hash());
	printf("Loaded function addr(k2h_second_hash):  %p\n", K2HashDynLib::get()->get_k2h_second_hash());
	printf("Loaded function addr(k2h_hash_version): %p\n", K2HashDynLib::get()->get_k2h_hash_version());
	printf("\n");
	printf("-------------------------------------------------------\n");
	printf("K2HASH TRANSACTION FUNCTION\n");
	printf("-------------------------------------------------------\n");
	printf("Transaction functions version:          %s\n", K2H_TRANS_VER_FUNC());
	printf("Coded function addr(k2h_trans):         %p\n", k2h_trans);
	printf("Coded function addr(k2h_trans_version): %p\n", k2h_trans_version);
	printf("Coded function addr(k2h_trans_cntl):    %p\n", k2h_trans_cntl);
	printf("Loaded function addr(k2h_trans):        %p\n", K2HTransDynLib::get()->get_k2h_trans());
	printf("Loaded function addr(k2h_trans_version):%p\n", K2HTransDynLib::get()->get_k2h_trans_version());
	printf("Loaded function addr(k2h_trans_cntl):   %p\n", K2HTransDynLib::get()->get_k2h_trans_cntl());
	printf("\n");

	// print test hash
	char	szBuff[] = "0123456789";
	printf("-------------------------------------------------------\n");
	printf("Hash function test\n");
	printf("-------------------------------------------------------\n");
	printf("Test hashed value by base value         %s\n", szBuff);
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress internalAstError
	printf("First hashed value:                     %p(0x%" PRIx64 ")\n", reinterpret_cast<void*>(K2H_HASH_FUNC(szBuff, strlen(szBuff))), K2H_HASH_FUNC(szBuff, strlen(szBuff)));
	printf("Second hashed value:                    %p(0x%" PRIx64 ")\n", reinterpret_cast<void*>(K2H_2ND_HASH_FUNC(szBuff, strlen(szBuff))), K2H_2ND_HASH_FUNC(szBuff, strlen(szBuff)));
	printf("\n");

	// test transaction
	printf("-------------------------------------------------------\n");
	printf("Transaction function test\n");
	printf("-------------------------------------------------------\n");
	printf("Transaction test:\n");
	printf("  1)  Set Transaction ON.\n");
	printf("  2)  Write \"TestKey_0\" without value.\n");
	printf("  3)  Write \"TestKey_1\" with value \"Value_1\".\n");
	printf("  4)  Write \"TestKey_2\" without value and it has subkey \"TestSubkey_0\" with \"Value_Sub_0\"\n");
	printf("  5)  Write \"TestKey_3\" without value and it has subkey \"TestSubkey_1\" with \"Value_Sub_1\"\n");
	printf("  6)  Remove \"TestKey_2\" without subkeys\n");
	printf("  7)  Remove \"TestKey_3\" with subkey \"TestSubkey_1\"\n");
	printf("  8)  Remove \"TestSubkey_0\" (without subkeys)\n");
	printf("  9)  Replace(add) value for \"TestKey_0\" as \"Add_Value_0\"\n");
	printf("  10) Replace(add) subkey for \"TestKey_1\" as \"TestSubkey_2\"\n");
	printf("  11) Remove \"TestKey_0\"\n");
	printf("  12) Remove \"TestKey_1\" with subkey \"TestSubkey_2\"\n");
	printf("\n");
	printf("Start transaction: \n");

	// transaction ON
	if(!k2hash.EnableTransaction(output.c_str())){
		ERR_K2HPRN("Could not set transaction ON.");
		exit(-1);
	}
	// set key
	if(!k2hash.Set("TestKey_0", NULL)){
		ERR_K2HPRN("Could not write TestKey_0.");
		exit(-1);
	}
	if(!k2hash.Set("TestKey_1", "Value_1")){
		ERR_K2HPRN("Could not write TestKey_1 = Value_1.");
		exit(-1);
	}
	if(!k2hash.AddSubkey("TestKey_2", "TestSubkey_0", "Value_Sub_0")){
		ERR_K2HPRN("Could not write TestKey_2 with TestSubkey_0 = Value_Sub_0.");
		exit(-1);
	}
	if(!k2hash.AddSubkey("TestKey_3", "TestSubkey_1", "Value_Sub_1")){
		ERR_K2HPRN("Could not write TestKey_2 with TestSubkey_0 = Value_Sub_0.");
		exit(-1);
	}
	// remove
	if(!k2hash.Remove("TestKey_2", false)){
		ERR_K2HPRN("Could not remove TestKey_2 without subkeys.");
		exit(-1);
	}
	if(!k2hash.Remove("TestKey_3", true)){
		ERR_K2HPRN("Could not remove TestKey_3 with subkey TestSubkey_1.");
		exit(-1);
	}
	if(!k2hash.Remove("TestSubkey_0", false)){
		ERR_K2HPRN("Could not remove TestSubkey_0 without subkey");
		exit(-1);
	}
	// replace
	if(!k2hash.ReplaceValue(reinterpret_cast<const unsigned char*>("TestKey_0"), strlen("TestKey_0") + 1, reinterpret_cast<const unsigned char*>("Add_Value_0"), strlen("Add_Value_0") + 1)){
		ERR_K2HPRN("Could not replace TestKey_0 value Add_Value_0.");
		exit(-1);
	}
	if(!k2hash.Set("TestSubkey_2", "Value_Sub_2")){
		ERR_K2HPRN("Could not write TestKey_1 = Value_1.");
		exit(-1);
	}
	K2HSubKeys		subkeys;
	unsigned char*	pSubKeys = NULL;
	size_t			subkey_length = 0UL;
	if(	subkeys.end() == subkeys.insert("TestSubkey_2") ||
		!subkeys.Serialize(&pSubKeys, subkey_length) )
	{
		ERR_K2HPRN("Failed to insert to subkeys object.");
		exit(-1);
	}
	if(!k2hash.ReplaceSubkeys(reinterpret_cast<const unsigned char*>("TestKey_1"), strlen("TestKey_1") + 1, pSubKeys, subkey_length)){
		ERR_K2HPRN("Could not replace TestKey_1 subkey TestSubkey_2.");
		K2H_Free(pSubKeys);
		exit(-1);
	}
	K2H_Free(pSubKeys);

	// cleanup
	if(!k2hash.Remove("TestKey_0", true)){
		ERR_K2HPRN("Could not remove TestKey_0.");
		exit(-1);
	}
	if(!k2hash.Remove("TestKey_1", true)){
		ERR_K2HPRN("Could not remove TestKey_1 with subkey TestSubkey_2.");
		exit(-1);
	}

	printf(": Finished transaction\n");
	printf("\n");
	printf("-------------------------------------------------------\n");
	printf("All test finish\n");
	printf("-------------------------------------------------------\n\n");

	if(!k2hash.Detach()){
		WAN_K2HPRN("Failed to detach.");
	}

	// Unload
	K2HashDynLib::get()->Unload();
	K2HTransDynLib::get()->Unload();

	exit(0);
}

//---------------------------------------------------------
// Test for Hash Function over write
//---------------------------------------------------------
// libk2hash.so has hash function which is FNV-1a in STD, but it supports 
// extending hash function.
// One of case for over writing is same prototype hash functions has
// inner binary. Then libk2hash.so has "weak" symbol function, so if 
// you need to over write these hash function, you can make same prototype 
// function in your source(binary).
//
// For example
// k2h_hash, k2h_second_hash can be calling your original function.
// This code is sample.

static const char	szHashVersion[] = "TEST HASH OW";

k2h_hash_t k2h_hash(const void* ptr, size_t length)
{
	MSG_K2HPRN("my hash function 1 call, %s", szHashVersion);
	if(!ptr || 1UL > length){
		return 0;
	}
	k2h_hash_t				result	= 0UL;
	const unsigned char*	pData	= reinterpret_cast<const unsigned char*>(ptr);
	for(size_t pos = 0L; pos < length; pos++){
		k2h_hash_t	lData = 0UL;
		lData |= (static_cast<k2h_hash_t>(pData[pos]) << 24) & 0xFF000000;
		lData |= pos + 1 < length ? ((static_cast<k2h_hash_t>(pData[pos + 1]) << 16) & 0x00FF0000) : 0UL;
		lData |= pos + 1 < length ? ((static_cast<k2h_hash_t>(pData[pos + 2]) << 8) & 0x0000FF00) : 0UL;
		lData |= pos + 1 < length ? (static_cast<k2h_hash_t>(pData[pos + 3]) & 0x000000FF) : 0UL;
		result = result ^ lData;
	}
	return result;
}

k2h_hash_t k2h_second_hash(const void* ptr, size_t length)
{
	MSG_K2HPRN("my hash function 2 call, %s", szHashVersion);

	if(!ptr || 1UL > length){
		return 0;
	}
	k2h_hash_t				result	= 0UL;
	const unsigned char*	pData	= reinterpret_cast<const unsigned char*>(ptr);
	for(size_t pos = 0L; pos < length; pos++){
		k2h_hash_t	lData = 0UL;
		lData |= static_cast<k2h_hash_t>(pData[pos]) & 0x000000FF;
		lData |= pos + 1 < length ? ((static_cast<k2h_hash_t>(pData[pos + 1]) << 8) & 0x0000FF00) : 0UL;
		lData |= pos + 1 < length ? ((static_cast<k2h_hash_t>(pData[pos + 2]) << 16) & 0x00FF0000) : 0UL;
		lData |= pos + 1 < length ? (static_cast<k2h_hash_t>(pData[pos + 3]) & 0xFF000000) : 0UL;
		result = result ^ lData;
	}
	return result;
}

const char* k2h_hash_version(void)
{
	return szHashVersion;
}

//---------------------------------------------------------
// Test for Transaction Function over write
//---------------------------------------------------------
static const char	szTransVersion[] = "TEST TRANS OW";

bool k2h_trans(k2h_h handle, PBCOM pBinCom)
{
	string	Mode	= "unknown";
	long	type	= -1L;
	string	key		= "key=";
	string	val		= "val=";
	string	skey	= "skey=";

	if(pBinCom){
		Mode	= &(pBinCom->scom.szCommand[1]);	// First character is "\n", so skip it
		type	= pBinCom->scom.type;

		if(0UL < pBinCom->scom.key_length){
			string	substr;
			substr.assign(reinterpret_cast<char*>(&(pBinCom->byData[pBinCom->scom.key_pos])), pBinCom->scom.key_length);
			key+= substr;
		}
		if(0UL < pBinCom->scom.val_length){
			string	substr;
			substr.assign(reinterpret_cast<char*>(&(pBinCom->byData[pBinCom->scom.val_pos])), pBinCom->scom.val_length);
			val+= substr;
		}
		if(0UL < pBinCom->scom.skey_length){
			string	substr;
			substr.assign(reinterpret_cast<char*>(&(pBinCom->byData[pBinCom->scom.skey_pos])), pBinCom->scom.skey_length);
			skey+= substr;
		}
	}

	printf("Transaction - handle=0x%" PRIx64 ", %s(%ld) : %s, %s, %s\n", handle, Mode.c_str(), type, key.c_str(), val.c_str(), skey.c_str());

	return true;
}

bool k2h_trans_cntl(k2h_h handle, PTRANSOPT pOpt)
{
	printf("Call transaction control.\n");
	printf("  k2h handle   = 0x%" PRIx64 "\n",	handle);
	printf("  file path    = %s\n",				pOpt ? pOpt->szFilePath : "null");
	printf("  control      = %s\n",				!pOpt ? "null" : pOpt->isEnable ? "enable" : "disable");

	return true;
}

const char* k2h_trans_version(void)
{
	return szTransVersion;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
