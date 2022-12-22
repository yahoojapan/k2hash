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

#include <fullock/flckstructure.h>
#include <fullock/flckbaselist.tcc>

#include "k2hcommon.h"
#include "k2hshmupdater.h"
#include "k2hshm.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// K2HShmUpdater Class Variable & Methods
//---------------------------------------------------------
int				K2HShmUpdater::lockval = FLCK_NOSHARED_MUTEX_VAL_UNLOCKED;
unsigned long	K2HShmUpdater::lockcnt = 0L;

bool K2HShmUpdater::UpdateCheck(K2HShm* pk2hshm)
{
	if(!pk2hshm){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(0L != oldcnt){
		// nothing to do
		return true;
	}
	return pk2hshm->CheckFileUpdate();
}

//
// Checking & updating k2hash area does not check nest(counter).
// So this method should be called any nest level.
//
bool K2HShmUpdater::UpdateArea(K2HShm* pk2hshm)
{
	if(!pk2hshm){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}

	while(!fullock::flck_trylock_noshared_mutex(&K2HShmUpdater::lockval));		// no call sched_yield()

	bool	change	= false;
	bool	result	= pk2hshm->CheckAreaUpdate(change);

	fullock::flck_unlock_noshared_mutex(&K2HShmUpdater::lockval);

	return result;
}

//---------------------------------------------------------
// K2HShmUpdater Methods
//---------------------------------------------------------
K2HShmUpdater::K2HShmUpdater() : oldcnt(0)
{
	oldcnt = fullock::flck_counter_up(&K2HShmUpdater::lockcnt);
}

K2HShmUpdater::~K2HShmUpdater()
{
	oldcnt = fullock::flck_counter_down(&K2HShmUpdater::lockcnt);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
