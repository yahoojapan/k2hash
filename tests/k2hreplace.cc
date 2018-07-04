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
 * CREATE:   Tue Apr 22 2014
 * REVISION:
 *
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <libgen.h>
#include <string>

#include <k2hash.h>
#include <k2hcommon.h>
#include <k2hshm.h>
#include <k2hdbg.h>
#include <k2hutil.h>

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	OLDFILE_SUFFIX		".BAK"

//---------------------------------------------------------
// Utilities
//---------------------------------------------------------
static inline void PRN(const char* format, ...)
{
	if(format){
		va_list ap;
		va_start(ap, format);
		vfprintf(stdout, format, ap); 
		va_end(ap);
	}
	fprintf(stdout, "\n");
}

static inline void ERR(const char* format, ...)
{
	fprintf(stderr, "[ERR] ");
	if(format){
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format, ap); 
		va_end(ap);
	}
	fprintf(stderr, "\n");
}

static inline char* programname(char* prgpath)
{
	if(!prgpath){
		return NULL;
	}
	char*	pprgname = basename(prgpath);
	if(0 == strncmp(pprgname, "lt-", strlen("lt-"))){
		pprgname = &pprgname[strlen("lt-")];
	}
	return pprgname;
}

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
// Parse parameters
//
// SOURCE               source file path
// DEST                 destination k2hash file path
//
static void Help(char* progname)
{
	PRN("");
	PRN("Usage: %s [-d] [-no_rm_bup] SOURCE DEST", progname ? programname(progname) : "program");
	PRN("");
	PRN("K2HASH REPLACE TOOL: Replace k2hash file from SOURCE to DEST.");
	PRN("");
	PRN("The source file is moved destination file path, and destination");
	PRN("file is moved with suffix.");
	PRN("");
	PRN("SOURCE               source file path.");
	PRN("DEST                 destination k2hash file path.");
	PRN("-no_rm_bup           not remove backup files before working.");
	PRN("-d                   debug option.");
	PRN("");
}

static bool ParameerParser(int argc, char** argv, bool& isdbg, bool& is_remove_old, string& srcfile, string& destfile)
{
	if(argc < 3 || 5 < argc){
		ERR("Parameters are wrong.");
		return false;
	}
	isdbg			= false;
	is_remove_old	= true;		// default

	int	pos = 1;
	if(5 == argc){
		if(ISEMPTYSTR(argv[pos]) || ISEMPTYSTR(argv[pos + 1])){
			ERR("first or second option is empty.");
			return false;
		}
		if(0 == strcasecmp(argv[pos], "-d")){
			isdbg = true;
		}else if(0 == strcasecmp(argv[pos], "-no_rm_bup")){
			is_remove_old = false;
		}else{
			ERR("1\'st parameter %s is wrong, should be \"-d\" or \"-no_rm_bup\" option.", argv[pos]);
			return false;
		}
		pos++;

		if(0 == strcasecmp(argv[pos], "-d")){
			isdbg = true;
		}else if(0 == strcasecmp(argv[pos], "-no_rm_bup")){
			is_remove_old = false;
		}else{
			ERR("2\'st parameter %s is wrong, should be \"-d\" or \"-no_rm_bup\" option.", argv[pos]);
			return false;
		}
		pos++;
	}else if(4 == argc){
		if(ISEMPTYSTR(argv[pos])){
			ERR("first option is empty?");
			return false;
		}
		if(0 == strcasecmp(argv[pos], "-d")){
			isdbg = true;
		}else if(0 == strcasecmp(argv[pos], "-no_rm_bup")){
			is_remove_old = false;
		}else{
			ERR("1\'st parameter %s is wrong, should be \"-d\" or \"-no_rm_bup\" option.", argv[pos]);
			return false;
		}
		pos++;
	}
	if(ISEMPTYSTR(argv[pos])){
		ERR("Source file path is empty.");
		return false;
	}
	if(ISEMPTYSTR(argv[pos + 1])){
		ERR("Destination file path is empty.");
		return false;
	}
	srcfile		= argv[pos];
	destfile	= argv[pos + 1];

	return true;
}

static bool CheckExistFile(const char* file)
{
	if(ISEMPTYSTR(file)){
		return false;
	}
	struct stat	st;
	if(0 != stat(file, &st)){
		return false;
	}
	if(!S_ISREG(st.st_mode)){
		return false;
	}
	return true;
}

static bool MakeBackupFilePath(const char* destfile, string& bupfile, bool is_remove_old = false)
{
	if(ISEMPTYSTR(destfile)){
		ERR("Parameters are wrong.");
		return false;
	}
	string	basepath = destfile;
	basepath += OLDFILE_SUFFIX;

	if(is_remove_old){
		for(int cnt = 0; true; cnt++){
			bupfile = basepath;
			if(0 != cnt){
				char	szBuff[32];
				sprintf(szBuff, "%d", cnt);
				bupfile += szBuff;
			}
			if(!CheckExistFile(bupfile.c_str())){
				// bupfile does not exist.
				break;
			}
			if(-1 == unlink(bupfile.c_str())){
				ERR("Could not unlink file %s. errno=%d", bupfile.c_str(), errno);
				break;
			}
		}
	}
	for(int cnt = 0; true; cnt++){
		bupfile = basepath;
		if(0 != cnt){
			char	szBuff[32];
			sprintf(szBuff, "%d", cnt);
			bupfile += szBuff;
		}
		if(!CheckExistFile(bupfile.c_str())){
			// bupfile does not exist.
			break;
		}
	}
	return true;
}

//---------------------------------------------------------
// Main
//---------------------------------------------------------
int main(int argc, char** argv)
{
	K2HShm	k2hash;
	bool	isdbg			= false;
	bool	is_remove_old	= true;
	string	srcfile;
	string	destfile;
	string	bupfile;

	// check parameters
	if(!ParameerParser(argc, argv, isdbg, is_remove_old, srcfile, destfile)){
		Help(argv[0]);
		exit(-1);
	}
	if(!CheckExistFile(srcfile.c_str())){
		ERR("File %s does not exist.", srcfile.c_str());
		exit(-1);
	}
	if(!CheckExistFile(destfile.c_str())){
		ERR("File %s does not exist.", destfile.c_str());
		exit(-1);
	}
	if(!MakeBackupFilePath(destfile.c_str(), bupfile, is_remove_old)){
		ERR("Something error occurred.");
		exit(-1);
	}

	// dbg mode
	if(isdbg){
		SetK2hDbgMode(K2HDBG_MSG);
	}

	// move old file to backup
	if(-1 == link(destfile.c_str(), bupfile.c_str())){
		ERR("Could not link file %s to %s. errno=%d", destfile.c_str(), bupfile.c_str(), errno);
		exit(-1);
	}
	if(-1 == unlink(destfile.c_str())){
		ERR("Could not unlink file %s. errno=%d", destfile.c_str(), errno);
		exit(-1);
	}

	// move new file to old file
	if(-1 == link(srcfile.c_str(), destfile.c_str())){
		ERR("Could not link file %s to %s. errno=%d", srcfile.c_str(), destfile.c_str(), errno);
		exit(-1);
	}
	if(-1 == unlink(srcfile.c_str())){
		ERR("Could not unlink file %s. errno=%d", srcfile.c_str(), errno);
		exit(-1);
	}

	// attach k2hash ( update monitor file automatically )
	if(!k2hash.Attach(destfile.c_str(), true, false, false, false)){
		ERR("Could not attach replaced file %s", destfile.c_str());
		exit(-1);
	}

	// detach k2hash
	if(!k2hash.Detach()){
		ERR("Failed to detach k2hash.");
	}

	PRN("Succeed to replace %s to %s k2hash file, and push notice other processes which open the k2hash file.", srcfile.c_str(), destfile.c_str());

	exit(0);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */

