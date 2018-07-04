/*
 * K2HASH
 *
 * Copyright 2013 Yahoo Japan corporation.
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

#ifndef	K2HCOMMON_H
#define K2HCOMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//---------------------------------------------------------
// Macros for compiler
//---------------------------------------------------------
#ifdef	K2HASH
#define	K2HASH_ATTR_WEAK			__attribute__ ((weak,unused))
#else	// K2HASH
#define	K2HASH_ATTR_WEAK
#endif	// K2HASH


#ifndef	K2HASH_NOPADDING
#define	K2HASH_ATTR_PACKED			__attribute__ ((packed))
#else	// K2HASH_NOPADDING
#define	K2HASH_ATTR_PACKED
#endif	// K2HASH_NOPADDING

#if defined(__cplusplus)
#define	DECL_EXTERN_C_START			extern "C" {
#define	DECL_EXTERN_C_END			}
#else	// __cplusplus
#define	DECL_EXTERN_C_START
#define	DECL_EXTERN_C_END
#endif	// __cplusplus

//---------------------------------------------------------
// Templates
//---------------------------------------------------------
#if defined(__cplusplus)
template<typename T> inline bool ISEMPTYSTR(const T& pstr)
{
	return (NULL == (pstr) || '\0' == *(pstr)) ? true : false;
}
#else	// __cplusplus
#define	ISEMPTYSTR(pstr)	(NULL == (pstr) || '\0' == *(pstr))
#endif	// __cplusplus

#define K2HSTRJOIN(first, second)			first ## second

#endif	// K2HCOMMON_H

//---------------------------------------------------------
// types
//---------------------------------------------------------
#ifndef	HAVE_FDATASYNC
#define	fdatasync			fsync
#endif

#define	__STDC_FORMAT_MACROS
#include <inttypes.h>

//---------------------------------------------------------
// For endian
//---------------------------------------------------------
#ifndef	_BSD_SOURCE
#define _BSD_SOURCE
#define	SET_LOCAL_BSD_SOURCE	1
#endif

#ifdef	HAVE_ENDIAN_H
#include <endian.h>
#else
#ifdef	HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif
#endif

#ifdef	SET_LOCAL_BSD_SOURCE
#undef _BSD_SOURCE
#endif

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
