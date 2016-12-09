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
 * CREATE:   Mon Feb 10 2014
 * REVISION:
 *
 */
#ifndef	K2HARCHIVE_H
#define	K2HARCHIVE_H

#include "k2hcommon.h"
#include "k2hcommand.h"
#include "k2hshm.h"

//---------------------------------------------------------
// K2HArchive Class
//---------------------------------------------------------
class K2HArchive
{
	protected:
		std::string	filepath;
		bool		isErrSkip;

	public:
		K2HArchive(const char* pFile = NULL, bool iserrskip = false);
		virtual ~K2HArchive();

		bool Initialize(const char* pFile, bool iserrskip);
		bool Serialize(K2HShm* pShm, bool isLoad) const;

	protected:
		bool Save(K2HShm* pShm) const;
		bool Load(K2HShm* pShm) const;
		void* ReadFile(int fd, size_t count, off_t offset) const;
};

#endif	// K2HARCHIVE_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
