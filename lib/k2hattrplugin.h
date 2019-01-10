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

#ifndef	K2HATTRPLUGIN_H
#define	K2HATTRPLUGIN_H

#include <map>
#include <vector>
#include <string>

#include "k2hattrop.h"
#include "k2hattrfunc.h"

class K2hAttrPlugin;

//---------------------------------------------------------
// Class K2hAttrPluginLib
//---------------------------------------------------------
class K2hAttrPluginLib
{
	friend class K2hAttrPlugin;

	protected:
		void*						hDynLib;
		Tfp_k2hattr_initialize		fp_k2hattr_initialize;
		Tfp_k2hattr_get_version		fp_k2hattr_get_version;
		Tfp_k2hattr_get_key_name	fp_k2hattr_get_key_name;
		Tfp_k2hattr_update			fp_k2hattr_update;

		std::string					verinfo;
		std::string					attrkey;

	protected:
		bool Unload(void);

	public:
		K2hAttrPluginLib(void);
		virtual ~K2hAttrPluginLib(void);

		bool Load(const char* path);

		const char* GetVersionInfo(void) const { return verinfo.c_str(); }

		bool compare(const K2hAttrPluginLib& other) const { return (hDynLib && (hDynLib == other.hDynLib)); }
		bool operator==(const K2hAttrPluginLib& other) const { return compare(other); }
};

typedef std::vector<K2hAttrPluginLib*>				k2hattrliblist_t;
typedef std::map<const K2HShm*, k2hattrliblist_t*>	k2hattrlibmap_t;

//---------------------------------------------------------
// Class K2hAttrPlugin
//---------------------------------------------------------
class K2hAttrPlugin : public K2hAttrOpsBase
{
	protected:
		Tfp_k2hattr_initialize		fp_k2hattr_initialize;
		Tfp_k2hattr_get_version		fp_k2hattr_get_version;
		Tfp_k2hattr_get_key_name	fp_k2hattr_get_key_name;
		Tfp_k2hattr_update			fp_k2hattr_update;

		std::string					attrkey;

	public:
		static const int			TYPE_ATTRPLUGIN = 2;

	public:
		explicit K2hAttrPlugin(const K2hAttrPluginLib* pLib = NULL);
		virtual ~K2hAttrPlugin(void);

		virtual int GetType(void) const { return TYPE_ATTRPLUGIN; }
		virtual bool IsHandleAttr(const unsigned char* key, size_t keylen) const;
		virtual bool UpdateAttr(K2HAttrs& attrs);
};

#endif	// K2HATTRPLUGIN_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
