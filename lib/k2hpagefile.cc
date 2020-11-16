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
 * CREATE:   Fri Dec 2 2013
 * REVISION:
 *
 */

#include <string.h>

#include "k2hcommon.h"
#include "k2hpagefile.h"
#include "k2hshm.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Class Methods
//---------------------------------------------------------
bool K2HPageFile::GetData(const K2HShm* pk2hshm, int fd, off_t offset, unsigned char** ppPageData, size_t* pLength)
{
	if(!ppPageData || !pLength){
		ERR_K2HPRN("ppPageData or pLength is null");
		return false;
	}

	K2HPageFile	k2hpage_obj;
	if(!k2hpage_obj.Initialize(pk2hshm, fd, offset)){
		return false;
	}
	if(!k2hpage_obj.LoadData()){
		return false;
	}
	if(NULL == (*ppPageData = (unsigned char*)malloc(k2hpage_obj.DataLength))){
		ERR_K2HPRN("Could not allocation memory.");
		// cppcheck-suppress unmatchedSuppression
		// cppcheck-suppress memleak
		return false;
	}
	memcpy(*ppPageData, k2hpage_obj.pPageData, k2hpage_obj.DataLength);

	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress memleak
	return true;
}

bool K2HPageFile::FreeData(unsigned char* pPageData)
{
	// cppcheck-suppress uselessAssignmentPtrArg
	K2H_Free(pPageData);
	return true;
}

//---------------------------------------------------------
// Constructor / Destructor
//---------------------------------------------------------
K2HPageFile::K2HPageFile(const K2HShm* pk2hshm, int fd, off_t offset, bool nodup) : K2HPage(NULL), PageFd(-1), FileOffset(0), isNodupfd(nodup)
{
	if(pk2hshm && -1 != fd && 0 < offset){
		Initialize(pk2hshm, fd, offset);
	}
}

K2HPageFile::K2HPageFile(const K2HShm* pk2hshm, int fd, off_t offset) : K2HPage(NULL), PageFd(-1), FileOffset(0), isNodupfd(false)
{
	if(pk2hshm && -1 != fd && 0 < offset){
		Initialize(pk2hshm, fd, offset);
	}
}

K2HPageFile::~K2HPageFile()
{
	K2HPageFile::Clean();
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
void K2HPageFile::Clean(void)
{
	CloseFd();
	CleanMemory();
}

void K2HPageFile::CleanPageHead(void)
{
	K2H_Free(pPageHead);
	K2HPage::CleanPageHead();
	isHeadLoaded = false;
}

bool K2HPageFile::CloseFd(void)
{
	if(isNodupfd){
		PageFd		= -1;
		FileOffset	= 0;
	}else{
		if(-1 != PageFd){
			if(-1 == close(PageFd)){
				ERR_K2HPRN("Failed to close fd(%d), errno = %d", PageFd, errno);
				return false;
			}
			PageFd		= -1;
			FileOffset	= 0;
		}
	}
	return true;
}

bool K2HPageFile::DuplicateFd(int fd)
{
	int	NewFd;
	if(-1 == (NewFd = dup(fd))){
		ERR_K2HPRN("Could not duplicate file descriptor(%d), errno = %d", fd, errno);
		return false;
	}
	off_t	offset	= FileOffset;	// backup

	CloseFd();
	isNodupfd		= false;
	PageFd			= NewFd;
	FileOffset		= offset;

	return true;
}

bool K2HPageFile::Initialize(const K2HShm* pk2hshm, int fd, off_t offset)
{
	if(0 >= offset){
		ERR_K2HPRN("Offset(%jd) is wrong.", static_cast<intmax_t>(offset));
		return false;
	}
	if(!CloseFd()){
		ERR_K2HPRN("Failed to close fd.");
		return false;
	}
	CleanMemory();

	if(!K2HPage::Initialize(pk2hshm)){
		return false;
	}

	if(isNodupfd){
		PageFd = fd;
	}else{
		if(-1 == (PageFd = dup(fd))){
			ERR_K2HPRN("Could not duplicate file descriptor(%d), errno = %d", fd, errno);
			return false;
		}
	}
	FileOffset = offset;

	return true;
}

bool K2HPageFile::UploadPageHead(void)
{
	if(!isHeadLoaded){
		ERR_K2HPRN("Not loaded page head yet.");
		return false;
	}

	PPAGEWRAP	pPageWrap = reinterpret_cast<PPAGEWRAP>(pPageHead);

	// upload page head
	if(-1 == k2h_pwrite(PageFd, &(pPageWrap->barray[0]), PAGEHEAD_SIZE, FileOffset)){
		ERR_K2HPRN("Failed to write page head from fd(%d:%jd), errno = %d", PageFd, static_cast<intmax_t>(FileOffset), errno);
		return false;
	}
	return true;
}

bool K2HPageFile::LoadPageHead(void)
{
	if(isHeadLoaded){
//		MSG_K2HPRN("Already load page head");
		return true;
	}
	if(-1 == PageFd || 0UL == PageSize){
		ERR_K2HPRN("There is no file descriptor, or pagesize is wrong.");
		return false;
	}

	// allocation for reading page head
	PPAGEWRAP	pPageWrap;
	if(NULL == (pPageWrap = (PPAGEWRAP)malloc(sizeof(PAGEWRAP)))){
		ERR_K2HPRN("Could not allocation memory.");
		// cppcheck-suppress unmatchedSuppression
		// cppcheck-suppress memleak
		return false;
	}

	// load page head
	if(-1 == k2h_pread(PageFd, &(pPageWrap->barray[0]), PAGEHEAD_SIZE, FileOffset)){
		ERR_K2HPRN("Failed to read page head from fd(%d:%jd), errno = %d", PageFd, static_cast<intmax_t>(FileOffset), errno);
		K2H_Free(pPageWrap);
		return false;
	}
	pPageHead = &(pPageWrap->pagehead);		// pPageWrap address is as same as pPageHead.

	isHeadLoaded = true;

	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress memleak
	return true;
}

bool K2HPageFile::LoadData(bool isPageHead, bool isAllPage)
{
	if(-1 == PageFd || 0UL == PageSize){
		ERR_K2HPRN("There is no file descriptor, or pagesize is wrong.");
		return false;
	}

	// Clear inner memory
	if(isPageHead){
		CleanMemory();
	}else{
		CleanLoadedData();
	}

	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head from fd(%d)", PageFd);
		return false;
	}

	// allocation for reading one page
	if(NULL == AllocateData(PageSize - PAGEHEAD_SIZE)){
		ERR_K2HPRN("Could not allocation memory.");
		return false;
	}

	// load page data
	if(-1 == k2h_pread(PageFd, pPageData, pPageHead->length, FileOffset + PAGEHEAD_DATA_OFFSET)){
		ERR_K2HPRN("Failed to read page head from fd(%d:%jd), errno = %d", PageFd, static_cast<intmax_t>(FileOffset + PAGEHEAD_DATA_OFFSET), errno);
		return false;
	}
	DataLength = pPageHead->length;

	if(isAllPage && pPageHead->next){
		// load after this
		for(PPAGEHEAD pNext = pPageHead->next, pTmpNext = NULL; pNext; pNext = pTmpNext){
			// make temporary object
			K2HPageFile	PageAfter(pK2HShm, PageFd, reinterpret_cast<off_t>(pNext), true);

			if(!PageAfter.LoadData(isPageHead, false)){
				ERR_K2HPRN("Failed to load data after this object.");
				return false;
			}
			// append after data
			if(PageAfter.pPageData && 0UL != PageAfter.DataLength){
				if(NULL == AppendData(PageAfter.pPageData, PageAfter.DataLength)){
					ERR_K2HPRN("Failed to append page data after this.");
					return false;
				}
			}
			pTmpNext = PageAfter.pPageHead->next;
		}
	}
	isLoaded = true;

	return true;
}

//
// This method is for iostream, and load only specified offset-length data into allocated buffer
// Returns last page object, if returned object is same as this, must not free it.
// If the result of this function is offset = next_read_pos, it means no more data.
//
K2HPage* K2HPageFile::LoadData(off_t offset, size_t length, unsigned char** byData, size_t& datalength, off_t& next_read_pos)
{
	if(-1 == PageFd || 0UL == PageSize || !byData){
		ERR_K2HPRN("There is no file descriptor, or pagesize is wrong, or parameter is wrong.");
		return NULL;
	}

	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head from fd(%d)", PageFd);
		return NULL;
	}

	// allocated
	bool	isAllocate = false;
	if(!(*byData)){
		if(NULL == (*byData = static_cast<unsigned char*>(malloc(length)))){
			ERR_K2HPRN("Could not allocate memory.");
			return NULL;
		}
		isAllocate = true;
	}

	// loop
	unsigned char*	byNext		= *byData;
	K2HPageFile*	pPageAfter	= NULL;
	off_t			next_offset;

	for(PPAGEHEAD pTarget = pPageHead; pTarget; offset = next_offset){
		if(pTarget->length <= static_cast<size_t>(offset)){
			// skip to next
			next_offset		= offset - pTarget->length;

		}else{
			// page has specified area.
			size_t	this_length = min((pTarget->length - offset), length);

			// load data by offset and length
			if(-1 == k2h_pread((pPageAfter ? pPageAfter->PageFd : this->PageFd), byNext, this_length, (pPageAfter ? pPageAfter->FileOffset : this->FileOffset) + PAGEHEAD_DATA_OFFSET + offset)){
				ERR_K2HPRN("Failed to read page head from fd(%d:%jd), errno = %d", pPageAfter ? pPageAfter->PageFd : this->PageFd, static_cast<intmax_t>((pPageAfter ? pPageAfter->FileOffset : this->FileOffset) + PAGEHEAD_DATA_OFFSET + offset), errno);
				return NULL;
			}

			length			-= this_length;
			datalength		+= this_length;
			byNext			= ADDPTR(byNext, this_length);
			offset			+= this_length;
			next_offset		= 0L;
		}

		if(0UL == length){
			break;
		}
		if(!pTarget->next){
			//MSG_K2HPRN("Could not read more length(%zu), because no more page chain.", length);
			if(0UL == datalength && isAllocate){
				K2H_Free(*byData);
			}
			break;
		}

		// set up next page
		K2H_Delete(pPageAfter);

		pPageAfter = new K2HPageFile(pK2HShm, PageFd, reinterpret_cast<off_t>(pTarget->next), false);	// fd is duplicated
		if(!pPageAfter->LoadPageHead() || !pPageAfter->pPageHead){
			ERR_K2HPRN("Failed to read after page head");
			K2H_Delete(pPageAfter);
			if(isAllocate){
				K2H_Free(*byData);
			}
			return NULL;
		}
		pTarget = pPageAfter->pPageHead;
	}

	if(!pPageAfter){
		pPageAfter = this;
	}
	next_read_pos = offset;

	return pPageAfter;
}

bool K2HPageFile::SetData(const unsigned char* byData, size_t length, bool isSetLoadedData)
{
	if(!byData && 0UL != length){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!pK2HShm || -1 == PageFd || 0L == FileOffset){
		ERR_K2HPRN("K2HShm or descriptor, or file offset is wrong.");
		return false;
	}

	// Clear loaded data
	CleanLoadedData();

	// loop
	K2HPageFile*			pPageAfter	= this;
	size_t					remlength	= length;
	const unsigned char*	byRead		= byData;
	while(0UL < remlength){
		// data length for this
		size_t	one_length = min(remlength, (pPageAfter->PageSize - PAGEHEAD_SIZE));

		// write data length
		if(!pPageAfter->SetPageHead(SETHEAD_LENGTH, NULL, NULL, one_length)){
			ERR_K2HPRN("Could not upload page head.");
			if(pPageAfter != this){
				K2H_Delete(pPageAfter);
			}
			return false;
		}

		// write page data
		if(0UL < one_length){
			if(-1 == k2h_pwrite(pPageAfter->PageFd, byRead, one_length, pPageAfter->FileOffset + PAGEHEAD_DATA_OFFSET)){
				ERR_K2HPRN("Failed to write page data to fd(%d:%jd), errno = %d", pPageAfter->PageFd, static_cast<intmax_t>(pPageAfter->FileOffset + PAGEHEAD_DATA_OFFSET), errno);
				if(pPageAfter != this){
					K2H_Delete(pPageAfter);
				}
				return false;
			}
		}

		// next pos
		byRead		= &byRead[one_length];
		remlength  -= one_length;

		// make next page object head
		PPAGEHEAD	pNext = pPageAfter->pPageHead->next;

		if(pPageAfter != this){
			K2H_Delete(pPageAfter);
		}
		if(0UL == remlength){
			break;
		}
		if(!pNext){
			ERR_K2HPRN("Failed to set data, because buffer does not exist.");
			return false;
		}
		pPageAfter = new K2HPageFile(pK2HShm, PageFd, reinterpret_cast<off_t>(pNext), true);	// [notice] specified this object's fd.
	}

	// set inner buffer
	if(isSetLoadedData && 0UL < length){
		if(NULL == AppendData(byData, length)){
			ERR_K2HPRN("Failed to copy loaded data buffer.");
			return false;
		}
	}
	return true;
}

//
// This method is for iostream, and always clear inner data buffer.
// Returns last page object, if returned object is same as this, must not free it.
//
K2HPage* K2HPageFile::SetData(const unsigned char* byData, size_t length, off_t offset, bool& isChangeSize, off_t& next_write_pos)
{
	if(!byData || 0UL == length){
		ERR_K2HPRN("Parameters are wrong.");
		return NULL;
	}
	if(!pK2HShm || -1 == PageFd || 0L == FileOffset){
		ERR_K2HPRN("K2HShm or descriptor, or file offset is wrong.");
		return NULL;
	}

	// Clear loaded data
	CleanLoadedData();

	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head");
		return NULL;
	}

	// loop
	K2HPageFile*	pCurrentPage;
	PPAGEHEAD		pCurPageHead;
	for(pCurrentPage = this, pCurPageHead = pCurrentPage->pPageHead; pCurPageHead; pCurPageHead = pCurrentPage->pPageHead){
		// set next writing position from current object
		next_write_pos = offset + length;

		// set data to current page
		if((PageSize - PAGEHEAD_SIZE) <= static_cast<size_t>(offset)){
			// offset is over current page maximum length.
			if(pCurPageHead->length < (PageSize - PAGEHEAD_SIZE)){
				// if current page length is small by max length, grown area to maximum.
				if(!pCurrentPage->SetPageHead(SETHEAD_LENGTH, NULL, NULL, (PageSize - PAGEHEAD_SIZE))){
					ERR_K2HPRN("Could not upload page head.");
					if(this != pCurrentPage){
						K2H_Delete(pCurrentPage);
					}
					return NULL;
				}
				isChangeSize = true;

				// [NOTICE]
				// does not initialize page data
			}
			offset -= (PageSize - PAGEHEAD_SIZE);

		}else{
			// page has specified area.
			size_t	current_length = min(((PageSize - PAGEHEAD_SIZE) - offset), length);

			if(pCurPageHead->length < (current_length + offset)){
				// current page length is needed to change.
				if(!pCurrentPage->SetPageHead(SETHEAD_LENGTH, NULL, NULL, (current_length + offset))){
					ERR_K2HPRN("Could not upload page head.");
					if(this != pCurrentPage){
						K2H_Delete(pCurrentPage);
					}
					return NULL;
				}
				isChangeSize = true;

				// [NOTICE]
				// does not initialize page data
			}

			// set data from offset and length
			if(-1 == k2h_pwrite(pCurrentPage->PageFd, byData, current_length, pCurrentPage->FileOffset + PAGEHEAD_DATA_OFFSET + offset)){
				ERR_K2HPRN("Failed to write page data to fd(%d:%jd), errno = %d", pCurrentPage->PageFd, static_cast<intmax_t>(pCurrentPage->FileOffset + PAGEHEAD_DATA_OFFSET + offset), errno);
				if(this != pCurrentPage){
					K2H_Delete(pCurrentPage);
				}
				return NULL;
			}
			length	-= current_length;
			byData	= ADDPTR(byData, current_length);
			offset	= 0L;
		}

		if(0L == length){
			// finished.
			break;
		}

		if(!(pCurPageHead->next)){
			// expand for next object
			if(!pK2HShm->AddPages(pCurrentPage, length)){
				ERR_K2HPRN("Failed to expand(add) page objects");
				if(this != pCurrentPage){
					K2H_Delete(pCurrentPage);
				}
				return NULL;
			}
			pCurrentPage->CleanPageHead();
			if(!pCurrentPage->LoadPageHead() || !pCurrentPage->pPageHead || !pCurrentPage->pPageHead->next){
				ERR_K2HPRN("Could not reload head");
				if(this != pCurrentPage){
					K2H_Delete(pCurrentPage);
				}
				return NULL;
			}
			// force reset page head pointer.
			pCurPageHead = pCurrentPage->pPageHead;
			isChangeSize = true;
		}

		// make new page object
		if(this != pCurrentPage){
			K2H_Delete(pCurrentPage);
		}
		pCurrentPage = new K2HPageFile(pK2HShm, PageFd, reinterpret_cast<off_t>(pPageHead->next), true);

		if(!pCurrentPage->LoadPageHead() || !pCurrentPage->pPageHead){
			ERR_K2HPRN("Failed to read page head");
			K2H_Delete(pCurrentPage);
			return NULL;
		}
	}

	// if pCurrentPage is temporary object, so duplicate fd because return it
	if(this != pCurrentPage){
		if(!pCurrentPage->DuplicateFd(PageFd)){
			ERR_K2HPRN("Could not duplicate file descriptor(%d), errno = %d", PageFd, errno);
			K2H_Delete(pCurrentPage);
			return NULL;
		}
	}

	return pCurrentPage;
}

//
// [NOTICE]
// This method only truncate length, so that new pages and area is not initialized.
//
bool K2HPageFile::Truncate(size_t length)
{
	if(-1 == PageFd || 0UL == PageSize || 0UL == length){
		ERR_K2HPRN("There is no file descriptor, or pagesize is wrong, or parameter is wrong.");
		return false;
	}

	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head from fd(%d)", PageFd);
		return false;
	}

	if(length < pPageHead->length){
		// to short length
		if(!SetPageHead(SETHEAD_LENGTH, NULL, NULL, length)){
			ERR_K2HPRN("Could not upload page head.");
			return false;
		}
		if(pPageHead->next){
			// free next pages(make temporary object)
			K2HPageFile	PageAfter(pK2HShm, PageFd, reinterpret_cast<off_t>(pPageHead->next), true);

			if(!PageAfter.Free(NULL, NULL, true)){
				ERR_K2HPRN("Could not free pages.");
				return false;
			}
		}
	}else if(pPageHead->length < length){
		// to large length
		if(pPageHead->length < (PageSize - PAGEHEAD_SIZE)){
			size_t	one_length = min(length, (PageSize - PAGEHEAD_SIZE));
			if(!SetPageHead(SETHEAD_LENGTH, NULL, NULL, one_length)){
				ERR_K2HPRN("Could not upload page head.");
				return false;
			}
			length -= one_length;
		}
		if(0UL < length){
			// over this page, need next page
			if(!pPageHead->next){
				// expand next
				if(!pK2HShm->AddPages(this, length)){
					ERR_K2HPRN("Failed to expand(add) page objects");
					return false;
				}
				CleanPageHead();
				if(!LoadPageHead() || !pPageHead || !pPageHead->next){
					ERR_K2HPRN("Could not reload head");
					return false;
				}
			}

			for(PPAGEHEAD pNext = pPageHead->next, pTmpNext = NULL; pNext && 0UL < length; pNext = pTmpNext){
				// make temporary object
				K2HPageFile	PageAfter(pK2HShm, PageFd, reinterpret_cast<off_t>(pNext), true);

				size_t		one_length = min(length, (PageSize - PAGEHEAD_SIZE));

				if(!PageAfter.SetPageHead(SETHEAD_LENGTH, NULL, NULL, one_length)){
					ERR_K2HPRN("Could not upload page head.");
					return false;
				}
				length	-= one_length;
				pTmpNext = PageAfter.pPageHead->next;
			}
			if(0UL < length){
				ERR_K2HPRN("There are not enough page objects.");
				return false;
			}
		}
	}
	return true;
}

PPAGEHEAD K2HPageFile::GetPageHeadRelAddress(void) const
{
	if(-1 == PageFd){
		ERR_K2HPRN("There is no file descriptor, so this object is not initialized.");
		return NULL;
	}
	return reinterpret_cast<PPAGEHEAD>(FileOffset);
}

PPAGEHEAD K2HPageFile::GetLastPageHead(unsigned long* pagecount, bool isAllPage)
{
	if(!pK2HShm || -1 == PageFd || 0L == FileOffset){
		ERR_K2HPRN("pK2HShm or file descriptor, or file offset is wrong.");
		return NULL;
	}
	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head from fd(%d)", PageFd);
		return NULL;
	}
	if(pagecount){
		(*pagecount)++;
	}
	PPAGEHEAD	pReturn = reinterpret_cast<PPAGEHEAD>(FileOffset);

	// next pages
	if(isAllPage && pPageHead->next){
		for(PPAGEHEAD pNext = pPageHead->next, pTmpNext = NULL; pNext; pNext = pTmpNext){
			// make temporary object
			K2HPageFile	PageAfter(pK2HShm, PageFd, reinterpret_cast<off_t>(pNext), true);

			if(NULL == (pReturn = PageAfter.GetLastPageHead(pagecount, false))){
				return NULL;
			}
			if(!(PageAfter.pPageHead->next)){
				break;
			}
			pTmpNext = PageAfter.pPageHead->next;
		}
	}
	return pReturn;
}

bool K2HPageFile::Free(PPAGEHEAD* ppRelLastPageHead, unsigned long* pPageCount, bool isAllPage)
{
	if(!pK2HShm || -1 == PageFd || 0L == FileOffset){
		WAN_K2HPRN("pK2HShm or file descriptor, or file offset is wrong.");
		return true;
	}

	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head from fd(%d)", PageFd);
		return false;
	}

	// Free Page count
	unsigned long LocalPageCount = 1UL;
	if(!pPageCount){
		pPageCount = &LocalPageCount;
	}else{
		(*pPageCount)++;
	}

	// set own length.
	if(!SetPageHead(SETHEAD_LENGTH, NULL, NULL, 0UL)){
		WAN_K2HPRN("Failed to clear length, but continue...");
	}

	PPAGEHEAD	pRelPageHead	= reinterpret_cast<PPAGEHEAD>(FileOffset);
	PPAGEHEAD	pRelLastPageHead= reinterpret_cast<PPAGEHEAD>(FileOffset);

	// Free next pages
	if(isAllPage && pPageHead->next){
		for(PPAGEHEAD pNext = pPageHead->next, pTmpNext = NULL; pNext; pNext = pTmpNext){
			// make temporary object
			K2HPageFile	PageAfter(pK2HShm, PageFd, reinterpret_cast<off_t>(pNext), true);

			if(!PageAfter.Free(&pRelLastPageHead, pPageCount, false)){
				WAN_K2HPRN("Failed to free pages, but continue...");
			}
			if(!(PageAfter.pPageHead->next)){
				break;
			}
			pTmpNext = PageAfter.pPageHead->next;
		}
	}
	if(ppRelLastPageHead){
		*ppRelLastPageHead = pRelLastPageHead;
	}

	// Re-chain into free list
	if(!ppRelLastPageHead){
		if(!pK2HShm->PutBackPages(pRelPageHead, pRelLastPageHead, *pPageCount)){
			ERR_K2HPRN("Failed to replace free page head.");
			return false;
		}
	}
	return true;
}

const unsigned char* K2HPageFile::GetData(const K2HShm* pk2hshm, int fd, off_t offset, size_t* pLength)
{
	if(!Initialize(pk2hshm, fd, offset)){
		return NULL;
	}
	if(!isLoaded){
		if(!LoadData()){
			return NULL;
		}
	}
	if(pLength){
		*pLength = DataLength;
	}
	return pPageData;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
