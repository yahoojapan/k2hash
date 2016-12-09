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
#include <functional>

#include "k2hashfunc.h"
#include "k2hcommon.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Variables
//---------------------------------------------------------
#ifdef	USE_STD_FNV_HASH_FUNCTION
static const char	szVersion[] = "STD::FNV BUILTIN";
#else
static const char	szVersion[] = "FNV-1A BUILTIN";
#endif

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
#ifndef	USE_STD_FNV_HASH_FUNCTION
//
// Build-in hash function k2h_Fnv_hash (FNV-1a)
// http://www.isthe.com/chongo/tech/comp/fnv/index.html
//
static k2h_hash_t k2h_Fnv_hash(const void* ptr, size_t len)
{
	k2h_hash_t hash = 14695981039346656037ULL;

	const char* cptr = static_cast<const char*>(ptr);
	for(; len; --len){
		hash ^= static_cast<k2h_hash_t>(*cptr++);
		hash *= static_cast<k2h_hash_t>(1099511628211ULL);
	}
	return hash;
}
#endif	// USE_STD_FNV_HASH_FUNCTION

k2h_hash_t k2h_hash(const void* ptr, size_t length)
{
	MSG_K2HPRN("hash function 1 call, %s", szVersion);

	if(!ptr || 1UL > length){
		return 0;
	}
#ifdef	USE_STD_FNV_HASH_FUNCTION
	return static_cast<k2h_hash_t>(_Fnv_hash_impl::hash(ptr, length));
#else
	return k2h_Fnv_hash(ptr, length);
#endif
}

k2h_hash_t k2h_second_hash(const void* ptr, size_t length)
{
	MSG_K2HPRN("hash function 2 call, %s", szVersion);

	if(!ptr || 1UL > length){
		return 0;
	}
	if(1UL < length){
		length--;
	}
#ifdef	USE_STD_FNV_HASH_FUNCTION
	return static_cast<k2h_hash_t>(_Fnv_hash_impl::hash(ptr, length));
#else
	return k2h_Fnv_hash(ptr, length);
#endif
}

const char* k2h_hash_version(void)
{
	return szVersion;
}

//---------------------------------------------------------
// K2HashDynLib Class
//---------------------------------------------------------
K2HashDynLib	K2HashDynLib::Singleton;

K2HashDynLib::K2HashDynLib() : hDynLib(NULL), fp_k2h_hash(NULL), fp_k2h_second_hash(NULL), fp_k2h_hash_version(NULL)
{
	if(this != K2HashDynLib::get()){
		assert(false);
	}
}

K2HashDynLib::~K2HashDynLib()
{
	if(this != K2HashDynLib::get()){
		assert(false);
	}
	Unload();
}

bool K2HashDynLib::Unload(void)
{
	if(hDynLib){
		dlclose(hDynLib);
		hDynLib = NULL;
	}
	fp_k2h_hash			= NULL;
	fp_k2h_second_hash	= NULL;
	fp_k2h_hash_version	= NULL;

	return true;
}

bool K2HashDynLib::Load(const char* path)
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
	if(	NULL == (fp_k2h_hash = reinterpret_cast<Tfp_k2h_hash>(dlsym(hDynLib, "k2h_hash"))) || 
		NULL == (fp_k2h_second_hash = reinterpret_cast<Tfp_k2h_second_hash>(dlsym(hDynLib, "k2h_second_hash"))) || 
		NULL == (fp_k2h_hash_version = reinterpret_cast<Tfp_k2h_hash_version>(dlsym(hDynLib, "k2h_hash_version"))) )
	{
		const char*	pError = dlerror();
		ERR_K2HPRN("Failed to load library(%s), error = %s", path, pError ? pError : "unknown");
		Unload();
		return false;
	}
	MSG_K2HPRN("Success loading library(%s). (Hash function version = %s)", path, (*fp_k2h_hash_version)());

	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
