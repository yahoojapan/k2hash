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
 * CREATE:   Fri Dec 2 2013
 * REVISION:
 *
 */
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>

#include "k2hdbg.h"

//---------------------------------------------------------
// Class K2HDbgControl
//---------------------------------------------------------
class K2HDbgControl
{
	protected:
		static const char*		DBGENVNAME;
		static const char*		DBGENVFILE;
		static K2HDbgControl	singleton;
		static bool				isSetSignal;

	public:
		static bool LoadEnv(void);
		static bool LoadEnvName(void);
		static bool LoadEnvFile(void);
		static void User1Handler(int Signal);
		static bool SetUser1Handler(bool isEnable);

		K2HDbgControl();
		virtual ~K2HDbgControl();
};

// Class valiables
const char*		K2HDbgControl::DBGENVNAME = "K2HDBGMODE";
const char*		K2HDbgControl::DBGENVFILE = "K2HDBGFILE";
K2HDbgControl	K2HDbgControl::singleton;
bool			K2HDbgControl::isSetSignal=	false;

// Constructor / Destructor
K2HDbgControl::K2HDbgControl()
{
	K2HDbgControl::LoadEnv();
}
K2HDbgControl::~K2HDbgControl()
{
	K2HDbgControl::SetUser1Handler(false);
}

// Class Methods
bool K2HDbgControl::LoadEnv(void)
{
	if(!K2HDbgControl::LoadEnvName() || !K2HDbgControl::LoadEnvFile()){
		return false;
	}
	return true;
}

bool K2HDbgControl::LoadEnvName(void)
{
	char*	pEnvVal;
	if(NULL == (pEnvVal = getenv(K2HDbgControl::DBGENVNAME))){
		MSG_K2HPRN("%s ENV is not set.", K2HDbgControl::DBGENVNAME);
		return true;
	}
	if(0 == strcasecmp(pEnvVal, "SILENT")){
		SetK2hDbgMode(K2HDBG_SILENT);
	}else if(0 == strcasecmp(pEnvVal, "ERR")){
		SetK2hDbgMode(K2HDBG_ERR);
	}else if(0 == strcasecmp(pEnvVal, "WAN")){
		SetK2hDbgMode(K2HDBG_WARN);
	}else if(0 == strcasecmp(pEnvVal, "INFO")){
		SetK2hDbgMode(K2HDBG_MSG);
	}else{
		MSG_K2HPRN("%s ENV is not unknown string(%s).", K2HDbgControl::DBGENVNAME, pEnvVal);
		return false;
	}
	return true;
}

bool K2HDbgControl::LoadEnvFile(void)
{
	char*	pEnvVal;
	if(NULL == (pEnvVal = getenv(K2HDbgControl::DBGENVFILE))){
		MSG_K2HPRN("%s ENV is not set.", K2HDbgControl::DBGENVFILE);
		return true;
	}
	if(!SetK2hDbgFile(pEnvVal)){
		MSG_K2HPRN("%s ENV is unsafe string(%s).", K2HDbgControl::DBGENVFILE, pEnvVal);
		return false;
	}
	return true;
}

void K2HDbgControl::User1Handler(int Signal)
{
	MSG_K2HPRN("Caught signal SIGUSR1(%d), bumpup the logging level.", Signal);
	BumpupK2hDbgMode();
}

bool K2HDbgControl::SetUser1Handler(bool isEnable)
{
	if(isEnable != K2HDbgControl::isSetSignal){
		struct sigaction	sa;

		sigemptyset(&sa.sa_mask);
		sigaddset(&sa.sa_mask, SIGUSR1);
		sa.sa_flags		= isEnable ? 0 : SA_RESETHAND;
		sa.sa_handler	= isEnable ? K2HDbgControl::User1Handler : SIG_DFL;

		if(0 > sigaction(SIGUSR1, &sa, NULL)){
			WAN_K2HPRN("Could not %s signal USER1 handler. errno = %d", isEnable ? "set" : "unset", errno);
			return false;
		}
		K2HDbgControl::isSetSignal = isEnable;
	}
	return true;
}

//---------------------------------------------------------
// Global variable
//---------------------------------------------------------
K2hDbgMode	debug_mode	= K2HDBG_SILENT;
FILE*		k2h_dbg_fp	= NULL;

K2hDbgMode SetK2hDbgMode(K2hDbgMode mode)
{
	K2hDbgMode oldmode = debug_mode;
	debug_mode = mode;
	return oldmode;
}

K2hDbgMode BumpupK2hDbgMode(void)
{
	K2hDbgMode	mode = GetK2hDbgMode();

	if(K2HDBG_SILENT == mode){
		mode = K2HDBG_ERR;
	}else if(K2HDBG_ERR == mode){
		mode = K2HDBG_WARN;
	}else if(K2HDBG_WARN == mode){
		mode = K2HDBG_MSG;
	}else{	// K2HDBG_MSG == mode
		mode = K2HDBG_SILENT;
	}
	return ::SetK2hDbgMode(mode);
}

K2hDbgMode GetK2hDbgMode(void)
{
	return debug_mode;
}

bool LoadK2hDbgEnv(void)
{
	return K2HDbgControl::LoadEnv();
}

bool SetK2hDbgFile(const char* filepath)
{
	if(ISEMPTYSTR(filepath)){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!UnsetK2hDbgFile()){
		return false;
	}
	FILE*	newfp;
	if(NULL == (newfp = fopen(filepath, "a+"))){
		ERR_K2HPRN("Could not open debug file(%s). errno = %d", filepath, errno);
		return false;
	}
	k2h_dbg_fp = newfp;
	return true;
}

bool UnsetK2hDbgFile(void)
{
	if(k2h_dbg_fp){
		if(0 != fclose(k2h_dbg_fp)){
			ERR_K2HPRN("Could not close debug file. errno = %d", errno);
			k2h_dbg_fp = NULL;		// On this case, k2h_dbg_fp is not correct pointer after error...
			return false;
		}
		k2h_dbg_fp = NULL;
	}
	return true;
}

bool SetSignalUser1(void)
{
	return K2HDbgControl::SetUser1Handler(true);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
