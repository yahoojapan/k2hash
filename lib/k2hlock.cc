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
 * CREATE:   Tue Feb 4 2014
 * REVISION:
 *
 */

#include "k2hcommon.h"
#include "k2hlock.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// K2HLock Class Variables/Methods
//---------------------------------------------------------
const bool	K2HLock::RDLOCK;
const bool	K2HLock::RWLOCK;

// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
fdmodemap_t& K2HLock::GetFdModes(void)
{
	// [NOTE] About fdmodes
	// We do not use lock for this mapping because this method is
	// called only at initializing under another exclusive control.
	//
	static fdmodemap_t	fmmap;				// singleton
	return fmmap;
}

bool K2HLock::AddReadModeFd(int fd)
{
	if(FLCK_INVALID_HANDLE == fd){
		return false;
	}
	K2HLock::GetFdModes()[fd] = true;		// value is dummy(always true)

	return true;
}

bool K2HLock::RemoveReadModeFd(int fd)
{
	if(FLCK_INVALID_HANDLE == fd){
		return false;
	}
	if(K2HLock::GetFdModes().end() == K2HLock::GetFdModes().find(fd)){
		return false;
	}
	K2HLock::GetFdModes().erase(fd);

	return true;
}

//---------------------------------------------------------
// K2HLock Constructor
//---------------------------------------------------------
K2HLock::K2HLock(bool isRead) : FLRwlRcsv(FLCK_INVALID_HANDLE, 0L, 1L, isRead)
{
}

K2HLock::K2HLock(int fd, off_t offset, bool isRead) : FLRwlRcsv((FLCK_INVALID_HANDLE == fd ? FLCK_RWLOCK_NO_FD(getpid()) : fd), offset, 1L, isRead)
{
}

K2HLock::K2HLock(const K2HLock& other) : FLRwlRcsv()
{
	Dup(other);
}

K2HLock::~K2HLock()
{
}

//---------------------------------------------------------
// K2HLock Methods
//---------------------------------------------------------
bool K2HLock::Lock(bool IsRead)
{
	if(FLCK_INVALID_HANDLE == lock_fd){
		MSG_K2HPRN("FD is not initialized.");
		return true;
	}
	if(lock_type == (IsRead ? FLCK_READ_LOCK : FLCK_WRITE_LOCK) && is_locked){
		//MSG_K2HPRN("Already locked.");
		return true;
	}
	// if fd is read only mode, do nothing.
	if(K2HLock::GetFdModes().end() != K2HLock::GetFdModes().find(lock_fd)){
		return true;
	}
	return FLRwlRcsv::Lock(IsRead);
}

bool K2HLock::Lock(int fd, off_t offset)
{
	if(FLCK_READ_LOCK != lock_type && FLCK_WRITE_LOCK != lock_type){
		ERR_K2HPRN("lock_type(%s) is wrong.", STR_FLCKLOCKTYPE(lock_type));
		return false;
	}
	if(FLCK_INVALID_HANDLE == fd){
		fd = FLCK_RWLOCK_NO_FD(getpid());
	}
	if(lock_fd == fd && lock_offset == offset && is_locked){
		//MSG_K2HPRN("Already locked.");
		return true;
	}
	// if fd is read only mode, do nothing.
	if(K2HLock::GetFdModes().end() != K2HLock::GetFdModes().find(lock_fd)){
		return true;
	}
	return FLRwlRcsv::Lock(fd, offset, 1L, (FLCK_READ_LOCK == lock_type));
}

bool K2HLock::Lock(int fd, off_t offset, bool IsRead)
{
	if(FLCK_INVALID_HANDLE == fd){
		fd = FLCK_RWLOCK_NO_FD(getpid());
	}
	if(lock_fd == fd && lock_offset == offset && is_locked){
		//MSG_K2HPRN("Already locked.");
		return true;
	}
	// if fd is read only mode, do nothing.
	if(K2HLock::GetFdModes().end() != K2HLock::GetFdModes().find(lock_fd)){
		return true;
	}
	return FLRwlRcsv::Lock(fd, offset, 1L, IsRead);
}

bool K2HLock::Lock(void)
{
	if(FLCK_INVALID_HANDLE == lock_fd){
		MSG_K2HPRN("FD is not initialized.");
		return true;
	}
	if(is_locked){
		//MSG_K2HPRN("Already locked.");
		return true;
	}
	// if fd is read only mode, do nothing.
	if(K2HLock::GetFdModes().end() != K2HLock::GetFdModes().find(lock_fd)){
		return true;
	}
	return FLRwlRcsv::Lock();
}

bool K2HLock::Unlock(void)
{
	if(FLCK_INVALID_HANDLE == lock_fd){
		//MSG_K2HPRN("FD is not initialized.");
		return true;
	}
	if(!is_locked){
		//MSG_K2HPRN("Already unlocked.");
		return true;
	}
	// if fd is read only mode, do nothing.
	if(K2HLock::GetFdModes().end() != K2HLock::GetFdModes().find(lock_fd)){
		return true;
	}
	return FLRwlRcsv::Unlock();
}

bool K2HLock::Dup(const K2HLock& other)
{
	bool	is_mutex_locked	= false;
	bool	bresult			= false;

	if(FLCK_READ_LOCK == other.lock_type || FLCK_WRITE_LOCK == other.lock_type){
		if(false == (bresult = Set(other.lock_fd, other.lock_offset, other.lock_length, (FLCK_READ_LOCK == other.lock_type), is_mutex_locked))){
			ERR_K2HPRN("Could not initialize object by other(fd(%d), offset(%zd), length(%zu), locktype(%s))", other.lock_fd, other.lock_offset, other.lock_length, STR_FLCKLOCKTYPE(other.lock_type));
		}else{
			if(other.is_locked){
				if(FLCK_READ_LOCK == lock_type){
					bresult = RawReadLock(is_mutex_locked);
				}else if(FLCK_WRITE_LOCK == lock_type){
					bresult = RawWriteLock(is_mutex_locked);
				}
			}
		}
	}else{
		if(false == (bresult = Set(other.lock_fd, other.lock_offset, other.lock_length, true, is_mutex_locked))){
			ERR_K2HPRN("Could not initialize object by other(fd(%d), offset(%zd), length(%zu), locktype(%s))", other.lock_fd, other.lock_offset, other.lock_length, STR_FLCKLOCKTYPE(other.lock_type));
		}else{
			// force set lock_type
			lock_type = FLCK_UNLOCK;
		}
	}

	if(is_mutex_locked){
		FLRwlRcsv::StackUnlock();
	}
	return bresult;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
