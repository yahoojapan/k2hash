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

#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

#include "k2hutil.h"
#include "k2hcommon.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// StringList
//---------------------------------------------------------
strarr_t::size_type ParseStringArray(const char* pData, size_t length, strarr_t& strarr)
{
	if(!pData || 0UL == length){
		WAN_K2HPRN("Parameters is wrong");
		return 0;
	}

	ssize_t	pos;
	ssize_t	nullpos;
	ssize_t	chcnt;

	strarr.clear();
	for(pos = 0, nullpos = -1, chcnt = 0; pos < static_cast<ssize_t>(length); pos++){
		if(0x00 == pData[pos]){
			if(0 < chcnt){
				strarr.push_back(string((const char*)&pData[nullpos + 1], (chcnt + 1)));
			}
			nullpos	= pos;
			chcnt	= 0;
		}else{
			chcnt++;
		}
	}
	if(0 < chcnt){
		string	strtmp((const char*)&pData[nullpos + 1], chcnt);
		strtmp += '\0';
		strarr.push_back(strtmp);
	}
	return strarr.size();
}

size_t GetTotalLengthByStringArray(strarr_t& strarr)
{
	size_t	length = 0UL;
	// cppcheck-suppress postfixOperator
 	for(strarr_t::const_iterator iter = strarr.begin(); iter != strarr.end(); iter++){
		length += (iter->length() + 1);
 	}
	return length;
}

ssize_t AppendStringArray(strarr_t& strarr, char* pData, size_t length)
{
	if(!pData || 0UL == length){
		ERR_K2HPRN("Parameters is wrong");
		return -1;
	}
	if(length < GetTotalLengthByStringArray(strarr)){
		ERR_K2HPRN("Parameter length is too short");
		return -1;
	}
	ssize_t SetCount = 0;
	// cppcheck-suppress postfixOperator
 	for(strarr_t::const_iterator iter = strarr.begin(); iter != strarr.end(); iter++){
		strncpy(&pData[SetCount], iter->c_str(), iter->length());
		SetCount += iter->length();
		pData[SetCount++] = '\0';
 	}
	return SetCount;
}

//---------------------------------------------------------
// Undefined functions
//---------------------------------------------------------
#ifndef HAVE_GETTID
pid_t gettid(void)
{
	return static_cast<pid_t>(syscall(SYS_gettid));
}
#endif

//---------------------------------------------------------
// Utilities
//---------------------------------------------------------
bool k2h_getenv(const char* pkey, string& value)
{
	if(ISEMPTYSTR(pkey)){
		MSG_K2HPRN("key is NULL.");
		return false;
	}
	char*	pEnvVal;
	if(NULL == (pEnvVal = getenv(pkey))){
		MSG_K2HPRN("%s ENV is not set.", pkey);
		return false;
	}
	value = pEnvVal;
	return true;
}

ssize_t k2h_pread(int fd, void *buf, size_t count, off_t offset)
{
	ssize_t	read_cnt;
	ssize_t	one_read;
	for(read_cnt = 0L, one_read = 0L; static_cast<size_t>(read_cnt) < count; read_cnt += one_read){
		if(-1 == (one_read = pread(fd, &(static_cast<unsigned char*>(buf))[read_cnt], (count - static_cast<size_t>(read_cnt)), (offset + read_cnt)))){
			WAN_K2HPRN("Failed to read from fd(%d:%jd:%zu), errno = %d", fd, static_cast<intmax_t>(offset + read_cnt), count - static_cast<size_t>(read_cnt), errno);
			return -1;
		}
		if(0 == one_read){
			break;
		}
	}
	return read_cnt;
}

ssize_t k2h_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
	ssize_t	write_cnt;
	ssize_t	one_write;
	for(write_cnt = 0L, one_write = 0L; static_cast<size_t>(write_cnt) < count; write_cnt += one_write){
		if(-1 == (one_write = pwrite(fd, &(static_cast<const unsigned char*>(buf))[write_cnt], (count - static_cast<size_t>(write_cnt)), (offset + write_cnt)))){
			WAN_K2HPRN("Failed to write from fd(%d:%jd:%zu), errno = %d", fd, static_cast<intmax_t>(offset + write_cnt), count - static_cast<size_t>(write_cnt), errno);
			return -1;
		}
	}
	return write_cnt;
}

int k2hbincmp(const unsigned char* bysrc, size_t srclen, const unsigned char* bydest, size_t destlen)
{
	if((!bysrc && !bydest) || (0 == srclen && 0 == destlen)){
		return 0;
	}
	if(!bysrc || 0 == srclen){
		return -1;
	}
	if(!bydest || 0 == destlen){
		return 1;
	}

	int		result;
	size_t	length = std::min(srclen, destlen);
	if(0 == (result = memcmp(bysrc, bydest, length))){
		if(srclen < destlen){
			result = -1;
		}else if(srclen > destlen){
			result = 1;
		}
	}
	return result;
}

unsigned char* k2hbindup(const unsigned char* bysrc, size_t length)
{
	if(!bysrc || 0 == length){
		return NULL;
	}

	unsigned char*	bydest;
	if(NULL == (bydest = reinterpret_cast<unsigned char*>(malloc(length)))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}
	memcpy(bydest, bysrc, length);
	return bydest;
}

unsigned char* k2hbinappend(const unsigned char* bybase, size_t blength, const unsigned char* byappend, size_t alength, size_t& length)
{
	length = 0;

	if(((NULL == bybase) != (0 == blength)) || ((NULL == byappend) != (0 == alength))){
		return NULL;
	}
	unsigned char*	bydest;
	if(!bybase){
		if(NULL != (bydest = k2hbindup(byappend, alength))){
			length = alength;
		}
	}else{
		if(NULL == (bydest = reinterpret_cast<unsigned char*>(malloc(blength + alength)))){
			ERR_K2HPRN("Could not allocation memory.");
		}else{
			length = blength + alength;
			memcpy(bydest, bybase, blength);
			memcpy(&bydest[blength], byappend, alength);
		}
	}
	return bydest;
}

unsigned char* k2hbinappendstr(const unsigned char* bybase, size_t blength, const char* pappend, size_t& length)
{
	return k2hbinappend(bybase, blength, reinterpret_cast<const unsigned char*>(pappend), (pappend ? strlen(pappend) + 1 : 0), length);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
