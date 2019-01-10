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
#ifndef	K2HLOCK_H
#define	K2HLOCK_H

#include <map>
#include <fullock/rwlockrcsv.h>

//---------------------------------------------------------
// struct
//---------------------------------------------------------
// for read only mode fd list
typedef std::map<int, bool>		fdmodemap_t;

//---------------------------------------------------------
// Class K2HLock
//---------------------------------------------------------
// This class wraps FLRwlRcsv class
//
class K2HLock : public FLRwlRcsv
{
	public:
		static const bool	RDLOCK = true;
		static const bool	RWLOCK = false;

	protected:
		static fdmodemap_t& GetFdModes(void);

		K2HLock& Dup(const K2HLock& other);

	public:
		static bool AddReadModeFd(int fd);
		static bool RemoveReadModeFd(int fd);

	public:
		explicit K2HLock(bool isRead = K2HLock::RDLOCK);
		K2HLock(int fd, off_t offset, bool isRead = K2HLock::RDLOCK);
		K2HLock(const K2HLock& other);
		virtual ~K2HLock();

		bool IsReadLock(void) const { return (FLCK_READ_LOCK == lock_type); }
		bool IsLocked(void) const { return is_locked; }

		bool Lock(void);
		bool Lock(bool IsRead);
		bool Lock(int fd, off_t offset);
		bool Lock(int fd, off_t offset, bool IsRead);
		bool Unlock(void);

		K2HLock& operator=(const K2HLock& other) { return Dup(other); }
};

#endif	// K2HLOCK_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
