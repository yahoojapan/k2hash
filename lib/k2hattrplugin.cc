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
 * CREATE:   Mon Dec 21 2015
 * REVISION:
 *
 */

#include <time.h>
#include <dlfcn.h>

#include "k2hcommon.h"
#include "k2hattrplugin.h"
#include "k2hattrs.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// K2hAttrPluginLib Methods
//---------------------------------------------------------
K2hAttrPluginLib::K2hAttrPluginLib(void) : hDynLib(NULL), fp_k2hattr_initialize(NULL), fp_k2hattr_get_version(NULL), fp_k2hattr_get_key_name(NULL), fp_k2hattr_update(NULL), verinfo(""), attrkey("")
{
}

K2hAttrPluginLib::~K2hAttrPluginLib(void)
{
	Unload();
}

bool K2hAttrPluginLib::Unload(void)
{
	if(hDynLib){
		dlclose(hDynLib);
		hDynLib = NULL;
	}
	fp_k2hattr_initialize	= NULL;
	fp_k2hattr_get_version	= NULL;
	fp_k2hattr_get_key_name	= NULL;
	fp_k2hattr_update		= NULL;

	verinfo.clear();
	attrkey.clear();

	return true;
}

bool K2hAttrPluginLib::Load(const char* path)
{
	Unload();

	if(ISEMPTYSTR(path)){
		ERR_K2HPRN("plugin library path is empty.");
		return false;
	}

	// try to load
	if(NULL == (hDynLib = dlopen(path, RTLD_LAZY))){
		const char*	pError = dlerror();
		ERR_K2HPRN("Failed to load library(%s), error = %s", path, pError ? pError : "unknown");
		return false;
	}

	// load function pointer
	if(	NULL == (fp_k2hattr_initialize	= reinterpret_cast<Tfp_k2hattr_initialize>(dlsym(hDynLib,	"k2hattr_initialize")))		||
		NULL == (fp_k2hattr_get_version	= reinterpret_cast<Tfp_k2hattr_get_version>(dlsym(hDynLib,	"k2hattr_get_version")))	||
		NULL == (fp_k2hattr_get_key_name= reinterpret_cast<Tfp_k2hattr_get_key_name>(dlsym(hDynLib,	"k2hattr_get_key_name")))	||
		NULL == (fp_k2hattr_update		= reinterpret_cast<Tfp_k2hattr_update>(dlsym(hDynLib,		"k2hattr_update")))			)
	{
		const char*	pError = dlerror();
		ERR_K2HPRN("Failed to load library(%s), error = %s", path, pError ? pError : "unknown");
		Unload();
		return false;
	}

	// initialize
	if(!fp_k2hattr_initialize()){
		ERR_K2HPRN("Got error from k2hattr_initialize function in library(%s)", path);
		Unload();
		return false;
	}

	// get key name
	const char*	ptmp;
	if(NULL == (ptmp = fp_k2hattr_get_key_name())){
		ERR_K2HPRN("Got nothing from k2hattr_get_key_name function in library(%s).", path);
		Unload();
		return false;
	}
	attrkey = ptmp;

	// get version information
	if(NULL == (ptmp = fp_k2hattr_get_version())){
		WAN_K2HPRN("Got nothing from k2hattr_get_version function in library(%s), so file path is set to version information.", path);
		verinfo = "unknown version(attribute plugin - ";
		verinfo += path;
		verinfo += ")";
	}else{
		verinfo = ptmp;
	}

	MSG_K2HPRN("Success loading library(%s) - %s", path, verinfo.c_str());

	return true;
}

//---------------------------------------------------------
// K2hAttrPlugin Class Variable
//---------------------------------------------------------
const int			K2hAttrPlugin::TYPE_ATTRPLUGIN;

//---------------------------------------------------------
// K2hAttrPlugin Methods
//---------------------------------------------------------
K2hAttrPlugin::K2hAttrPlugin(const K2hAttrPluginLib* pLib) : K2hAttrOpsBase(), fp_k2hattr_initialize(NULL), fp_k2hattr_get_version(NULL), fp_k2hattr_get_key_name(NULL), fp_k2hattr_update(NULL), attrkey("")
{
	if(pLib && pLib->hDynLib){
		fp_k2hattr_initialize	= pLib->fp_k2hattr_initialize;
		fp_k2hattr_get_version	= pLib->fp_k2hattr_get_version;
		fp_k2hattr_get_key_name	= pLib->fp_k2hattr_get_key_name;
		fp_k2hattr_update		= pLib->fp_k2hattr_update;
		VerInfo					= pLib->verinfo;
		attrkey					= pLib->attrkey;
	}
}

K2hAttrPlugin::~K2hAttrPlugin(void)
{
}

bool K2hAttrPlugin::IsHandleAttr(const unsigned char* key, size_t keylen) const
{
	if(!key || 0 == keylen){
		MSG_K2HPRN("target key name for attribute is empty.");
		return false;
	}
	if((attrkey.length() + 1) == keylen && 0 == memcmp(attrkey.c_str(), key, keylen)){
		return true;
	}
	return false;
}

bool K2hAttrPlugin::UpdateAttr(K2HAttrs& attrs)
{
	// update
	unsigned char*	attrval;
	size_t			attrvallen = 0;
	if(NULL == (attrval = fp_k2hattr_update(&attrvallen, byKey, KeyLen, byValue, ValLen))){
		MSG_K2HPRN("Got no value for attribute(%s), so remove it.", attrkey.c_str());
		RemoveAttr(attrs, attrkey.c_str());
	}else{
		if(!SetAttr(attrs, attrkey.c_str(), attrval, attrvallen)){
			ERR_K2HPRN("Could not set %s attribute.", attrkey.c_str());
			K2H_Free(attrval);
			return false;
		}
		K2H_Free(attrval);
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
