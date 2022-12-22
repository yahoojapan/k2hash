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
 * CREATE:   Mon Feb 17 2014
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <sstream>

#include <k2htransfunc.h>

using namespace std;

//---------------------------------------------------------
// Prototypes
//---------------------------------------------------------
extern "C" {

bool k2h_trans(k2h_h handle, PBCOM pBinCom);
const char* k2h_trans_version(void);
bool k2h_trans_cntl(k2h_h handle, PTRANSOPT pOpt);

}

//---------------------------------------------------------
// Transaction functions
//---------------------------------------------------------
static const char	szVersion[] = "DSO TRANS V1.0";
static char			szTFile[MAX_TRANSACTION_FILEPATH];		// [NOTICE] Not care for multi-shm files because this is test.
static bool			isSetFile = false;						// [NOTICE] Not care for multi-shm files because this is test.

bool k2h_trans(k2h_h handle, PBCOM pBinCom)
{
	//printf("dso trans function call, %s\n", szVersion);

	string	strTrans;
	if(pBinCom){
		string	Mode	= &(pBinCom->scom.szCommand[1]);	// First character is "\n", so skip it
		long	type	= pBinCom->scom.type;
		string	key		= "key=";
		string	val		= "val=";
		string	skey	= "skey=";

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

		stringstream	ssData;
		ssData << "TS: " << Mode << "(" << type << "): handle=" << handle << ",";
		ssData << key << "," << val << "," << skey << "\n";

		strTrans = ssData.str();
	}

	if(isSetFile){
		FILE*	fp;
		if(NULL == (fp = fopen(szTFile, "a"))){
			fprintf(stderr, "[ERR] Trans Function: Could not open file(%s), errno = %d.\n", szTFile, errno);
			// cppcheck-suppress resourceLeak
			return false;
		}
		if(strTrans.length() != fwrite(strTrans.c_str(), sizeof(char), strTrans.length(), fp)){
			fprintf(stderr, "[ERR] Trans Function: Failed to write file(%s).\n", szTFile);
			fclose(fp);
			return false;
		}
		fclose(fp);
	}else{
		printf("%s\n", strTrans.c_str());
	}

	return true;

}

const char* k2h_trans_version(void)
{
	return szVersion;
}

bool k2h_trans_cntl(k2h_h handle, PTRANSOPT pOpt)
{
	//printf("dso trans control function call, %s\n", szVersion);

	printf("Call transaction control.\n");
	printf("  k2h handle    = %lu\n", handle);
	printf("  file path     = %s\n", pOpt ? pOpt->szFilePath : "null");
	printf("  control       = %s\n", !pOpt ? "null" : pOpt->isEnable ? "enable" : "disable");
	printf("  prefix        = (string)%s\n", !pOpt ? "null" : reinterpret_cast<const char*>(pOpt->byTransPrefix));
	printf("  prefix length = %zd\n", !pOpt ? 0 : pOpt->PrefixLength);

	if(!pOpt){
		fprintf(stderr, "[ERR] Trans Control Function: Parameters are wrong.\n");
		return false;
	}
	isSetFile = pOpt->isEnable;
	if(pOpt->isEnable){
		// If can close file, you should open file here.
		strcpy(szTFile, pOpt->szFilePath);
	}else{
		szTFile[0] = '\0';
	}
	return true;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
