/*
 * K2HASH
 *
 * Copyright 2013 Yahoo! JAPAN corporation.
 *
 * K2HASH is key-valuew store base libraries.
 * K2HASH is made for the purpose of the construction of
 * original KVS system and the offer of the library.
 * The characteristic is this KVS library which Key can
 * layer. And can support multi-processing and multi-thread,
 * and is provided safely as available KVS.
 *
 * For the full copyright and license information, please view
 * the LICENSE file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Fri Dec 2 2013
 * REVISION:
 *
 */

#include <assert.h>

#include "k2hshm.h"
#include "k2hcommon.h"
#include "k2hmmapinfo.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Utility
//---------------------------------------------------------
inline std::string safe_file_path(const char* file)
{
	if(ISEMPTYSTR(file)){
		return string("");
	}else{
		return string(file);
	}
}

//---------------------------------------------------------
// K2HMmapMan: Constructor / Destructor
//---------------------------------------------------------
K2HMmapMan::K2HMmapMan() : lockval(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED)
{
}

K2HMmapMan::~K2HMmapMan()
{
	for(k2hfmapgrps_t::const_iterator iter = fmapgrps.begin(); iter != fmapgrps.end(); ++iter){
		PK2HMMAPGRP	pmmapgrp = iter->second;
		K2H_Delete(pmmapgrp);
	}
	for(k2homapgrps_t::const_iterator iter = omapgrps.begin(); iter != omapgrps.end(); ++iter){
		PK2HMMAPGRP	pmmapgrp = iter->second;
		K2H_Delete(pmmapgrp);
	}
}

//---------------------------------------------------------
// K2HMmapMan: Methods
//---------------------------------------------------------
bool K2HMmapMan::GetFd(const char* file, int* pfd, bool needlock)
{
	if(ISEMPTYSTR(file)){
		return false;
	}
	string	filepath = safe_file_path(file);

	if(needlock){
		Lock();
	}

	bool	result = false;
	k2hfmapgrps_t::const_iterator	iter;
	if(fmapgrps.end() != (iter = fmapgrps.find(filepath))){
		// check reference count
		if(0 < iter->second->refcnt){
			if(pfd){
				*pfd = iter->second->fd;
			}
			result = true;
		}
	}
	if(needlock){
		Unlock();
	}
	return result;
}

PK2HMMAPINFO* K2HMmapMan::AddMapInfo(const K2HShm* pk2hshm, const char* file, int fd, bool is_read, bool needlock)
{
	if(!pk2hshm){
		ERR_K2HPRN("pk2hshm object pointer is NULL.");
		return NULL;
	}

	if(needlock){
		Lock();
	}

	PK2HMMAPGRP	pmmapgrp = NULL;

	if(ISEMPTYSTR(file)){
		// object mapinfo
		if(omapgrps.end() == omapgrps.find(pk2hshm)){
			// add new(initialize)
			MSG_K2HPRN("Add new mapping info for K2HShm(%p).", pk2hshm);

			pmmapgrp			= new K2HMMAPGRP;
			pmmapgrp->refcnt	= 1;				// reference count
			pmmapgrp->fd		= fd;
			pmmapgrp->is_read	= is_read;

			omapgrps[pk2hshm]	= pmmapgrp;
		}else{
			// replace
			//
			// [NOTICE]
			// mmap for only memory is only one for each K2HShm object, so do not increment
			// reference count.
			//
			ERR_K2HPRN("There is already map information for K2HShm(%p).", pk2hshm);
			if(needlock){
				Unlock();
			}
			return NULL;
		}
	}else{
		// file mapinfo
		string	filepath = safe_file_path(file);

		k2hfmapgrps_t::const_iterator	iter;
		if(fmapgrps.end() == (iter = fmapgrps.find(filepath))){
			// add new(initialize)
			MSG_K2HPRN("Add new mapping info for \"%s\"", filepath.c_str());

			pmmapgrp			= new K2HMMAPGRP;
			pmmapgrp->refcnt	= 1;				// reference count
			pmmapgrp->fd		= fd;
			pmmapgrp->is_read	= is_read;

			fmapgrps[filepath]	= pmmapgrp;
		}else{
			pmmapgrp			= iter->second;

			// reference count up
			MSG_K2HPRN("Increment reference count for mapping info of %s", filepath.c_str());

			// check fd
			if(pmmapgrp->fd != fd){
				ERR_K2HPRN("fd(%d) is not same the fd(%d) in mapping info of %s", fd, pmmapgrp->fd, filepath.c_str());
				if(needlock){
					Unlock();
				}
				return NULL;
			}
			// check mode
			if(pmmapgrp->is_read != is_read){
				ERR_K2HPRN("Now mode is %s, but adding new is %s mode.", pmmapgrp->is_read ? "only read" : "writable", is_read ? "only read" : "writable");
				if(needlock){
					Unlock();
				}
				return NULL;
			}

			// count up
			pmmapgrp->refcnt++;
		}
	}

	if(needlock){
		Unlock();
	}
	return &(pmmapgrp->pmmapinfos);
}

bool K2HMmapMan::RemoveMapInfo(const K2HShm* pk2hshm, const char* file, bool needlock)
{
	if(!pk2hshm){
		ERR_K2HPRN("pk2hshm object pointer is NULL.");
		return false;
	}

	if(needlock){
		Lock();
	}

	// At first unmap all
	UnmapAll(pk2hshm, file, false);

	if(ISEMPTYSTR(file)){
		// object mapinfo
		k2homapgrps_t::iterator	iter;
		if(omapgrps.end() != (iter = omapgrps.find(pk2hshm))){
			// always remove map info
			MSG_K2HPRN("Destroy mapping info for for K2HShm(%p).", pk2hshm);

			K2H_Delete(iter->second);
			omapgrps.erase(iter);
		}

	}else{
		// file mapinfo
		string	filepath = safe_file_path(file);

		k2hfmapgrps_t::iterator	iter;
		if(fmapgrps.end() != (iter = fmapgrps.find(filepath))){
			// check reference count
			if(0 >= iter->second->refcnt){
				// already unmap all, so close fd and remove this map group.
				MSG_K2HPRN("Destroy mapping info for \"%s\", and close fd(%d).", filepath.c_str(), iter->second->fd);

				if(-1 != iter->second->fd){
					K2H_CLOSE(iter->second->fd);
					K2HLock::RemoveReadModeFd(iter->second->fd);
				}
				K2H_Delete(iter->second);
				fmapgrps.erase(iter);
			}
		}
	}
	if(needlock){
		Unlock();
	}
	return true;
}

PK2HMMAPINFO* K2HMmapMan::ReplaceMapInfo(const K2HShm* pk2hshm, const char* oldfile, const char* newfile, int newfd, bool is_read, bool needlock)
{
	if(needlock){
		Lock();
	}
	PK2HMMAPINFO*	ppInfo = NULL;

	// destroy now mapping by k2hshm object
	if(RemoveMapInfo(pk2hshm, oldfile, false)){
		// make new mapping info by file.
		ppInfo = AddMapInfo(pk2hshm, newfile, newfd, is_read, false);
	}
	if(needlock){
		Unlock();
	}
	return ppInfo;
}

void K2HMmapMan::UnmapAll(const K2HShm* pk2hshm, const char* file, bool needlock)
{
	if(!pk2hshm){
		ERR_K2HPRN("pk2hshm object pointer is NULL.");
		return;
	}

	if(needlock){
		Lock();
	}

	if(ISEMPTYSTR(file)){
		// object mapinfo
		k2homapgrps_t::iterator	iter;
		if(omapgrps.end() == (iter = omapgrps.find(pk2hshm))){
			WAN_K2HPRN("No map info for K2HShm(%p).", pk2hshm);

		}else{
			// unmap all
			MSG_K2HPRN("unmap all info for K2HShm(%p).", pk2hshm);

			// check
			if(1 != iter->second->refcnt){
				ERR_K2HPRN("map info reference count for K2HShm(%p) is not 1, should always be 1.", pk2hshm);
			}
			K2H_Delete(iter->second);		// unmap all in destructor
			omapgrps.erase(iter);
		}

	}else{
		// file mapinfo
		string	filepath = safe_file_path(file);

		k2hfmapgrps_t::iterator	iter;
		if(fmapgrps.end() == (iter = fmapgrps.find(filepath))){
			WAN_K2HPRN("No map info for \"%s\"", filepath.c_str());

		}else{
			// decrement reference count
			MSG_K2HPRN("Decrement reference count for mapping info of %s", filepath.c_str());

			iter->second->refcnt--;
			if(0 >= iter->second->refcnt){
				// unmap all
				MSG_K2HPRN("Unmap all for \"%s\"", filepath.c_str());

				if(-1 != iter->second->fd){
					K2H_CLOSE(iter->second->fd);
					K2HLock::RemoveReadModeFd(iter->second->fd);
				}
				K2H_Delete(iter->second);	// unmap all in destructor
				fmapgrps.erase(iter);
			}
		}
	}
	if(needlock){
		Unlock();
	}
}

bool K2HMmapMan::Unmap(const K2HShm* pk2hshm, const char* file, long type, off_t file_offset, size_t length, bool needlock)
{
	if(!pk2hshm){
		ERR_K2HPRN("pk2hshm object pointer is NULL.");
		return false;
	}

	if(needlock){
		Lock();
	}

	// get map info
	PK2HMMAPINFO*	ppmmapinfos = GetMmapInfo(pk2hshm, file, false);
	if(ppmmapinfos && *ppmmapinfos){
		// Find target offset and unmap.
		for(PK2HMMAPINFO pinfo = *ppmmapinfos; pinfo; pinfo = pinfo->next){
			if(file_offset == pinfo->file_offset && length == pinfo->length){
				if(type != pinfo->type){
					WAN_K2HPRN("offset=%jd, length=%zu area is not same type(%ld : %ld), but continue to unmmap.", static_cast<intmax_t>(file_offset), length, type, pinfo->type);
				}
				MSG_K2HPRN("Unmap offset=%jd, length=%zu for \"%s\"(%p)", static_cast<intmax_t>(file_offset), length, file ? file : "", pk2hshm);

				k2h_mmap_info_list_unmap(ppmmapinfos, pinfo);

				if(needlock){
					Unlock();
				}
				return true;
			}
		}
	}
	if(needlock){
		Unlock();
	}
	MSG_K2HPRN("K2HShm=%p, file=\"%s\", offset=%jd, length=%zu area is not found in mmap list.", pk2hshm, file ? file : "", static_cast<intmax_t>(file_offset), length);
	return false;
}

PK2HMMAPINFO* K2HMmapMan::GetMmapInfo(const K2HShm* pk2hshm, const char* file, bool needlock)
{
	if(!pk2hshm){
		ERR_K2HPRN("pk2hshm object pointer is NULL.");
		return false;
	}

	if(needlock){
		Lock();
	}

	PK2HMMAPINFO*	ppmmapinfos = NULL;
	if(ISEMPTYSTR(file)){
		// object mapinfo
		k2homapgrps_t::const_iterator	iter;
		if(omapgrps.end() != (iter = omapgrps.find(pk2hshm))){
			// Do not care for reference count, it always is 1.
			ppmmapinfos = &(iter->second->pmmapinfos);
		}
	}else{
		// file mapinfo
		string	filepath = safe_file_path(file);

		k2hfmapgrps_t::const_iterator	iter;
		if(fmapgrps.end() != (iter = fmapgrps.find(filepath))){
			if(0 < iter->second->refcnt){
				ppmmapinfos = &(iter->second->pmmapinfos);
			}
		}
	}
	if(needlock){
		Unlock();
	}
	return ppmmapinfos;
}

bool K2HMmapMan::IsMmaped(const K2HShm* pk2hshm, const char* file, bool needlock)
{
	if(!pk2hshm){
		ERR_K2HPRN("pk2hshm object pointer is NULL.");
		return false;
	}

	if(needlock){
		Lock();
	}

	bool			result		= false;
	PK2HMMAPINFO*	ppmmapinfos	= GetMmapInfo(pk2hshm, file, false);
	if(ppmmapinfos && *ppmmapinfos){
		result = true;
	}else{
		MSG_K2HPRN("There is no mapping info for \"%s\"(%p)", file ? file : "", pk2hshm);
	}
	if(needlock){
		Unlock();
	}
	return result;
}

//---------------------------------------------------------
// K2HMmapInfo: Class valiable
//---------------------------------------------------------
K2HMmapMan	K2HMmapInfo::mmapman;

//---------------------------------------------------------
// K2HMmapInfo: Constructor / Destructor
//---------------------------------------------------------
K2HMmapInfo::K2HMmapInfo(K2HShm* pk2hshm) : pK2Hshm(pk2hshm), ppInfoTop(NULL)
{
	assert(NULL != pK2Hshm);

	ppInfoTop = K2HMmapInfo::mmapman.AddMapInfo(pK2Hshm, pK2Hshm->GetRawK2hashFilePath(), pK2Hshm->GetRawK2hashFd(), pK2Hshm->GetRawK2hashReadMode(), true);
}

K2HMmapInfo::~K2HMmapInfo()
{
	K2HMmapInfo::mmapman.RemoveMapInfo(pK2Hshm, pK2Hshm->GetRawK2hashFilePath(), true);
}

//---------------------------------------------------------
// K2HMmapInfo: Methods
//---------------------------------------------------------
//
// Take care for this method
//
inline bool K2HMmapInfo::SetInternalMmapInfo(void) const
{
	if(ppInfoTop){
		return true;
	}
	if(NULL == ((const_cast<K2HMmapInfo*>(this))->ppInfoTop = K2HMmapInfo::mmapman.GetMmapInfo(pK2Hshm, pK2Hshm->GetRawK2hashFilePath(), false))){
		MSG_K2HPRN("There is no mapping info for \"%s\"(%p)", pK2Hshm->GetRawK2hashFilePath(), pK2Hshm);
		return false;
	}
	return true;
}

//
// This function does not use cache(ppInfoTop).
//
bool K2HMmapInfo::GetFd(const char* file, int& fd)
{
	return K2HMmapInfo::mmapman.GetFd(file, &fd, true);
}

bool K2HMmapInfo::ReplaceMapInfo(const char* oldfile)
{
	PK2HMMAPINFO*	ppTmp;
	if(NULL == (ppTmp = K2HMmapInfo::mmapman.ReplaceMapInfo(pK2Hshm, oldfile, pK2Hshm->GetRawK2hashFilePath(), pK2Hshm->GetRawK2hashFd(), pK2Hshm->GetRawK2hashReadMode(), true))){
		return false;
	}
	ppInfoTop = ppTmp;
	return true;
}

//
// This function does not use cache(ppInfoTop).
//
bool K2HMmapInfo::IsMmaped(void) const
{
	return K2HMmapInfo::mmapman.IsMmaped(pK2Hshm, pK2Hshm->GetRawK2hashFilePath(), true);
}

void K2HMmapInfo::UnmapAll(void)
{
	K2HMmapInfo::mmapman.UnmapAll(pK2Hshm, pK2Hshm->GetRawK2hashFilePath(), true);
	ppInfoTop = NULL;
}

bool K2HMmapInfo::Unmap(long type, off_t file_offset, size_t length)
{
	return K2HMmapInfo::mmapman.Unmap(pK2Hshm, pK2Hshm->GetRawK2hashFilePath(), type, file_offset, length, true);
}

bool K2HMmapInfo::Unmap(PK2HMMAPINFO pexistareatop)
{
	K2HMmapInfo::mmapman.Lock();

	if(!SetInternalMmapInfo()){
		K2HMmapInfo::mmapman.Unlock();
		return false;
	}

	// Find target offsets and unmap.
	for(PK2HMMAPINFO pinfo = *ppInfoTop; pinfo; ){

		bool	isFound = false;
		for(PK2HMMAPINFO pexist = pexistareatop; pexist; pexist = pexist->next){
			if(pinfo->type == pexist->type && pinfo->file_offset == pexist->file_offset && pinfo->length == pexist->length){
				isFound = true;
				break;
			}
		}
		if(!isFound){
			MSG_K2HPRN("Unmmap Area(type=%ld, file_offset=%jd, length=%zu, base=%p)", pinfo->type, static_cast<intmax_t>(pinfo->file_offset), pinfo->length, pinfo->mmap_base);

			PK2HMMAPINFO	pbupnext = pinfo->next;
			k2h_mmap_info_list_unmap(ppInfoTop, pinfo);
			pinfo = pbupnext;
		}else{
			pinfo = pinfo->next;
		}
	}

	K2HMmapInfo::mmapman.Unlock();
	return true;
}

bool K2HMmapInfo::AddArea(long type, off_t file_offset, void* mmap_base, size_t length)
{
	K2HMmapInfo::mmapman.Lock();

	if(!SetInternalMmapInfo()){
		K2HMmapInfo::mmapman.Unlock();
		return false;
	}

	PK2HMMAPINFO	pinfo	= new K2HMMAPINFO;
	pinfo->type				= type;
	pinfo->file_offset		= file_offset;
	pinfo->mmap_base		= mmap_base;
	pinfo->length			= length;

	k2h_mmap_info_list_add(ppInfoTop, pinfo);

	MSG_K2HPRN("Added Area(type=%ld, file_offset=%jd, length=%zu, base=%p) for \"%s\"(%p)", type, static_cast<intmax_t>(file_offset), length, mmap_base, pK2Hshm->GetRawK2hashFilePath(), pK2Hshm);

	K2HMmapInfo::mmapman.Unlock();
	return true;
}

void* K2HMmapInfo::GetMmapAddrBase(off_t file_offset, bool is_update_check) const
{
	K2HMmapInfo::mmapman.Lock();

	if(!SetInternalMmapInfo()){
		K2HMmapInfo::mmapman.Unlock();
		return NULL;
	}

	for(PK2HMMAPINFO pinfo = *ppInfoTop; pinfo; pinfo = pinfo->next){
		if(pinfo->file_offset <= file_offset && file_offset < static_cast<off_t>(pinfo->file_offset + pinfo->length)){
			K2HMmapInfo::mmapman.Unlock();
			return pinfo->mmap_base;
		}
	}
	K2HMmapInfo::mmapman.Unlock();

	MSG_K2HPRN("Could not find base(file_offset=%jd, file=\"%s\", K2HShm=%p)", static_cast<intmax_t>(file_offset), pK2Hshm->GetRawK2hashFilePath(), pK2Hshm);

	if(is_update_check && pK2Hshm){
		// try to update area information & retry to search
		bool	is_change = false;
		if((const_cast<K2HShm*>(pK2Hshm))->CheckAreaUpdate(is_change) && is_change){
			return GetMmapAddrBase(file_offset, false);
		}
	}
	return NULL;
}

off_t K2HMmapInfo::GetMmapAddrOffset(off_t file_offset, bool is_update_check) const
{
	K2HMmapInfo::mmapman.Lock();

	if(!SetInternalMmapInfo()){
		K2HMmapInfo::mmapman.Unlock();
		return -1;
	}

	for(PK2HMMAPINFO pinfo = *ppInfoTop; pinfo; pinfo = pinfo->next){
		if(pinfo->file_offset <= file_offset && file_offset < static_cast<off_t>(pinfo->file_offset + pinfo->length)){
			K2HMmapInfo::mmapman.Unlock();
			return reinterpret_cast<off_t>(pinfo->mmap_base) - pinfo->file_offset;
		}
	}
	K2HMmapInfo::mmapman.Unlock();

	MSG_K2HPRN("Could not find base(file_offset=%jd, file=\"%s\", K2HShm=%p)", static_cast<intmax_t>(file_offset), pK2Hshm->GetRawK2hashFilePath(), pK2Hshm);

	if(is_update_check && pK2Hshm){
		// try to update area information & retry to search
		bool	is_change = false;
		if((const_cast<K2HShm*>(pK2Hshm))->CheckAreaUpdate(is_change) && is_change){
			return GetMmapAddrOffset(file_offset, false);
		}
	}
	return -1;
}

off_t K2HMmapInfo::GetFileOffsetBase(void* address, bool is_update_check) const
{
	K2HMmapInfo::mmapman.Lock();

	if(!SetInternalMmapInfo()){
		K2HMmapInfo::mmapman.Unlock();
		return 0;
	}

	for(PK2HMMAPINFO pinfo = *ppInfoTop; pinfo; pinfo = pinfo->next){
		if(pinfo->mmap_base <= address && reinterpret_cast<size_t>(address) < (reinterpret_cast<size_t>(pinfo->mmap_base) + pinfo->length)){
			K2HMmapInfo::mmapman.Unlock();
			return pinfo->file_offset;
		}
	}
	K2HMmapInfo::mmapman.Unlock();

	MSG_K2HPRN("Could not find offset(address=%p, file=\"%s\", K2HShm=%p)", address, pK2Hshm->GetRawK2hashFilePath(), pK2Hshm);

	if(is_update_check && pK2Hshm){
		// try to update area information & retry to search
		bool	is_change = false;
		if((const_cast<K2HShm*>(pK2Hshm))->CheckAreaUpdate(is_change) && is_change){
			return GetFileOffsetBase(address, false);
		}
	}
	return 0;
}

off_t K2HMmapInfo::GetMmapAddressToFileOffset(void* address, bool is_update_check) const
{
	K2HMmapInfo::mmapman.Lock();

	if(!SetInternalMmapInfo()){
		K2HMmapInfo::mmapman.Unlock();
		return 0;
	}

	for(PK2HMMAPINFO pinfo = *ppInfoTop; pinfo; pinfo = pinfo->next){
		if(pinfo->mmap_base <= address && reinterpret_cast<size_t>(address) < (reinterpret_cast<size_t>(pinfo->mmap_base) + pinfo->length)){
			K2HMmapInfo::mmapman.Unlock();
			return pinfo->file_offset - reinterpret_cast<off_t>(pinfo->mmap_base);
		}
	}
	K2HMmapInfo::mmapman.Unlock();

	MSG_K2HPRN("Could not find offset(address=%p, file=\"%s\", K2HShm=%p)", address, pK2Hshm->GetRawK2hashFilePath(), pK2Hshm);

	if(is_update_check && pK2Hshm){
		// try to update area information & retry to search
		bool	is_change = false;
		if((const_cast<K2HShm*>(pK2Hshm))->CheckAreaUpdate(is_change) && is_change){
			return GetMmapAddressToFileOffset(address, false);
		}
	}
	return 0;
}

void* K2HMmapInfo::CvtAbs(off_t file_offset, bool isAllowNull, bool is_update_check) const
{
	if(!isAllowNull && 0 == file_offset){
		return NULL;
	}
	off_t mmap_offset = GetMmapAddrOffset(file_offset, is_update_check);

	return (-1 == mmap_offset ? NULL : reinterpret_cast<void*>(mmap_offset + file_offset));
}

off_t K2HMmapInfo::CvtRel(void* address, bool isAllowNull) const
{
	if(!isAllowNull && NULL == address){
		return 0L;
	}
	off_t mmap_to_file_offset = GetMmapAddressToFileOffset(address);

	return mmap_to_file_offset + reinterpret_cast<off_t>(address);
}

void* K2HMmapInfo::begin(int type, bool is_update_check) const
{
	K2HMmapInfo::mmapman.Lock();

	if(!SetInternalMmapInfo()){
		K2HMmapInfo::mmapman.Unlock();
		return NULL;
	}

	for(PK2HMMAPINFO pinfo = *ppInfoTop; pinfo; pinfo = pinfo->next){
		if(type == pinfo->type){
			K2HMmapInfo::mmapman.Unlock();
			return pinfo->mmap_base;
		}
	}
	K2HMmapInfo::mmapman.Unlock();

	MSG_K2HPRN("Could not get begin(file=\"%s\", K2HShm=%p)", pK2Hshm->GetRawK2hashFilePath(), pK2Hshm);

	if(is_update_check && pK2Hshm){
		// try to update area information & retry to search
		bool	is_change = false;
		if((const_cast<K2HShm*>(pK2Hshm))->CheckAreaUpdate(is_change) && is_change){
			return begin(type, false);
		}
	}
	return NULL;
}

void* K2HMmapInfo::next(void* address, size_t datasize, bool is_update_check) const
{
	off_t	file_offset = CvtRel(address);

	K2HMmapInfo::mmapman.Lock();

	if(!SetInternalMmapInfo()){
		K2HMmapInfo::mmapman.Unlock();
		return NULL;
	}

	for(PK2HMMAPINFO pinfo = *ppInfoTop; pinfo; pinfo = pinfo->next){
		if(pinfo->file_offset <= file_offset && file_offset < static_cast<off_t>(pinfo->file_offset + pinfo->length)){
			// found area
			if((file_offset + datasize) < (pinfo->file_offset + pinfo->length)){
				// return address in same area
				K2HMmapInfo::mmapman.Unlock();
				return CvtAbs(file_offset + datasize);
			}
			// next address is over area => search next area
			long	Type = pinfo->type;
			for(pinfo = pinfo->next; pinfo; pinfo = pinfo->next){
				if(Type == pinfo->type){
					// found same area type => return head address of area
					K2HMmapInfo::mmapman.Unlock();
					return pinfo->mmap_base;
				}
			}
			break;
		}
	}
	K2HMmapInfo::mmapman.Unlock();

	MSG_K2HPRN("Could not get next from (address=%p, length=%zd, file=\"%s\", K2HShm=%p)", address, datasize, pK2Hshm->GetRawK2hashFilePath(), pK2Hshm);

	if(is_update_check && pK2Hshm){
		// try to update area information & retry to search
		bool	is_change = false;
		if((const_cast<K2HShm*>(pK2Hshm))->CheckAreaUpdate(is_change) && is_change){
			return next(address, datasize, false);
		}
	}
	return NULL;
}

bool K2HMmapInfo::AreaMsync(void* address) const
{
	// If address is NULL, all area is updated.
	off_t	file_offset = -1L;
	if(address){
		file_offset = CvtRel(address);
	}

	K2HMmapInfo::mmapman.Lock();

	if(!SetInternalMmapInfo()){
		K2HMmapInfo::mmapman.Unlock();
		return NULL;
	}

	bool	result = true;
	for(PK2HMMAPINFO pinfo = *ppInfoTop; pinfo; pinfo = pinfo->next){
		if(-1L == file_offset || (pinfo->file_offset <= file_offset && file_offset < static_cast<off_t>(pinfo->file_offset + pinfo->length))){
			// target area
			if(-1 == msync(pinfo->mmap_base, pinfo->length, MS_ASYNC)){
				// error occurred, but continue...
				ERR_K2HPRN("Failed to msync %p(%zu) area(file=\"%s\", K2HShm=%p). errno=%d", pinfo->mmap_base, pinfo->length, pK2Hshm->GetRawK2hashFilePath(), pK2Hshm, errno);
				result = false;
			}
		}
	}
	K2HMmapInfo::mmapman.Unlock();

	return result;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
