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
 * CREATE:   Mon Dec 21 2015
 * REVISION:
 *
 */

#ifndef	K2ATTRFUNC_H
#define	K2ATTRFUNC_H

#include "k2hash.h"
#include "k2hcommon.h"

//---------------------------------------------------------
// Readme about attribute functions by plugin library
//---------------------------------------------------------
// K2Hash library can load plugin library for k2hash attribute.
// The plugin library must have following function.
// 
//	1) k2hattr_initialize
//		This function is called when k2hash library initializes
//		plugin after the plugin library. This gives the timing 
//		of the initialization of the internal data to the plugin.
//
//	2) k2hattr_get_version
//		returns the plugin library version and any plugin
//		information.
//
//	3) k2hattr_get_key_name
//		returns the key name of attribute for plugin. The key
//		name must not be changed on one plugin, k2hash calls 
//		this function just onece.
//
//	4) k2hattr_update
//		returns the value and value length of attribute when 
//		this function is called. This function is called with 
//		key and value for target k2hash data.
//

//---------------------------------------------------------
// Prototype function
//---------------------------------------------------------
DECL_EXTERN_C_START		// extern "C" - start

typedef bool (*Tfp_k2hattr_initialize)(void);
typedef const char* (*Tfp_k2hattr_get_version)(void);
typedef const char* (*Tfp_k2hattr_get_key_name)(void);
typedef unsigned char* (*Tfp_k2hattr_update)(size_t* psize, const unsigned char* key, size_t keylen, const unsigned char* value, size_t vallen);

DECL_EXTERN_C_END		// extern "C" - end

#endif	// K2ATTRFUNC_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
