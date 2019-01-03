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
#ifndef	K2HPAGEFILE_H
#define	K2HPAGEFILE_H

#include "k2hpage.h"

//---------------------------------------------------------
// Class K2HPageFile
//---------------------------------------------------------
class K2HPageFile : public K2HPage
{
	protected:
		int		PageFd;
		off_t	FileOffset;
		bool	isNodupfd;

	protected:
		K2HPageFile(const K2HShm* pk2hshm, int fd, off_t offset, bool nodup);

		void Clean(void);
		void CleanPageHead(void);
		bool CloseFd(void);
		bool DuplicateFd(int fd);

		virtual bool Free(PPAGEHEAD* ppRelLastPageHead, unsigned long* pPageCount, bool isAllPage);

	public:
		static bool GetData(const K2HShm* pk2hshm, int fd, off_t offset, unsigned char** ppPageData, size_t* pLength);
		static bool FreeData(unsigned char* pPageData);

		K2HPageFile(const K2HShm* pk2hshm = NULL, int fd = -1, off_t offset = 0);
		virtual ~K2HPageFile();

		bool Initialize(const K2HShm* pk2hshm, int fd, off_t offset);
		virtual bool UploadPageHead(void);
		virtual bool LoadPageHead(void);
		virtual bool LoadData(bool isPageHead = false, bool isAllPage = true);
		virtual K2HPage* LoadData(off_t offset, size_t length, unsigned char** byData, size_t& datalength, off_t& next_read_pos);
		virtual bool SetData(const unsigned char* byData, size_t length, bool isSetLoadedData = false);
		virtual K2HPage* SetData(const unsigned char* byData, size_t length, off_t offset, bool& isChangeSize, off_t& next_write_pos);
		virtual bool Truncate(size_t length);
		virtual PPAGEHEAD GetPageHeadRelAddress(void) const;
		virtual PPAGEHEAD GetLastPageHead(unsigned long* pagecount = NULL, bool isAllPage = true);

		const unsigned char* GetData(const K2HShm* pk2hshm, int fd, off_t offset, size_t* pLength = NULL);
};

#endif	// K2HPAGEFILE_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
