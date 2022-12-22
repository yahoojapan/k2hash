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
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>

#include "k2hdbg.h"

//---------------------------------------------------------
// Global variable
//---------------------------------------------------------
K2hDbgMode	debug_mode	= K2HDBG_SILENT;
FILE*		k2h_dbg_fp	= NULL;

//---------------------------------------------------------
// Class K2HDbgControl
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
class K2HDbgControl
{
	protected:
		static const char*	DBGENVNAME;
		static const char*	DBGENVFILE;
		static bool			isSetSignal;

		K2hDbgMode*			pdebug_mode;			// pointer to global variable
		FILE**				pk2h_dbg_fp;			// pointer to global variable

	protected:
		static void User1Handler(int Signal);

		K2HDbgControl() : pdebug_mode(&debug_mode), pk2h_dbg_fp(&k2h_dbg_fp)
		{
			*pdebug_mode = K2HDBG_SILENT;
			*pk2h_dbg_fp = NULL;
			LoadEnv();
		}

		virtual ~K2HDbgControl()
		{
			SetUser1Handler(false);
		}

		bool LoadEnvName(void);
		bool LoadEnvFile(void);

	public:
		static K2HDbgControl& GetDbgCntl(void)
		{
			static K2HDbgControl	singleton;		// singleton
			return singleton;
		}

		bool SetUser1Handler(bool isEnable);
		bool LoadEnv(void);
		K2hDbgMode SetDbgMode(K2hDbgMode mode);
		K2hDbgMode BumpupDbgMode(void);
		K2hDbgMode GetDbgMode(void);
		bool SetDbgFile(const char* filepath);
		bool UnsetDbgFile(void);
};

//
// Class variables
//
const char*	K2HDbgControl::DBGENVNAME = "K2HDBGMODE";
const char*	K2HDbgControl::DBGENVFILE = "K2HDBGFILE";
bool		K2HDbgControl::isSetSignal=	false;

//
// Methods
//
void K2HDbgControl::User1Handler(int Signal)
{
	MSG_K2HPRN("Caught signal SIGUSR1(%d), bumpup the logging level.", Signal);
	K2HDbgControl::GetDbgCntl().BumpupDbgMode();
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

bool K2HDbgControl::LoadEnv(void)
{
	if(!LoadEnvName() || !LoadEnvFile()){
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
		SetDbgMode(K2HDBG_SILENT);
	}else if(0 == strcasecmp(pEnvVal, "ERR")){
		SetDbgMode(K2HDBG_ERR);
	}else if(0 == strcasecmp(pEnvVal, "WAN")){
		SetDbgMode(K2HDBG_WARN);
	}else if(0 == strcasecmp(pEnvVal, "INFO")){
		SetDbgMode(K2HDBG_MSG);
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
	if(!SetDbgFile(pEnvVal)){
		MSG_K2HPRN("%s ENV is unsafe string(%s).", K2HDbgControl::DBGENVFILE, pEnvVal);
		return false;
	}
	return true;
}

K2hDbgMode K2HDbgControl::SetDbgMode(K2hDbgMode mode)
{
	K2hDbgMode oldmode = *pdebug_mode;
	*pdebug_mode = mode;
	return oldmode;
}

K2hDbgMode K2HDbgControl::BumpupDbgMode(void)
{
	K2hDbgMode	mode = GetDbgMode();

	if(K2HDBG_SILENT == mode){
		mode = K2HDBG_ERR;
	}else if(K2HDBG_ERR == mode){
		mode = K2HDBG_WARN;
	}else if(K2HDBG_WARN == mode){
		mode = K2HDBG_MSG;
	}else{	// K2HDBG_MSG == mode
		mode = K2HDBG_SILENT;
	}
	return SetDbgMode(mode);
}

K2hDbgMode K2HDbgControl::GetDbgMode(void)
{
	return *pdebug_mode;
}

bool K2HDbgControl::SetDbgFile(const char* filepath)
{
	if(ISEMPTYSTR(filepath)){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!UnsetDbgFile()){
		return false;
	}
	FILE*	newfp;
	if(NULL == (newfp = fopen(filepath, "a+"))){
		ERR_K2HPRN("Could not open debug file(%s). errno = %d", filepath, errno);
		// cppcheck-suppress resourceLeak
		return false;
	}
	*pk2h_dbg_fp = newfp;
	return true;
}

bool K2HDbgControl::UnsetDbgFile(void)
{
	if(*pk2h_dbg_fp){
		if(0 != fclose(*pk2h_dbg_fp)){
			ERR_K2HPRN("Could not close debug file. errno = %d", errno);
			*pk2h_dbg_fp = NULL;		// On this case, k2h_dbg_fp is not correct pointer after error...
			return false;
		}
		*pk2h_dbg_fp = NULL;
	}
	return true;
}

//---------------------------------------------------------
// Global Functions
//---------------------------------------------------------
K2hDbgMode SetK2hDbgMode(K2hDbgMode mode)
{
	return K2HDbgControl::GetDbgCntl().SetDbgMode(mode);
}

K2hDbgMode BumpupK2hDbgMode(void)
{
	return K2HDbgControl::GetDbgCntl().BumpupDbgMode();
}

K2hDbgMode GetK2hDbgMode(void)
{
	return K2HDbgControl::GetDbgCntl().GetDbgMode();
}

bool LoadK2hDbgEnv(void)
{
	return K2HDbgControl::GetDbgCntl().LoadEnv();
}

bool SetK2hDbgFile(const char* filepath)
{
	return K2HDbgControl::GetDbgCntl().SetDbgFile(filepath);
}

bool UnsetK2hDbgFile(void)
{
	return K2HDbgControl::GetDbgCntl().UnsetDbgFile();
}

bool SetSignalUser1(void)
{
	return K2HDbgControl::GetDbgCntl().SetUser1Handler(true);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
