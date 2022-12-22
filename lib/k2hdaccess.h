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
#ifndef	K2HDACCESS_H
#define	K2HDACCESS_H

class K2HShm;
class K2HPage;
class K2HDAccess;

//---------------------------------------------------------
// Class K2HDALock
//---------------------------------------------------------
class K2HDALock
{
	friend class K2HDAccess;

	protected:
		K2HShm*				pK2HShm;
		bool				IsReadOnlyLock;
		K2HLock*			pALObjCKI;
		PELEMENT			pElement;

	public:
		explicit K2HDALock(K2HShm* pk2hshm = NULL, bool is_read_lock = true, const unsigned char* byKey = NULL, size_t keylength = 0L);
		K2HDALock(K2HShm* pk2hshm, bool is_read_lock, const char* pKey);
		virtual ~K2HDALock();

		bool Unlock(void);
		bool Lock(const char* pKey);
		bool Lock(const unsigned char* byKey, size_t keylength);

		bool IsLocked(void) const { return (NULL != pElement);}
};

//---------------------------------------------------------
// Class K2HDAccess
//---------------------------------------------------------
class K2HDAccess
{
	public:
		typedef enum _access_mode{
			READ_ACCESS		= 1,
			WRITE_ACCESS	= 2,
			RW_ACCESS		= 3
		}ACSMODE;

	protected:
		static const int	FILE_IO_BLOCK = 4096 * 100;	// default system page size * 100

		K2HShm*				pK2HShm;
		K2HDALock*			pALObjCKI;
		ACSMODE				AccessMode;
		unsigned char*		byOpKey;
		size_t				opkeylength;
		size_t				fileio_size;

		K2HPage*			pWriteValPage;				// for writing
		off_t				abs_woffset;				// for writing
		off_t				valpage_woffset;			// for writing

		K2HPage*			pReadValPage;				// for reading
		off_t				abs_roffset;				// for reading
		off_t				valpage_roffset;			// for reading

	private:
		K2HDAccess();

		bool Reset(K2HShm* pk2hshm = NULL, K2HDAccess::ACSMODE access_mode = K2HDAccess::READ_ACCESS);
		void ResetOffsets(bool isRead);
		void ResetOffsets(void) { ResetOffsets(true); ResetOffsets(false); }

		bool IsOpen(void) const { return (NULL != pK2HShm && NULL != pALObjCKI && NULL != pALObjCKI->pElement); }
		bool IsSetValPage(bool isRead) const;

	public:
		K2HDAccess(K2HShm* pk2hshm, K2HDAccess::ACSMODE access_mode);
		virtual ~K2HDAccess();

		bool IsInitialize(void) const { return (NULL != pK2HShm && NULL != pALObjCKI); }

		bool Open(const char* pKey);
		bool Open(const unsigned char* byKey, size_t keylength);
		bool Close(void);

		bool SetOffset(off_t offset, size_t length, bool isRead);
		bool SetWriteOffset(off_t offset) { return SetOffset(offset, 0UL, false); }
		bool SetReadOffset(off_t offset) { return SetOffset(offset, 0UL, true); }
		bool SetOffset(off_t offset) { return (SetOffset(offset, 0UL, false) && SetOffset(offset, 0UL, true)); }

		bool GetSize(size_t& size) const;
		size_t SetFioSize(size_t size);
		size_t GetFioSize(void) const { return fileio_size; }

		off_t GetWriteOffset(void) const { return abs_woffset; }
		off_t GetReadOffset(void) const { return abs_roffset; }

		bool Write(const char* pValue);
		bool Write(const unsigned char* byValue, size_t vallength);
		bool Write(int fd, size_t& wlength);
		bool Read(char** ppValue);
		bool Read(unsigned char** byValue, size_t& vallength);
		bool Read(int fd, size_t& rlength);
};

#endif	// K2HDACCESS_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
