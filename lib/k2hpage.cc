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
#include <assert.h>

#include "k2hcommon.h"
#include "k2hpage.h"
#include "k2hshm.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Constructor / Destructor
//---------------------------------------------------------
K2HPage::K2HPage(const K2HShm* pk2hshm) : pK2HShm(pk2hshm), pPageHead(NULL), pPageData(NULL), AllocLength(0), DataLength(0), PageSize(0UL), isHeadLoaded(false), isLoaded(false)
{
	if(pk2hshm){
		PageSize = pk2hshm->GetPageSize();
	}
}

K2HPage::~K2HPage()
{
	K2HPage::Clean();
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
bool K2HPage::Initialize(const K2HShm* pk2hshm)
{
	if(!pk2hshm){
		WAN_K2HPRN("K2HShm is NULL.");
		return false;
	}
	CleanMemory();
	pK2HShm		= pk2hshm;
	PageSize	= pk2hshm->GetPageSize();
	return true;
}

void K2HPage::Clean(void)
{
	CleanMemory();
}

void K2HPage::CleanMemory(void)
{
	CleanLoadedData();
	CleanPageHead();
}

void K2HPage::CleanPageHead(void)
{
	pPageHead	= NULL;
	isHeadLoaded= false;
}

void K2HPage::CleanLoadedData(void)
{
	K2H_Free(pPageData);
	pPageData	= NULL;
	AllocLength = 0;
	DataLength	= 0;
	isLoaded	= false;
}

size_t K2HPage::SetPageSize(size_t page_size)
{
	assert(0UL != page_size);

	size_t	old = PageSize;
	PageSize = page_size;
	return old;
}

unsigned char* K2HPage::AllocateData(ssize_t length)
{
	if(!pK2HShm || 0 == length){
		return NULL;
	}
	CleanLoadedData();

	if(NULL == (pPageData = static_cast<unsigned char*>(malloc(length)))){
		ERR_K2HPRN("Could not allocation memory.");
		// cppcheck-suppress unmatchedSuppression
		// cppcheck-suppress memleak
		return NULL;
	}
	AllocLength	= length;

	return pPageData;
}

unsigned char* K2HPage::AppendData(const unsigned char* pAppend, ssize_t length)
{
	if(!pK2HShm){
		return NULL;
	}
	if(!pPageData || AllocLength < (DataLength + length)){
		// reallocation
		unsigned char*	pTmp;
		size_t			NewLength = (((DataLength + length) / PageSize) + 1) * PageSize;
		if(NULL == (pTmp = static_cast<unsigned char*>(realloc(pPageData, NewLength)))){
			ERR_K2HPRN("Could not allocation memory.");
			return NULL;
		}
		pPageData	= pTmp;
		AllocLength = NewLength;
	}
	memcpy(&pPageData[DataLength], pAppend, length);
	DataLength += length;

	return pPageData;
}

bool K2HPage::SetPageHead(int type, PPAGEHEAD prev, PPAGEHEAD next, size_t length)
{
	if(!isHeadLoaded){
		if(!LoadPageHead()){
			ERR_K2HPRN("Could not load page head");
			return false;
		}
	}
	// overwrite header
	if(SETHEAD_PREV == (type & SETHEAD_PREV)){
		pPageHead->prev		= prev;
	}
	if(SETHEAD_NEXT == (type & SETHEAD_NEXT)){
		pPageHead->next		= next;
	}
	if(SETHEAD_LENGTH == (type & SETHEAD_LENGTH)){
		pPageHead->length	= length;
	}
	if(!UploadPageHead()){
		ERR_K2HPRN("Could not upload page head");
		isHeadLoaded = false;
		return false;
	}
	return true;
}

bool K2HPage::SetPageHead(PPAGEHEAD ppagehead)
{
	if(!ppagehead){
		ERR_K2HPRN("PPAGEHEAD is NULL");
		return false;
	}
	if(!isHeadLoaded){
		if(!LoadPageHead()){
			ERR_K2HPRN("Could not load page head");
			return false;
		}
	}
	// overwrite header
	pPageHead->prev		= ppagehead->prev;
	pPageHead->next		= ppagehead->next;
	pPageHead->length	= ppagehead->length;
	if(!UploadPageHead()){
		ERR_K2HPRN("Could not upload page head");
		isHeadLoaded = false;
		return false;
	}
	return true;
}

bool K2HPage::GetPageHead(PPAGEHEAD ppagehead)
{
	if(!ppagehead){
		ERR_K2HPRN("PPAGEHEAD is NULL");
		return false;
	}
	if(!isHeadLoaded){
		if(!LoadPageHead()){
			ERR_K2HPRN("Could not load page head");
			return false;
		}
	}
	ppagehead->prev		= pPageHead->prev;
	ppagehead->next		= pPageHead->next;
	ppagehead->length	= pPageHead->length;
	return true;
}

bool K2HPage::CopyData(unsigned char** ppPageData, size_t* pLength) const
{
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return false;
	}
	if(NULL == ppPageData || 0 == DataLength){
		return false;
	}
	if(NULL == (*ppPageData = static_cast<unsigned char*>(malloc(sizeof(unsigned char) * DataLength)))){
		ERR_K2HPRN("Could not allocation memory.");
		// cppcheck-suppress unmatchedSuppression
		// cppcheck-suppress memleak
		return false;
	}
	memcpy(*ppPageData, pPageData, DataLength);
	*pLength = DataLength;

	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress memleak
	return true;
}

bool K2HPage::GetData(const unsigned char** ppPageData, size_t* pLength) const
{
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return false;
	}
	if(NULL == ppPageData){
		return false;
	}
	*ppPageData = pPageData;
	if(pLength){
		*pLength = DataLength;
	}
	return true;
}

const unsigned char* K2HPage::GetData(size_t* pLength) const
{
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return NULL;
	}
	if(pLength){
		*pLength = DataLength;
	}
	return pPageData;
}

const char* K2HPage::GetString(size_t* pLength) const
{
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return NULL;
	}
	if(pLength){
		*pLength = DataLength;
	}
	return reinterpret_cast<const char*>(pPageData);
}

strarr_t::size_type K2HPage::ParseStringArray(strarr_t& strarr) const
{
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return 0;
	}
	return ::ParseStringArray(reinterpret_cast<const char*>(pPageData), DataLength, strarr);
}

K2HSubKeys* K2HPage::GetSubKeys(void) const
{
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return 0;
	}
	K2HSubKeys*	pResult = new K2HSubKeys();
	if(!pResult->Serialize(pPageData, DataLength)){
		ERR_K2HPRN("Could not convert binary to K2HSubKeys object.");
		K2H_Delete(pResult);
	}
	return pResult;
}

size_t K2HPage::GetSubKeys(K2HSubKeys& SubKeys) const
{
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return 0;
	}
	if(!SubKeys.Serialize(pPageData, DataLength)){
		ERR_K2HPRN("Could not convert binary to K2HSubKeys object.");
		SubKeys.clear();
	}
	return SubKeys.size();
}

K2HAttrs* K2HPage::GetAttrs(void) const
{
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return 0;
	}
	K2HAttrs*	pResult = new K2HAttrs();
	if(!pResult->Serialize(pPageData, DataLength)){
		ERR_K2HPRN("Could not convert binary to K2HAttrs object.");
		K2H_Delete(pResult);
	}
	return pResult;
}

size_t K2HPage::GetAttrs(K2HAttrs& Attrs) const
{
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return 0;
	}
	if(!Attrs.Serialize(pPageData, DataLength)){
		ERR_K2HPRN("Could not convert binary to K2HAttrs object.");
		Attrs.clear();
	}
	return Attrs.size();
}

bool K2HPage::Compare(const unsigned char* pData, size_t length) const
{
	if(!pData || 0 == length){
		return false;
	}
	if(!isLoaded){
		WAN_K2HPRN("Object is not initializing(loading).");
		return false;
	}
	if(length != DataLength || !pPageData){
		return false;
	}
	return (0 == memcmp(pPageData, pData, DataLength) ? true : false);
}

inline bool K2HPage::Compare(const char* pData) const
{
	return Compare(reinterpret_cast<const unsigned char*>(pData), strlen(pData));
}

inline bool K2HPage::operator==(const char* pData) const
{
	return Compare(pData);
}

inline bool K2HPage::operator!=(const char* pData) const
{
	return (Compare(pData) ? false : true);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
