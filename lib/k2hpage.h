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
#ifndef	K2HPAGE_H
#define	K2HPAGE_H

#include "k2hstructure.h"
#include "k2hshm.h"
#include "k2hsubkeys.h"
#include "k2hattrs.h"
#include "k2hutil.h"

//---------------------------------------------------------
// Extern
//---------------------------------------------------------
class K2HShm;

//---------------------------------------------------------
// Class K2HPage
//---------------------------------------------------------
class K2HPage
{
	public:
		enum{
			SETHEAD_PREV	= 1,
			SETHEAD_NEXT	= 2,
			SETHEAD_LENGTH	= 4
		};

	protected:
		const K2HShm*	pK2HShm;
		PPAGEHEAD		pPageHead;
		unsigned char*	pPageData;			// Data internal pointer by allocation
		size_t			AllocLength;		// Data internal buffer allocation size
		size_t			DataLength;			// Data size in internal buffer
		size_t			PageSize;			// Page size
		bool			isHeadLoaded;
		bool			isLoaded;

	protected:
		unsigned char* AllocateData(ssize_t length);
		virtual unsigned char* AppendData(const unsigned char* pAppend, ssize_t length);

		bool Initialize(const K2HShm* pk2hshm);

		virtual bool Free(PPAGEHEAD* ppRelLastPageHead, unsigned long* pPageCount, bool isAllPage) = 0;

		void Clean(void);
		void CleanMemory(void);
		void CleanPageHead(void);
		void CleanLoadedData(void);

	public:
		K2HPage(const K2HShm* pk2hshm = NULL);
		virtual ~K2HPage();

		bool SetPageHead(int type, PPAGEHEAD prev, PPAGEHEAD next = NULL, size_t length = 0UL);
		bool SetPageHead(PPAGEHEAD ppagehead);
		bool GetPageHead(PPAGEHEAD ppagehead);
		virtual bool UploadPageHead(void) = 0;
		virtual bool LoadPageHead(void) = 0;
		virtual bool LoadData(bool isPageHead = false, bool isAllPage = true) = 0;
		virtual K2HPage* LoadData(off_t offset, size_t length, unsigned char** byData, size_t& datalength, off_t& next_read_pos) = 0;
		virtual bool SetData(const unsigned char* byData, size_t length, bool isSetLoadedData = false) = 0;
		virtual K2HPage* SetData(const unsigned char* byData, size_t length, off_t offset, bool& isChangeSize, off_t& next_write_pos) = 0;
		virtual bool Truncate(size_t length) = 0;
		virtual PPAGEHEAD GetPageHeadRelAddress(void) const = 0;
		virtual PPAGEHEAD GetLastPageHead(unsigned long* pagecount = NULL, bool isAllPage = true) = 0;
		bool Free(void) { return Free(NULL, NULL, true); };

		size_t SetPageSize(size_t page_size);

		size_t GetLength(void) const { return DataLength; };
		bool CopyData(unsigned char** ppPageData, size_t* pLength) const;
		bool GetData(const unsigned char** ppPageData, size_t* pLength) const;
		const unsigned char* GetData(size_t* pLength = NULL) const;
		const char* GetString(size_t* pLength = NULL) const;
		strarr_t::size_type ParseStringArray(strarr_t& strarr) const;
		K2HSubKeys* GetSubKeys(void) const;
		size_t GetSubKeys(K2HSubKeys& SubKeys) const;
		K2HAttrs* GetAttrs(void) const;
		size_t GetAttrs(K2HAttrs& Attrs) const;

		bool Compare(const unsigned char* pData, size_t length) const;
		bool Compare(const char* pData) const;
		bool operator==(const char* pData) const;
		bool operator!=(const char* pData) const;
};

#endif	// K2HPAGE_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
