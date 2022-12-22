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
#define	MONITORFILE_DIRPATH		"/var/lib/antpickax"
#define	MONITORFILE_DIRPATH_SUB	"/tmp"
#define	MONITORFILE_BASE_NAME	".k2h_"

#define	OPEN_LOCK_POS			0L
#define	INODE_LOCK_POS			static_cast<off_t>(sizeof(unsigned char) * sizeof(long))
#define	AREA_LOCK_POS			(INODE_LOCK_POS + static_cast<off_t>(sizeof(unsigned char) * sizeof(long)))
#define	INODE_VAL_POS			(AREA_LOCK_POS + static_cast<off_t>(sizeof(unsigned char) * sizeof(long)))
#define	WAIT_INITOPEN_TIME		100				// 100ms
#define	WAIT_INITOPEN_MAXTIME	(30 * 1000)		// 30s
#define	WAIT_INITOPEN_CNT		(WAIT_INITOPEN_MAXTIME / WAIT_INITOPEN_TIME)

#define	ARR_INODECNT_VALPOS		1
#define	ARR_AREACNT_VALPOS		1

//---------------------------------------------------------
// const variables in local
//---------------------------------------------------------
const char		_base_prefix[]		= MONITORFILE_DIRPATH "/" MONITORFILE_BASE_NAME;
const char		_base_prefix_sub[]	= MONITORFILE_DIRPATH_SUB "/" MONITORFILE_BASE_NAME;

//---------------------------------------------------------
// Class variable
//---------------------------------------------------------
const char*		K2HFileMonitor::base_prefix		= NULL;
mode_t			K2HFileMonitor::file_umask		= 0;

//---------------------------------------------------------
// Constructor / Destructor
//---------------------------------------------------------
K2HFileMonitor::K2HFileMonitor() : bup_shmfile(""), bup_monfile(""), psfmon(NULL), fmfd(-1), bup_inode_cnt(0), bup_area_cnt(0), bup_inode_val(0)
{
	if(!K2HFileMonitor::base_prefix){
		struct stat	st;
		if(0 != stat(MONITORFILE_DIRPATH, &st)){
			WAN_K2HPRN("%s directory is not existed, then use %s", MONITORFILE_DIRPATH, MONITORFILE_DIRPATH_SUB);
			K2HFileMonitor::base_prefix = _base_prefix_sub;
		}else{
			if(0 == (st.st_mode & S_IFDIR)){
				WAN_K2HPRN("%s is not directory, then use %s", MONITORFILE_DIRPATH, MONITORFILE_DIRPATH_SUB);
				K2HFileMonitor::base_prefix = _base_prefix_sub;
			}else{
				K2HFileMonitor::base_prefix = _base_prefix;
			}
		}
	}
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

// [NOTE]
// In older versions, the last process attached to the k2hash file
// deleted the monitor file on exit.
// However, this method causes failure at the very short period of
// timing. If a process can get a Write Lock for a monitor file at
// the process end, it means that it is the last process. And this
// last process deletes the file after getting the Write Lock.
// However, when another process attaches to the same k2hash file
// in a short period of time after last process is acquiring the
// Write Lock and before deleting it, the monitor file may be opened
// (and waiting for the Read Lock).
// This case will occure propabry fatal error.
// Therefore, we changed this method so that the monitor file is not
// deleted.
//
bool K2HFileMonitor::CloseOnlyFile(void)
{
	if(psfmon){
		munmap(psfmon, sizeof(SFMONWRAP));
		psfmon = NULL;
	}
	if(-1 != fmfd){
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
		if(!CheckInode(is_change)){
			ERR_K2HPRN("Something error occurred in first checking inode.");
			Close();
			return false;
		}
		if(is_change){
			// update inode
			if(!UpdateInode(is_change)){
				ERR_K2HPRN("Something error occurred in first updating inode.");
				Close();
				return false;
			}
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
		struct timespec	sleeptime = {0L, 100 * 1000};					// 100us
		struct stat		st;
		bool			has_write_lock;
		bool			need_init_size;

		has_write_lock	= false;
		need_init_size	= false;

		// open or create file
		if(-1 != (fmfd = open(bup_monfile.c_str(), O_RDWR | O_CREAT | O_EXCL | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
			// created new file

			// [NOTE]
			// We lock for writing monitor file ASAP before setting file size.
			// The lock pos in file is over file size, but it works without a problem.
			//
			// get write lock(open)
			if(!WriteLock(OPEN_LOCK_POS, false)){
				WAN_K2HPRN("Could not get lock for writing monitor file(%s) after creating it, probably conflict initializing by other processes, do retry...", bup_monfile.c_str());
				CloseOnlyFile();
				nanosleep(&sleeptime, NULL);
				continue;
			}
			has_write_lock = true;

		}else if(-1 != (fmfd = open(bup_monfile.c_str(), O_RDWR))){
			// opened existed file

			// [NOTE]
			// First, try to get write lock.
			// This is to detect that the file exists but is not initialized.
			// If multiple processes perform initialization at the same time,
			// other processes may open the file before the process that created
			// the file acquires the write lock.
			//
			if(WriteLock(OPEN_LOCK_POS, false)){
				// got write lock(open)
				WAN_K2HPRN("Got lock for writing monitor file(%s) after opening it, then this case needs to initialize.", bup_monfile.c_str());
				has_write_lock = true;

			}else if(ReadLock(OPEN_LOCK_POS, false)){
				// get read lock(open)
				has_write_lock = false;

			}else{
				// probabry another process is initializing now.
				WAN_K2HPRN("Failed to get read lock monitor file %s, probabry another process is initializing now. then retry...", bup_monfile.c_str());
				CloseOnlyFile();
				nanosleep(&sleeptime, NULL);
				continue;
			}

		}else{
			// could not open/create file
			// cppcheck-suppress knownConditionTrueFalse
			if(ENOENT != errno){
				ERR_K2HPRN("Could not create/open file %s by errno(%d).", bup_monfile.c_str(), errno);
				umask(old_umask);
				return false;
			}
			// retry to open/create file.
			WAN_K2HPRN("File %s does not exist(errno=%d), so try to create the file.", bup_monfile.c_str(), errno);
			continue;
		}

		// check the file exists and file size(do not use fd)
		if(0 == stat(bup_monfile.c_str(), &st)){
			if(sizeof(SFMONWRAP) != st.st_size){
				if(!has_write_lock){
					// [NOTE]
					// The monitor file size is incorrect, even though an existing file
					// was opened without write lock.
					// This may be due to another process initializing, or other reasons.
					// Therefore, it is necessary to start over from the file open again.
					//
					WAN_K2HPRN("file %s size is not sizeof(SFMONWRAP) after getting only read lock, do retry for waiting initializing.", bup_monfile.c_str());
					Unlock(OPEN_LOCK_POS);
					CloseOnlyFile();
					nanosleep(&sleeptime, NULL);
					continue;
				}
				need_init_size = true;

			}else{
				// correct file size
				MSG_K2HPRN("file %s size is sizeof(SFMONWRAP), it is safe file.", bup_monfile.c_str());
				need_init_size = false;
			}

		}else{
			// file does not exist
			Unlock(OPEN_LOCK_POS);
			CloseOnlyFile();

			// cppcheck-suppress knownConditionTrueFalse
			if(ENOENT != errno){
				ERR_K2HPRN("Could not get stat for file %s by errno(%d).", bup_monfile.c_str(), errno);
				umask(old_umask);
				return false;
			}
			// retry to create file.
			WAN_K2HPRN("File %s does not exist(errno=%d), so try to create the file.", bup_monfile.c_str(), errno);
			nanosleep(&sleeptime, NULL);
			continue;
		}

		// initialize file
		if(has_write_lock){
			// set file size
			if(need_init_size){
				if(-1 == ftruncate(fmfd, sizeof(SFMONWRAP))){
					WAN_K2HPRN("Failed to truncate to %zu monitor file %s by errno(%d), so Remove monitor file and Retry.", sizeof(SFMONWRAP), bup_monfile.c_str(), errno);
					unlink(bup_monfile.c_str());
					Unlock(OPEN_LOCK_POS);
					CloseOnlyFile();
					nanosleep(&sleeptime, NULL);
					continue;
				}
			}

			// write to file
			if(-1 == k2h_pwrite(fmfd, pfmonwrap->barray, sizeof(SFMONWRAP), 0L)){
				WAN_K2HPRN("Failed to write SFMONWRAP data to monitor file %s, so Remove monitor file and Retry.", bup_monfile.c_str());
				unlink(bup_monfile.c_str());
				Unlock(OPEN_LOCK_POS);
				CloseOnlyFile();
				nanosleep(&sleeptime, NULL);
				continue;
			}

			// force sync file
			if(-1 == fsync(fmfd)){
				WAN_K2HPRN("Failed to sync SFMONWRAP data to monitor file %s by errno(%d), so Remove monitor file and Retry.", bup_monfile.c_str(), errno);
				unlink(bup_monfile.c_str());
				Unlock(OPEN_LOCK_POS);
				CloseOnlyFile();
				nanosleep(&sleeptime, NULL);
				continue;
			}

			// change lock type to read from write(open)
			if(!ReadLock(OPEN_LOCK_POS)){
				WAN_K2HPRN("Failed to read lock monitor file %s, so retry...", bup_monfile.c_str());
				Unlock(OPEN_LOCK_POS);
				CloseOnlyFile();
				nanosleep(&sleeptime, NULL);
				continue;
			}
		}

		// mmap
		void*	pmmap;
		if(MAP_FAILED != (pmmap = mmap(NULL, sizeof(SFMONWRAP), PROT_READ | PROT_WRITE, MAP_SHARED, fmfd, 0L))){
			// succeed mmap, finally check
			if(0 != stat(bup_monfile.c_str(), &st) || sizeof(SFMONWRAP) != st.st_size){
				WAN_K2HPRN("File %s does not exist or its size is incorrect after mmap, then retry...", bup_monfile.c_str());
				munmap(pmmap, sizeof(SFMONWRAP));
				Unlock(OPEN_LOCK_POS);
				CloseOnlyFile();
				nanosleep(&sleeptime, NULL);
				continue;
			}
			// break loop
			psfmon	= reinterpret_cast<PSFMON>(pmmap);
			is_loop	= false;
		}else{
			WAN_K2HPRN("Could not mmap file(%s), errno = %d", bup_monfile.c_str(), errno);
			Unlock(OPEN_LOCK_POS);
			CloseOnlyFile();
			nanosleep(&sleeptime, NULL);
			//continue;
		}
	}

	// write lock during initializing
	if(!noupdate){
		if(!WriteLock(INODE_LOCK_POS)){
			ERR_K2HPRN("Failed to write lock inode monitor file %s.", bup_monfile.c_str());
			Close();
			return false;
		}
		MSG_K2HPRN("InitializeFileMonitor with noupdate flag, so lock writing inode if monitor file exists.");
	}

	// update area count value
	if(!ReadLock(AREA_LOCK_POS)){
		ERR_K2HPRN("Could not lock for area check.");
		Close();
		return false;
	}
	bup_area_cnt = psfmon->area_cnt[ARR_AREACNT_VALPOS];
	Unlock(AREA_LOCK_POS);

	return true;
}

bool K2HFileMonitor::CheckInode(bool& is_change, bool valupdate)
{
	if(!IsOpen()){
		ERR_K2HPRN("Monitor File is not opened.");
		return false;
	}

	is_change = false;
	if(	bup_inode_cnt != psfmon->inode_cnt[ARR_INODECNT_VALPOS]	||
		bup_inode_val != psfmon->inode_val						)
	{
		if(!ReadLock(INODE_LOCK_POS)){
			ERR_K2HPRN("Could not lock for inode check.");
			return false;
		}
		if(	bup_inode_cnt != psfmon->inode_cnt[ARR_INODECNT_VALPOS]	||
			bup_inode_val != psfmon->inode_val						)
		{
			MSG_K2HPRN("k2hash file inode is not same, it means file is updated!");
			is_change = true;

			if(valupdate){
				bup_inode_cnt	= psfmon->inode_cnt[ARR_INODECNT_VALPOS];
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

	if(bup_area_cnt != psfmon->area_cnt[ARR_AREACNT_VALPOS]){
		MSG_K2HPRN("k2hash area counter is not same, it means the area is updated!");
		is_change = true;

		if(valupdate){
			if(!ReadLock(AREA_LOCK_POS)){
				ERR_K2HPRN("Could not lock for area check.");
				return false;
			}
			bup_area_cnt = psfmon->area_cnt[ARR_AREACNT_VALPOS];
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
		bup_inode_cnt = psfmon->inode_cnt[ARR_INODECNT_VALPOS];
		bup_inode_val = psfmon->inode_val;
		Unlock(INODE_LOCK_POS);
		return true;
	}
	MSG_K2HPRN("k2hash file inode is not same, update inode values in monitor file.");

	// inode is changed, so update
	++(psfmon->inode_cnt[ARR_INODECNT_VALPOS]);
	psfmon->inode_val = inode;
	if(-1 == msync(psfmon, sizeof(SFMONWRAP), MS_SYNC)){
		WAN_K2HPRN("Failed msync.");
		// continue...
	}

	// update backup values
	bup_inode_cnt = psfmon->inode_cnt[ARR_INODECNT_VALPOS];
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

	if(bup_area_cnt != psfmon->area_cnt[ARR_AREACNT_VALPOS]){
		is_need_check = true;
	}

	// update
	++(psfmon->area_cnt[ARR_AREACNT_VALPOS]);
	if(-1 == msync(psfmon, sizeof(SFMONWRAP), MS_SYNC)){
		WAN_K2HPRN("Failed msync.");
		// continue...
	}

	// update backup values
	bup_area_cnt = psfmon->area_cnt[ARR_AREACNT_VALPOS];

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
	// cppcheck-suppress internalAstError
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
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
