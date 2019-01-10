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
#ifndef	K2HFIND_H
#define	K2HFIND_H

#include "k2hlock.h"
#include "k2hshm.h"
#include "k2hsubkeys.h"
#include "k2hshmupdater.h"

class K2HShm;

//---------------------------------------------------------
// Class K2HIterator
//---------------------------------------------------------
// cppcheck-suppress copyCtorAndEqOperator
class K2HIterator : public std::iterator<std::forward_iterator_tag, PELEMENT>
{
		friend class K2HShm;

	protected:
		const K2HShm*	pK2HShm;
		bool			isOnlySubKey;
		K2HSubKeys		SubKeys;			// If isOnlySubKey is false, this is empty.
		K2HSubKeys::iterator	sk_iter;	// If isOnlySubKey is false, this is ignore.
		PELEMENT		pAbsElement;		// target element
		ELEMENT			EmptyElement;		// dummy element
		PELEMENT		pEmptyElement;		// for dummy element
		K2HLock			ALObjCKI;
		K2HShmUpdater*	pShmUpdater;		// For blocking check monitor file

	public:
		K2HIterator(const K2HIterator& other);
		virtual ~K2HIterator();

	private:
		K2HIterator();
		K2HIterator(const K2HShm* pk2hshm, bool isSubKey);
		K2HIterator(const K2HShm* pk2hshm, PELEMENT pElement, const K2HLock& lockobj);
		K2HIterator(const K2HShm* pk2hshm, PELEMENT pElement, const K2HSubKeys* pSKeys, const K2HSubKeys::iterator& iter, const K2HLock& lockobj);

	public:
		K2HIterator& operator++(void);
		K2HIterator operator++(int);
		PELEMENT& operator*(void);
		PELEMENT* operator->(void);
		bool operator==(const K2HIterator& other);
		bool operator!=(const K2HIterator& other);

	protected:
		void Initialize(bool isSubKey);
		bool IsInit(void) const { return (NULL != pK2HShm); }
		bool Reset(const K2HShm* pk2hshm = NULL, bool isSubKey = false);
		bool Reset(const K2HShm* pk2hshm, PELEMENT pElement, const K2HLock& lockobj);
		bool Reset(const K2HShm* pk2hshm, PELEMENT pElement, const K2HSubKeys* pSKeys, const K2HSubKeys::iterator& iter, const K2HLock& lockobj);
		bool Next(void);
};

#endif	// K2HFIND_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
