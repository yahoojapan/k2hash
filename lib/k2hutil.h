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
#ifndef	K2HUTIL_H
#define	K2HUTIL_H

#include <string>
#include <vector>

#include "k2hcommon.h"

//---------------------------------------------------------
// StringList
//---------------------------------------------------------
typedef std::vector<std::string>	strarr_t;

strarr_t::size_type ParseStringArray(const char* pData, size_t length, strarr_t& strarr);
size_t GetTotalLengthByStringArray(strarr_t& strarr);
ssize_t AppendStringArray(strarr_t& strarr, char* pData, size_t length);

//---------------------------------------------------------
// Utilities
//---------------------------------------------------------
#define	K2H_Free(ptr)	\
		{ \
			if(ptr){ \
				free(ptr); \
				ptr = NULL; \
			} \
		}

#define	K2H_Delete(ptr)	\
		{ \
			if(ptr){ \
				delete ptr; \
				ptr = NULL; \
			} \
		}

#define	K2H_CLOSE(fd)	\
		{ \
			if(-1 != fd){ \
				close(fd); \
				fd = -1; \
			} \
		}

#ifndef HAVE_GETTID
pid_t gettid(void);
#endif

bool k2h_getenv(const char* pkey, std::string& value);

DECL_EXTERN_C_START

ssize_t k2h_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t k2h_pwrite(int fd, const void *buf, size_t count, off_t offset);
int k2hbincmp(const unsigned char* bysrc, size_t srclen, const unsigned char* bydest, size_t destlen);
unsigned char* k2hbindup(const unsigned char* bysrc, size_t length);
unsigned char* k2hbinappend(const unsigned char* bybase, size_t blength, const unsigned char* byappend, size_t alength, size_t& length);
unsigned char* k2hbinappendstr(const unsigned char* bybase, size_t blength, const char* pappend, size_t& length);

DECL_EXTERN_C_END

#endif	// K2HUTIL_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
