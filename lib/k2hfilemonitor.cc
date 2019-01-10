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
 * CREATE:   Wed Apr 16 2014
 * REVISION:
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <string>

#include "k2hcommon.h"
#include "k2hfilemonitor.h"
#include "k2hashfunc.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Symbols & macros
//---------------------------------------------------------
#define	MONITORFILE_BASE		"/tmp/.k2h_"

#define	OPEN_LOCK_POS			0L
#define	INODE_LOCK_POS			static_cast<off_t>(sizeof(unsigned char) * sizeof(long))
#define	AREA_LOCK_POS			(INODE_LOCK_POS + static_cast<off_t>(sizeof(unsigned char) * sizeof(long)))
#define	INODE_VAL_POS			(AREA_LOCK_POS + static_cast<off_t>(sizeof(unsigned char) * sizeof(long)))
#define	WAIT_INITOPEN_TIME		100				// 100ms
#define	WAIT_INITOPEN_MAXTIME	(30 * 1000)		// 30s
#define	WAIT_INITOPEN_CNT		(WAIT_INITOPEN_MAXTIME / WAIT_INITOPEN_TIME)

//---------------------------------------------------------
// Class variable
//---------------------------------------------------------
const char		K2HFileMonitor::base_prefix[]	= MONITORFILE_BASE;
mode_t			K2HFileMonitor::file_umask		= 0;

//---------------------------------------------------------
// Constructor / Destructor
//---------------------------------------------------------
K2HFileMonitor::K2HFileMonitor() : bup_shmfile(""), bup_monfile(""), psfmon(NULL), fmfd(-1), bup_inode_cnt(0), bup_area_cnt(0), bup_inode_val(0)
{
}

K2HFileMonitor::~K2HFileMonitor()
{
	Close();
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
bool K2HFileMonitor::Close(void)
{
	CloseOnlyFile();		// always return true.
	bup_shmfile		= "";
	bup_monfile		= "";
	bup_inode_cnt	= 0;
	bup_area_cnt	= 0;
	bup_inode_val	= 0;

	return true;
}

bool K2HFileMonitor::CloseOnlyFile(void)
{
	if(psfmon){
		munmap(psfmon, sizeof(SFMONWRAP));
		psfmon = NULL;
	}
	if(-1 != fmfd){
		if(WriteLock(OPEN_LOCK_POS, false)){
			// succeed write lock on open_lock_pos, this means this process is last process.
			// then should remove monitor file.
			unlink(bup_monfile.c_str());
			MSG_K2HPRN("Remove monitor file.");
		}else{
			MSG_K2HPRN("Do not remove monitor file.");
		}
		Unlock(OPEN_LOCK_POS, false);
	}
	K2H_CLOSE(fmfd);

	return true;
}

bool K2HFileMonitor::Open(const char* shmfile, bool noupdate)
{
	if(ISEMPTYSTR(shmfile)){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(IsOpen()){
		Close();
	}

	// set shmfile path to back up buffer.
	// this path changes realpath in GetInode method.
	bup_shmfile	= shmfile;

	// get inode
	ino_t	inode = 0;
	if(!GetInode(inode)){
		ERR_K2HPRN("Could not get inode by shm file(%s).", bup_shmfile.c_str());
		Close();
		return false;
	}

	// make monitor file path
	if(!SetMonitorFilePath()){
		ERR_K2HPRN("Failed to make monitor file path from shm file(%s).", bup_shmfile.c_str());
		Close();
		return false;
	}

	// make SFMONWRAP
	SFMONWRAP	fmonwrap;
	memset(&fmonwrap, 0, sizeof(SFMONWRAP));
	fmonwrap.sfmon.inode_val = inode;

	// open/lock monitor file with read lock(if not exists, make it)
	if(!InitializeFileMonitor(&fmonwrap, noupdate)){
		ERR_K2HPRN("Could not initialize(create/write/open read/mmap/lock) File %s.", bup_monfile.c_str());
		Close();
		return false;
	}

	// first check inode value
	if(!noupdate){
		// if noupdate is true, inode area is locked writing in InitializeFileMonitor().
		// And call UpdateInode() and UpdateArea() methods by client function after calling this method.
		//
		bool	is_change = false;
		if(!UpdateInode(is_change)){
			ERR_K2HPRN("Something error occurred in first updating inode.");
			Close();
			return false;
		}
		if(is_change){
			// re-set all value
			bup_inode_cnt	= psfmon->inode_cnt[0];
			bup_area_cnt	= psfmon->area_cnt[0];
			bup_inode_val	= psfmon->inode_val;
		}
	}else{
		MSG_K2HPRN("Open with noupdate flag, so do not update values if monitor file exists.");
	}
	return true;
}

bool K2HFileMonitor::InitializeFileMonitor(PSFMONWRAP pfmonwrap, bool noupdate)
{
	if(!pfmonwrap){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(IsOpen()){
		ERR_K2HPRN("fmfd already open.");
		return false;
	}
	mode_t	old_umask = umask(K2HFileMonitor::file_umask);

	for(bool is_loop = true; is_loop; ){
		// test by creating
		if(-1 != (fmfd = open(bup_monfile.c_str(), O_RDWR | O_CREAT | O_EXCL | O_FSYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
			// [NOTE]
			// We lock for writing monitor file ASAP before setting file size.
			// The lock pos in file is over file size, but it works without a problem.
			//
			// write lock(blocking)
			if(!WriteLock(OPEN_LOCK_POS)){
				WAN_K2HPRN("Could not get lock for writing monitor file(%s) after creating it, probably conflict initializing by other processes, do retry...", bup_monfile.c_str());
				unlink(bup_monfile.c_str());
				CloseOnlyFile();
				continue;
			}
			MSG_K2HPRN("Monitor File %s does not exist and create it, and lock writing it", bup_monfile.c_str());

			// truncate
			if(-1 == ftruncate(fmfd, sizeof(SFMONWRAP))){
				ERR_K2HPRN("Failed to truncate to %zd monitor file %s by errno(%d), so Remove monitor file.", sizeof(SFMONWRAP), bup_monfile.c_str(), errno);
				unlink(bup_monfile.c_str());
				CloseOnlyFile();
				umask(old_umask);
				return false;
			}

			// write to file
			if(-1 == k2h_pwrite(fmfd, pfmonwrap->barray, sizeof(SFMONWRAP), 0L)){
				ERR_K2HPRN("Failed to write SFMONWRAP data to monitor file %s, so Remove monitor file.", bup_monfile.c_str());
				unlink(bup_monfile.c_str());
				CloseOnlyFile();
				umask(old_umask);
				return false;
			}

			// sync file
			if(-1 == fsync(fmfd)){
				ERR_K2HPRN("Failed to sync SFMONWRAP data to monitor file %s by errno(%d), so Remove monitor file.", bup_monfile.c_str(), errno);
				unlink(bup_monfile.c_str());
				CloseOnlyFile();
				umask(old_umask);
				return false;
			}

			// last check file exists(without checking file size)
			struct stat	st;
			if(0 != stat(bup_monfile.c_str(), &st)){
				// retry to create file.
				WAN_K2HPRN("File %s does not exist(errno=%d) after creating/writing, so try to create the file.", bup_monfile.c_str(), errno);
				unlink(bup_monfile.c_str());		// force
				CloseOnlyFile();
				continue;
			}

			// read lock(write --> read)
			if(!ReadLock(OPEN_LOCK_POS)){
				ERR_K2HPRN("Failed to read lock monitor file %s.", bup_monfile.c_str());
				CloseOnlyFile();
				umask(old_umask);
				return false;
			}

			// finished creating file
			is_loop = false;

		}else{
			// wait for initializing monitor file
			MSG_K2HPRN("Monitor File %s exists(errno=%d), so check file size and try to open it.", bup_monfile.c_str(), errno);

			// open file
			if(-1 == (fmfd = open(bup_monfile.c_str(), O_RDWR))){
				if(ENOENT != errno){
					ERR_K2HPRN("Could not open file %s by errno(%d).", bup_monfile.c_str(), errno);
					umask(old_umask);
					return false;
				}
				// retry to create file.
				WAN_K2HPRN("File %s does not exist(errno=%d), so try to create the file.", bup_monfile.c_str(), errno);
				continue;
			}

			// read lock
			if(!ReadLock(OPEN_LOCK_POS)){
				ERR_K2HPRN("Failed to read lock monitor file %s.", bup_monfile.c_str());
				CloseOnlyFile();
				umask(old_umask);
				return false;
			}

			// check the file exists
			struct stat	st;
			if(0 == stat(bup_monfile.c_str(), &st)){
				if(sizeof(SFMONWRAP) != st.st_size){
					// try to get write lock
					if(WriteLock(OPEN_LOCK_POS, false)){
						// [NOTE]
						// This case which is opened file(exists) -> got read lock -> got file stat -> BUT file size is wrong & GOT write lock.
						// It means probably the process which initialize the file exited during initializing it.
						// So the file is wrong, we need to remove it for no dead locking.
						//
						WAN_K2HPRN("Got lock for writing monitor file(%s) after (opening -> get read lock -> file size is wrong), it means no write/read process to it, and is needed to initialize.", bup_monfile.c_str());
						unlink(bup_monfile.c_str());
						CloseOnlyFile();
					}else{
						WAN_K2HPRN("file %s size is not sizeof(SFMONWRAP) after getting read lock, do retry for waiting initializing.", bup_monfile.c_str());
						CloseOnlyFile();

						struct timespec	sleeptime = {0L, 100 * 1000};	// 100us
						nanosleep(&sleeptime, NULL);
					}
					// do retry

				}else{
					// finished opening correct file
					MSG_K2HPRN("file %s size is sizeof(SFMONWRAP), it is safe file.", bup_monfile.c_str());
					is_loop = false;
				}
			}else{
				if(ENOENT != errno){
					ERR_K2HPRN("Could not get stat for file %s by errno(%d).", bup_monfile.c_str(), errno);
					CloseOnlyFile();
					umask(old_umask);
					return false;
				}
				// monitor file does not exist, retry to create file.
				CloseOnlyFile();
				WAN_K2HPRN("File %s does not exist(errno=%d), so try to create the file.", bup_monfile.c_str(), errno);
			}
		}
	}
	umask(old_umask);

	// mmap
	void*	pmmap;
	if(MAP_FAILED == (pmmap = mmap(NULL, sizeof(SFMONWRAP), PROT_READ | PROT_WRITE, MAP_SHARED, fmfd, 0L))){
		ERR_K2HPRN("Could not mmap file(%s), errno = %d", bup_monfile.c_str(), errno);
		Close();
		return false;
	}
	psfmon = reinterpret_cast<PSFMON>(pmmap);

	// write lock during initializing
	if(noupdate){
		if(!WriteLock(INODE_LOCK_POS)){
			ERR_K2HPRN("Failed to write lock inode monitor file %s.", bup_monfile.c_str());
			Close();
			return false;
		}
		MSG_K2HPRN("InitializeFileMonitor with noupdate flag, so lock writing inode if monitor file exists.");
	}

	// init value and counters(about inode are already update in UpdateInode)
	bup_area_cnt = psfmon->area_cnt[0];

	return true;
}

bool K2HFileMonitor::CheckInode(bool& is_change, bool valupdate)
{
	if(!IsOpen()){
		ERR_K2HPRN("Monitor File is not opened.");
		return false;
	}

	is_change = false;
	if(bup_inode_cnt != psfmon->inode_cnt[0]){
		if(!ReadLock(INODE_LOCK_POS)){
			ERR_K2HPRN("Could not lock for inode check.");
			return false;
		}
		if(bup_inode_val != psfmon->inode_val){
			MSG_K2HPRN("k2hash file inode is not same, it means file is updated!");
			is_change = true;

			if(valupdate){
				bup_inode_cnt	= psfmon->inode_cnt[0];
				bup_inode_val	= psfmon->inode_val;
			}
		}else{
			is_change = false;
		}
		Unlock(INODE_LOCK_POS);
	}
	return true;
}

bool K2HFileMonitor::CheckArea(bool& is_change, bool valupdate)
{
	if(!IsOpen()){
		ERR_K2HPRN("Monitor File is not opened.");
		return false;
	}

	if(bup_area_cnt != psfmon->area_cnt[0]){
		MSG_K2HPRN("k2hash area counter is not same, it means the area is updated!");
		is_change = true;

		if(valupdate){
			if(!ReadLock(AREA_LOCK_POS)){
				ERR_K2HPRN("Could not lock for area check.");
				return false;
			}
			bup_area_cnt = psfmon->area_cnt[0];
			Unlock(AREA_LOCK_POS);
		}
	}else{
		is_change = false;
	}
	return true;
}

bool K2HFileMonitor::UpdateInode(bool& is_change)
{
	if(!IsOpen()){
		ERR_K2HPRN("Monitor File is not opened.");
		return false;
	}
	is_change = false;

	// lock
	if(!WriteLock(INODE_LOCK_POS)){
		ERR_K2HPRN("Could not write lock for inode update.");
		return false;
	}

	// get now inode
	ino_t	inode = 0;
	if(!GetInode(inode)){
		ERR_K2HPRN("Could not get inode by shm file(%s).", bup_shmfile.c_str());
		Unlock(INODE_LOCK_POS);
		return false;
	}

	// check inode
	if(inode == psfmon->inode_val){
		//MSG_K2HPRN("inode is same, so nothing to do.");
		bup_inode_cnt = psfmon->inode_cnt[0];
		bup_inode_val = psfmon->inode_val;
		Unlock(INODE_LOCK_POS);
		return true;
	}
	MSG_K2HPRN("k2hash file inode is not same, update inode values in monitor file.");

	// inode is changed, so update
	++(psfmon->inode_cnt[0]);
	psfmon->inode_val = inode;
	if(-1 == msync(psfmon, sizeof(SFMONWRAP), MS_SYNC)){
		WAN_K2HPRN("Failed msync.");
		// continue...
	}

	// update backup values
	bup_inode_cnt = psfmon->inode_cnt[0];
	bup_inode_val = psfmon->inode_val;

	// Unlock
	Unlock(INODE_LOCK_POS);

	is_change = true;

	return true;
}

// [NOTE]
// If area_cnt is changed by another process/thread before calling this method,
// we need to check updating area after count up area_cnt.
// This method returns the flag for "need to check area".
//
bool K2HFileMonitor::UpdateArea(bool& is_need_check)
{
	is_need_check = false;

	if(!IsOpen()){
		ERR_K2HPRN("Monitor File is not opened.");
		return false;
	}

	if(!WriteLock(AREA_LOCK_POS)){
		ERR_K2HPRN("Could not write lock for area update.");
		return false;
	}
	MSG_K2HPRN("update k2hash area counter in monitor file.");

	if(bup_area_cnt != psfmon->area_cnt[0]){
		is_need_check = true;
	}

	// update
	++(psfmon->area_cnt[0]);
	if(-1 == msync(psfmon, sizeof(SFMONWRAP), MS_SYNC)){
		WAN_K2HPRN("Failed msync.");
		// continue...
	}

	// update backup values
	bup_area_cnt = psfmon->area_cnt[0];

	// Unlock
	Unlock(AREA_LOCK_POS);

	return true;
}

bool K2HFileMonitor::GetInode(ino_t& inode)
{
	if(bup_shmfile.empty()){
		ERR_K2HPRN("shmfile path is empty.");
		return false;
	}

	// get real path and reset backup path
	char*	prealpath;
	if(NULL == (prealpath = realpath(bup_shmfile.c_str(), NULL))){
		WAN_K2HPRN("Could not get real path from %s, so do not change backup path and continue.", bup_shmfile.c_str());
	}else{
		bup_shmfile	= prealpath;
		K2H_Free(prealpath);
	}

	// check
	struct stat	st;
	if(0 != stat(bup_shmfile.c_str(), &st)){
		ERR_K2HPRN("File %s does not exist.", bup_shmfile.c_str());
		return false;
	}
	if(!S_ISREG(st.st_mode)){
		ERR_K2HPRN("File %s is not regular file.", bup_shmfile.c_str());
		return false;
	}
	inode = st.st_ino;

	return true;
}

bool K2HFileMonitor::SetMonitorFilePath(void)
{
	if(bup_shmfile.empty()){
		ERR_K2HPRN("shmfile path is empty.");
		return false;
	}

	// make hash code
	k2h_hash_t	hash = K2H_HASH_FUNC(bup_shmfile.c_str(), bup_shmfile.length());

	// make monitor file path
	char	szBuff[32];
	sprintf(szBuff, "%" PRIx64, hash);

	bup_monfile = K2HFileMonitor::base_prefix;
	bup_monfile += szBuff;

	MSG_K2HPRN("k2hash monitor file path = %s", bup_monfile.c_str());

	return true;
}

// [NOTE]
// Here, we do not use fullock_rwlock, because fcntl is switch writer to reader lock
// directly. But when fullock change lock type to writer to reader, do unlock.
// Then there is possibility which another process(thread) gets write lock.
//
bool K2HFileMonitor::RawLock(short type, off_t offset, bool block) const
{
	if(-1 == fmfd){
		ERR_K2HPRN("fmfd is not opened.");
		return false;
	}

	struct flock	fl;
	fl.l_type	= type;
	fl.l_whence	= SEEK_SET;
	fl.l_start	= offset;
	fl.l_len	= 1L;

	while(true){
		if(0 == fcntl(fmfd, block ? F_SETLKW : F_SETLK, &fl)){
			MSG_K2HPRN("Monitor file is Locked %s mode: offset=%jd, %s", F_UNLCK == type ? "unlock" : F_RDLCK == type ? "read " : F_WRLCK == type ? "write " : "unknown ", static_cast<intmax_t>(offset), block ? "blocked" : "non blocked");
			return true;
		}
		if(!block){
			if(EACCES != errno && EAGAIN != errno){
				MSG_K2HPRN("Could not %slock because something unnormal wrong occured. errno=%d", (F_UNLCK == type ? "un" : ""), errno);
			}
			break;
		}else{
			if(EINTR != errno){
				MSG_K2HPRN("Could not %slock because something unnormal wrong occured. errno=%d", (F_UNLCK == type ? "un" : ""), errno);
				break;
			}else{
				MSG_K2HPRN("Signal occurred during %slocking.", F_UNLCK == type ? "un" : "");
			}
		}
	}
    return false;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
