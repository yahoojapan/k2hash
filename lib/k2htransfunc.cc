/*
 * K2HASH
 *
 * Copyright 2013 Yahoo! JAPAN corporation.
 *
 * K2HASH is key-valuew store base libraries.
 * K2HASH is made for the purpose of the construction of
 * original KVS system and the offer of the library.
 * The characteristic is this KVS library which Key can
 * layer. And can support multi-processing and multi-thread,
 * and is provided safely as available KVS.
 *
 * For the full copyright and license information, please view
 * the LICENSE file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Fri Dec 2 2013
 * REVISION:
 *
 */

#include <sys/types.h>
#include <dlfcn.h>
#include <assert.h>
#include <string>

#include "k2htransfunc.h"
#include "k2htrans.h"
#include "k2hcommon.h"
#include "k2hlock.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Variables
//---------------------------------------------------------
static const char	szVersion[] = "K2H TRANSFUNC BUILTIN";

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
bool k2h_trans(k2h_h handle, PBCOM pBinCom)
{
	MSG_K2HPRN("transaction function call, %s", szVersion);

	// Do not check dis/enable transaction because already check it before comming this function.
	// (so do not call K2HTransManager::isEnable)
	//
	int	arfd;
	if(-1 == (arfd = K2HTransManager::Get()->GetArchiveFd(reinterpret_cast<const K2HShm*>(handle)))){
		ERR_K2HPRN("There is no archive file discripter.");
		return false;
	}

	K2HLock	AutoLock(arfd, 0L, K2HLock::RWLOCK);		// LOCK

	// seek
	off_t	fendpos;
	if(-1 == (fendpos = lseek(arfd, 0, SEEK_END))){
		ERR_K2HPRN("Could not seek file to end: errno(%d)", errno);
		return false;
	}

	// write
	size_t	write_length = scom_total_length(pBinCom->scom);
	if(-1 == k2h_pwrite(arfd, pBinCom->byData, write_length, fendpos)){
		ERR_K2HPRN("Failed to write command transaction.");
		return false;
	}
	return true;
}

const char* k2h_trans_version(void)
{
	return szVersion;
}

//
// This default transaction control function needs a file path when enabling.
// The file path parameter is checked here.
//
bool k2h_trans_cntl(k2h_h handle, PTRANSOPT pOpt)
{
	if(!pOpt){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(pOpt->isEnable && '\0' == pOpt->szFilePath[0]){
		ERR_K2HPRN("Transaction enabled flag but file path is empty.");
		return false;
	}
	// if enabled, this default transaction control function does not use param.

	return true;
}

//---------------------------------------------------------
// K2HTransDynLib Class
//---------------------------------------------------------
K2HTransDynLib	K2HTransDynLib::Singleton;

K2HTransDynLib::K2HTransDynLib() : hDynLib(NULL), fp_k2h_trans(NULL), fp_k2h_trans_version(NULL), fp_k2h_trans_cntl(NULL)
{
	if(this != K2HTransDynLib::get()){
		assert(false);
	}
}

K2HTransDynLib::~K2HTransDynLib()
{
	if(this != K2HTransDynLib::get()){
		assert(false);
	}
	Unload();
}

bool K2HTransDynLib::Unload(void)
{
	if(hDynLib){
		dlclose(hDynLib);
		hDynLib = NULL;
	}
	fp_k2h_trans		= NULL;
	fp_k2h_trans_version= NULL;
	fp_k2h_trans_cntl	= NULL;

	return true;
}

bool K2HTransDynLib::Load(const char* path)
{
	if(ISEMPTYSTR(path)){
		ERR_K2HPRN("Path is NULL");
		return false;
	}
	// close
	Unload();

	// open
	if(NULL == (hDynLib = dlopen(path, RTLD_LAZY))){
		const char*	pError = dlerror();
		ERR_K2HPRN("Failed to load library(%s), error = %s", path, pError ? pError : "unknown");
		return false;
	}

	// check symbol
	if(	NULL == (fp_k2h_trans = reinterpret_cast<Tfp_k2h_trans>(dlsym(hDynLib, "k2h_trans"))) || 
		NULL == (fp_k2h_trans_version = reinterpret_cast<Tfp_k2h_trans_version>(dlsym(hDynLib, "k2h_trans_version"))) ||
		NULL == (fp_k2h_trans_cntl = reinterpret_cast<Tfp_k2h_trans_cntl>(dlsym(hDynLib, "k2h_trans_cntl"))) )
	{
		const char*	pError = dlerror();
		ERR_K2HPRN("Failed to load library(%s), error = %s", path, pError ? pError : "unknown");
		Unload();
		return false;
	}
	MSG_K2HPRN("Success loading library(%s). (Transaction function version = %s)", path, (*fp_k2h_trans_version)());

	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
