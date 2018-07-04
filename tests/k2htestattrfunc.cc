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
 * CREATE:   Tue Dec 22 2015
 * REVISION:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <iostream>

#include <k2hattrfunc.h>

using namespace std;

//---------------------------------------------------------
// Prototypes
//---------------------------------------------------------
extern "C" {

bool k2hattr_initialize(void);
const char* k2hattr_get_version(void);
const char* k2hattr_get_key_name(void);
unsigned char* k2hattr_update(size_t* psize, const unsigned char* key, size_t keylen, const unsigned char* value, size_t vallen);

}

//---------------------------------------------------------
// Attribute plugin functions
//---------------------------------------------------------
static const char	szVersion[] = "DSO TEST ATTR PLUGIN V1.0";
static const char	szAttrKey[] = "test_attr_plugin_uid";
static char			szUid[256]	= "";

bool k2hattr_initialize(void)
{
	uid_t	uid = getuid();
	sprintf(szUid, "%d", uid);
	return true;
}

const char* k2hattr_get_version(void)
{
	return szVersion;
}

const char* k2hattr_get_key_name(void)
{
	return szAttrKey;
}

unsigned char* k2hattr_update(size_t* psize, const unsigned char* key, size_t keylen, const unsigned char* value, size_t vallen)
{
	if(!key || 0 == keylen || !value || 0 == vallen){
		cerr << "ERROR(" << szVersion << ") : key or value is empty." << endl;
		if(psize){
			*psize = 0;
		}
		return NULL;
	}
	char*	pvalue = strdup(szUid);
	if(psize){
		*psize = strlen(pvalue) + 1;
	}
	return reinterpret_cast<unsigned char*>(pvalue);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
