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
 * CREATE:   Mon Feb 10 2014
 * REVISION:
 *
 */
#include <string.h>

#include "k2hcommand.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Constructor / Destructor
//---------------------------------------------------------
K2HCommandArchive::K2HCommandArchive() : pBinCom(NULL)
{
}

K2HCommandArchive::~K2HCommandArchive()
{
	K2H_Free(pBinCom);
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
bool K2HCommandArchive::SetAll(const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength, const unsigned char* bySKey, size_t skeylength, const unsigned char* byAttrs, size_t attrlength)
{
	return Put(SCOM_SET_ALL, byKey, keylength, byVal, vallength, bySKey, skeylength, byAttrs, attrlength, NULL, 0UL);
}

bool K2HCommandArchive::ReplaceVal(const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength)
{
	return Put(SCOM_REPLACE_VAL, byKey, keylength, byVal, vallength, NULL, 0UL, NULL, 0UL, NULL, 0UL);
}

bool K2HCommandArchive::ReplaceSKey(const unsigned char* byKey, size_t keylength, const unsigned char* bySKey, size_t skeylength)
{
	return Put(SCOM_REPLACE_SKEY, byKey, keylength, NULL, 0UL, bySKey, skeylength, NULL, 0UL, NULL, 0UL);
}

bool K2HCommandArchive::ReplaceAttrs(const unsigned char* byKey, size_t keylength, const unsigned char* byAttrs, size_t attrlength)
{
	return Put(SCOM_REPLACE_ATTRS, byKey, keylength, NULL, 0UL, NULL, 0UL, byAttrs, attrlength, NULL, 0UL);
}

bool K2HCommandArchive::DelKey(const unsigned char* byKey, size_t keylength)
{
	return Put(SCOM_DEL_KEY, byKey, keylength, NULL, 0UL, NULL, 0UL, NULL, 0UL, NULL, 0UL);
}

bool K2HCommandArchive::Put(long type, const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength, const unsigned char* bySKey, size_t skeylength, const unsigned char* byAttrs, size_t attrlength, const unsigned char* byExdata, size_t exdatalength)
{
	if(!byKey || 0L == keylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	// length
	size_t	total = sizeof(SCOM) + keylength;
	if(SCOM_SET_ALL == type){
		total += vallength;
		total += skeylength;
		total += attrlength;
	}else if(SCOM_REPLACE_VAL == type){
		total += vallength;
	}else if(SCOM_REPLACE_SKEY == type){
		total += skeylength;
	}else if(SCOM_REPLACE_ATTRS == type){
		total += attrlength;
	}else if(SCOM_DEL_KEY == type){
		// nothing
	}else if(SCOM_OW_VAL == type){
		total += vallength;
		total += exdatalength;
	}else if(SCOM_RENAME == type){
		total += attrlength;
		total += exdatalength;
	}else{
		ERR_K2HPRN("Parameter type(%ld) are wrong.", type);
		return false;
	}

	K2H_Free(pBinCom);

	// allocation
	if(NULL == (pBinCom = static_cast<PBCOM>(calloc(total, sizeof(unsigned char))))){
		ERR_K2HPRN("Could not allocation memory for %zu bytes.", total);
		return false;
	}
	scom_init(pBinCom->scom);

	// set
	pBinCom->scom.type 			= type;
	pBinCom->scom.key_length	= keylength;
	pBinCom->scom.key_pos		= static_cast<off_t>(sizeof(struct serialize_command));
	memcpy(&(pBinCom->byData[pBinCom->scom.key_pos]), byKey, keylength);

	if(SCOM_SET_ALL == type){
		strcpy(pBinCom->scom.szCommand, SCOM_COMSTR_SET_ALL);

		pBinCom->scom.val_length	= vallength;
		pBinCom->scom.val_pos		= pBinCom->scom.key_pos + pBinCom->scom.key_length;
		if(0UL < vallength){
			memcpy(&(pBinCom->byData[pBinCom->scom.val_pos]), byVal, vallength);
		}

		pBinCom->scom.skey_length	= skeylength;
		pBinCom->scom.skey_pos		= pBinCom->scom.val_pos + pBinCom->scom.val_length;
		if(0UL < skeylength){
			memcpy(&(pBinCom->byData[pBinCom->scom.skey_pos]), bySKey, skeylength);
		}

		pBinCom->scom.attr_length	= attrlength;
		pBinCom->scom.attrs_pos		= pBinCom->scom.skey_pos + pBinCom->scom.skey_length;
		if(0UL < attrlength){
			memcpy(&(pBinCom->byData[pBinCom->scom.attrs_pos]), byAttrs, attrlength);
		}

	}else if(SCOM_REPLACE_VAL == type){
		strcpy(pBinCom->scom.szCommand, SCOM_COMSTR_REPLACE_VAL);

		pBinCom->scom.val_length	= vallength;
		pBinCom->scom.val_pos		= pBinCom->scom.key_pos + pBinCom->scom.key_length;
		if(0UL < vallength){
			memcpy(&(pBinCom->byData[pBinCom->scom.val_pos]), byVal, vallength);
		}

	}else if(SCOM_REPLACE_SKEY == type){
		strcpy(pBinCom->scom.szCommand, SCOM_COMSTR_REPLACE_SKEY);

		pBinCom->scom.skey_length	= skeylength;
		pBinCom->scom.skey_pos		= pBinCom->scom.key_pos + pBinCom->scom.key_length;
		if(0UL < skeylength){
			memcpy(&(pBinCom->byData[pBinCom->scom.skey_pos]), bySKey, skeylength);
		}

	}else if(SCOM_REPLACE_ATTRS == type){
		strcpy(pBinCom->scom.szCommand, SCOM_COMSTR_REPLACE_ATTRS);

		pBinCom->scom.attr_length	= attrlength;
		pBinCom->scom.attrs_pos		= pBinCom->scom.key_pos + pBinCom->scom.key_length;
		if(0UL < attrlength){
			memcpy(&(pBinCom->byData[pBinCom->scom.attrs_pos]), byAttrs, attrlength);
		}

	}else if(SCOM_DEL_KEY == type){
		strcpy(pBinCom->scom.szCommand, SCOM_COMSTR_DEL_KEY);

	}else if(SCOM_OW_VAL == type){
		strcpy(pBinCom->scom.szCommand, SCOM_COMSTR_OW_VAL);

		pBinCom->scom.val_length	= vallength;
		pBinCom->scom.val_pos		= pBinCom->scom.key_pos + pBinCom->scom.key_length;
		if(0UL < vallength){
			memcpy(&(pBinCom->byData[pBinCom->scom.val_pos]), byVal, vallength);
		}

		//
		// exdata_length = sizeof(off_t)
		// *exdata_pos   = valoffset
		//
		pBinCom->scom.exdata_length	= exdatalength;
		pBinCom->scom.exdata_pos	= pBinCom->scom.val_pos + pBinCom->scom.val_length;
		if(0UL < exdatalength){
			memcpy(&(pBinCom->byData[pBinCom->scom.exdata_pos]), byExdata, exdatalength);
		}

	}else{	// SCOM_RENAME == type
		strcpy(pBinCom->scom.szCommand, SCOM_COMSTR_RENAME);

		pBinCom->scom.attr_length	= attrlength;
		pBinCom->scom.attrs_pos		= pBinCom->scom.key_pos + pBinCom->scom.key_length;
		if(0UL < attrlength){
			memcpy(&(pBinCom->byData[pBinCom->scom.attrs_pos]), byAttrs, attrlength);
		}

		pBinCom->scom.exdata_length	= exdatalength;
		pBinCom->scom.exdata_pos	= pBinCom->scom.attrs_pos + pBinCom->scom.attr_length;
		if(0UL < exdatalength){
			memcpy(&(pBinCom->byData[pBinCom->scom.exdata_pos]), byExdata, exdatalength);
		}
	}
	return true;
}

bool K2HCommandArchive::OverWriteValue(const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength, off_t valoffset)
{
	OWVAL_EXDATA	exdata;
	exdata.valoffset = valoffset;

	return Put(SCOM_OW_VAL, byKey, keylength, byVal, vallength, NULL, 0UL, NULL, 0UL, exdata.byData, sizeof(OWVAL_EXDATA));
}

bool K2HCommandArchive::Rename(const unsigned char* byOldKey, size_t oldkeylength, const unsigned char* byNewKey, size_t newkeylength, const unsigned char* byAttrs, size_t attrlength)
{
	// extra data is new key name
	//
	return Put(SCOM_RENAME, byOldKey, oldkeylength, NULL, 0UL, NULL, 0UL, byAttrs, attrlength, byNewKey, newkeylength);
}

const BCOM* K2HCommandArchive::Get(void) const
{
	return pBinCom;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
