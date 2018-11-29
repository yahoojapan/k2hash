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
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>

#include "k2hcommon.h"
#include "k2hshm.h"
#include "k2hpage.h"
#include "k2hpagefile.h"
#include "k2hpagemem.h"
#include "k2hshmupdater.h"
#include "k2hashfunc.h"
#include "k2htransfunc.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Symbols / Macros
//---------------------------------------------------------
#define	DUMP_SPACERMACRO_BUFF_SIZE		64
#define	DUMP_NAME_CHAR_COUNT			24
#define	DUMP_SPACER_RAITO				2

static inline void DUMP_SPACER_PRINT(FILE* stream, int nest)
{
	for(int cnt = nest * DUMP_SPACER_RAITO; 0 < cnt; cnt--){
		fprintf(stream, " ");
	}
}

static inline void DUMP_LOWPRINT(FILE* stream, int nest, const char* format, ...)
{
	if(format){
		for(int cnt = nest * DUMP_SPACER_RAITO; 0 < cnt; cnt--){
			fprintf(stream, " ");
		}
		va_list ap;
		va_start(ap, format);
		vfprintf(stream, format, ap); 
		va_end(ap);
	}
}

static inline void DUMP_PRINT_NV(FILE* stream, int nest, const char* name_format, const void* name, const char* value_format, ...)
{
	char	buff[DUMP_SPACERMACRO_BUFF_SIZE];
	buff[0] = '\0';

	if(NULL != name_format){
		sprintf(&buff[0], name_format, name);
	}
	for(int cnt = DUMP_NAME_CHAR_COUNT - strlen(&buff[0]); 0 < cnt; cnt--){
		strcat(&buff[0], " ");
	}
	for(int cnt = nest * DUMP_SPACER_RAITO; 0 < cnt; cnt--){
		fprintf(stream, " ");
	}
	fprintf(stream, "%s", &buff[0]);

	va_list ap;
	va_start(ap, value_format);
	vfprintf(stream, value_format, ap); 
	va_end(ap);
}

static inline void CVT_BINARY_TO_PRINTABLE_STRING(string& output, const unsigned char* bydata, size_t datalen)
{
	output.erase();

	if(!bydata || 0 == datalen){
		return;
	}
	for(size_t pos = 0; pos < datalen; pos++){
		if(0 != isprint(bydata[pos])){
			output += static_cast<const char>(bydata[pos]);
		}else{
			output += static_cast<const char>(0xFF);
		}
	}
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
bool K2HShm::Dump(FILE* stream, int dumpmask) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(const_cast<K2HShm*>(this));

	int		nCnt;
	int		nest = 0;
	char	szTime[32];
	string	strTmp;

	{
		K2HLock	ALObjCMask(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RDLOCK);			// LOCK

		// [NOTICE]
		// "last_update" is not locking now.
		// K2HLock	ALObjAUpdate(ShmFd, Rel(&(pHead->last_update)), K2HLock::RDLOCK);	// LOCK

		DUMP_PRINT_NV(stream, nest, "K2H(%p)", reinterpret_cast<void*>(pHead), "= {\n");
		nest++;
		DUMP_PRINT_NV(stream, nest, "Version",			NULL, "= %s\n",				pHead->version);
		DUMP_PRINT_NV(stream, nest, "Hash func Version",NULL, "= %s\n",				pHead->hash_version);
		DUMP_PRINT_NV(stream, nest, "Toal Size",		NULL, "= %zu\n",			pHead->total_size);
		DUMP_PRINT_NV(stream, nest, "Page Size",		NULL, "= %zu\n",		 	pHead->page_size);
		DUMP_PRINT_NV(stream, nest, "Muximum Mask",		NULL, "= %p(%" PRIx64 ")\n",reinterpret_cast<void*>(pHead->max_mask), pHead->max_mask);
		DUMP_PRINT_NV(stream, nest, "Minimum Mask",		NULL, "= %p(%" PRIx64 ")\n",reinterpret_cast<void*>(pHead->min_mask), pHead->min_mask);
		DUMP_PRINT_NV(stream, nest, "Current Mask",		NULL, "= %p(%" PRIx64 ")\n",reinterpret_cast<void*>(pHead->cur_mask), pHead->cur_mask);
		DUMP_PRINT_NV(stream, nest, "Collision Mask",	NULL, "= %p(%" PRIx64 ")\n",reinterpret_cast<void*>(pHead->collision_mask), pHead->collision_mask);
		DUMP_PRINT_NV(stream, nest, "Max element cnt",	NULL, "= %p(%lu)\n",		reinterpret_cast<void*>(pHead->max_element_count), pHead->max_element_count);
		DUMP_LOWPRINT(stream, nest, "Last update {\n");
		nest++;
		DUMP_PRINT_NV(stream, nest, "Date",				NULL, "= %s",				ctime_r(&(pHead->last_update.tv_sec), szTime));
		DUMP_PRINT_NV(stream, nest, "sec",				NULL, "= %jd\n",			static_cast<intmax_t>(pHead->last_update.tv_sec));
		DUMP_PRINT_NV(stream, nest, "usec",				NULL, "= %jd\n",			static_cast<intmax_t>(pHead->last_update.tv_usec));
		nest--;
		DUMP_LOWPRINT(stream, nest, "}\n");
		DUMP_LOWPRINT(stream, nest, "Last area update {\n");
		nest++;
		DUMP_PRINT_NV(stream, nest, "Date",				NULL, "= %s",				ctime_r(&(pHead->last_area_update.tv_sec), szTime));
		DUMP_PRINT_NV(stream, nest, "sec",				NULL, "= %jd\n",			static_cast<intmax_t>(pHead->last_area_update.tv_sec));
		DUMP_PRINT_NV(stream, nest, "usec",				NULL, "= %jd\n",			static_cast<intmax_t>(pHead->last_area_update.tv_usec));
		nest--;
		DUMP_LOWPRINT(stream, nest, "}\n");
	}

	if(DUMP_HEAD == dumpmask){	// only head
		return true;
	}

	// Area
	DUMP_LOWPRINT(stream, nest, "K2H Areas Array {\n");
	nest++;
	for(nCnt = 0; nCnt < MAX_K2HAREA_COUNT; nCnt++){
		strTmp.erase();

		if(K2H_AREA_UNKNOWN == pHead->areas[nCnt].type){
			// finish
			break;
		}
		if(K2H_AREA_K2H == (pHead->areas[nCnt].type & K2H_AREA_K2H)){
			if(!strTmp.empty()){
				strTmp += ",";
			}
			strTmp += "H2H Header";
		}
		if(K2H_AREA_KINDEX == (pHead->areas[nCnt].type & K2H_AREA_KINDEX)){
			if(!strTmp.empty()){
				strTmp += ",";
			}
			strTmp += "Key Index";
		}
		if(K2H_AREA_CKINDEX == (pHead->areas[nCnt].type & K2H_AREA_CKINDEX)){
			if(!strTmp.empty()){
				strTmp += ",";
			}
			strTmp += "Collision Key Index";
		}
		if(K2H_AREA_PAGELIST == (pHead->areas[nCnt].type & K2H_AREA_PAGELIST)){
			if(!strTmp.empty()){
				strTmp += ",";
			}
			strTmp += "Element";
		}
		if(K2H_AREA_PAGE == (pHead->areas[nCnt].type & K2H_AREA_PAGE)){
			if(!strTmp.empty()){
				strTmp += ",";
			}
			strTmp += "Pages";
		}
		DUMP_LOWPRINT(stream, nest, "[%d] = {\n",	nCnt);
		nest++;
		DUMP_PRINT_NV(stream, nest, "type",		NULL, "= %s\n",			strTmp.c_str());
		DUMP_PRINT_NV(stream, nest, "offset",	NULL, "= %p(%jd)\n",	reinterpret_cast<void*>(pHead->areas[nCnt].file_offset), static_cast<intmax_t>(pHead->areas[nCnt].file_offset));
		DUMP_PRINT_NV(stream, nest, "length",	NULL, "= %p(%zu)\n",	reinterpret_cast<void*>(pHead->areas[nCnt].length), pHead->areas[nCnt].length);
		nest--;
		DUMP_LOWPRINT(stream, nest, "}\n");
	}
	nest--;
	DUMP_LOWPRINT(stream, nest, "}\n");

	// Not Assigned area
	{
		K2HLock	ALObjUnArea(ShmFd, Rel(&(pHead->unassign_area)), K2HLock::RDLOCK);	// LOCK

		DUMP_PRINT_NV(stream, nest, "Unassaigned area", NULL, "%p(%jd)\n",	reinterpret_cast<void*>(pHead->unassign_area), static_cast<intmax_t>(pHead->unassign_area));
	}

	// Extra pointer
	DUMP_PRINT_NV(stream, nest, "Extra pointer", NULL, "%p(%lu)\n", pHead->pextra, reinterpret_cast<unsigned long>(pHead->pextra));

	// Free Element area
	{
		K2HLock	ALObjFEC(ShmFd, Rel(&(pHead->free_element_count)), K2HLock::RDLOCK);	// LOCK

		if(!DumpFreeElements(stream, nest, pHead->pfree_elements, pHead->free_element_count)){
			ERR_K2HPRN("Somthing error occurred in dumping Free Elements.");
			return false;
		}
	}

	// Free Page area
	{
		K2HLock	ALObjFPC(ShmFd, Rel(&(pHead->free_page_count)), K2HLock::RDLOCK);		// LOCK

		if(!DumpFreePages(stream, nest, pHead->pfree_pages, pHead->free_page_count)){
			ERR_K2HPRN("Somthing error occurred in dumping Free Pages.");
			return false;
		}
	}

	// Key Index
	if(!DumpKeyIndex(stream, nest, &(pHead->key_index_area[0]), K2HShm::GetMaskBitCount(pHead->collision_mask), dumpmask)){
		ERR_K2HPRN("Somthing error occurred in dumping Key Index array.");
		return false;
	}

	return true;
}

bool K2HShm::DumpFreeElements(FILE* stream, int nest, PELEMENT pRelElements, long count) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!pRelElements){
		if(0 != count){
			WAN_K2HPRN("PELEMENT is null, but count(%ld) is not Zero.", count);
		}
		DUMP_LOWPRINT(stream, nest, "Free Element Area = [%ld] {\n", count);
		DUMP_LOWPRINT(stream, nest, "} = [%ld]\n", count);
		return true;
	}

	int			nCnt;
	char		szBuff[128];
	PELEMENT	pElement;

	// Free Element area
	DUMP_LOWPRINT(stream, nest, "Free Element Area = [%ld] {\n", count);
	nest++;
	for(nCnt = 0, pElement = static_cast<PELEMENT>(Abs(pRelElements)); pElement; pElement = static_cast<PELEMENT>(Abs(pElement->same)), nCnt++){
		if(0 == (nCnt % 5)){
			szBuff[0] = '\0';
		}
		sprintf(&szBuff[strlen(szBuff)], "%p ", pElement);
		if(4 == (nCnt % 5)){
			DUMP_PRINT_NV(stream, nest, NULL, NULL, "%s\n",	szBuff);
		}
	}
	if(0 != nCnt && 0 != (nCnt % 5)){
		DUMP_PRINT_NV(stream, nest, NULL, NULL, "%s\n",	szBuff);
	}
	nest--;
	DUMP_LOWPRINT(stream, nest, "} = [%d]\n", nCnt);

	return true;
}

bool K2HShm::DumpFreePages(FILE* stream, int nest, PPAGEHEAD pRelPages, long count) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!pRelPages){
		if(0 != count){
			WAN_K2HPRN("PPAGEHEAD is null, but count(%ld) is not Zero.", count);
		}
		DUMP_LOWPRINT(stream, nest, "Free Pages Area = [%ld] {\n", count);
		DUMP_LOWPRINT(stream, nest, "} = [%ld]\n", count);
		return true;
	}
	int			nCnt;
	char		szBuff[128];
	PPAGEHEAD	pPage;
	PPAGEHEAD	pNextPage = NULL;

	// Free pagehead areas
	DUMP_LOWPRINT(stream, nest, "Free Pages Area = [%ld] {\n", count);
	nest++;
	for(nCnt = 0, pPage = static_cast<PPAGEHEAD>(Abs(pRelPages)); pPage; pPage = pNextPage, nCnt++){
		if(0 == (nCnt % 5)){
			szBuff[0] = '\0';
		}
		sprintf(&szBuff[strlen(szBuff)], "%p ", pPage);
		if(4 == (nCnt % 5)){
			DUMP_PRINT_NV(stream, nest, NULL, NULL, "%s\n",	szBuff);
		}
		// make next pointer
		if(isFullMapping){
			pNextPage = static_cast<PPAGEHEAD>(Abs(pPage->next));
		}else{
			PAGEWRAP	pagewrap;
			if(-1 == k2h_pread(ShmFd, &(pagewrap.barray[0]), PAGEHEAD_SIZE, reinterpret_cast<off_t>(pPage))){
				ERR_K2HPRN("Failed to read page head from fd(%d:%jd), errno = %d", ShmFd, static_cast<intmax_t>(reinterpret_cast<off_t>(pPage)), errno);
				return false;
			}
			pNextPage = static_cast<PPAGEHEAD>(Abs(pagewrap.pagehead.next));
		}
	}
	if(0 != nCnt && 0 != (nCnt % 5)){
		DUMP_PRINT_NV(stream, nest, NULL, NULL, "%s\n",	szBuff);
	}
	nest--;
	DUMP_LOWPRINT(stream, nest, "} = [%d]\n", nCnt);

	return true;
}

bool K2HShm::DumpKeyIndex(FILE* stream, int nest, PKINDEX* ppKeyIndex, int cmask_bitcnt, int dumpmask) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!ppKeyIndex){
		ERR_K2HPRN("PKINDEX is null.");
		return false;
	}
	if(DUMP_RAW_KINDEX_ARRAY != (DUMP_RAW_KINDEX_ARRAY & dumpmask)){
		return true;
	}

	// Key Index Array
	DUMP_LOWPRINT(stream, nest, "Key Index = [%d] {\n", MAX_KINDEX_AREA_COUNT);
	nest++;
	for(int kindex_pos = 0; kindex_pos < MAX_KINDEX_AREA_COUNT; kindex_pos++){
		// key_index_area[kindex_pos]
		PKINDEX	pKeyIndex	= static_cast<PKINDEX>(Abs(ppKeyIndex[kindex_pos]));
		int		kindex_count= 1 << (0 < kindex_pos ? kindex_pos - 1 : kindex_pos);	// notice: if kindex_pos is 0 or 1, kindex_count is 1, but it is OK!

		if(!pKeyIndex){
			DUMP_LOWPRINT(stream, nest, "Key Index Pointer[%d] = Not Allocated\n", kindex_pos);
		}else{
			for(int nCnt = 0; nCnt < kindex_count; nCnt++){
				// key_index_area[kindex_pos][nCnt]
				DUMP_LOWPRINT(stream, nest, "Key Index Pointer[%d][%d] = {\n", kindex_pos, nCnt);
				nest++;

				DUMP_PRINT_NV(stream, nest, "Assign",		NULL, "= %s\n",					KINDEX_ASSIGNED == pKeyIndex[nCnt].assign ? "Assigned" : "Not assigned");
				DUMP_PRINT_NV(stream, nest, "Shifted Mask",	NULL, "= %p(0x%" PRIx64 ")\n",	reinterpret_cast<void*>(pKeyIndex[nCnt].shifted_mask), pKeyIndex[nCnt].shifted_mask);
				DUMP_PRINT_NV(stream, nest, "Masked Hash",	NULL, "= %p(0x%" PRIx64 ")\n",	reinterpret_cast<void*>(pKeyIndex[nCnt].masked_hash), pKeyIndex[nCnt].masked_hash);

				// Collision Key Index
				if(!DumpCKeyIndex(stream, nest, pKeyIndex[nCnt].ckey_list, cmask_bitcnt, dumpmask)){
					ERR_K2HPRN("Something error occured in printing for Collision Key Index.");
					return false;
				}

				nest--;
				DUMP_LOWPRINT(stream, nest, "}\n");
			}
		}
	}
	nest--;
	DUMP_LOWPRINT(stream, nest, "}\n");

	return true;
}

bool K2HShm::DumpCKeyIndex(FILE* stream, int nest, PCKINDEX pRelCKeyIndex, int cmask_bitcnt, int dumpmask) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!pRelCKeyIndex){
		ERR_K2HPRN("PCKINDEX is null.");
		return false;
	}
	PCKINDEX pCKeyIndex = static_cast<PCKINDEX>(Abs(pRelCKeyIndex));

	if(DUMP_RAW_CKINDEX_ARRAY != (DUMP_RAW_CKINDEX_ARRAY & dumpmask)){
		// print only address
		DUMP_PRINT_NV(stream, nest, "CKey Index[%ld]", reinterpret_cast<void*>(CKINDEX_BYKINDEX_CNT(cmask_bitcnt)), "= %p(%jd)\n", reinterpret_cast<void*>(pCKeyIndex), static_cast<intmax_t>(reinterpret_cast<off_t>(pCKeyIndex)));

	}else{
		int	nCnt;
		int	element_count;

		// Collision Key Index
		DUMP_LOWPRINT(stream, nest, "CKey Index = [%ld] {\n", CKINDEX_BYKINDEX_CNT(cmask_bitcnt));
		nest++;
		for(nCnt = 0, element_count = 0; nCnt < CKINDEX_BYKINDEX_CNT(cmask_bitcnt); nCnt++, element_count = 0){
			DUMP_LOWPRINT(stream, nest, "[%d] = {\n",	nCnt);
			nest++;
			DUMP_PRINT_NV(stream, nest, "element count", NULL, "= %p(%lu)\n", reinterpret_cast<void*>(pCKeyIndex[nCnt].element_count), pCKeyIndex[nCnt].element_count);

			// Element
			DUMP_LOWPRINT(stream, nest, "element_list {\n");
			nest++;

			if(pCKeyIndex[nCnt].element_list){
				if(!DumpElement(stream, nest, pCKeyIndex[nCnt].element_list, dumpmask, element_count, NULL)){
					return false;
				}
			}

			nest--;
			DUMP_LOWPRINT(stream, nest, "} = [%d]\n", element_count);

			nest--;
			DUMP_LOWPRINT(stream, nest, "}\n");
		}
		nest--;
		DUMP_LOWPRINT(stream, nest, "} = [%d]\n", nCnt);
	}
	return true;
}

bool K2HShm::DumpElement(FILE* stream, int nest, PELEMENT pRelElement, int dumpmask, int& element_count, string* pstring) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	PELEMENT pElement = static_cast<PELEMENT>(Abs(pRelElement));
	if(!pElement){
		ERR_K2HPRN("PELEMENT is null.");
		return false;
	}

	if(DUMP_RAW_ELEMENT_LIST != (DUMP_RAW_ELEMENT_LIST & dumpmask)){
		// print only address
		bool	isFirst = false;
		bool	isAlloc = false;
		char	szBuff[32];
		if(0 == element_count){
			isFirst = true;
		}else if(0 == element_count % 5){
			pstring->append("\n");
			for(int cnt = nest * DUMP_SPACER_RAITO; 0 < cnt; cnt--){
				pstring->append(" ");
			}
		}
		if(!pstring){
			pstring = new string();
			isAlloc = true;
		}

		sprintf(szBuff, "%p ", pElement);
		pstring->append(szBuff);
		element_count++;

		// same / small / big
		if(	(pElement->same  && !DumpElement(stream, nest, pElement->same, dumpmask, element_count, pstring)) ||
			(pElement->small && !DumpElement(stream, nest, pElement->small, dumpmask, element_count, pstring)) ||
			(pElement->big   && !DumpElement(stream, nest, pElement->big, dumpmask, element_count, pstring)) )
		{
			if(isAlloc){
				delete pstring;
			}
			return false;
		}

		if(isFirst){
			DUMP_PRINT_NV(stream, nest, NULL, NULL, "%s\n",	pstring->c_str());
		}
		if(isAlloc){
			delete pstring;
		}

	}else{
		// print each element detail(rentrant)
		DUMP_LOWPRINT(stream, nest, "[%d] = {\n", element_count++);
		nest++;

		DUMP_PRINT_NV(stream, nest, "hash",		NULL, "= %p(0x%" PRIx64 ")\n",	reinterpret_cast<void*>(pElement->hash), pElement->hash);
		DUMP_PRINT_NV(stream, nest, "subhash",	NULL, "= %p(0x%" PRIx64 ")\n",	reinterpret_cast<void*>(pElement->subhash), pElement->subhash);

		if(	!DumpPageData(stream, nest, pElement->key, "key", dumpmask)			||
			!DumpPageData(stream, nest, pElement->value, "value", dumpmask)		||
			!DumpPageData(stream, nest, pElement->subkeys, "subkeys", dumpmask)	||
			!DumpPageData(stream, nest, pElement->attrs, "attrs", dumpmask)		)
		{
			ERR_K2HPRN("Something error occurred in dumping PELEMENT menbers.");
			return false;
		}

		nest--;
		DUMP_LOWPRINT(stream, nest, "}\n");

		// same / small / big
		if(	(pElement->same  && !DumpElement(stream, nest, pElement->same, dumpmask, element_count, pstring)) ||
			(pElement->small && !DumpElement(stream, nest, pElement->small, dumpmask, element_count, pstring)) ||
			(pElement->big   && !DumpElement(stream, nest, pElement->big, dumpmask, element_count, pstring)) )
		{
			return false;
		}
	}
	return true;
}

bool K2HShm::DumpPageData(FILE* stream, int nest, PPAGEHEAD pRelPageHead, const char* pName, int dumpmask) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!pRelPageHead){
//		MSG_K2HPRN("PPAGEHEAD is null.");
		return true;
	}

	if(DUMP_RAW_PAGE_LIST != (DUMP_RAW_PAGE_LIST & dumpmask)){
		// print only address
		if(isFullMapping){
			PPAGEHEAD pPageHead = static_cast<PPAGEHEAD>(Abs(pRelPageHead));
			DUMP_PRINT_NV(stream, nest, "%s", (pName ? pName : ""), "= %p(%jd)\n", reinterpret_cast<void*>(pPageHead), static_cast<intmax_t>(reinterpret_cast<off_t>(pPageHead)));
		}else{
			DUMP_PRINT_NV(stream, nest, "%s(offset)", (pName ? pName : ""), "= %p(%jd)\n", reinterpret_cast<void*>(pRelPageHead), static_cast<intmax_t>(reinterpret_cast<off_t>(pRelPageHead)));
		}

	}else{
		DUMP_LOWPRINT(stream, nest, "%s = {\n",	(pName ? pName : ""));
		nest++;

		// Dump Array
		// ex:	name = {
		//		  41 42 43 44 45 46 47 48    ABCD EFGH
		//		  49 50 51                   IJK
		//		}
		//
		K2HPage*		pPage;
		unsigned char*	pData;
		size_t			one_length = (GetPageSize() - PAGEHEAD_SIZE) * 8;		// one page data length * 8

		if(NULL == (pPage = GetPageObject(pRelPageHead, false))){
			ERR_K2HPRN("Failed to load PPAGEHEAD data.");
			return false;
		}
		if(NULL == (pData = reinterpret_cast<unsigned char*>(malloc(one_length)))){
			ERR_K2HPRN("Could not allocation memory.");
			K2H_Delete(pPage);
			return false;
		}

		size_t		read_length;
		size_t		total_length;
		K2HPage*	pNextPage;
		off_t		next_offset;
		off_t		offset;
		for(offset = 0L, total_length = 0UL; pPage; pPage = pNextPage, offset = next_offset, total_length += read_length){
			// read data
			read_length	= 0UL;
			next_offset	= 0L;
			if(NULL == (pNextPage = pPage->LoadData(offset, one_length, &pData, read_length, next_offset))){
				ERR_K2HPRN("Failed to read value data with offset.");
				K2H_Delete(pPage);
				K2H_Free(pData);
				return false;
			}
			// output
			{
				char	szBinBuff[64];
				char	szChBuff[32];
				char	szTmpBuff[16];
				size_t	pos;
				size_t	wpos;
				for(pos = 0; pos < read_length; pos++){
					if(0 == (pos % 8)){
						sprintf(szBinBuff, "                            ");	// Base for "00 00 00 00  00 00 00 00    "
						sprintf(szChBuff, "         ");						// Base for "SSSS SSSS"
					}
					wpos = pos % 8;
					sprintf(szTmpBuff, "%02X", pData[pos]);
					memcpy(&szBinBuff[(3 < wpos ? (wpos * 3) + 1 : wpos * 3)], szTmpBuff, 2);
					sprintf(szTmpBuff, "%c", (isprint(pData[pos]) ? pData[pos] : 0x00 == pData[pos] ? ' ' : 0xFF));
					memcpy(&szChBuff[(3 < wpos ? wpos + 1 : wpos)], szTmpBuff, 1);

					if(7 == (pos % 8)){
						DUMP_PRINT_NV(stream, nest, NULL, NULL, "%s - %s\n", szBinBuff, szChBuff);
					}
				}
				if(0 != (pos % 8)){
					DUMP_PRINT_NV(stream, nest, NULL, NULL, "%s - %s\n", szBinBuff, szChBuff);
				}
			}
			// next
			if(pPage == pNextPage && read_length < one_length){
				total_length += read_length;
				break;
			}
			K2H_Delete(pPage);
		}
		K2H_Delete(pPage);
		K2H_Free(pData);

		nest--;
		DUMP_LOWPRINT(stream, nest, "}\n");
	}
	return true;
}

//---------------------------------------------------------
// Dump Queue
//---------------------------------------------------------
bool K2HShm::DumpQueue(FILE* stream, const unsigned char* byMark, size_t marklength)
{
	if(!byMark || 0 == marklength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	// Lock cindex for writing marker.
	k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byMark), marklength);

	K2HLock		ALObjCKI(K2HLock::RWLOCK);			// LOCK
	PCKINDEX	pCKIndex;
	if(NULL == (pCKIndex = GetCKIndex(hash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		return false;
	}

	// Read current marker.
	unsigned char*	pmkval	= NULL;
	ssize_t			mkvallength;
	if(-1 == (mkvallength = Get(byMark, marklength, &pmkval)) || !pmkval){
		// There is no marker
		return true;
	}

	PBK2HMARKER		pmarker = reinterpret_cast<PBK2HMARKER>(pmkval);
	size_t			marklen = static_cast<size_t>(mkvallength);
	// Check current marker size
	if(marklen != (sizeof(K2HMARKER) + pmarker->marker.startlen + pmarker->marker.endlen)){
		ERR_K2HPRN("The marker is not same size which is calculated.");
		K2H_Free(pmkval);
		return false;
	}

	// Dump marker
	string		strtmp;
	CVT_BINARY_TO_PRINTABLE_STRING(strtmp, byMark, marklength);
	DUMP_PRINT_NV(stream, 0, "MARKER(%s)", strtmp.c_str(), "= {\n");

	CVT_BINARY_TO_PRINTABLE_STRING(strtmp, &(pmarker->byData[pmarker->marker.startoff]), pmarker->marker.startlen);
	DUMP_PRINT_NV(stream, 4, "START KEY", NULL, "= %s\n", strtmp.c_str());

	CVT_BINARY_TO_PRINTABLE_STRING(strtmp, &(pmarker->byData[pmarker->marker.endoff]), pmarker->marker.endlen);
	DUMP_PRINT_NV(stream, 4, "END KEY  ", NULL, "= %s\n", strtmp.c_str());
	DUMP_LOWPRINT(stream, 0, "}\n\n");

	if(K2HShm::IsEmptyK2HMarker(pmarker)){
		// There is no queued key in marker
		return true;
	}

	// Loop dumping from start key to end key
	unsigned char*	bykey	= k2hbindup(&(pmarker->byData[pmarker->marker.startoff]), pmarker->marker.startlen);
	size_t			keylen	= pmarker->marker.startlen;
	for(int cnt = 0; bykey && 0 != keylen; cnt++){
		// print
		CVT_BINARY_TO_PRINTABLE_STRING(strtmp, bykey, keylen);
		DUMP_PRINT_NV(stream, 0, " [%d]", reinterpret_cast<const void*>(cnt), "= %s\n", strtmp.c_str());

		if(0 == k2hbincmp(bykey, keylen, &(pmarker->byData[pmarker->marker.endoff]), pmarker->marker.endlen)){
			// reached end key
			K2H_Free(bykey);
			break;
		}

		// Get subkeys.(without checking attribute)
		K2HSubKeys*	psubkeys;
		if(NULL == (psubkeys = GetSubKeys(bykey, keylen, false))){
			// There is no key nor subkeys
			ERR_K2HPRN("The key does not have subkeys.");
			K2H_Free(bykey);
			K2H_Free(pmkval);
			return false;
		}

		K2HSubKeys::iterator iter = psubkeys->begin();
		if(iter == psubkeys->end()){
			// Current key does not have any subkey.
			ERR_K2HPRN("The key does not have subkeys.");
			K2H_Free(bykey);
			K2H_Delete(psubkeys);
			K2H_Free(pmkval);
			return false;
		}

		// set next key
		K2H_Free(bykey);
		bykey	= k2hbindup(iter->pSubKey, iter->length);
		keylen	= iter->length;

		K2H_Delete(psubkeys);
	}
	K2H_Free(bykey);	// for safty

	return true;
}

//---------------------------------------------------------
// Print Usage
//---------------------------------------------------------
bool K2HShm::PrintState(FILE* stream) const
{
	PK2HSTATE	pState = GetState();
	if(!pState){
		ERR_K2HPRN("Something error occurred during getting k2hash state.");
		return false;
	}

	DUMP_PRINT_NV(stream, 0, "Total File size",		NULL, "= %zu byte\n",			pState->file_size);
	DUMP_PRINT_NV(stream, 0, "Used size",			NULL, "= %zu byte\n",			pState->total_used_size);
	DUMP_PRINT_NV(stream, 0, "Total mmap size",		NULL, "= %zu byte\n",			pState->total_map_size);
	DUMP_PRINT_NV(stream, 0, "Assigned areas",		NULL, "= %ld ( total %d )\n\n",	pState->assigned_area_count,		MAX_K2HAREA_COUNT);

	DUMP_PRINT_NV(stream, 0, "Total Key index",		NULL, "= %ld ( %zu byte )\n",	pState->assigned_key_count,			static_cast<size_t>(pState->assigned_key_count * sizeof(KINDEX)));
	DUMP_PRINT_NV(stream, 0, "Total CKey index",	NULL, "= %ld ( %zu byte )\n\n",	pState->assigned_ckey_count,		static_cast<size_t>(pState->assigned_ckey_count * sizeof(CKINDEX)));

	DUMP_PRINT_NV(stream, 0, "Total Element",		NULL, "= %ld ( %zu byte )\n",	pState->total_element_count,		static_cast<size_t>(pState->total_element_count * sizeof(ELEMENT)));
	DUMP_PRINT_NV(stream, 1, "Assigned Element",	NULL, "= %ld ( %zu byte )\n",	pState->assigned_element_count,		static_cast<size_t>(pState->assigned_element_count * sizeof(ELEMENT)));
	DUMP_PRINT_NV(stream, 1, "Free Element",		NULL, "= %ld ( %zu byte )\n\n",	pState->unassigned_element_count,	static_cast<size_t>(pState->unassigned_element_count * sizeof(ELEMENT)));

	DUMP_PRINT_NV(stream, 0, "Total Page",			NULL, "= %ld ( %zu byte )\n",	pState->total_page_count,			static_cast<size_t>(pState->total_page_count * pState->page_size));
	DUMP_PRINT_NV(stream, 1, "Assigned Page",		NULL, "= %ld ( %zu byte )\n",	pState->assigned_page_count,		static_cast<size_t>(pState->assigned_page_count * pState->page_size));
	DUMP_PRINT_NV(stream, 1, "Free Page",			NULL, "= %ld ( %zu byte )\n",	pState->unassigned_page_count,		static_cast<size_t>(pState->unassigned_page_count * pState->page_size));
	DUMP_PRINT_NV(stream, 1, "Data raito per Page",	NULL, "= %zu %% ( %zu byte / %zu byte)\n\n",	(((pState->page_size - PAGEHEAD_SIZE) * 100) / pState->page_size), pState->page_size - PAGEHEAD_SIZE, pState->page_size);

	DUMP_PRINT_NV(stream, 0, "Total Key count",		NULL, "= %ld\n",				pState->total_element_count);
	DUMP_PRINT_NV(stream, 0, "System usage",		NULL, "= %zu byte\n",			(pState->total_used_size - static_cast<size_t>(pState->total_page_count * (pState->page_size - PAGEHEAD_SIZE))));
	DUMP_PRINT_NV(stream, 0, "Total real data size",NULL, "= %zu byte\n",			static_cast<size_t>(pState->total_page_count * (pState->page_size - PAGEHEAD_SIZE)));
	DUMP_PRINT_NV(stream, 0, "real data raito",		NULL, "= %zu %%\n",				(static_cast<size_t>(pState->total_page_count * (pState->page_size - PAGEHEAD_SIZE) * 100) / pState->total_used_size));

	K2H_Free(pState);

	return true;
}

//---------------------------------------------------------
// Get Status
//---------------------------------------------------------
// [NOTE]
// Must free the result pointer of this method.
//
PK2HSTATE K2HShm::GetState(void) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	K2HFILE_UPDATE_CHECK(const_cast<K2HShm*>(this));

	PK2HSTATE	pState;
 	if(NULL == (pState = reinterpret_cast<PK2HSTATE>(calloc(1, sizeof(K2HSTATE))))){
		ERR_K2HPRN("Could not allocate memory.");
		return NULL;
 	}

	// versions
	{
		memcpy(pState->version, pHead->version, K2H_VERSION_LENGTH);

		const char*	ptmp = K2H_HASH_VER_FUNC();
		if(!ISEMPTYSTR(ptmp)){
			memcpy(pState->hash_version, ptmp, min(strlen(ptmp), static_cast<size_t>(K2H_HASH_FUNC_VER_LENGTH)));
		}
		ptmp = K2H_TRANS_VER_FUNC();
		if(!ISEMPTYSTR(ptmp)){
			memcpy(pState->trans_version, ptmp, min(strlen(ptmp), static_cast<size_t>(K2H_HASH_FUNC_VER_LENGTH)));
		}
	}

	// const values
	{
		pState->trans_pool_count	= K2HShm::GetTransThreadPool();
		pState->max_mask			= pHead->max_mask;
		pState->min_mask			= pHead->min_mask;
		pState->cur_mask			= pHead->cur_mask;
		pState->collision_mask		= pHead->collision_mask;
		pState->max_element_count	= pHead->max_element_count;
		pState->total_size			= pHead->total_size;
		pState->page_size			= pHead->page_size;
	}

	// dynamic values
	{
		K2HLock	ALObjCMask(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RDLOCK);		// LOCK

		pState->file_size					= static_cast<size_t>(pHead->unassign_area);
		pState->total_area_count			= MAX_K2HAREA_COUNT;
		pState->assigned_key_count			= static_cast<long>(pHead->cur_mask + 1);
		pState->assigned_ckey_count			= static_cast<long>((pHead->cur_mask + 1) * (pHead->collision_mask + 1));
		pState->unassigned_element_count	= pHead->free_element_count;
		pState->unassigned_page_count		= pHead->free_page_count;

		// calac
		for(int nCnt = 0; nCnt < MAX_K2HAREA_COUNT; nCnt++){
			if(K2H_AREA_UNKNOWN != pHead->areas[nCnt].type){
				(pState->assigned_area_count)++;
				pState->total_used_size += pHead->areas[nCnt].length;

				if(isFullMapping || K2H_AREA_PAGE != (pHead->areas[nCnt].type & K2H_AREA_PAGE)){
					pState->total_map_size += pHead->areas[nCnt].length;
				}

				// [NOTICE]
				// Now areas.type is not set some types(bit or). If set some types into areas.type,
				// this logic must be changed.
				// See initializing function.
				//
				if(K2H_AREA_PAGELIST == (pHead->areas[nCnt].type & K2H_AREA_PAGELIST)){
					pState->total_element_size += pHead->areas[nCnt].length;
				}
				if(K2H_AREA_PAGE == (pHead->areas[nCnt].type & K2H_AREA_PAGE)){
					pState->total_page_size += pHead->areas[nCnt].length;
				}
			}
		}
		pState->total_element_count			= static_cast<long>(pState->total_element_size / sizeof(ELEMENT));
		pState->total_page_count			= static_cast<long>(pState->total_page_size / pState->page_size);
		pState->assigned_element_count		= (pState->total_element_count - pState->unassigned_element_count);
		pState->assigned_page_count			= (pState->total_page_count - pState->unassigned_page_count);

		pState->last_update.tv_sec			= pHead->last_update.tv_sec;
		pState->last_update.tv_usec			= pHead->last_update.tv_usec;
		pState->last_area_update.tv_sec		= pHead->last_area_update.tv_sec;
		pState->last_area_update.tv_usec	= pHead->last_area_update.tv_usec;

		ALObjCMask.Unlock();
	}

	return pState;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
