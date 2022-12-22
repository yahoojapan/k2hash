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
 * CREATE:   Fri Apr 18 2014
 * REVISION:
 *
 */
#ifndef	K2HSHMUPDATER_H
#define	K2HSHMUPDATER_H

#include "k2hdbg.h"

class K2HShm;

//---------------------------------------------------------
// Class K2HShmUpdater
//---------------------------------------------------------
class K2HShmUpdater
{
	protected:
		static int				lockval;		// for update area
		static unsigned long	lockcnt;		// for update checking

	protected:
		unsigned long			oldcnt;

	public:
		K2HShmUpdater();
		virtual ~K2HShmUpdater();

		bool UpdateCheck(K2HShm* pk2hshm);
		bool UpdateArea(K2HShm* pk2hshm);
};

//---------------------------------------------------------
// Update macro
//---------------------------------------------------------
inline void K2HFILE_UPDATE_CHECK(K2HShm* pk2hshm)
{
	K2HShmUpdater	updaterobj;
	if(!updaterobj.UpdateCheck(pk2hshm)){
		WAN_K2HPRN("Something wrong occurred in check and update k2hash file, but continue...");
	}
}

inline void K2HFILE_UPDATE_AREA(K2HShm* pk2hshm)
{
	K2HShmUpdater	updaterobj;
	if(!updaterobj.UpdateArea(pk2hshm)){
		WAN_K2HPRN("Something wrong occurred in update k2hash area, but continue...");
	}
}

#endif	// K2HSHMUPDATER_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
