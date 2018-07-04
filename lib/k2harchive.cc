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
#include <string>

#include "k2harchive.h"
#include "k2hdaccess.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Constructor / Destructor
//---------------------------------------------------------
K2HArchive::K2HArchive(const char* pFile, bool iserrskip)
{
	Initialize(pFile, iserrskip);
}

K2HArchive::~K2HArchive()
{
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
bool K2HArchive::Initialize(const char* pFile, bool iserrskip)
{
	if(ISEMPTYSTR(pFile)){
		filepath.erase();
	}else{
		filepath = pFile;
	}
	isErrSkip = iserrskip;

	return true;
}

bool K2HArchive::Serialize(K2HShm* pShm, bool isLoad) const
{
	if(!pShm || !pShm->IsAttached()){
		ERR_K2HPRN("Paramter is wrong.");
		return false;
	}

	bool	result;
	if(isLoad){
		result = Load(pShm);
	}else{
		result = Save(pShm);
	}
	if(!result){
		ERR_K2HPRN("Failed to Serialize.");
	}
	return result;
}

// [TODO]
// This function does not care for output order by subkeys.
// The best way to output is that parenet key puts after putting subkey elements.
// But K2HShm::begin() and iterator returns keys by sequential now.
// If do the best way, probably this function should memorize put subkeys.
// Now the serializing is like snapshot, and if removing(changing/adding) subkeys
// during saving to a file, these subkeys are inconsistent.
//
#define	MAX_VALLENGTH_FOR_SERIALIZE		(1024 * 1024 * 10)		// 10MB is max val length for serializing without over write mode.

bool K2HArchive::Save(K2HShm* pShm) const
{
	int		fd;
	bool	result = true;

	if(-1 == (fd = open(filepath.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
		ERR_K2HPRN("Could not open/create file(%s): errno(%d)", filepath.c_str(), errno);
		return false;
	}

	K2HDAccess*		pDAccess= NULL;
	unsigned char*	byKey	= NULL;
	unsigned char*	byVal	= NULL;
	unsigned char*	bySKey	= NULL;
	unsigned char*	byAttrs	= NULL;
	ssize_t			keylength;
	ssize_t			vallength;
	ssize_t			skeylength;
	ssize_t			attrlength;
	for(K2HShm::iterator iter = pShm->begin(); iter != pShm->end(); iter++){
		K2H_Delete(pDAccess);
		K2H_Free(byKey);
		K2H_Free(byVal);
		K2H_Free(bySKey);
		K2H_Free(byAttrs);

		PELEMENT	pElement = *iter;
		if(!pElement){
			if(isErrSkip){
				MSG_K2HPRN("Element is NULL, skipping.");
				continue;
			}else{
				ERR_K2HPRN("Element is NULL, so this is fatal error because of something wrong in logic.");
				result = false;
				break;
			}
		}

		// get element data
		if(-1 == (keylength = pShm->Get(pElement, &byKey, K2HShm::PAGEOBJ_KEY)) || !byKey){
			K2H_Free(byKey);
			if(isErrSkip){
				MSG_K2HPRN("Element does not have key-data, skipping.");
				continue;
			}else{
				ERR_K2HPRN("Element does not have key-data, so this is fatal error because of something wrong in logic.");
				result = false;
				break;
			}
		}
		if(MAX_VALLENGTH_FOR_SERIALIZE < pElement->vallength){
			// large value case
			vallength = 0L;
			if(NULL == (pDAccess = pShm->GetDAccessObj(byKey, static_cast<size_t>(keylength)))){
				K2H_Free(byKey);
				if(isErrSkip){
					MSG_K2HPRN("Failed to get direct access object, skipping.");
					continue;
				}else{
					ERR_K2HPRN("Failed to get direct access object, so this is fatal error because of something wrong in logic.");
					result = false;
					break;
				}
			}
		}else{
			if(-1 == (vallength = pShm->Get(pElement, &byVal, K2HShm::PAGEOBJ_VALUE)) || !byVal){
				vallength = 0L;
				K2H_Free(byVal);
			}
		}
		if(-1 == (skeylength = pShm->Get(pElement, &bySKey, K2HShm::PAGEOBJ_SUBKEYS)) || !bySKey){
			skeylength = 0L;
			K2H_Free(bySKey);
		}
		if(-1 == (attrlength = pShm->Get(pElement, &byAttrs, K2HShm::PAGEOBJ_ATTRS)) || !byAttrs){
			attrlength = 0L;
			K2H_Free(byAttrs);
		}

		// make command archive
		K2HCommandArchive	ArCom;
		if(false == (result = ArCom.SetAll(byKey, keylength, byVal, vallength, bySKey, skeylength, byAttrs, attrlength))){
			if(isErrSkip){
				MSG_K2HPRN("Failed to make command archive, skipping.");
				result = true;
				continue;
			}else{
				ERR_K2HPRN("Failed to make command archive, so this is fatal error because of something wrong in logic.");
				break;
			}
		}
		const BCOM*	pBinCom	= ArCom.Get();

		// write to archve
		size_t	write_length = scom_total_length(pBinCom->scom);
		off_t	fendpos;
		if(-1 == (fendpos = lseek(fd, 0, SEEK_END)) || -1 == k2h_pwrite(fd, pBinCom->byData, write_length, fendpos)){
			if(isErrSkip){
				MSG_K2HPRN("Failed to write command archive to fule, skipping.");
				continue;
			}else{
				ERR_K2HPRN("Failed to write command archive to fule, so this is fatal error because of something wrong in logic.");
				result = false;
				break;
			}
		}

		// for large value
		if(pDAccess){
			// put only value
			if(NULL == (byVal = reinterpret_cast<unsigned char*>(malloc(MAX_VALLENGTH_FOR_SERIALIZE)))){
				ERR_K2HPRN("Could not allocation memory, skipping, so this is fatal error because of something wrong in logic.");
				result = false;
				break;
			}

			// loop for over writing....
			bool 	looperr = false;
			size_t	vlength;
			for(off_t valoffset = 0L; true; valoffset += static_cast<off_t>(vlength)){
				// read value
				vlength = MAX_VALLENGTH_FOR_SERIALIZE;
				if(!pDAccess->Read(&byVal, vlength)){
					looperr = true;
					break;
				}
				if(0UL == vlength || !byVal){
					// no more data
					break;
				}
				// make archive command
				if(!ArCom.OverWriteValue(byKey, keylength, byVal, vlength, valoffset)){
					looperr = true;
					break;
				}
				const BCOM*	pBinCom2 = ArCom.Get();

				// write to archve
				write_length = scom_total_length(pBinCom2->scom);
				if(-1 == (fendpos = lseek(fd, 0, SEEK_END)) || -1 == k2h_pwrite(fd, pBinCom2->byData, write_length, fendpos)){
					looperr = true;
					break;
				}

				if(MAX_VALLENGTH_FOR_SERIALIZE != vlength){
					break;
				}
			}
			if(looperr){
				if(isErrSkip){
					MSG_K2HPRN("Failed to large value directly, skipping.");
					continue;
				}else{
					ERR_K2HPRN("Failed to large value directly, so this is fatal error because of something wrong in logic.");
					result = false;
					break;
				}
			}
		}
		result = true;
	}
	K2H_Delete(pDAccess);
	K2H_Free(byKey);
	K2H_Free(byVal);
	K2H_Free(bySKey);
	K2H_Free(byAttrs);
	K2H_CLOSE(fd);

	return result;
}

// [TODO]
// This function does not care for output order by subkeys.
// The best way to output is that parenet key puts after putting subkey elements.
// But K2HShm::begin() and iterator returns keys by sequential now.
// If do the best way, probably this function should memorize put subkeys.
// Now the serializing is like snapshot, and if removing(changing/adding) subkeys
// during saving to a file, these subkeys are inconsistent.
//

// utility
inline void k2harchive_load_init_vals(PBCOM& pBinCom, unsigned char* &byKey, unsigned char* &byVal, unsigned char* &bySKey, unsigned char* &byAttrs, unsigned char* &byExdata)
{
	K2H_Free(pBinCom);
	K2H_Free(byKey);
	K2H_Free(byVal);
	K2H_Free(bySKey);
	K2H_Free(byAttrs);
	K2H_Free(byExdata);
}

bool K2HArchive::Load(K2HShm* pShm) const
{
	int		fd;
	bool	result = true;

	if(-1 == (fd = open(filepath.c_str(), O_RDONLY))){
		ERR_K2HPRN("Could not open file(%s): errno(%d)", filepath.c_str(), errno);
		return false;
	}

	PBCOM			pBinCom;
	off_t			offset;
	unsigned char*	byKey;
	unsigned char*	byVal;
	unsigned char*	bySKey;
	unsigned char*	byAttrs;
	unsigned char*	byExdata;
	for(offset = 0L, byKey = NULL, byVal = NULL, bySKey = NULL, byAttrs = NULL; NULL != (pBinCom = static_cast<PBCOM>(ReadFile(fd, sizeof(BCOM), offset))); offset += scom_total_length(pBinCom->scom), k2harchive_load_init_vals(pBinCom, byKey, byVal, bySKey, byAttrs, byExdata)){
		// check type
		if(pBinCom->scom.type < SCOM_TYPE_MIN || SCOM_TYPE_MAX < pBinCom->scom.type){
			if(isErrSkip){
				MSG_K2HPRN("Load data-set\'s type(%ld: %s) is unknown, so this data-set is skipped.", pBinCom->scom.type, pBinCom->scom.szCommand);
				continue;
			}else{
				ERR_K2HPRN("Load data-set\'s type(%ld: %s) is unknown.", pBinCom->scom.type, pBinCom->scom.szCommand);
				result = false;
				break;
			}
		}

		// read data
		if(	(0L < pBinCom->scom.key_length && NULL == (byKey = static_cast<unsigned char*>(ReadFile(fd, pBinCom->scom.key_length, offset + pBinCom->scom.key_pos)))) ||
			(0L < pBinCom->scom.val_length && NULL == (byVal = static_cast<unsigned char*>(ReadFile(fd, pBinCom->scom.val_length, offset + pBinCom->scom.val_pos)))) ||
			(0L < pBinCom->scom.skey_length && NULL == (bySKey = static_cast<unsigned char*>(ReadFile(fd, pBinCom->scom.skey_length, offset + pBinCom->scom.skey_pos)))) ||
			(0L < pBinCom->scom.attr_length && NULL == (byAttrs = static_cast<unsigned char*>(ReadFile(fd, pBinCom->scom.attr_length, offset + pBinCom->scom.attrs_pos)))) ||
			(0L < pBinCom->scom.exdata_length && NULL == (byExdata = static_cast<unsigned char*>(ReadFile(fd, pBinCom->scom.exdata_length, offset + pBinCom->scom.exdata_pos)))) )
		{
			if(isErrSkip){
				MSG_K2HPRN("Could not read data from file, skipping, so this data-set is skipped.");
				continue;
			}else{
				ERR_K2HPRN("Could not read data from file.");
				result = false;
				break;
			}
		}

		// set
		bool	CommandResult;
		if(SCOM_SET_ALL == pBinCom->scom.type){
			CommandResult = pShm->ReplaceAll(byKey, pBinCom->scom.key_length, byVal, pBinCom->scom.val_length, bySKey, pBinCom->scom.skey_length, byAttrs, pBinCom->scom.attr_length);

		}else if(SCOM_REPLACE_VAL == pBinCom->scom.type){
			CommandResult = pShm->ReplaceValue(byKey, pBinCom->scom.key_length, byVal, pBinCom->scom.val_length);

		}else if(SCOM_REPLACE_SKEY == pBinCom->scom.type){
			CommandResult = pShm->ReplaceSubkeys(byKey, pBinCom->scom.key_length, bySKey, pBinCom->scom.skey_length);

		}else if(SCOM_REPLACE_ATTRS == pBinCom->scom.type){
			CommandResult = pShm->ReplaceAttrs(byKey, pBinCom->scom.key_length, byAttrs, pBinCom->scom.attr_length);

		}else if(SCOM_DEL_KEY == pBinCom->scom.type){
			CommandResult = pShm->Remove(byKey, pBinCom->scom.key_length, false);

		}else if(SCOM_OW_VAL == pBinCom->scom.type){
			if(!byExdata || 0UL == pBinCom->scom.exdata_length){
				CommandResult = false;
			}else{
				K2HDAccess		daccess(pShm, K2HDAccess::WRITE_ACCESS);
				OWVAL_EXDATA	exdata;

				memcpy(exdata.byData, byExdata, pBinCom->scom.exdata_length);
				if(	!daccess.Open(byKey, pBinCom->scom.key_length) ||
					!daccess.SetWriteOffset(exdata.valoffset) ||
					!daccess.Write(byVal, pBinCom->scom.val_length) )
				{
					CommandResult = false;
				}else{
					CommandResult = true;
				}
			}
		}else{	// SCOM_RENAME == pBinCom->scom.type
			CommandResult = pShm->Rename(byKey, pBinCom->scom.key_length, byExdata, pBinCom->scom.exdata_length, byAttrs, pBinCom->scom.attr_length);
		}

		// check error
		if(!CommandResult){
			if(isErrSkip){
				MSG_K2HPRN("Failed loading data-set\'s type(%ld: %s), so this data-set is skipped.", pBinCom->scom.type, pBinCom->scom.szCommand);
				continue;
			}else{
				ERR_K2HPRN("Failed loading data-set\'s type(%ld: %s).", pBinCom->scom.type, pBinCom->scom.szCommand);
				result = false;
				break;
			}
		}
	}
	k2harchive_load_init_vals(pBinCom, byKey, byVal, bySKey, byAttrs, byExdata);
	K2H_CLOSE(fd);

	return result;
}

void* K2HArchive::ReadFile(int fd, size_t count, off_t offset) const
{
	if(-1 == fd || 0UL == count || 0L > offset){
		ERR_K2HPRN("Parameters wrong.");
		return NULL;
	}
	void*	pReadData;

	if(NULL == (pReadData = malloc(count))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}
	ssize_t	read_length;
	if(-1 == (read_length = k2h_pread(fd, pReadData, count, offset))){
		K2H_Free(pReadData);
		return NULL;
	}
	if(static_cast<size_t>(read_length) != count){
		K2H_Free(pReadData);
		return NULL;
	}
	return pReadData;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
