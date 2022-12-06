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
#ifndef	K2HMMAPINFO_H
#define	K2HMMAPINFO_H

#include <list>
#include <map>

#include <fullock/flckstructure.h>
#include <fullock/flckbaselist.tcc>

//---------------------------------------------------------
// Extern
//---------------------------------------------------------
class K2HShm;
class K2HMmapInfo;

//---------------------------------------------------------
// Structures
//---------------------------------------------------------
// [NOTICE]
// We make static mmap information which is used to convert address to offset.
// In process, we have only one mmap information(mmap base address and offset, fd).
// So if many thread is running, we only one mmap for the file and using only it.
// The mmap information key is filepath, it works good for real file.
// But for only memory( not mmap to file ), so the filepath is NULL and fd is -1,
// it does not work.
// Thus we have one more map information for K2HShm object pointer key for it.
// If there are many K2HShm object which is mapping only memory in one process,
// mmapped address is uniq in one process. Then it works good.
//
//
// K2HMMAPINFO
//
typedef struct k2h_mmap_info{
	struct k2h_mmap_info*	next;
	long					type;
	off_t					file_offset;		// offset in K2H file for this area
	void*					mmap_base;			// start mapping address for this area
	size_t					length;				// length for this area

	k2h_mmap_info() : next(NULL), type(0), file_offset(0L), mmap_base(NULL), length(0L) {}

}K2HMMAPINFO, *PK2HMMAPINFO;

inline void k2h_mmap_info_list_add(PK2HMMAPINFO* ptop, PK2HMMAPINFO addinfo)
{
	PK2HMMAPINFO	parent = NULL;
	// cppcheck-suppress nullPointerRedundantCheck
	for(PK2HMMAPINFO base = *ptop; base; parent = base, base = base->next){
		if(addinfo->file_offset < base->file_offset){
			addinfo->next = base;
			if(parent){
				parent->next = addinfo;
			}else{
				*ptop = addinfo;
			}
			return;
		}
	}
	if(parent){
		parent->next = addinfo;
	}else{
		*ptop = addinfo;
	}
	addinfo->next = NULL;
}

inline void k2h_mmap_info_list_unmapall(PK2HMMAPINFO* ptop)
{
	for(PK2HMMAPINFO base = *ptop, next = NULL; base; base = next){
		munmap(base->mmap_base, base->length);
		next = base->next;
		delete base;
	}
	*ptop = NULL;
}

inline void k2h_mmap_info_list_freeall(PK2HMMAPINFO* ptop)
{
	for(PK2HMMAPINFO base = *ptop, next = NULL; base; base = next){
		next = base->next;
		delete base;
	}
	*ptop = NULL;
}

inline void k2h_mmap_info_list_unmap(PK2HMMAPINFO* ptop, PK2HMMAPINFO info)
{
	for(PK2HMMAPINFO parent = NULL, base = *ptop; base; parent = base, base = base->next){
		if(base == info){
			if(parent){
				parent->next = base->next;
			}else{
				*ptop = base->next;
			}
			break;
		}
	}
	munmap(info->mmap_base, info->length);
	delete info;
}

//
// K2HMMAPGRP
//
typedef struct k2h_mmap_group{
	int				refcnt;						// reference count
	int				fd;							// file descriptor
	bool			is_read;					// file open mode(only read/writable)
	PK2HMMAPINFO	pmmapinfos;					// mmap information

	k2h_mmap_group() : refcnt(0), fd(-1), is_read(true), pmmapinfos(NULL) {}

	~k2h_mmap_group()
	{
		k2h_mmap_info_list_unmapall(&pmmapinfos);
	}

}K2HMMAPGRP, *PK2HMMAPGRP;

typedef std::map<std::string, PK2HMMAPGRP>		k2hfmapgrps_t;		// map of file path
typedef std::map<const K2HShm*, PK2HMMAPGRP>	k2homapgrps_t;		// map of K2HShm object pointer

//---------------------------------------------------------
// Class K2HMmapMan
//---------------------------------------------------------
class K2HMmapMan
{
		friend class K2HMmapInfo;

	private:
		int					lockval;		// like mutex
		k2hfmapgrps_t		fmapgrps;
		k2homapgrps_t		omapgrps;

	private:
		K2HMmapMan();
		virtual ~K2HMmapMan();

		inline void Lock(void) { while(!fullock::flck_trylock_noshared_mutex(&lockval)); }	// no call sched_yield()
		inline void Unlock(void) { fullock::flck_unlock_noshared_mutex(&lockval); }

		bool GetFd(const char* file, int* pfd, bool needlock);

		PK2HMMAPINFO* AddMapInfo(const K2HShm* pk2hshm, const char* file, int fd, bool is_read, bool needlock);
		PK2HMMAPINFO* ReplaceMapInfo(const K2HShm* pk2hshm, const char* oldfile, const char* newfile, int newfd, bool is_read, bool needlock);
		bool RemoveMapInfo(const K2HShm* pk2hshm, const char* file, bool needlock);

		void UnmapAll(const K2HShm* pk2hshm, const char* file, bool needlock);
		bool Unmap(const K2HShm* pk2hshm, const char* file, long type, off_t file_offset, size_t length, bool needlock);

		PK2HMMAPINFO* GetMmapInfo(const K2HShm* pk2hshm, const char* file, bool needlock);
		bool IsMmaped(const K2HShm* pk2hshm, const char* file, bool needlock);
};

//---------------------------------------------------------
// Class K2HMmapInfo
//---------------------------------------------------------
class K2HMmapInfo
{
	private:
		K2HShm*			pK2Hshm;
		PK2HMMAPINFO*	ppInfoTop;

	public:
		explicit K2HMmapInfo(K2HShm* pk2hshm = NULL);
		virtual ~K2HMmapInfo();

	private:
		static K2HMmapMan& GetMan(void);

		inline bool SetInternalMmapInfo(void) const;

		void* GetMmapAddrBase(off_t file_offset, bool is_update_check = true) const;
		off_t GetMmapAddrOffset(off_t file_offset, bool is_update_check = true) const;
		off_t GetFileOffsetBase(void* address, bool is_update_check = true) const;
		off_t GetMmapAddressToFileOffset(void* address, bool is_update_check = true) const;

		void* begin(int type, bool is_update_check) const;
		void* next(void* address, size_t datasize, bool is_update_check) const;

	public:
		bool GetFd(const char* file, int& fd);
		bool ReplaceMapInfo(const char* oldfile);

		bool IsMmaped(void) const;
		void UnmapAll(void);
		bool Unmap(long type, off_t file_offset, size_t length);
		bool Unmap(PK2HMMAPINFO pexistareatop);

		bool AddArea(long type, off_t file_offset, void* mmap_base, size_t length);
		void* CvtAbs(off_t file_offset, bool isAllowNull = false, bool is_update_check = true) const;
		off_t CvtRel(void* address, bool isAllowNull = false) const;

		void* begin(int type) const { return begin(type, true); }
		void* next(void* address, size_t datasize) const { return next(address, datasize, true); }

		bool AreaMsync(void* address = NULL) const;
};

#endif	// K2HMMAPINFO_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
