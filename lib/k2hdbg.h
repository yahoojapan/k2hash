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
#ifndef	K2HDBG_H
#define K2HDBG_H

#include "k2hcommon.h"

DECL_EXTERN_C_START

//---------------------------------------------------------
// Debug
//---------------------------------------------------------
typedef enum k2h_dbg_mode{
	K2HDBG_SILENT	= 0,
	K2HDBG_ERR		= 1,
	K2HDBG_WARN		= 3,
	K2HDBG_MSG		= 7
}K2hDbgMode;

extern K2hDbgMode	debug_mode;		// Do not use directly this variable.
extern FILE*		k2h_dbg_fp;

K2hDbgMode SetK2hDbgMode(K2hDbgMode mode);
K2hDbgMode BumpupK2hDbgMode(void);
K2hDbgMode GetK2hDbgMode(void);
bool LoadK2hDbgEnv(void);
bool SetK2hDbgFile(const char* filepath);
bool UnsetK2hDbgFile(void);
bool SetSignalUser1(void);

//---------------------------------------------------------
// Debugging Macros
//---------------------------------------------------------
#define	K2HDBGMODE_STR(mode)	K2HDBG_SILENT	== mode ? "SLT" : \
								K2HDBG_ERR		== mode ? "ERR" : \
								K2HDBG_WARN		== mode ? "WAN" : \
								K2HDBG_MSG		== mode ? "MSG" : ""

#define	LOW_K2HPRINT(mode, fmt, ...) \
		fprintf((k2h_dbg_fp ? k2h_dbg_fp : stderr), "[%s] %s(%d) : " fmt "%s\n", K2HDBGMODE_STR(mode), __func__, __LINE__, __VA_ARGS__)

#define	K2HPRINT(mode, fmt, ...) \
		if((debug_mode & mode) == mode){ \
			LOW_K2HPRINT(mode, fmt, __VA_ARGS__); \
		}

#define	SLT_K2HPRN(fmt, ...)	K2HPRINT(K2HDBG_SILENT,	fmt, ##__VA_ARGS__, "")	// This means nothing...
#define	ERR_K2HPRN(fmt, ...)	K2HPRINT(K2HDBG_ERR,	fmt, ##__VA_ARGS__, "")
#define	WAN_K2HPRN(fmt, ...)	K2HPRINT(K2HDBG_WARN,	fmt, ##__VA_ARGS__, "")
#define	MSG_K2HPRN(fmt, ...)	K2HPRINT(K2HDBG_MSG,	fmt, ##__VA_ARGS__, "")

DECL_EXTERN_C_END

#endif	// K2HDBG_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
