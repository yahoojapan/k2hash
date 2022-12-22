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
 * CREATE:   Fri Mar 28 2014
 * REVISION:
 *
 */

#include <assert.h>
#include <string>

#include "k2hshm.h"
#include "k2hdaccess.h"
#include "k2htrans.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// K2HDALock : Constructor / Destructor
//---------------------------------------------------------
K2HDALock::K2HDALock(K2HShm* pk2hshm, bool is_read_lock, const unsigned char* byKey, size_t keylength) : pK2HShm(pk2hshm), IsReadOnlyLock(is_read_lock), pALObjCKI(NULL), pElement(NULL)
{
	if(!Lock(byKey, keylength)){
		//MSG_K2HPRN("Start without locking.");
	}
}

K2HDALock::K2HDALock(K2HShm* pk2hshm, bool is_read_lock, const char* pKey) : pK2HShm(pk2hshm), IsReadOnlyLock(is_read_lock), pALObjCKI(NULL), pElement(NULL)
{
	if(!Lock(pKey)){
		//MSG_K2HPRN("Start without locking.");
	}
}

K2HDALock::~K2HDALock()
{
	Unlock();
}

//---------------------------------------------------------
// K2HDALock : Methods
//---------------------------------------------------------
bool K2HDALock::Unlock(void)
{
	K2H_Delete(pALObjCKI);
	pElement = NULL;
	return true;
}

bool K2HDALock::Lock(const char* pKey)
{
	return Lock(reinterpret_cast<const unsigned char*>(pKey), pKey ? strlen(pKey) + 1 : 0UL);
}

bool K2HDALock::Lock(const unsigned char* byKey, size_t keylength)
{
	if(!byKey || 0 == keylength){
		//MSG_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!pK2HShm || !pK2HShm->IsAttached()){
		//MSG_K2HPRN("K2HShm object is NULL or not attached k2h.");
		return false;
	}
	// remake lock object(unlock to lock)
	K2H_Delete(pALObjCKI);
	pALObjCKI	= new K2HLock(IsReadOnlyLock ? K2HLock::RDLOCK : K2HLock::RWLOCK);

	// get element & lock it
	if(NULL == (pElement = pK2HShm->GetElement(byKey, keylength, *pALObjCKI))){
		//MSG_K2HPRN("Not found key.");
		return false;
	}
	return true;
}

//---------------------------------------------------------
// K2HDAccess : Variables
//---------------------------------------------------------
const int	K2HDAccess::FILE_IO_BLOCK;

//---------------------------------------------------------
// K2HDAccess : Constructor / Destructor
//---------------------------------------------------------
K2HDAccess::K2HDAccess() : 
	pK2HShm(NULL), pALObjCKI(NULL), AccessMode(K2HDAccess::READ_ACCESS), byOpKey(NULL), opkeylength(0UL), fileio_size(FILE_IO_BLOCK), 
	pWriteValPage(NULL), abs_woffset(EOF), valpage_woffset(EOF), 
	pReadValPage(NULL), abs_roffset(EOF), valpage_roffset(EOF)
{
}

K2HDAccess::K2HDAccess(K2HShm* pk2hshm, K2HDAccess::ACSMODE access_mode) : 
	pK2HShm(NULL), pALObjCKI(NULL), AccessMode(access_mode), byOpKey(NULL), opkeylength(0UL), fileio_size(FILE_IO_BLOCK), 
	pWriteValPage(NULL), abs_woffset(EOF), valpage_woffset(EOF), 
	pReadValPage(NULL), abs_roffset(EOF), valpage_roffset(EOF)
{
	if(!Reset(pk2hshm, access_mode)){
		ERR_K2HPRN("Some parameters are wrong.");
		assert(false);
	}
}

K2HDAccess::~K2HDAccess()
{
	Reset();
}

//---------------------------------------------------------
// K2HDAccess : Methods
//---------------------------------------------------------
bool K2HDAccess::Reset(K2HShm* pk2hshm, K2HDAccess::ACSMODE access_mode)
{
	K2H_Delete(pALObjCKI);
	K2H_Delete(pWriteValPage);
	K2H_Delete(pReadValPage);
	pK2HShm			= NULL;
	AccessMode		= access_mode;
	opkeylength		= 0UL;
	K2H_Free(byOpKey);
	ResetOffsets();

	if(pk2hshm && pk2hshm->IsAttached()){
		pK2HShm			= pk2hshm;
		pALObjCKI= new K2HDALock(pk2hshm, K2HDAccess::READ_ACCESS == access_mode ? true : false);
	}else if(pk2hshm && !pk2hshm->IsAttached()){
		ERR_K2HPRN("K2HShm object is not attached k2h.");
		return false;
	}
	return true;
}

void K2HDAccess::ResetOffsets(bool isRead)
{
	if(isRead){
		K2H_Delete(pReadValPage);
		abs_roffset		= EOF;
		valpage_roffset	= EOF;
	}else{
		if(AccessMode == K2HDAccess::READ_ACCESS){
			//MSG_K2HPRN("This object is not writing mode.");
		}
		K2H_Delete(pWriteValPage);
		abs_woffset		= EOF;
		valpage_woffset	= EOF;
	}
}

bool K2HDAccess::Open(const char* pKey)
{
	return Open(reinterpret_cast<const unsigned char*>(pKey), pKey ? strlen(pKey) + 1 : 0UL);
}

bool K2HDAccess::Open(const unsigned char* byKey, size_t keylength)
{
	if(!byKey || 0 == keylength){
		ERR_K2HPRN("Key is empty, so could not lock it.");
		return false;
	}
	if(!IsInitialize()){
		ERR_K2HPRN("This object is not initialized.");
		return false;
	}
	if(!Close()){
		ERR_K2HPRN("Could not close.");
		return false;
	}

	// test lock
	if(!pALObjCKI->Lock(byKey, keylength)){
		if(AccessMode == K2HDAccess::READ_ACCESS){
			MSG_K2HPRN("Not found key.");
			return false;
		}

		// [NOTE]
		// if key does not exist, K2HDALock fails to lock it.
		// So try to make new key here.
		//
		if(!pK2HShm->Set(byKey, keylength, NULL, 0UL)){
			ERR_K2HPRN("Failed to make new key.");
			return false;
		}
		// retry to lock
		if(!pALObjCKI->Lock(byKey, keylength)){
			ERR_K2HPRN("Failed to get existed object, something wrong.");
			return false;
		}
	}

	if(NULL == (byOpKey = reinterpret_cast<unsigned char*>(malloc(keylength)))){
		ERR_K2HPRN("Could not allocation memory.");
		return false;
	}
	// backup key
	memcpy(byOpKey, byKey, keylength);
	opkeylength = keylength;

	return true;
}

bool K2HDAccess::Close(void)
{
	if(!IsInitialize()){
		ERR_K2HPRN("This object is not initialized.");
		return false;
	}
	if(IsOpen()){
		ResetOffsets();
		pALObjCKI->Unlock();
		opkeylength	= 0UL;
		K2H_Free(byOpKey);
	}
	return true;
}

bool K2HDAccess::IsSetValPage(bool isRead) const
{
	if(!IsOpen()){
		return false;
	}
	if(AccessMode == K2HDAccess::READ_ACCESS && !isRead){
		ERR_K2HPRN("This object is not write mode.");
		return false;
	}
	if(isRead){
		if(abs_roffset == EOF){
			return false;
		}
	}else{
		if(abs_woffset == EOF){
			return false;
		}
	}
	// If abs_woffset(abs_roffset) is over 0L, it means already initialized value page object and it's
	// offset. Then value page object is NULL and it's offset is NA, it is no data for key.

	return true;
}

//
// This method initializes internal page object pointer and it's offsets.
// If the key does not have value, page object is NULL and it's relative offset is NA, but it's absolute
// offset is not NA.
//
// [BE CAREFUL]
// If the value area is expanded because it's length is short when isRead is false, the expanding value
// area is not initialized.
//
bool K2HDAccess::SetOffset(off_t offset, size_t length, bool isRead)
{
	if(offset < 0L){			// length is allowed 0UL
		ERR_K2HPRN("Parameter(offset) is wrong.");
		return false;
	}
	if(!IsOpen()){
		ERR_K2HPRN("This object is not initialized or opened.");
		return false;
	}
	if((AccessMode == K2HDAccess::READ_ACCESS && !isRead) || (AccessMode == K2HDAccess::WRITE_ACCESS && isRead)){
		ERR_K2HPRN("This object is not write/read mode.");
		return false;
	}
	ResetOffsets(isRead);

	K2HPage*	pValPage = NULL;
	off_t		valpage_offset = EOF;
	if(!pALObjCKI->pElement->value){
		// element does not have value
		if(AccessMode != K2HDAccess::READ_ACCESS){
			if((length + offset) <= 0UL){
				// [NOTICE] value is empty and request length + offset is 0.
				// This case is that page object is null, asb_offset = offset(0), BE CAREFUL!
				//
			}else{
				// make new key-value without initializing
				if(NULL == (pValPage = pK2HShm->ReservePages(length + offset))){
					ERR_K2HPRN("Failed to reserve value page object.");
					return false;
				}
				if(!pValPage->Truncate(length + offset)){
					ERR_K2HPRN("Failed to truncate value data length.");
					pValPage->Free();
					K2H_Delete(pValPage);
					return false;
				}
				if(!pK2HShm->ReplacePage(pALObjCKI->pElement, pValPage, (length + offset), K2HShm::PAGEOBJ_VALUE)){
					ERR_K2HPRN("Failed to set value data pointer into element head.");
					pValPage->Free();
					K2H_Delete(pValPage);
					return false;
				}
				valpage_offset = offset;
			}
		}

	}else if((length + offset) <= pALObjCKI->pElement->vallength){
		// element's value length is enough
		if(NULL == (pValPage = pK2HShm->GetPageObject(pALObjCKI->pElement->value, false))){
			ERR_K2HPRN("Failed to get value page object.");
			return false;
		}
		valpage_offset = offset;

	}else if(static_cast<size_t>(offset) <= pALObjCKI->pElement->vallength){
		// element does not have enough length
		if(NULL == (pValPage = pK2HShm->GetPageObject(pALObjCKI->pElement->value, false))){
			ERR_K2HPRN("Failed to get value page object.");
			return false;
		}
		if(AccessMode != K2HDAccess::READ_ACCESS){
			// if could expand, do it.
			if(!pValPage->Truncate(length + offset)){
				ERR_K2HPRN("Failed to truncate value data length.");
				K2H_Delete(pValPage);
				return false;
			}
			pALObjCKI->pElement->vallength = length + offset;
		}
		valpage_offset = offset;

	}else{
		// element has short value
		if(AccessMode != K2HDAccess::READ_ACCESS){
			if(NULL == (pValPage = pK2HShm->GetPageObject(pALObjCKI->pElement->value, false))){
				ERR_K2HPRN("Failed to get value page object.");
				return false;
			}
			// if could expand, do it.
			if(!pValPage->Truncate(length + offset)){
				ERR_K2HPRN("Failed to truncate value data length.");
				K2H_Delete(pValPage);
				return false;
			}
			pALObjCKI->pElement->vallength	= length + offset;
			valpage_offset					= offset;
		}
	}

	if(isRead){
		pReadValPage	= pValPage;
		abs_roffset		= offset;
		valpage_roffset	= valpage_offset;
	}else{
		pWriteValPage	= pValPage;
		abs_woffset		= offset;
		valpage_woffset	= valpage_offset;
	}

	return true;
}

bool K2HDAccess::GetSize(size_t& size) const
{
	if(!IsOpen()){
		return false;
	}
	size = pALObjCKI->pElement->vallength;
	return true;
}

size_t K2HDAccess::SetFioSize(size_t size)
{
	size_t	old	= fileio_size;
	fileio_size	= size;
	return old;
}

//
// This method writes data directly which specified offset.
// If the value area is short, the value area is stretched automatically.
//
bool K2HDAccess::Write(const char* pValue)
{
	return Write(reinterpret_cast<const unsigned char*>(pValue), pValue ? strlen(pValue) + 1 : 0UL);
}

bool K2HDAccess::Write(const unsigned char* byValue, size_t vallength)
{
	if(!byValue || 0UL == vallength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(AccessMode == K2HDAccess::READ_ACCESS){
		ERR_K2HPRN("This object is not write mode.");
		return false;
	}
	if(!IsSetValPage(false)){
		ERR_K2HPRN("This object is not initialized value page.");
		return false;
	}

	// [NOTICE] Check page object
	// If value is empty, abs_woffset is 0 and pWriteValPage is NULL.
	// So make new page here.
	//
	if(0L == abs_woffset && !pWriteValPage){
		if(!SetOffset(abs_woffset, vallength, false) || !pWriteValPage){
			ERR_K2HPRN("Could not make new page object.");
			return false;
		}
	}

	// Set values
	K2HPage*	pLastValPage;
	off_t		next_offset;
	off_t		abs_woffset_bup	= abs_woffset;
	bool		isChangeLength	= false;
	if(NULL == (pLastValPage = pWriteValPage->SetData(byValue, vallength, valpage_woffset, isChangeLength, next_offset))){
		ERR_K2HPRN("Failed to write value data with offset.");
		return false;
	}
	if(pLastValPage != pWriteValPage){
		K2H_Delete(pWriteValPage);
		pWriteValPage = pLastValPage;
	}
	abs_woffset		+= vallength;
	valpage_woffset = next_offset;

	if(isChangeLength){
		pALObjCKI->pElement->vallength	= static_cast<size_t>(abs_woffset);
	}
	pALObjCKI->Unlock();							// [NOTICE] Unlock

	// transaction
	K2HTransaction	transobj(pK2HShm);
	if(!transobj.OverWriteValue(byOpKey, opkeylength, byValue, vallength, abs_woffset_bup)){
		WAN_K2HPRN("Failed to put setting transaction.");
	}

	if(!pALObjCKI->Lock(byOpKey, opkeylength)){		// [NOTICE] Lock
		ERR_K2HPRN("Failed to relock key, but continue...");
	}

	// update time
	if(!pK2HShm->UpdateTimeval()){
		WAN_K2HPRN("Failed to update timeval for data update.");
	}

	return true;
}

bool K2HDAccess::Write(int fd, size_t& wlength)
{
	if(0 > fd || 0UL == wlength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(AccessMode == K2HDAccess::READ_ACCESS){
		ERR_K2HPRN("This object is not write mode.");
		return false;
	}
	if(!IsSetValPage(false)){
		ERR_K2HPRN("This object is not initialized value page.");
		return false;
	}

	// get file current pos
	off_t	fpos;
	if(-1L == (fpos = lseek(fd, 0L, SEEK_CUR))){
		ERR_K2HPRN("Could not get current pos from fd. errno = %d", errno);
		return false;
	}

	// file size
	struct stat	st;
	if(-1 == fstat(fd, &st)){
		ERR_K2HPRN("Could not get stat from fd. errno = %d", errno);
		return false;
	}

	// allocate buffer
	unsigned char*	pBuff;
	if(NULL == (pBuff = reinterpret_cast<unsigned char*>(malloc(fileio_size)))){
		ERR_K2HPRN("Could not allocation memory");
		return false;
	}

	// read & write
	size_t	total_size	= min(static_cast<size_t>(st.st_size - fpos), wlength);
	size_t	total_read	= 0UL;
	ssize_t	one_read;
	for(total_read = 0UL; 0UL < total_size; total_size -= static_cast<size_t>(one_read), total_read += static_cast<size_t>(one_read), fpos += static_cast<off_t>(one_read)){
		// read from file
		one_read = static_cast<ssize_t>(min(total_size, fileio_size));
		if(-1 == (one_read = k2h_pread(fd, pBuff, static_cast<size_t>(one_read), fpos))){
			WAN_K2HPRN("Could not read data from file.");
			break;
		}
		// write
		if(!Write(pBuff, static_cast<size_t>(one_read))){
			WAN_K2HPRN("Could not write data.");
			K2H_Free(pBuff);
			return false;
		}
	}
	K2H_Free(pBuff);

	wlength = total_read;

	return true;
}

//
// This method reads value directly.
//
bool K2HDAccess::Read(char** ppValue)
{
	size_t	vallength = 0UL;
	if(!GetSize(vallength)){
		return false;
	}
	return Read(reinterpret_cast<unsigned char**>(ppValue), vallength);
}

bool K2HDAccess::Read(unsigned char** byValue, size_t& vallength)
{
	if(!byValue || 0UL == vallength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(AccessMode == K2HDAccess::WRITE_ACCESS){
		ERR_K2HPRN("This object is not write mode.");
		return false;
	}
	if(!IsSetValPage(true)){
		ERR_K2HPRN("This object is not initialized value page.");
		return false;
	}
	// Get values
	K2HPage*	pLastValPage;
	off_t		next_offset;
	size_t		read_length = 0UL;
	*byValue = NULL;
	if(NULL == (pLastValPage = pReadValPage->LoadData(valpage_roffset, vallength, byValue, read_length, next_offset))){
		ERR_K2HPRN("Failed to read value data with offset.");
		return false;
	}
	if(pLastValPage == pReadValPage){
		if(read_length < vallength){
			valpage_roffset = EOF;
			abs_roffset		= EOF;
		}else{
			valpage_roffset = next_offset;
			abs_roffset		+= read_length;
		}
	}else{
		K2H_Delete(pReadValPage);
		pReadValPage	= pLastValPage;
		valpage_roffset	= next_offset;
		abs_roffset		+= read_length;
	}
	vallength = read_length;

	return true;
}

bool K2HDAccess::Read(int fd, size_t& rlength)
{
	if(0 > fd || 0UL == rlength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(AccessMode == K2HDAccess::WRITE_ACCESS){
		ERR_K2HPRN("This object is not write mode.");
		return false;
	}
	if(!IsSetValPage(true)){
		ERR_K2HPRN("This object is not initialized value page.");
		return false;
	}

	// get file current pos
	off_t	fpos;
	if(-1L == (fpos = lseek(fd, 0L, SEEK_CUR))){
		ERR_K2HPRN("Could not get current pos from fd. errno = %d", errno);
		return false;
	}

	// total size
	size_t	total_size	= min((pALObjCKI->pElement->vallength - static_cast<size_t>(abs_roffset)), rlength);
	size_t	total_write	= 0UL;
	size_t	one_read;
	for(total_write = 0UL; 0UL < total_size; total_size -= static_cast<size_t>(one_read), total_write += static_cast<size_t>(one_read), fpos += static_cast<off_t>(one_read)){
		// read
		unsigned char*	pBuff	= NULL;
		one_read				= static_cast<ssize_t>(min(total_size, fileio_size));
		if(!Read(&pBuff, one_read) || !pBuff){
			WAN_K2HPRN("Could not read data.");
			K2H_Free(pBuff);
			break;
		}

		// write
		ssize_t	result;
		if(-1 == (result = k2h_pwrite(fd, pBuff, one_read, fpos))){
			WAN_K2HPRN("Could not write data to file.");
			K2H_Free(pBuff);
			return false;
		}
		one_read = static_cast<size_t>(result);
		K2H_Free(pBuff);
	}
	rlength = total_write;

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
