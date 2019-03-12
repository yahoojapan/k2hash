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

#include <sys/types.h>
#include <fullock/fullock.h>
#include <sstream>

#include "k2hash.h"
#include "k2hcommon.h"
#include "k2hdbg.h"
#include "k2hutil.h"
#include "k2hshm.h"
#include "k2hashfunc.h"
#include "k2htransfunc.h"
#include "k2htrans.h"
#include "k2harchive.h"
#include "k2hqueue.h"
#include "k2hcryptcommon.h"

using namespace std;

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
typedef struct k2h_find_handle{
	K2HShm*				pShm;
	K2HShm::iterator*	piter;
	bool				iskey;
}K2HFINDHANDLE, *PK2HFINDHANDLE;

//---------------------------------------------------------
// Functions : debug
//---------------------------------------------------------
void k2h_bump_debug_level(void)
{
	::BumpupK2hDbgMode();
}

void k2h_set_debug_level_silent(void)
{
	::SetK2hDbgMode(K2HDBG_SILENT);
}

void k2h_set_debug_level_error(void)
{
	::SetK2hDbgMode(K2HDBG_ERR);
}

void k2h_set_debug_level_warning(void)
{
	::SetK2hDbgMode(K2HDBG_WARN);
}

void k2h_set_debug_level_message(void)
{
	::SetK2hDbgMode(K2HDBG_MSG);
}

bool k2h_set_debug_file(const char* filepath)
{
	bool result;
	if(ISEMPTYSTR(filepath)){
		result = ::UnsetK2hDbgFile();
	}else{
		result = ::SetK2hDbgFile(filepath);
	}
	return result;
}

bool k2h_unset_debug_file(void)
{
	return ::UnsetK2hDbgFile();
}

bool k2h_load_debug_env(void)
{
	return ::LoadK2hDbgEnv();
}

bool k2h_set_bumpup_debug_signal_user1(void)
{
	return ::SetSignalUser1();
}

//---------------------------------------------------------
// Functions : extra library
//---------------------------------------------------------
bool k2h_load_hash_library(const char* libpath)
{
	if(ISEMPTYSTR(libpath)){
		ERR_K2HPRN("Extra hash library path is empty.");
		return false;
	}
	if(!K2HashDynLib::get()->Load(libpath)){
		ERR_K2HPRN("Could not load extra hash library(library: %s).", libpath);
		return false;
	}
	return true;
}

bool k2h_unload_hash_library(void)
{
	if(!K2HashDynLib::get()->Unload()){
		ERR_K2HPRN("Could not unload extra hash library.");
		return false;
	}
	return true;
}

bool k2h_load_transaction_library(const char* libpath)
{
	if(ISEMPTYSTR(libpath)){
		ERR_K2HPRN("Extra transaction library path is empty.");
		return false;
	}
	if(!K2HTransDynLib::get()->Load(libpath)){
		ERR_K2HPRN("Could not load extra transaction library(library: %s).", libpath);
		return false;
	}
	return true;
}

bool k2h_unload_transaction_library(void)
{
	if(!K2HTransDynLib::get()->Unload()){
		ERR_K2HPRN("Could not unload extra transaction library.");
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Functions : create / open / close
//---------------------------------------------------------
bool k2h_create(const char* filepath, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
{
	K2HShm	k2hshm;

	if(!k2hshm.Create(filepath, false, maskbitcnt, cmaskbitcnt, maxelementcnt, pagesize)){
		ERR_K2HPRN("Could not create k2hash file.");
		return false;
	}
	return true;
}

k2h_h k2h_open(const char* filepath, bool readonly, bool removefile, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
{
	K2HShm*	pShm = new K2HShm();

	if(!pShm->Attach(filepath, readonly, readonly ? false : true, removefile, fullmap, maskbitcnt, cmaskbitcnt, maxelementcnt, pagesize)){
		ERR_K2HPRN("Could not attach(create) k2hash file(memory).");
		K2H_Delete(pShm);
		return K2H_INVALID_HANDLE;
	}
	return reinterpret_cast<k2h_h>(pShm);
}

k2h_h k2h_open_rw(const char* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
{
	return k2h_open(filepath, false, false, fullmap, maskbitcnt, cmaskbitcnt, maxelementcnt, pagesize);
}

k2h_h k2h_open_ro(const char* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
{
	return k2h_open(filepath, true, false, fullmap, maskbitcnt, cmaskbitcnt, maxelementcnt, pagesize);
}

k2h_h k2h_open_tempfile(const char* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
{
	return k2h_open(filepath, false, true, fullmap, maskbitcnt, cmaskbitcnt, maxelementcnt, pagesize);
}

k2h_h k2h_open_mem(int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize)
{
	K2HShm*	pShm = new K2HShm();

	if(!pShm->AttachMem(maskbitcnt, cmaskbitcnt, maxelementcnt, pagesize)){
		ERR_K2HPRN("Could not attach(create) k2hash file(memory).");
		K2H_Delete(pShm);
		return 0;
	}
	return reinterpret_cast<k2h_h>(pShm);
}

bool k2h_close_wait(k2h_h handle, long waitms)
{
	if(waitms < -1){
		ERR_K2HPRN("Parameter is wrong");
		return false;
	}
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(-1 == waitms){
		waitms = K2HShm::DETACH_BLOCK_WAIT;
	}else if(0 == waitms){
		waitms = K2HShm::DETACH_NO_WAIT;
	}
	if(!pShm->Detach(waitms)){
		ERR_K2HPRN("Could not detach k2hash file(memory).");
		return false;
	}
	K2H_Delete(pShm);
	return true;
}

bool k2h_close(k2h_h handle)
{
	return k2h_close_wait(handle, 0);
}

//---------------------------------------------------------
// Functions : transaction
//---------------------------------------------------------
bool k2h_transaction_param_we(k2h_h handle, bool enable, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen, const time_t* expire)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(enable){
		if(ISEMPTYSTR(transfile)){
			MSG_K2HPRN("Transaction file path is empty, but continue because of checking it in transaction function.");
		}
		if(!pprefix || 0 == prefixlen){
			MSG_K2HPRN("Transaction queue prefix is empty, so use default queue prefix.");
		}
		if(!pparam || 0 == paramlen){
			MSG_K2HPRN("Transaction param is empty.");
		}
		if(!pShm->EnableTransaction(transfile, pprefix, prefixlen, pparam, paramlen, expire)){
			ERR_K2HPRN("Could not enable transaction(file: %s).", transfile);
			return false;
		}
	}else{
		if(!pShm->DisableTransaction()){
			ERR_K2HPRN("Could not disable transaction(file: %s).", transfile);
			return false;
		}
	}
	return true;
}

bool k2h_transaction_param(k2h_h handle, bool enable, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen)
{
	return k2h_transaction_param_we(handle, enable, transfile, pprefix, prefixlen, pparam, paramlen, NULL);
}

bool k2h_transaction_prefix(k2h_h handle, bool enable, const char* transfile, const unsigned char* pprefix, size_t prefixlen)
{
	return k2h_transaction_param_we(handle, enable, transfile, pprefix, prefixlen, NULL, 0, NULL);
}

bool k2h_transaction(k2h_h handle, bool enable, const char* transfile)
{
	return k2h_transaction_param_we(handle, enable, transfile, NULL, 0, NULL, 0, NULL);
}

bool k2h_enable_transaction(k2h_h handle, const char* transfile)
{
	return k2h_transaction_param_we(handle, true, transfile, NULL, 0, NULL, 0, NULL);
}

bool k2h_enable_transaction_prefix(k2h_h handle, const char* transfile, const unsigned char* pprefix, size_t prefixlen)
{
	return k2h_transaction_param_we(handle, true, transfile, pprefix, prefixlen, NULL, 0, NULL);
}

bool k2h_enable_transaction_param(k2h_h handle, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen)
{
	return k2h_transaction_param_we(handle, true, transfile, pprefix, prefixlen, pparam, paramlen, NULL);
}

bool k2h_enable_transaction_param_we(k2h_h handle, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen, const time_t* expire)
{
	return k2h_transaction_param_we(handle, true, transfile, pprefix, prefixlen, pparam, paramlen, expire);
}

bool k2h_disable_transaction(k2h_h handle)
{
	return k2h_transaction_param_we(handle, false, NULL, NULL, 0, NULL, 0, NULL);
}

int k2h_get_transaction_archive_fd(k2h_h handle)
{
	return K2HTransManager::Get()->GetArchiveFd(handle);
}

static bool k2h_archive_ext(k2h_h handle, const char* filepath, bool isload, bool errskip)
{
	if(ISEMPTYSTR(filepath)){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	K2HArchive	archiveobj;
	if(!archiveobj.Initialize(filepath, errskip)){
		ERR_K2HPRN("Could not open file(%s).", filepath);
		return false;
	}
	if(!archiveobj.Serialize(pShm, isload)){
		ERR_K2HPRN("Failed to %s file(%s).", isload ? "load" : "put", filepath);
		return false;
	}
	return true;
}

bool k2h_load_archive(k2h_h handle, const char* filepath, bool errskip)
{
	return k2h_archive_ext(handle, filepath, true, errskip);
}

bool k2h_put_archive(k2h_h handle, const char* filepath, bool errskip)
{
	return k2h_archive_ext(handle, filepath, false, errskip);
}

int k2h_get_transaction_thread_pool()
{
	return K2HShm::GetTransThreadPool();
}

bool k2h_set_transaction_thread_pool(int count)
{
	return K2HShm::SetTransThreadPool(count);
}

bool k2h_unset_transaction_thread_pool(void)
{
	return K2HShm::UnsetTransThreadPool();
}

//---------------------------------------------------------
// Functions : attribute
//---------------------------------------------------------
bool k2h_set_common_attr(k2h_h handle, const bool* is_mtime, const bool* is_defenc, const char* passfile, const bool* is_history, const time_t* expire)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->SetCommonAttribute(is_mtime, is_defenc, passfile, is_history, expire, NULL)){
		ERR_K2HPRN("Could not set common attributes.");
		return false;
	}
	return true;
}

bool k2h_clean_common_attr(k2h_h handle)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->CleanCommonAttribute()){
		ERR_K2HPRN("Could not clean common attributes.");
		return false;
	}
	return true;
}

bool k2h_add_attr_plugin_library(k2h_h handle, const char* libpath)
{
	if(ISEMPTYSTR(libpath)){
		ERR_K2HPRN("Library file path is empty.");
		return false;
	}
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->AddAttrPluginLib(libpath)){
		ERR_K2HPRN("Could not add attribute plugin library(file: %s).", libpath);
		return false;
	}
	return true;
}

bool k2h_add_attr_crypt_pass(k2h_h handle, const char* pass, bool is_default_encrypt)
{
	if(ISEMPTYSTR(pass)){
		ERR_K2HPRN("Pass phrase is empty.");
		return false;
	}
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->AddAttrCryptPass(pass, is_default_encrypt)){
		ERR_K2HPRN("Could not add pass phrase for crypt.");
		return false;
	}
	return true;
}

bool k2h_print_attr_version(k2h_h handle, FILE* stream)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!stream){
		stream = stdout;
	}

	strarr_t	verinfos;
	if(!pShm->GetAttrVersionInfos(verinfos)){
		ERR_K2HPRN("Could not get attribute library version information.");
		return false;
	}
	fprintf(stream, "K2HASH attribute libraries:");
	for(strarr_t::const_iterator iter = verinfos.begin(); iter != verinfos.end(); ++iter){
		fprintf(stream, "  %s", iter->c_str());
	}
	return true;
}

bool k2h_print_attr_information(k2h_h handle, FILE* stream)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!stream){
		stream = stdout;
	}

	stringstream	ss;
	pShm->GetAttrInfos(ss);
	fprintf(stream, "%s", ss.str().c_str());

	return true;
}

//---------------------------------------------------------
// Functions : Get
//---------------------------------------------------------
bool k2h_get_value(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength)
{
	return k2h_get_value_wp(handle, pkey, keylength, ppval, pvallength, NULL);
}

unsigned char* k2h_get_direct_value(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength)
{
	return k2h_get_direct_value_wp(handle, pkey, keylength, pvallength, NULL);
}

bool k2h_get_str_value(k2h_h handle, const char* pkey, char** ppval)
{
	return k2h_get_str_value_wp(handle, pkey, ppval, NULL);
}

char* k2h_get_str_direct_value(k2h_h handle, const char* pkey)
{
	return k2h_get_str_direct_value_wp(handle, pkey, NULL);
}

bool k2h_get_value_wp(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, const char* pass)
{
	if(!pkey || 0UL == keylength || !ppval || !pvallength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppval		= NULL;
	*pvallength	= 0UL;

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	ssize_t	result;
	if(-1 == (result = pShm->Get(pkey, keylength, ppval, true, pass))){
		MSG_K2HPRN("Not found key or not have value.");
		return false;
	}
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress redundantAssignment
	*pvallength = static_cast<size_t>(result);

	return true;
}

unsigned char* k2h_get_direct_value_wp(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, const char* pass)
{
	unsigned char*	pval = NULL;
	if(!k2h_get_value_wp(handle, pkey, keylength, &pval, pvallength, pass)){
		return NULL;
	}
	return pval;
}

bool k2h_get_str_value_wp(k2h_h handle, const char* pkey, char** ppval, const char* pass)
{
	if(ISEMPTYSTR(pkey) || !ppval){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	size_t	vallength = 0UL;
	return k2h_get_value_wp(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<unsigned char**>(ppval), &vallength, pass);
}

char* k2h_get_str_direct_value_wp(k2h_h handle, const char* pkey, const char* pass)
{
	char*	pval = NULL;
	if(!k2h_get_str_value_wp(handle, pkey, &pval, pass)){
		return NULL;
	}
	return pval;
}

bool k2h_get_value_np(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength)
{
	if(!pkey || 0UL == keylength || !ppval || !pvallength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppval		= NULL;
	*pvallength	= 0UL;

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	ssize_t	result;
	if(-1 == (result = pShm->Get(pkey, keylength, ppval, false, reinterpret_cast<const char*>(NULL)))){
		MSG_K2HPRN("Not found key or not have value.");
		return false;
	}
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress redundantAssignment
	*pvallength = static_cast<size_t>(result);

	return true;
}

unsigned char* k2h_get_direct_value_np(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength)
{
	unsigned char*	pval = NULL;
	if(!k2h_get_value_np(handle, pkey, keylength, &pval, pvallength)){
		return NULL;
	}
	return pval;
}

bool k2h_get_str_value_np(k2h_h handle, const char* pkey, char** ppval)
{
	if(ISEMPTYSTR(pkey) || !ppval){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	size_t	vallength = 0UL;
	return k2h_get_value_np(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<unsigned char**>(ppval), &vallength);
}

char* k2h_get_str_direct_value_np(k2h_h handle, const char* pkey)
{
	char*	pval = NULL;
	if(!k2h_get_str_value_np(handle, pkey, &pval)){
		return NULL;
	}
	return pval;
}

bool k2h_get_value_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData)
{
	return k2h_get_value_wp_ext(handle, pkey, keylength, ppval, pvallength, fp, pExtData, NULL);
}

unsigned char* k2h_get_direct_value_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData)
{
	return k2h_get_direct_value_wp_ext(handle, pkey, keylength, pvallength, fp, pExtData, NULL);
}

bool k2h_get_str_value_ext(k2h_h handle, const char* pkey, char** ppval, k2h_get_trial_callback fp, void* pExtData)
{
	return k2h_get_str_value_wp_ext(handle, pkey, ppval, fp, pExtData, NULL);
}

char* k2h_get_str_direct_value_ext(k2h_h handle, const char* pkey, k2h_get_trial_callback fp, void* pExtData)
{
	return k2h_get_str_direct_value_wp_ext(handle, pkey, fp, pExtData, NULL);
}

bool k2h_get_value_wp_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData, const char* pass)
{
	if(!pkey || 0UL == keylength || !ppval || !pvallength || !fp){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppval		= NULL;
	*pvallength	= 0UL;

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	ssize_t	result;
	if(-1 == (result = pShm->Get(pkey, keylength, ppval, fp, pExtData, true, pass))){
		MSG_K2HPRN("Not found key or not have value.");
		return false;
	}
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress redundantAssignment
	*pvallength = static_cast<size_t>(result);

	return true;
}

unsigned char* k2h_get_direct_value_wp_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData, const char* pass)
{
	unsigned char*	pval = NULL;
	if(!k2h_get_value_wp_ext(handle, pkey, keylength, &pval, pvallength, fp, pExtData, pass)){
		return NULL;
	}
	return pval;
}

bool k2h_get_str_value_wp_ext(k2h_h handle, const char* pkey, char** ppval, k2h_get_trial_callback fp, void* pExtData, const char* pass)
{
	if(ISEMPTYSTR(pkey) || !ppval){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	size_t	vallength = 0UL;
	return k2h_get_value_wp_ext(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<unsigned char**>(ppval), &vallength, fp, pExtData, pass);
}

char* k2h_get_str_direct_value_wp_ext(k2h_h handle, const char* pkey, k2h_get_trial_callback fp, void* pExtData, const char* pass)
{
	char*	pval = NULL;
	if(!k2h_get_str_value_wp_ext(handle, pkey, &pval, fp, pExtData, pass)){
		return NULL;
	}
	return pval;
}

bool k2h_get_value_np_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData)
{
	if(!pkey || 0UL == keylength || !ppval || !pvallength || !fp){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppval		= NULL;
	*pvallength	= 0UL;

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	ssize_t	result;
	if(-1 == (result = pShm->Get(pkey, keylength, ppval, fp, pExtData, false, NULL))){
		MSG_K2HPRN("Not found key or not have value.");
		return false;
	}
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress redundantAssignment
	*pvallength = static_cast<size_t>(result);

	return true;
}

unsigned char* k2h_get_direct_value_np_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData)
{
	unsigned char*	pval = NULL;
	if(!k2h_get_value_np_ext(handle, pkey, keylength, &pval, pvallength, fp, pExtData)){
		return NULL;
	}
	return pval;
}

bool k2h_get_str_value_np_ext(k2h_h handle, const char* pkey, char** ppval, k2h_get_trial_callback fp, void* pExtData)
{
	if(ISEMPTYSTR(pkey) || !ppval){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	size_t	vallength = 0UL;
	return k2h_get_value_np_ext(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<unsigned char**>(ppval), &vallength, fp, pExtData);
}

char* k2h_get_str_direct_value_np_ext(k2h_h handle, const char* pkey, k2h_get_trial_callback fp, void* pExtData)
{
	char*	pval = NULL;
	if(!k2h_get_str_value_np_ext(handle, pkey, &pval, fp, pExtData)){
		return NULL;
	}
	return pval;
}

bool k2h_get_subkeys(k2h_h handle, const unsigned char* pkey, size_t keylength, PK2HKEYPCK* ppskeypck, int* pskeypckcnt)
{
	if(!pkey || 0UL == keylength || !ppskeypck || !pskeypckcnt){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppskeypck	= NULL;
	*pskeypckcnt= 0UL;

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	K2HSubKeys*	pSubKeys;
	if(NULL == (pSubKeys = pShm->GetSubKeys(pkey, keylength))){
		MSG_K2HPRN("Not found key or not have subkeys.");
		return false;
	}

	size_t		subkeycnt = pSubKeys->size();
	PK2HKEYPCK	pskeypck;
	if(NULL == (pskeypck = reinterpret_cast<PK2HKEYPCK>(calloc(subkeycnt, sizeof(K2HKEYPCK))))){
		ERR_K2HPRN("Could not allocate memory.");
		K2H_Delete(pSubKeys);
		return false;
	}

	// copy
	int	setpos = 0;
	// cppcheck-suppress postfixOperator
	for(K2HSubKeys::iterator iter = pSubKeys->begin(); iter != pSubKeys->end(); iter++){
		if(0UL == iter->length){
			WAN_K2HPRN("Subkey is empty.");
			continue;
		}
		if(NULL == (pskeypck[setpos].pkey = reinterpret_cast<unsigned char*>(malloc(iter->length)))){
			ERR_K2HPRN("Could not allocate memory.");
			k2h_free_keypack(pskeypck, subkeycnt);
			K2H_Delete(pSubKeys);
			return false;
		}
		memcpy(pskeypck[setpos].pkey, iter->pSubKey, iter->length);
		pskeypck[setpos].length = iter->length;
		setpos++;
	}

	if(0 == setpos){
		MSG_K2HPRN("Not have subkeys.");
		k2h_free_keypack(pskeypck, subkeycnt);
		K2H_Delete(pSubKeys);
		return false;
	}
	K2H_Delete(pSubKeys);
	*ppskeypck	= pskeypck;
	*pskeypckcnt= setpos;

	return true;
}

PK2HKEYPCK k2h_get_direct_subkeys(k2h_h handle, const unsigned char* pkey, size_t keylength, int* pskeypckcnt)
{
	PK2HKEYPCK	pskeypck = NULL;

	if(!k2h_get_subkeys(handle, pkey, keylength, &pskeypck, pskeypckcnt)){
		return NULL;
	}
	return pskeypck;
}

// This function returns string array pointer which is null terminated.
// So each subkey must be terminated null byte, if not maybe something error is occurred.
//
int k2h_get_str_subkeys(k2h_h handle, const char* pkey, char*** ppskeyarray)
{
	if(ISEMPTYSTR(pkey) || !ppskeyarray){
		ERR_K2HPRN("Parameters are wrong.");
		return -1;
	}
	*ppskeyarray = NULL;

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return -1;
	}

	K2HSubKeys*	pSubKeys;
	if(NULL == (pSubKeys = pShm->GetSubKeys(reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1))){
		MSG_K2HPRN("Not found key or not have subkeys.");
		return -1;
	}

	size_t	subkeycnt = pSubKeys->size();
	char**	pskeyarray;
	if(NULL == (pskeyarray = reinterpret_cast<char**>(calloc(subkeycnt + 1UL, sizeof(char*))))){
		ERR_K2HPRN("Could not allocate memory.");
		K2H_Delete(pSubKeys);
		return -1;
	}

	// copy
	int	setpos = 0;
	// cppcheck-suppress postfixOperator
	for(K2HSubKeys::iterator iter = pSubKeys->begin(); iter != pSubKeys->end(); iter++){
		if(0UL == iter->length){
			WAN_K2HPRN("Subkey is empty.");
			continue;
		}
		if(NULL == (pskeyarray[setpos] = reinterpret_cast<char*>(malloc(iter->length + 1UL)))){
			ERR_K2HPRN("Could not allocate memory.");
			k2h_free_keyarray(pskeyarray);
			K2H_Delete(pSubKeys);
			return -1;
		}
		memcpy(pskeyarray[setpos], iter->pSubKey, iter->length);
		(pskeyarray[setpos])[iter->length] = '\0';						// for safe
		setpos++;
	}

	if(0 == setpos){
		MSG_K2HPRN("Not have subkeys.");
		k2h_free_keyarray(pskeyarray);
		K2H_Delete(pSubKeys);
		return -1;
	}
	K2H_Delete(pSubKeys);
	*ppskeyarray = pskeyarray;

	return setpos;
}

char** k2h_get_str_direct_subkeys(k2h_h handle, const char* pkey)
{
	char**	pskeyarray = NULL;
	if(-1 == k2h_get_str_subkeys(handle, pkey, &pskeyarray)){
		return NULL;
	}
	return pskeyarray;
}

bool k2h_get_subkeys_np(k2h_h handle, const unsigned char* pkey, size_t keylength, PK2HKEYPCK* ppskeypck, int* pskeypckcnt)
{
	if(!pkey || 0UL == keylength || !ppskeypck || !pskeypckcnt){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppskeypck	= NULL;
	*pskeypckcnt= 0UL;

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	K2HSubKeys*	pSubKeys;
	if(NULL == (pSubKeys = pShm->GetSubKeys(pkey, keylength, false))){
		MSG_K2HPRN("Not found key or not have subkeys.");
		return false;
	}

	size_t		subkeycnt = pSubKeys->size();
	PK2HKEYPCK	pskeypck;
	if(NULL == (pskeypck = reinterpret_cast<PK2HKEYPCK>(calloc(subkeycnt, sizeof(K2HKEYPCK))))){
		ERR_K2HPRN("Could not allocate memory.");
		K2H_Delete(pSubKeys);
		return false;
	}

	// copy
	int	setpos = 0;
	// cppcheck-suppress postfixOperator
	for(K2HSubKeys::iterator iter = pSubKeys->begin(); iter != pSubKeys->end(); iter++){
		if(0UL == iter->length){
			WAN_K2HPRN("Subkey is empty.");
			continue;
		}
		if(NULL == (pskeypck[setpos].pkey = reinterpret_cast<unsigned char*>(malloc(iter->length)))){
			ERR_K2HPRN("Could not allocate memory.");
			k2h_free_keypack(pskeypck, subkeycnt);
			K2H_Delete(pSubKeys);
			return false;
		}
		memcpy(pskeypck[setpos].pkey, iter->pSubKey, iter->length);
		pskeypck[setpos].length = iter->length;
		setpos++;
	}

	if(0 == setpos){
		MSG_K2HPRN("Not have subkeys.");
		k2h_free_keypack(pskeypck, subkeycnt);
		K2H_Delete(pSubKeys);
		return false;
	}
	K2H_Delete(pSubKeys);
	*ppskeypck	= pskeypck;
	*pskeypckcnt= setpos;

	return true;
}

PK2HKEYPCK k2h_get_direct_subkeys_np(k2h_h handle, const unsigned char* pkey, size_t keylength, int* pskeypckcnt)
{
	PK2HKEYPCK	pskeypck = NULL;

	if(!k2h_get_subkeys_np(handle, pkey, keylength, &pskeypck, pskeypckcnt)){
		return NULL;
	}
	return pskeypck;
}

// This function returns string array pointer which is null terminated.
// So each subkey must be terminated null byte, if not maybe something error is occurred.
//
int k2h_get_str_subkeys_np(k2h_h handle, const char* pkey, char*** ppskeyarray)
{
	if(ISEMPTYSTR(pkey) || !ppskeyarray){
		ERR_K2HPRN("Parameters are wrong.");
		return -1;
	}
	*ppskeyarray = NULL;

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return -1;
	}

	K2HSubKeys*	pSubKeys;
	if(NULL == (pSubKeys = pShm->GetSubKeys(reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, false))){
		MSG_K2HPRN("Not found key or not have subkeys.");
		return -1;
	}

	size_t	subkeycnt = pSubKeys->size();
	char**	pskeyarray;
	if(NULL == (pskeyarray = reinterpret_cast<char**>(calloc(subkeycnt + 1UL, sizeof(char*))))){
		ERR_K2HPRN("Could not allocate memory.");
		K2H_Delete(pSubKeys);
		return -1;
	}

	// copy
	int	setpos = 0;
	// cppcheck-suppress postfixOperator
	for(K2HSubKeys::iterator iter = pSubKeys->begin(); iter != pSubKeys->end(); iter++){
		if(0UL == iter->length){
			WAN_K2HPRN("Subkey is empty.");
			continue;
		}
		if(NULL == (pskeyarray[setpos] = reinterpret_cast<char*>(malloc(iter->length + 1UL)))){
			ERR_K2HPRN("Could not allocate memory.");
			k2h_free_keyarray(pskeyarray);
			K2H_Delete(pSubKeys);
			return -1;
		}
		memcpy(pskeyarray[setpos], iter->pSubKey, iter->length);
		(pskeyarray[setpos])[iter->length] = '\0';						// for safe
		setpos++;
	}

	if(0 == setpos){
		MSG_K2HPRN("Not have subkeys.");
		k2h_free_keyarray(pskeyarray);
		K2H_Delete(pSubKeys);
		return -1;
	}
	K2H_Delete(pSubKeys);
	*ppskeyarray = pskeyarray;

	return setpos;
}

char** k2h_get_str_direct_subkeys_np(k2h_h handle, const char* pkey)
{
	char**	pskeyarray = NULL;
	if(-1 == k2h_get_str_subkeys_np(handle, pkey, &pskeyarray)){
		return NULL;
	}
	return pskeyarray;
}

bool k2h_free_keypack(PK2HKEYPCK pkeys, int keycnt)
{
	if(!pkeys || 0 >= keycnt){
		return true;
	}
	for(int cnt = 0; cnt < keycnt; cnt++){
		K2H_Free(pkeys[cnt].pkey);
	}
	// cppcheck-suppress uselessAssignmentPtrArg
	K2H_Free(pkeys);

	return true;
}

bool k2h_free_keyarray(char** pkeys)
{
	if(!pkeys){
		return true;
	}
	for(char** ptmp = pkeys; *ptmp; ptmp++){
		// cppcheck-suppress unmatchedSuppression
		// cppcheck-suppress identicalInnerCondition
		K2H_Free(*ptmp);
	}
	// cppcheck-suppress uselessAssignmentPtrArg
	K2H_Free(pkeys);

	return true;
}

// Utility for converting
PK2HATTRPCK k2h_cvt_attrs_to_bin(K2HAttrs* pAttrs, int& attrspckcnt)
{
	if(!pAttrs || 0 == pAttrs->size()){
		attrspckcnt = 0;
		return NULL;
	}

	PK2HATTRPCK	pattrspck;
	attrspckcnt	= static_cast<int>(pAttrs->size());
	if(NULL == (pattrspck = reinterpret_cast<PK2HATTRPCK>(calloc(attrspckcnt, sizeof(K2HATTRPCK))))){
		ERR_K2HPRN("Could not allocate memory.");
		attrspckcnt = 0;
		return NULL;
	}

	// copy
	int	setpos = 0;
	// cppcheck-suppress postfixOperator
	for(K2HAttrs::iterator iter = pAttrs->begin(); iter != pAttrs->end(); iter++){
		if(0UL == iter->keylength){
			WAN_K2HPRN("Attr key is empty, skip this.");
			continue;
		}
		if(NULL == (pattrspck[setpos].pkey = reinterpret_cast<unsigned char*>(malloc(iter->keylength)))){
			ERR_K2HPRN("Could not allocate memory.");
			k2h_free_attrpack(pattrspck, attrspckcnt);
			attrspckcnt = 0;
			return NULL;
		}
		memcpy(pattrspck[setpos].pkey, iter->pkey, iter->keylength);
		pattrspck[setpos].keylength = iter->keylength;

		if(NULL == (pattrspck[setpos].pval = reinterpret_cast<unsigned char*>(malloc(iter->vallength)))){
			ERR_K2HPRN("Could not allocate memory.");
			k2h_free_attrpack(pattrspck, attrspckcnt);
			attrspckcnt = 0;
			return NULL;
		}
		memcpy(pattrspck[setpos].pval, iter->pval, iter->vallength);
		pattrspck[setpos].vallength = iter->vallength;

		setpos++;
	}

	if(0 == setpos){
		MSG_K2HPRN("Not have attributes.");
		k2h_free_attrpack(pattrspck, attrspckcnt);
		attrspckcnt = 0;
		return NULL;
	}
	attrspckcnt = setpos;
	return pattrspck;
}

// Utility for converting
static K2HAttrs* k2h_cvt_bin_to_attrs(const PK2HATTRPCK pattrspck, int attrspckcnt)
{
	if(!pattrspck || attrspckcnt <= 0){
		return NULL;
	}

	K2HAttrs*	pAttrs = new K2HAttrs();
	for(int cnt = 0; cnt < attrspckcnt; ++cnt){
		if(pAttrs->end() == pAttrs->insert(pattrspck[cnt].pkey, pattrspck[cnt].keylength, pattrspck[cnt].pval, pattrspck[cnt].vallength)){
			WAN_K2HPRN("Failed to set one attribute.");
		}
	}
	if(0 == pAttrs->size()){
		K2H_Delete(pAttrs);
	}
	return pAttrs;
}

bool k2h_get_attrs(k2h_h handle, const unsigned char* pkey, size_t keylength, PK2HATTRPCK* ppattrspck, int* pattrspckcnt)
{
	if(!pkey || 0UL == keylength || !ppattrspck || !pattrspckcnt){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppattrspck	= NULL;
	*pattrspckcnt= 0UL;

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	K2HAttrs*	pAttrs;
	if(NULL == (pAttrs = pShm->GetAttrs(pkey, keylength))){
		MSG_K2HPRN("Not found key or not have attributes.");
		return false;
	}

	// convert to binary structure
	*ppattrspck = k2h_cvt_attrs_to_bin(pAttrs, *pattrspckcnt);
	K2H_Delete(pAttrs);

	return (0 < *pattrspckcnt);
}

PK2HATTRPCK k2h_get_direct_attrs(k2h_h handle, const unsigned char* pkey, size_t keylength, int* pattrspckcnt)
{
	PK2HATTRPCK	pattrspck = NULL;

	if(!k2h_get_attrs(handle, pkey, keylength, &pattrspck, pattrspckcnt)){
		return NULL;
	}
	return pattrspck;
}

PK2HATTRPCK k2h_get_str_direct_attrs(k2h_h handle, const char* pkey, int* pattrspckcnt)
{
	return k2h_get_direct_attrs(handle, reinterpret_cast<const unsigned char*>(pkey), pkey ? strlen(pkey) + 1 : 0, pattrspckcnt);
}

bool k2h_free_attrpack(PK2HATTRPCK pattrs, int attrcnt)
{
	if(!pattrs || 0 >= attrcnt){
		return true;
	}
	for(int cnt = 0; cnt < attrcnt; cnt++){
		K2H_Free(pattrs[cnt].pkey);
		K2H_Free(pattrs[cnt].pval);
	}
	// cppcheck-suppress uselessAssignmentPtrArg
	K2H_Free(pattrs);

	return true;
}

//---------------------------------------------------------
// Functions : Set
//---------------------------------------------------------
bool k2h_set_all(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength, const PK2HKEYPCK pskeypck, int skeypckcnt)
{
	return k2h_set_all_wa(handle, pkey, keylength, pval, vallength, pskeypck, skeypckcnt, NULL, NULL);
}

bool k2h_set_str_all(k2h_h handle, const char* pkey, const char* pval, const char** pskeyarray)
{
	return k2h_set_str_all_wa(handle, pkey, pval, pskeyarray, NULL, NULL);
}

// This function keeps subkeys
//
bool k2h_set_value(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength)
{
	return k2h_set_value_wa(handle, pkey, keylength, pval, vallength, NULL, NULL);
}

// This function keeps subkeys
//
bool k2h_set_str_value(k2h_h handle, const char* pkey, const char* pval)
{
	return k2h_set_str_value_wa(handle, pkey, pval, NULL, NULL);
}

bool k2h_set_all_wa(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength, const PK2HKEYPCK pskeypck, int skeypckcnt, const char* pass, const time_t* expire)
{
	if(!pkey || 0UL == keylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	K2HSubKeys	Subkeys;
	for(int cnt = 0; cnt < skeypckcnt; cnt++){
		if(Subkeys.end() == Subkeys.insert(pskeypck[cnt].pkey, pskeypck[cnt].length)){
			ERR_K2HPRN("Subkeys array is something wrong.");
			return false;
		}
	}
	K2HSubKeys*	pSubKeys = (0UL < Subkeys.size() ? &Subkeys : NULL);

	if(!pShm->Set(pkey, keylength, pval, vallength, pSubKeys, true, NULL, pass, expire)){
		ERR_K2HPRN("Failed to set value and subkeys to key.");
		return false;
	}
	return true;
}

bool k2h_set_str_all_wa(k2h_h handle, const char* pkey, const char* pval, const char** pskeyarray, const char* pass, const time_t* expire)
{
	if(ISEMPTYSTR(pkey)){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	K2HSubKeys	Subkeys;
	for(; pskeyarray && *pskeyarray; pskeyarray++){
		if(Subkeys.end() == Subkeys.insert(*pskeyarray)){
			ERR_K2HPRN("Subkeys array is something wrong.");
			return false;
		}
	}
	K2HSubKeys*	pSubKeys = (0UL < Subkeys.size() ? &Subkeys : NULL);

	if(!pShm->Set(reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<const unsigned char*>(pval), pval ? strlen(pval) + 1 : 0UL, pSubKeys, true, NULL, pass, expire)){
		ERR_K2HPRN("Failed to set value and subkeys to key.");
		return false;
	}
	return true;
}

// This function keeps subkeys
//
bool k2h_set_value_wa(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength, const char* pass, const time_t* expire)
{
	if(!pkey || 0UL == keylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->Set(pkey, keylength, pval, vallength, pass, expire)){
		ERR_K2HPRN("Failed to set value to key.");
		return false;
	}
	return true;
}

// This function keeps subkeys
//
bool k2h_set_str_value_wa(k2h_h handle, const char* pkey, const char* pval, const char* pass, const time_t* expire)
{
	if(ISEMPTYSTR(pkey)){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	return k2h_set_value_wa(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<const unsigned char*>(pval), pval ? strlen(pval) + 1 : 0UL, pass, expire);
}

// This function keeps value
//
bool k2h_set_subkeys(k2h_h handle, const unsigned char* pkey, size_t keylength, const PK2HKEYPCK pskeypck, int skeypckcnt)
{
	if(!pkey || 0UL == keylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	K2HSubKeys	Subkeys;
	for(int cnt = 0; cnt < skeypckcnt; cnt++){
		if(Subkeys.end() == Subkeys.insert(pskeypck[cnt].pkey, pskeypck[cnt].length)){
			ERR_K2HPRN("Subkeys array is something wrong.");
			return false;
		}
	}
	unsigned char*	bySubkeys = NULL;
	size_t			skeylength= 0UL;
	if(0UL < Subkeys.size()){
		if(!Subkeys.Serialize(&bySubkeys, skeylength)){
			ERR_K2HPRN("Could not make subkeys binary array.");
			return false;
		}
	}
	if(!pShm->ReplaceSubkeys(pkey, keylength, bySubkeys, skeylength)){
		ERR_K2HPRN("Failed to set value and subkeys to key.");
		K2H_Free(bySubkeys);
		return false;
	}
	K2H_Free(bySubkeys);
	return true;
}

// This function keeps value
//
bool k2h_set_str_subkeys(k2h_h handle, const char* pkey, const char** pskeyarray)
{
	if(ISEMPTYSTR(pkey)){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	K2HSubKeys	Subkeys;
	for(; pskeyarray && *pskeyarray; pskeyarray++){
		if(Subkeys.end() == Subkeys.insert(*pskeyarray)){
			ERR_K2HPRN("Subkeys array is something wrong.");
			return false;
		}
	}
	unsigned char*	bySubkeys = NULL;
	size_t			skeylength= 0UL;
	if(0UL < Subkeys.size()){
		if(!Subkeys.Serialize(&bySubkeys, skeylength)){
			ERR_K2HPRN("Could not make subkeys binary array.");
			return false;
		}
	}
	if(!pShm->ReplaceSubkeys(reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, bySubkeys, skeylength)){
		ERR_K2HPRN("Failed to set value and subkeys to key.");
		K2H_Free(bySubkeys);
		return false;
	}
	K2H_Free(bySubkeys);
	return true;
}

bool k2h_add_subkey(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* psubkey, size_t skeylength, const unsigned char* pval, size_t vallength)
{
	return k2h_add_subkey_wa(handle, pkey, keylength, psubkey, skeylength, pval, vallength, NULL, NULL);
}

bool k2h_add_str_subkey(k2h_h handle, const char* pkey, const char* psubkey, const char* pval)
{
	return k2h_add_str_subkey_wa(handle, pkey, psubkey, pval, NULL, NULL);
}

bool k2h_add_subkey_wa(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* psubkey, size_t skeylength, const unsigned char* pval, size_t vallength, const char* pass, const time_t* expire)
{
	if(!pkey || 0UL == keylength || !psubkey || 0UL == skeylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->AddSubkey(pkey, keylength, psubkey, skeylength, pval, vallength, pass, expire)){
		ERR_K2HPRN("Failed to set subkey and value to key.");
		return false;
	}
	return true;
}

bool k2h_add_str_subkey_wa(k2h_h handle, const char* pkey, const char* psubkey, const char* pval, const char* pass, const time_t* expire)
{
	if(ISEMPTYSTR(pkey) || ISEMPTYSTR(psubkey)){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	return k2h_add_subkey_wa(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<const unsigned char*>(psubkey), strlen(psubkey) + 1, reinterpret_cast<const unsigned char*>(pval), pval ? strlen(pval) + 1 : 0UL, pass, expire);
}

bool k2h_add_attr(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pattrkey, size_t attrkeylength, const unsigned char* pattrval, size_t attrvallength)
{
	if(!pkey || 0UL == keylength || !pattrkey || 0UL == attrkeylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->AddAttr(pkey, keylength, pattrkey, attrkeylength, pattrval, attrvallength)){
		ERR_K2HPRN("Failed to add attribute to key.");
		return false;
	}
	return true;
}

bool k2h_add_str_attr(k2h_h handle, const char* pkey, const char* pattrkey, const char* pattrval)
{
	if(ISEMPTYSTR(pkey) || ISEMPTYSTR(pattrkey)){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	return k2h_add_attr(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<const unsigned char*>(pattrkey), strlen(pattrkey) + 1, reinterpret_cast<const unsigned char*>(pattrval), pattrval ? strlen(pattrval) + 1 : 0UL);
}

//---------------------------------------------------------
// Functions : Remove
//---------------------------------------------------------
static bool k2h_remove_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, bool issubkey)
{
	if(!pkey || 0UL == keylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->Remove(pkey, keylength, issubkey)){
		ERR_K2HPRN("Failed to remove key %s subkeys.", issubkey ? "with" : "without");
		return false;
	}
	return true;
}

bool k2h_remove_all(k2h_h handle, const unsigned char* pkey, size_t keylength)
{
	return k2h_remove_ext(handle, pkey, keylength, true);
}

bool k2h_remove_str_all(k2h_h handle, const char* pkey)
{
	if(ISEMPTYSTR(pkey)){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	return k2h_remove_ext(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, true);
}

bool k2h_remove(k2h_h handle, const unsigned char* pkey, size_t keylength)
{
	return k2h_remove_ext(handle, pkey, keylength, false);
}

bool k2h_remove_str(k2h_h handle, const char* pkey)
{
	if(ISEMPTYSTR(pkey)){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	return k2h_remove_ext(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, false);
}

bool k2h_remove_subkey(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* psubkey, size_t skeylength)
{
	if(!pkey || 0UL == keylength || !psubkey || 0UL == skeylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->Remove(pkey, keylength, psubkey, skeylength)){
		ERR_K2HPRN("Failed to remove subkey and subkeys list in key.");
		return false;
	}
	return true;
}

bool k2h_remove_str_subkey(k2h_h handle, const char* pkey, const char* psubkey)
{
	if(ISEMPTYSTR(pkey) || ISEMPTYSTR(psubkey)){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	return k2h_remove_subkey(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<const unsigned char*>(psubkey), strlen(psubkey) + 1);
}

//---------------------------------------------------------
// Functions : Rename
//---------------------------------------------------------
bool k2h_rename(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pnewkey, size_t newkeylength)
{
	if(!pkey || 0UL == keylength || !pnewkey || 0UL == newkeylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	if(!pShm->Rename(pkey, keylength, pnewkey, newkeylength)){
		ERR_K2HPRN("Failed to rename key.");
		return false;
	}
	return true;
}

bool k2h_rename_str(k2h_h handle, const char* pkey, const char* pnewkey)
{
	if(ISEMPTYSTR(pkey) || ISEMPTYSTR(pnewkey)){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	return k2h_rename(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1, reinterpret_cast<const unsigned char*>(pnewkey), strlen(pnewkey) + 1);
}

//---------------------------------------------------------
// Functions : Direct set/get
//---------------------------------------------------------
bool k2h_get_elements_by_hash(k2h_h handle, const k2h_hash_t starthash, const struct timespec startts, const struct timespec endts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check, k2h_hash_t* pnexthash, PK2HBIN* ppbindatas, size_t* pdatacnt)
{
	if(!pnexthash || !ppbindatas || !pdatacnt){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	if(!pShm->GetElementsByHash(starthash, startts, endts, target_hash, target_max_hash, old_hash, old_max_hash, target_hash_range, is_expire_check, pnexthash, ppbindatas, pdatacnt)){
		ERR_K2HPRN("Failed to get direct elements binary data.");
		return false;
	}
	return true;
}

bool k2h_set_element_by_binary(k2h_h handle, const PK2HBIN pbindatas, const struct timespec* pts)
{
	if(!pbindatas || !pbindatas->byptr || 0 == pbindatas->length || !pts){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}

	PBALLEDATA	pbinall = reinterpret_cast<PBALLEDATA>(pbindatas->byptr);
	if(!pShm->SetElementByBinArray(&(pbinall->rawdata), pts)){
		ERR_K2HPRN("Failed to set direct element binary data.");
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Functions : Search
//---------------------------------------------------------
k2h_find_h k2h_find_first(k2h_h handle)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return K2H_INVALID_HANDLE;
	}

	PK2HFINDHANDLE	pFindHandle = new K2HFINDHANDLE;
	pFindHandle->pShm	= pShm;
	pFindHandle->iskey	= false;
	pFindHandle->piter	= new K2HShm::iterator(pShm->begin());

	if(pShm->end() == *(pFindHandle->piter)){
		k2h_find_free(reinterpret_cast<k2h_find_h>(pFindHandle));
		return K2H_INVALID_HANDLE;
	}
	return reinterpret_cast<k2h_find_h>(pFindHandle);
}

k2h_find_h k2h_find_first_subkey(k2h_h handle, const unsigned char* pkey, size_t keylength)
{
	if(!pkey || 0UL == keylength){
		ERR_K2HPRN("Parameters are wrong.");
		return K2H_INVALID_HANDLE;
	}

	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return K2H_INVALID_HANDLE;
	}

	PK2HFINDHANDLE	pFindHandle = new K2HFINDHANDLE;
	pFindHandle->pShm	= pShm;
	pFindHandle->iskey	= true;
	pFindHandle->piter	= new K2HShm::iterator(pShm->begin(pkey, keylength));

	if(pShm->end(true) == *(pFindHandle->piter)){
		k2h_find_free(reinterpret_cast<k2h_find_h>(pFindHandle));
		return K2H_INVALID_HANDLE;
	}
	return reinterpret_cast<k2h_find_h>(pFindHandle);
}

k2h_find_h k2h_find_first_str_subkey(k2h_h handle, const char* pkey)
{
	if(ISEMPTYSTR(pkey)){
		ERR_K2HPRN("Parameter is wrong.");
		return K2H_INVALID_HANDLE;
	}
	return k2h_find_first_subkey(handle, reinterpret_cast<const unsigned char*>(pkey), strlen(pkey) + 1);
}

k2h_find_h k2h_find_next(k2h_find_h findhandle)
{
	PK2HFINDHANDLE	pFindHandle = reinterpret_cast<PK2HFINDHANDLE>(findhandle);
	if(!pFindHandle){
		return K2H_INVALID_HANDLE;
	}
	(*(pFindHandle->piter))++;

	if(pFindHandle->pShm->end(pFindHandle->iskey) == *(pFindHandle->piter)){
		k2h_find_free(findhandle);
		return K2H_INVALID_HANDLE;
	}
	return findhandle;
}

bool k2h_find_free(k2h_find_h findhandle)
{
	PK2HFINDHANDLE	pFindHandle = reinterpret_cast<PK2HFINDHANDLE>(findhandle);
	if(!pFindHandle){
		return true;
	}
	K2H_Delete(pFindHandle->piter);
	K2H_Delete(pFindHandle);
	return true;
}

static bool k2h_find_get_ext(k2h_find_h findhandle, unsigned char** ppdata, size_t* pdatalength, int type)
{
	if(!ppdata || !pdatalength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppdata		= NULL;
	*pdatalength= 0UL;

	PK2HFINDHANDLE	pFindHandle = reinterpret_cast<PK2HFINDHANDLE>(findhandle);
	if(!pFindHandle){
		ERR_K2HPRN("Invalid k2h_find_h handle.");
		return false;
	}
	if(!pFindHandle->piter){
		ERR_K2HPRN("Invalid iterator in k2h_find_h handle.");
		return false;
	}

	K2HShm::iterator*	iter = pFindHandle->piter;
	ssize_t				result;
	if(-1 == (result = pFindHandle->pShm->Get(*(*iter), ppdata, type))){
		ERR_K2HPRN("Could not get type(%d) data from k2h_find_h.", type);
		return false;
	}
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress redundantAssignment
	*pdatalength = static_cast<size_t>(result);

	return true;
}

bool k2h_find_get_key(k2h_find_h findhandle, unsigned char** ppkey, size_t* pkeylength)
{
	return k2h_find_get_ext(findhandle, ppkey, pkeylength, K2HShm::PAGEOBJ_KEY);
}

char* k2h_find_get_str_key(k2h_find_h findhandle)
{
	unsigned char*	pkey		= NULL;
	size_t			keylength	= 0UL;

	if(!k2h_find_get_ext(findhandle, &pkey, &keylength, K2HShm::PAGEOBJ_KEY)){
		return NULL;
	}
	return reinterpret_cast<char*>(pkey);
}

bool k2h_find_get_value(k2h_find_h findhandle, unsigned char** ppval, size_t* pvallength)
{
	return k2h_find_get_ext(findhandle, ppval, pvallength, K2HShm::PAGEOBJ_VALUE);
}

char* k2h_find_get_direct_value(k2h_find_h findhandle)
{
	unsigned char*	pval		= NULL;
	size_t			vallength	= 0UL;

	if(!k2h_find_get_ext(findhandle, &pval, &vallength, K2HShm::PAGEOBJ_VALUE)){
		return NULL;
	}
	return reinterpret_cast<char*>(pval);
}

bool k2h_find_get_subkeys(k2h_find_h findhandle, PK2HKEYPCK* ppskeypck, int* pskeypckcnt)
{
	if(!ppskeypck || !pskeypckcnt){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppskeypck	= NULL;
	*pskeypckcnt= 0;

	unsigned char*	bysubkeys	= NULL;
	size_t			skeylength	= 0UL;
	if(!k2h_find_get_ext(findhandle, &bysubkeys, &skeylength, K2HShm::PAGEOBJ_SUBKEYS) || !bysubkeys || 0UL == skeylength){
		MSG_K2HPRN("Not have subkeys.");
		K2H_Free(bysubkeys);
		return false;
	}

	K2HSubKeys	Subkeys(bysubkeys, skeylength);
	K2H_Free(bysubkeys);

	size_t		subkeycnt = Subkeys.size();
	PK2HKEYPCK	pskeypck;
	if(NULL == (pskeypck = reinterpret_cast<PK2HKEYPCK>(calloc(subkeycnt, sizeof(K2HKEYPCK))))){
		ERR_K2HPRN("Could not allocate memory.");
		return false;
	}

	int	setpos = 0;
	// cppcheck-suppress postfixOperator
	for(K2HSubKeys::iterator iter = Subkeys.begin(); iter != Subkeys.end(); iter++){
		if(0UL == iter->length){
			WAN_K2HPRN("Subkey is empty.");
			continue;
		}
		if(NULL == (pskeypck[setpos].pkey = reinterpret_cast<unsigned char*>(malloc(iter->length)))){
			ERR_K2HPRN("Could not allocate memory.");
			k2h_free_keypack(pskeypck, subkeycnt);
			return false;
		}
		memcpy(pskeypck[setpos].pkey, iter->pSubKey, iter->length);
		pskeypck[setpos].length = iter->length;
		setpos++;
	}

	if(0 == setpos){
		MSG_K2HPRN("Not have subkeys.");
		k2h_free_keypack(pskeypck, subkeycnt);
		return false;
	}
	*ppskeypck	= pskeypck;
	*pskeypckcnt= setpos;

	return true;
}

PK2HKEYPCK k2h_find_get_direct_subkeys(k2h_find_h findhandle, int* pskeypckcnt)
{
	PK2HKEYPCK	pskeypck = NULL;
	if(!k2h_find_get_subkeys(findhandle, &pskeypck, pskeypckcnt)){
		return NULL;
	}
	return pskeypck;
}

// This function returns string array pointer which is null terminated.
// So each subkey must be terminated null byte, if not maybe something error is occurred.
//
int k2h_find_get_str_subkeys(k2h_find_h findhandle, char*** ppskeyarray)
{
	if(!ppskeyarray){
		ERR_K2HPRN("Parameter is wrong.");
		return -1;
	}
	*ppskeyarray = NULL;

	unsigned char*	bysubkeys	= NULL;
	size_t			skeylength	= 0UL;
	if(!k2h_find_get_ext(findhandle, &bysubkeys, &skeylength, K2HShm::PAGEOBJ_SUBKEYS) || !bysubkeys || 0UL == skeylength){
		MSG_K2HPRN("Not have subkeys.");
		K2H_Free(bysubkeys);
		return -1;
	}

	K2HSubKeys	Subkeys(bysubkeys, skeylength);
	K2H_Free(bysubkeys);

	size_t	subkeycnt = Subkeys.size();
	char**	pskeyarray;
	if(NULL == (pskeyarray = reinterpret_cast<char**>(calloc(subkeycnt + 1UL, sizeof(char*))))){
		ERR_K2HPRN("Could not allocate memory.");
		return -1;
	}

	// copy
	int	setpos = 0;
	// cppcheck-suppress postfixOperator
	for(K2HSubKeys::iterator iter = Subkeys.begin(); iter != Subkeys.end(); iter++){
		if(0UL == iter->length){
			WAN_K2HPRN("Subkey is empty.");
			continue;
		}
		if(NULL == (pskeyarray[setpos] = reinterpret_cast<char*>(malloc(iter->length + 1UL)))){
			ERR_K2HPRN("Could not allocate memory.");
			k2h_free_keyarray(pskeyarray);
			return -1;
		}
		memcpy(pskeyarray[setpos], iter->pSubKey, iter->length);
		(pskeyarray[setpos])[iter->length] = '\0';						// for safe
		setpos++;
	}

	if(0 == setpos){
		MSG_K2HPRN("Not have subkeys.");
		k2h_free_keyarray(pskeyarray);
		return -1;
	}
	*ppskeyarray = pskeyarray;

	return setpos;
}

// This function returns string array pointer which is null terminated.
// So each subkey must be terminated null byte, if not maybe something error is occurred.
//
char** k2h_find_get_str_direct_subkeys(k2h_find_h findhandle)
{
	char**	pskeyarray = NULL;
	if(!k2h_find_get_str_subkeys(findhandle, &pskeyarray)){
		return NULL;
	}
	return pskeyarray;
}

//---------------------------------------------------------
// Functions : Direct Access
//---------------------------------------------------------
k2h_da_h k2h_da_handle(k2h_h handle, const unsigned char* pkey, size_t keylength, K2HDAMODE mode)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return K2H_INVALID_HANDLE;
	}

	// open mode
	K2HDAccess::ACSMODE	acsmode;
	if(K2H_DA_READ == mode){
		acsmode = K2HDAccess::READ_ACCESS;
	}else if(K2H_DA_WRITE == mode){
		acsmode = K2HDAccess::WRITE_ACCESS;
	}else if(K2H_DA_RW == mode){
		acsmode = K2HDAccess::RW_ACCESS;
	}else{
		ERR_K2HPRN("Unknown K2HDAMODE %d.", mode);
		return K2H_INVALID_HANDLE;
	}

	// attach object
	K2HDAccess*	pAccess;
	if(NULL == (pAccess = pShm->GetDAccessObj(pkey, keylength, acsmode))){
		ERR_K2HPRN("Could not initialize internal K2HDAccess object.");
		return K2H_INVALID_HANDLE;
	}

	return reinterpret_cast<k2h_da_h>(pAccess);
}

k2h_da_h k2h_da_handle_read(k2h_h handle, const unsigned char* pkey, size_t keylength)
{
	return k2h_da_handle(handle, pkey, keylength, K2H_DA_READ);
}

k2h_da_h k2h_da_handle_write(k2h_h handle, const unsigned char* pkey, size_t keylength)
{
	return k2h_da_handle(handle, pkey, keylength, K2H_DA_WRITE);
}

k2h_da_h k2h_da_handle_rw(k2h_h handle, const unsigned char* pkey, size_t keylength)
{
	return k2h_da_handle(handle, pkey, keylength, K2H_DA_RW);
}

k2h_da_h k2h_da_str_handle(k2h_h handle, const char* pkey, K2HDAMODE mode)
{
	return k2h_da_handle(handle, reinterpret_cast<const unsigned char*>(pkey), pkey ? strlen(pkey) + 1 : 0UL, mode);
}

k2h_da_h k2h_da_str_handle_read(k2h_h handle, const char* pkey)
{
	return k2h_da_handle(handle, reinterpret_cast<const unsigned char*>(pkey), pkey ? strlen(pkey) + 1 : 0UL, K2H_DA_READ);
}

k2h_da_h k2h_da_str_handle_write(k2h_h handle, const char* pkey)
{
	return k2h_da_handle(handle, reinterpret_cast<const unsigned char*>(pkey), pkey ? strlen(pkey) + 1 : 0UL, K2H_DA_WRITE);
}

k2h_da_h k2h_da_str_handle_rw(k2h_h handle, const char* pkey)
{
	return k2h_da_handle(handle, reinterpret_cast<const unsigned char*>(pkey), pkey ? strlen(pkey) + 1 : 0UL, K2H_DA_RW);
}

bool k2h_da_free(k2h_da_h dahandle)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return false;
	}
	K2H_Delete(pAccess);

	return true;
}

ssize_t k2h_da_get_length(k2h_da_h dahandle)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return -1L;
	}
	size_t	size = 0UL;
	if(!pAccess->GetSize(size)){
		ERR_K2HPRN("Failed to get value length.");
		return -1L;
	}
	return static_cast<ssize_t>(size);
}

ssize_t k2h_da_get_buf_size(k2h_da_h dahandle)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return -1L;
	}
	return static_cast<ssize_t>(pAccess->GetFioSize());
}

bool k2h_da_set_buf_size(k2h_da_h dahandle, size_t bufsize)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return false;
	}
	pAccess->SetFioSize(bufsize);

	return true;
}

off_t k2h_da_get_offset(k2h_da_h dahandle, bool is_read)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return -1L;
	}
	off_t	offset;
	if(is_read){
		offset = pAccess->GetReadOffset();
	}else{
		offset = pAccess->GetWriteOffset();
	}
	return offset;
}

off_t k2h_da_get_read_offset(k2h_da_h dahandle)
{
	return k2h_da_get_offset(dahandle, true);
}

off_t k2h_da_get_write_offset(k2h_da_h dahandle)
{
	return k2h_da_get_offset(dahandle, false);
}

bool k2h_da_set_offset(k2h_da_h dahandle, off_t offset, bool is_read)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return false;
	}
	return pAccess->SetOffset(offset, 0UL, is_read);
}

bool k2h_da_set_read_offset(k2h_da_h dahandle, off_t offset)
{
	return k2h_da_set_offset(dahandle, offset, true);
}

bool k2h_da_set_write_offset(k2h_da_h dahandle, off_t offset)
{
	return k2h_da_set_offset(dahandle, offset, false);
}

bool k2h_da_get_value(k2h_da_h dahandle, unsigned char** ppval, size_t* pvallength)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return false;
	}
	if(!ppval || !pvallength || 0UL == *pvallength){
		ERR_K2HPRN("Invalid parameters.");
		return false;
	}
	*ppval = NULL;
	return pAccess->Read(ppval, (*pvallength));
}

bool k2h_da_get_value_offset(k2h_da_h dahandle, unsigned char** ppval, size_t* pvallength, off_t offset)
{
	if(!k2h_da_set_read_offset(dahandle, offset)){
		return false;
	}
	return k2h_da_get_value(dahandle, ppval, pvallength);
}

bool k2h_da_get_value_to_file(k2h_da_h dahandle, int fd, size_t* pvallength)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return false;
	}
	if(0 > fd || !pvallength || 0UL == *pvallength){
		ERR_K2HPRN("Invalid parameters.");
		return false;
	}
	return pAccess->Read(fd, (*pvallength));
}

unsigned char* k2h_da_read(k2h_da_h dahandle, size_t* pvallength)
{
	unsigned char*	pval = NULL;

	if(!k2h_da_get_value(dahandle, &pval, pvallength)){
		K2H_Free(pval);
		return NULL;
	}
	return pval;
}

unsigned char* k2h_da_read_offset(k2h_da_h dahandle, size_t* pvallength, off_t offset)
{
	if(!k2h_da_set_read_offset(dahandle, offset)){
		return NULL;
	}
	return k2h_da_read(dahandle, pvallength);
}

char* k2h_da_read_str(k2h_da_h dahandle)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return NULL;
	}
	char*	pval = NULL;
	if(!pAccess->Read(&pval)){
		K2H_Free(pval);
		return NULL;
	}
	return pval;
}

bool k2h_da_set_value(k2h_da_h dahandle, const unsigned char* pval, size_t vallength)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return false;
	}
	if(!pval || 0UL == vallength){
		ERR_K2HPRN("Invalid parameters.");
		return false;
	}
	return pAccess->Write(pval, vallength);
}

bool k2h_da_set_value_offset(k2h_da_h dahandle, const unsigned char* pval, size_t vallength, off_t offset)
{
	if(!k2h_da_set_write_offset(dahandle, offset)){
		return false;
	}
	return k2h_da_set_value(dahandle, pval, vallength);
}

bool k2h_da_set_value_from_file(k2h_da_h dahandle, int fd, size_t* pvallength)
{
	K2HDAccess*	pAccess = reinterpret_cast<K2HDAccess*>(dahandle);
	if(!pAccess){
		ERR_K2HPRN("Invalid k2h_da_h handle.");
		return false;
	}
	if(0 > fd || !pvallength || 0UL == *pvallength){
		ERR_K2HPRN("Invalid parameters.");
		return false;
	}
	return pAccess->Write(fd, (*pvallength));
}

bool k2h_da_set_value_str(k2h_da_h dahandle, const char* pval)
{
	return k2h_da_set_value(dahandle, reinterpret_cast<const unsigned char*>(pval), pval ? strlen(pval) + 1 : 0UL);
}

//---------------------------------------------------------
// Functions : Queue
//---------------------------------------------------------
k2h_q_h k2h_q_handle(k2h_h handle, bool is_fifo)
{
	return k2h_q_handle_prefix(handle, is_fifo, NULL, 0);
}

k2h_q_h k2h_q_handle_prefix(k2h_h handle, bool is_fifo, const unsigned char* pref, size_t preflen)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return K2H_INVALID_HANDLE;
	}

	K2HQueue*	pQueue = new K2HQueue;
	if(!pQueue->Init(pShm, is_fifo, pref, preflen)){
		ERR_K2HPRN("Could not initialize internal K2HQueue object.");
		K2H_Delete(pQueue);
		return K2H_INVALID_HANDLE;
	}
	return reinterpret_cast<k2h_q_h>(pQueue);
}

k2h_q_h k2h_q_handle_str_prefix(k2h_h handle, bool is_fifo, const char* pref)
{
	const unsigned char*	bypref = reinterpret_cast<const unsigned char*>(pref);
	size_t					preflen= pref ? strlen(pref) : 0;						// do not count end of nil
	if(ISEMPTYSTR(pref)){
		bypref = NULL;
		preflen= 0;
	}
	return k2h_q_handle_prefix(handle, is_fifo, bypref, preflen);
}

bool k2h_q_free(k2h_q_h qhandle)
{
	K2HQueue*	pQueue = reinterpret_cast<K2HQueue*>(qhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_q_h handle.");
		return false;
	}
	K2H_Delete(pQueue);
	return true;
}

bool k2h_q_empty(k2h_q_h qhandle)
{
	K2HQueue*	pQueue = reinterpret_cast<K2HQueue*>(qhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_q_h handle.");
		return false;
	}
	return pQueue->IsEmpty();
}

int k2h_q_count(k2h_q_h qhandle)
{
	K2HQueue*	pQueue = reinterpret_cast<K2HQueue*>(qhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_q_h handle.");
		return 0;
	}
	return pQueue->GetCount();
}

bool k2h_q_read(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, int pos)
{
	return k2h_q_read_wp(qhandle, ppdata, pdatalen, pos, NULL);
}

bool k2h_q_str_read(k2h_q_h qhandle, char** ppdata, int pos)
{
	return k2h_q_str_read_wp(qhandle, ppdata, pos, NULL);
}

bool k2h_q_push(k2h_q_h qhandle, const unsigned char* bydata, size_t datalen)
{
	return k2h_q_push_wa(qhandle, bydata, datalen, NULL, 0, NULL, NULL);
}

bool k2h_q_str_push(k2h_q_h qhandle, const char* pdata)
{
	return k2h_q_str_push_wa(qhandle, pdata, NULL, 0, NULL, NULL);
}

bool k2h_q_pop(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen)
{
	return k2h_q_pop_wp(qhandle, ppdata, pdatalen, NULL);
}

bool k2h_q_str_pop(k2h_q_h qhandle, char** ppdata)
{
	return k2h_q_str_pop_wp(qhandle, ppdata, NULL);
}

bool k2h_q_remove(k2h_q_h qhandle, int count)
{
	return k2h_q_remove_wp(qhandle, count, NULL);
}

int k2h_q_remove_ext(k2h_q_h qhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata)
{
	return k2h_q_remove_wp_ext(qhandle, count, fp, pextdata, NULL);
}

bool k2h_q_read_wp(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, int pos, const char* encpass)
{
	if(!ppdata || !pdatalen || pos < -1){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HQueue*	pQueue = reinterpret_cast<K2HQueue*>(qhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_q_h handle.");
		return false;
	}
	if(!pQueue->Read(ppdata, (*pdatalen), pos, encpass)){
		ERR_K2HPRN("Could not read to queue.");
		return false;
	}
	return true;
}

bool k2h_q_str_read_wp(k2h_q_h qhandle, char** ppdata, int pos, const char* encpass)
{
	size_t	tmplen = 0;
	return k2h_q_read_wp(qhandle, reinterpret_cast<unsigned char**>(ppdata), &tmplen, pos, encpass);
}

bool k2h_q_push_wa(k2h_q_h qhandle, const unsigned char* bydata, size_t datalen, const PK2HATTRPCK pattrspck, int attrspckcnt, const char* encpass, const time_t* expire)
{
	K2HQueue*	pQueue = reinterpret_cast<K2HQueue*>(qhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_q_h handle.");
		return false;
	}

	K2HAttrs*	pAttr = k2h_cvt_bin_to_attrs(pattrspck, attrspckcnt);

	if(!pQueue->Push(bydata, datalen, pAttr, encpass, expire)){
		ERR_K2HPRN("Could not push to queue.");
		K2H_Delete(pAttr);
		return false;
	}
	K2H_Delete(pAttr);

	return true;
}

bool k2h_q_str_push_wa(k2h_q_h qhandle, const char* pdata, const PK2HATTRPCK pattrspck, int attrspckcnt, const char* encpass, const time_t* expire)
{
	return k2h_q_push_wa(qhandle, reinterpret_cast<const unsigned char*>(pdata), (pdata ? strlen(pdata) + 1 : 0), pattrspck, attrspckcnt, encpass, expire);
}

bool k2h_q_pop_wa(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, PK2HATTRPCK* ppattrspck, int* pattrspckcnt, const char* encpass)
{
	if(!ppdata || !pdatalen){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HQueue*	pQueue = reinterpret_cast<K2HQueue*>(qhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_q_h handle.");
		return false;
	}
	K2HAttrs*	pAttrs = NULL;
	if(!pQueue->Pop(ppdata, (*pdatalen), &pAttrs, encpass)){
		ERR_K2HPRN("Could not pop to queue.");
		return false;
	}
	if(ppattrspck && pattrspckcnt){
		if(pAttrs){
			*ppattrspck = k2h_cvt_attrs_to_bin(pAttrs, *pattrspckcnt);
		}else{
			*ppattrspck		= NULL;
			*pattrspckcnt	= 0;
		}
	}
	K2H_Delete(pAttrs);

	return true;
}

bool k2h_q_str_pop_wa(k2h_q_h qhandle, char** ppdata, PK2HATTRPCK* ppattrspck, int* pattrspckcnt, const char* encpass)
{
	size_t	tmplen = 0;
	return k2h_q_pop_wa(qhandle, reinterpret_cast<unsigned char**>(ppdata), &tmplen, ppattrspck, pattrspckcnt, encpass);
}

bool k2h_q_pop_wp(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, const char* encpass)
{
	return k2h_q_pop_wa(qhandle, ppdata, pdatalen, NULL, NULL, encpass);
}

bool k2h_q_str_pop_wp(k2h_q_h qhandle, char** ppdata, const char* encpass)
{
	size_t	tmplen = 0;
	return k2h_q_pop_wp(qhandle, reinterpret_cast<unsigned char**>(ppdata), &tmplen, encpass);
}

bool k2h_q_remove_wp(k2h_q_h qhandle, int count, const char* encpass)
{
	K2HQueue*	pQueue = reinterpret_cast<K2HQueue*>(qhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_q_h handle.");
		return false;
	}
	if(-1 == pQueue->Remove(count, NULL, NULL, encpass)){
		ERR_K2HPRN("Could not pop to queue.");
		return false;
	}
	return true;
}

int k2h_q_remove_wp_ext(k2h_q_h qhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata, const char* encpass)
{
	K2HQueue*	pQueue = reinterpret_cast<K2HQueue*>(qhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_q_h handle.");
		return false;
	}
	return pQueue->Remove(count, fp, pextdata, encpass);
}

bool k2h_q_dump(k2h_q_h qhandle, FILE* stream)
{
	K2HQueue*	pQueue = reinterpret_cast<K2HQueue*>(qhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_q_h handle.");
		return false;
	}
	return pQueue->Dump(stream ? stream : stdout);
}

k2h_keyq_h k2h_keyq_handle(k2h_h handle, bool is_fifo)
{
	return k2h_keyq_handle_prefix(handle, is_fifo, NULL, 0);
}

k2h_keyq_h k2h_keyq_handle_prefix(k2h_h handle, bool is_fifo, const unsigned char* pref, size_t preflen)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return K2H_INVALID_HANDLE;
	}

	K2HKeyQueue*	pQueue = new K2HKeyQueue;
	if(!pQueue->Init(pShm, is_fifo, pref, preflen)){
		ERR_K2HPRN("Could not initialize internal K2HKeyQueue object.");
		K2H_Delete(pQueue);
		return K2H_INVALID_HANDLE;
	}
	return reinterpret_cast<k2h_keyq_h>(pQueue);
}

k2h_keyq_h k2h_keyq_handle_str_prefix(k2h_h handle, bool is_fifo, const char* pref)
{
	const unsigned char*	bypref = reinterpret_cast<const unsigned char*>(pref);
	size_t					preflen= pref ? strlen(pref) : 0;						// do not count end of nil
	if(ISEMPTYSTR(pref)){
		bypref = NULL;
		preflen= 0;
	}
	return k2h_keyq_handle_prefix(handle, is_fifo, bypref, preflen);
}

bool k2h_keyq_free(k2h_keyq_h keyqhandle)
{
	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	K2H_Delete(pQueue);
	return true;
}

bool k2h_keyq_empty(k2h_keyq_h keyqhandle)
{
	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	return pQueue->IsEmpty();
}

int k2h_keyq_count(k2h_keyq_h keyqhandle)
{
	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return 0;
	}
	return pQueue->GetCount();
}

bool k2h_keyq_read(k2h_keyq_h keyqhandle, unsigned char** ppdata, size_t* pdatalen, int pos)
{
	return k2h_keyq_read_wp(keyqhandle, ppdata, pdatalen, pos, NULL);
}

bool k2h_keyq_read_keyval(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen, int pos)
{
	return k2h_keyq_read_keyval_wp(keyqhandle, ppkey, pkeylen, ppval, pvallen, pos, NULL);
}

bool k2h_keyq_str_read(k2h_keyq_h keyqhandle, char** ppdata, int pos)
{
	return k2h_keyq_str_read_wp(keyqhandle, ppdata, pos, NULL);
}

bool k2h_keyq_str_read_keyval(k2h_keyq_h keyqhandle, char** ppkey, char** ppval, int pos)
{
	return k2h_keyq_str_read_keyval_wp(keyqhandle, ppkey, ppval, pos, NULL);
}

bool k2h_keyq_push(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen)
{
	return k2h_keyq_push_wa(keyqhandle, bykey, keylen, NULL, NULL);
}

bool k2h_keyq_push_keyval(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen, const unsigned char* byval, size_t vallen)
{
	return k2h_keyq_push_keyval_wa(keyqhandle, bykey, keylen, byval, vallen, NULL, NULL);
}

bool k2h_keyq_str_push(k2h_keyq_h keyqhandle, const char* pkey)
{
	return k2h_keyq_push(keyqhandle, reinterpret_cast<const unsigned char*>(pkey), (pkey ? strlen(pkey) + 1 : 0));
}

bool k2h_keyq_str_push_keyval(k2h_keyq_h keyqhandle, const char* pkey, const char* pval)
{
	return k2h_keyq_push_keyval(keyqhandle, reinterpret_cast<const unsigned char*>(pkey), (pkey ? strlen(pkey) + 1 : 0), reinterpret_cast<const unsigned char*>(pval), (pval ? strlen(pval) + 1 : 0));
}

bool k2h_keyq_pop(k2h_keyq_h keyqhandle, unsigned char** ppval, size_t* pvallen)
{
	return k2h_keyq_pop_wp(keyqhandle, ppval, pvallen, NULL);
}

bool k2h_keyq_pop_keyval(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen)
{
	return k2h_keyq_pop_keyval_wp(keyqhandle, ppkey, pkeylen, ppval, pvallen, NULL);
}

bool k2h_keyq_str_pop(k2h_keyq_h keyqhandle, char** ppval)
{
	return k2h_keyq_str_pop_wp(keyqhandle, ppval, NULL);
}

bool k2h_keyq_str_pop_keyval(k2h_keyq_h keyqhandle, char** ppkey, char** ppval)
{
	return k2h_keyq_str_pop_keyval_wp(keyqhandle, ppkey, ppval, NULL);
}

bool k2h_keyq_remove(k2h_keyq_h keyqhandle, int count)
{
	return k2h_keyq_remove_wp(keyqhandle, count, NULL);
}

int k2h_keyq_remove_ext(k2h_keyq_h keyqhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata)
{
	return k2h_keyq_remove_wp_ext(keyqhandle, count, fp, pextdata, NULL);
}

bool k2h_keyq_read_wp(k2h_keyq_h keyqhandle, unsigned char** ppdata, size_t* pdatalen, int pos, const char* encpass)
{
	if(!ppdata || !pdatalen || pos < -1){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	if(!pQueue->Read(ppdata, (*pdatalen), pos, encpass)){
		ERR_K2HPRN("Could not read to queue.");
		return false;
	}
	return true;
}

bool k2h_keyq_read_keyval_wp(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen, int pos, const char* encpass)
{
	if(!ppkey || !pkeylen || !ppval || !pvallen || pos < -1){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	if(!pQueue->Read(ppkey, (*pkeylen), ppval, (*pvallen), pos, encpass)){
		ERR_K2HPRN("Could not read to queue.");
		return false;
	}
	return true;
}

bool k2h_keyq_str_read_wp(k2h_keyq_h keyqhandle, char** ppdata, int pos, const char* encpass)
{
	size_t	tmplen = 0;
	return k2h_keyq_read_wp(keyqhandle, reinterpret_cast<unsigned char**>(ppdata), &tmplen, pos, encpass);
}

bool k2h_keyq_str_read_keyval_wp(k2h_keyq_h keyqhandle, char** ppkey, char** ppval, int pos, const char* encpass)
{
	size_t	keylen = 0;
	size_t	vallen = 0;
	return k2h_keyq_read_keyval_wp(keyqhandle, reinterpret_cast<unsigned char**>(ppkey), &keylen, reinterpret_cast<unsigned char**>(ppval), &vallen, pos, encpass);
}

bool k2h_keyq_push_wa(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen, const char* encpass, const time_t* expire)
{
	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	if(!pQueue->Push(bykey, keylen, NULL, encpass, expire)){
		ERR_K2HPRN("Could not push to queue.");
		return false;
	}
	return true;
}

bool k2h_keyq_push_keyval_wa(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen, const unsigned char* byval, size_t vallen, const char* encpass, const time_t* expire)
{
	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	if(!pQueue->Push(bykey, keylen, byval, vallen, encpass, expire)){
		ERR_K2HPRN("Could not push to queue.");
		return false;
	}
	return true;
}

bool k2h_keyq_str_push_wa(k2h_keyq_h keyqhandle, const char* pkey, const char* encpass, const time_t* expire)
{
	return k2h_keyq_push_wa(keyqhandle, reinterpret_cast<const unsigned char*>(pkey), (pkey ? strlen(pkey) + 1 : 0), encpass, expire);
}

bool k2h_keyq_str_push_keyval_wa(k2h_keyq_h keyqhandle, const char* pkey, const char* pval, const char* encpass, const time_t* expire)
{
	return k2h_keyq_push_keyval_wa(keyqhandle, reinterpret_cast<const unsigned char*>(pkey), (pkey ? strlen(pkey) + 1 : 0), reinterpret_cast<const unsigned char*>(pval), (pval ? strlen(pval) + 1 : 0), encpass, expire);
}

bool k2h_keyq_pop_wp(k2h_keyq_h keyqhandle, unsigned char** ppval, size_t* pvallen, const char* encpass)
{
	if(!ppval || !pvallen){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	if(!pQueue->Pop(ppval, (*pvallen), NULL, encpass)){
		ERR_K2HPRN("Could not pop to queue.");
		return false;
	}
	return true;
}

bool k2h_keyq_pop_keyval_wp(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen, const char* encpass)
{
	if(!ppkey || !pkeylen || !ppval || !pvallen){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	if(!pQueue->Pop(ppkey, (*pkeylen), ppval, (*pvallen), encpass)){
		ERR_K2HPRN("Could not pop to queue.");
		return false;
	}
	return true;
}

bool k2h_keyq_str_pop_wp(k2h_keyq_h keyqhandle, char** ppval, const char* encpass)
{
	size_t	tmplen = 0;
	return k2h_keyq_pop_wp(keyqhandle, reinterpret_cast<unsigned char**>(ppval), &tmplen, encpass);
}

bool k2h_keyq_str_pop_keyval_wp(k2h_keyq_h keyqhandle, char** ppkey, char** ppval, const char* encpass)
{
	size_t	keylen = 0;
	size_t	vallen = 0;
	return k2h_keyq_pop_keyval_wp(keyqhandle, reinterpret_cast<unsigned char**>(ppkey), &keylen, reinterpret_cast<unsigned char**>(ppval), &vallen, encpass);
}

bool k2h_keyq_remove_wp(k2h_keyq_h keyqhandle, int count, const char* encpass)
{
	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	if(-1 == pQueue->Remove(count, NULL, NULL, encpass)){
		ERR_K2HPRN("Could not pop to queue.");
		return false;
	}
	return true;
}

int k2h_keyq_remove_wp_ext(k2h_keyq_h keyqhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata, const char* encpass)
{
	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	return pQueue->Remove(count, fp, pextdata, encpass);
}

bool k2h_keyq_dump(k2h_keyq_h keyqhandle, FILE* stream)
{
	K2HKeyQueue*	pQueue = reinterpret_cast<K2HKeyQueue*>(keyqhandle);
	if(!pQueue){
		ERR_K2HPRN("Invalid k2h_keyq_h handle.");
		return false;
	}
	return pQueue->Dump(stream ? stream : stdout);
}

//---------------------------------------------------------
// Functions : Print / Dump
//---------------------------------------------------------
static bool k2h_dump_ext(k2h_h handle, FILE* stream, int mode)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->Dump(stream ? stream : stdout, mode)){
		ERR_K2HPRN("Could not dump k2hash file(memory).");
		return false;
	}
	return true;
}

bool k2h_dump_head(k2h_h handle, FILE* stream)
{
	return k2h_dump_ext(handle, stream, K2HShm::DUMP_HEAD);
}

bool k2h_dump_keytable(k2h_h handle, FILE* stream)
{
	return k2h_dump_ext(handle, stream, K2HShm::DUMP_KINDEX_ARRAY);
}

bool k2h_dump_full_keytable(k2h_h handle, FILE* stream)
{
	return k2h_dump_ext(handle, stream, K2HShm::DUMP_CKINDEX_ARRAY);
}

bool k2h_dump_elementtable(k2h_h handle, FILE* stream)
{
	return k2h_dump_ext(handle, stream, K2HShm::DUMP_ELEMENT_LIST);
}

bool k2h_dump_full(k2h_h handle, FILE* stream)
{
	return k2h_dump_ext(handle, stream, K2HShm::DUMP_PAGE_LIST);
}

bool k2h_print_state(k2h_h handle, FILE* stream)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return false;
	}
	if(!pShm->PrintState(stream ? stream : stdout)){
		ERR_K2HPRN("Could not print state for k2hash file(memory).");
		return false;
	}
	return true;
}

extern char k2h_commit_hash[];

void k2h_print_version(FILE* stream)
{
	static const char format[] =
		"\n"
		"K2HASH library Version %s (commit: %s) with %s\n"
		"\n"
		"Copyright(C) 2013 Yahoo Japan Corporation.\n"
		"\n"
		"K2HASH is key-valuew store base libraries. K2HASH is made for\n"
		"the purpose of the construction of original KVS system and the\n"
		"offer of the library. The characteristic is this KVS library\n"
		"which Key can layer. And can support multi-processing and\n"
		"multi-thread, and is provided safely as available KVS.\n"
		"\n";

	if(!stream){
		stream = stdout;
	}
	fprintf(stream, format, VERSION, k2h_commit_hash, k2h_crypt_lib_name());

	fullock_print_version(stream);
}

PK2HSTATE k2h_get_state(k2h_h handle)
{
	K2HShm*	pShm = reinterpret_cast<K2HShm*>(handle);
	if(!pShm){
		ERR_K2HPRN("Invalid k2hash handle.");
		return NULL;
	}
	return pShm->GetState();
}

//---------------------------------------------------------
// Functions : Utility
//---------------------------------------------------------
void free_k2hbin(PK2HBIN pk2hbin)
{
	if(pk2hbin){
		K2H_Free(pk2hbin->byptr);
	}
	// cppcheck-suppress uselessAssignmentPtrArg
	K2H_Free(pk2hbin);
}

void free_k2hbins(PK2HBIN pk2hbin, size_t count)
{
	for(size_t cnt = 0; pk2hbin && cnt < count; ++cnt){
		K2H_Free(pk2hbin[cnt].byptr);
	}
	// cppcheck-suppress uselessAssignmentPtrArg
	K2H_Free(pk2hbin);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
