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
#ifndef	K2HFILEMONITOR_H
#define	K2HFILEMONITOR_H

// 
// [NOTICE]
// 
// This class is for checking k2hash file replaced and mmap erea expanded.
// The logic for these is check one byte data in temporary file for each 
// k2hash file. The temporary file makes this class from k2hash file path 
// which has prefix ".k2_" in "/tmp" directory. After prefix for the file 
// is number of 64bit hex string which is made from hashed k2hash file 
// full path.
// 
// So the temporary file has three bytes arraies, those arraies is 8 bytes 
// and this class uses first one byte. This class locks only first byte
// (one byte). One of arraies is for checking whichever this temporary 
// file is opened, it is "open_lock" member in structure. So the temporary 
// file should be removed when all process detach the k2hash file, this 
// member is for checking lastest process. Other two arraies are for 
// checking inode of k2hash file and area increment(decrement). If k2hash 
// file is replaced another file, this process needs to reload new one.
// So that this "inode_cnt" is increment by another process which replaces 
// k2hash file. Then this process(class) checks "inode_cnt" byte, if it 
// is changed, this process must reload the file. Tha same way, if mmap 
// area is expanded by another process, "area_cnt" is increment.
// 
// The reason of only one byte is that we do not use lock logic for 
// reading buzz flag data. The replacing file and expanding area is not 
// often occurred, and all process is only reading flag. So the proces 
// which changes these flag only locks this flag and etc, other processes 
// do not need to lock it. But other processes lock flags when they read 
// value(inode_val).
// 
// In other words this class minimizes lock time and can confirm the 
// k2hash file update.
// 
//---------------------------------------------------------
// Structures
//---------------------------------------------------------
typedef struct share_file_monitor{
	unsigned char	open_lock[sizeof(long)];	// Use only first byte
	unsigned char	inode_cnt[sizeof(long)];	// Use only first byte
	unsigned char	area_cnt[sizeof(long)];		// Use only first byte
	ino_t			inode_val;
}K2HASH_ATTR_PACKED SFMON, *PSFMON;

typedef union share_file_monitor_wrap{
	unsigned char	barray[sizeof(SFMON)];
	SFMON			sfmon;
}K2HASH_ATTR_PACKED SFMONWRAP, *PSFMONWRAP;

//---------------------------------------------------------
// Class K2HFileMonitor
//---------------------------------------------------------
class K2HFileMonitor
{
	private:
		static const char	base_prefix[];
		static mode_t		file_umask;								// umask for monitor file

		std::string			bup_shmfile;
		std::string			bup_monfile;
		PSFMON				psfmon;
		int					fmfd;
		unsigned char		bup_inode_cnt;
		unsigned char		bup_area_cnt;
		ino_t				bup_inode_val;

	public:
		K2HFileMonitor();
		~K2HFileMonitor();

		bool IsOpen(void) const { return (psfmon && -1 != fmfd); }
		bool Open(const char* shmfile, bool noupdate = false);		// If noupdate is true, inode area is locked writing. must call UpdateInode after open.
		bool Close(void);

		bool PreCheckInode(bool& is_change) { return CheckInode(is_change, false); }
		bool PreCheckArea(bool& is_change) { return CheckArea(is_change, false); }
		bool CheckInode(bool& is_change) { return CheckInode(is_change, true); }
		bool CheckArea(bool& is_change) { return CheckArea(is_change, true); }

		bool UpdateInode(bool& is_change);
		bool UpdateArea(bool& is_need_check);

	private:
		bool CloseOnlyFile(void);
		bool InitializeFileMonitor(PSFMONWRAP pfmonwrap, bool noupdate);

		bool CheckInode(bool& is_change, bool valupdate);
		bool CheckArea(bool& is_change, bool valupdate);

		bool GetInode(ino_t& inode);
		bool SetMonitorFilePath(void);

		bool RawLock(short type, off_t offset, bool block) const;
		bool Unlock(off_t offset, bool block = true) const { return RawLock(F_UNLCK, offset, block); }
		bool ReadLock(off_t offset, bool block = true) const { return RawLock(F_RDLCK, offset, block); }
		bool WriteLock(off_t offset, bool block = true) const { return RawLock(F_WRLCK, offset, block); }

	public:
};

#endif	// K2HFILEMONITOR_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
