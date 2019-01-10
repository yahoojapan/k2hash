/*
 * K2HASH
 *
 * Copyright 2018 Yahoo Japan Corporation.
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
 * CREATE:   Mon May 7 2018
 * REVISION:
 *
 */

#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include "k2hcommon.h"
#include "k2hcryptcommon.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Common Utilities
//---------------------------------------------------------
//
// Get unique ID
//
// [NOTE]
// This function generates system-wide unique ID.
// If generated ID does not unique, but we do not care for it.
// Because k2hash needs unique ID for only one key's history.
// This means that it may be a unique only to one key.
//
// We generates it by following seed's data.
//	unique ID =
//		base64(
//			sha256(
//				bytes array(
//					8 bytes		- nano seconds(by clock_gettime) at calling this.
//					8 bytes		- seconds(by clock_gettime) at calling this.
//					8 bytes		- thread id on the box.
//					8 bytes		- random value by rand() function.
//					n bytes		- hostname
//				)
//			)
//		);
//
string k2h_get_uniqid_for_history(const struct timespec& rtime)
{
	static unsigned int	seed = 0;
	static char			hostname[NI_MAXHOST];
	static size_t		hostnamelen;
	static bool			init = false;
	if(!init){
		// seed for rand()
		struct timespec	rtime = {0, 0};
		if(-1 == clock_gettime(CLOCK_REALTIME_COARSE, &rtime)){
			WAN_K2HPRN("could not get clock time by errno(%d), so unix time is instead of it.", errno);
			seed = static_cast<unsigned int>(time(NULL));			// base is sec
		}else{
			seed = static_cast<unsigned int>(rtime.tv_nsec / 1000);	// base is us
		}

		// local hostname
		if(0 != gethostname(hostname, sizeof(hostname))){
			WAN_K2HPRN("Could not get localhost name by errno(%d), so \"localhost\" is set.", errno);
			strcpy(hostname, "localhost");
		}
		hostnamelen = strlen(hostname);

		init = true;
	}

	// set datas
	uint64_t	ard64[4];								// [1]->nsec, [2]->sec, [3]->tid, [4]->rand
	ard64[0] = static_cast<uint64_t>(rtime.tv_nsec);
	ard64[1] = static_cast<uint64_t>(rtime.tv_sec);
	ard64[2] = static_cast<uint64_t>(gettid());
	ard64[3] = static_cast<uint64_t>(rand_r(&seed));	// [TODO] should use MAC address instead of rand().
	seed++;												// not need barrier

	// set to binary array
	unsigned char	bindata[NI_MAXHOST + (sizeof(uint64_t) * 4)];
	size_t			setpos;
	for(setpos = 0; setpos < (sizeof(uint64_t) * 4); ++setpos){
		bindata[setpos] = static_cast<unsigned char>(((setpos % 8) ? (ard64[setpos / 8] >> ((setpos % 8) * 8)) : ard64[setpos / 8]) & 0xff);
	}
	memcpy(&bindata[setpos], hostname, hostnamelen);
	setpos += hostnamelen;

	// SHA256
	return to_sha256_string(bindata, setpos);
}

//
// Utilities for AES256 Salt
//
// [NOTE]
// We might should be used such as std::random rather than rand().
// However, this function is performed only generation of salt, 
// salt need not be strictly random. And salt is not a problem 
// even if collision between threads. Thus, we use the rand.
// However, this function is to be considerate of another functions
// using rand, so this uses rand_r for not changing seed.
//
bool k2h_pkcs5_salt(unsigned char* salt, size_t length)
{
	static unsigned int	seed = 0;
	static bool			init = false;
	if(!init){
		struct timespec	rtime = {0, 0};
		if(-1 == clock_gettime(CLOCK_REALTIME_COARSE, &rtime)){
			WAN_K2HPRN("could not get clock time by errno(%d), so unix time is instead of it.", errno);
			seed = static_cast<unsigned int>(time(NULL));			// base is sec
		}else{
			seed = static_cast<unsigned int>(rtime.tv_nsec / 1000);	// base is us
		}
		init = true;
	}
	if(!salt || length <= 0){
		ERR_K2HPRN("parameters are wrong.");
		return false;
	}
	for(size_t cnt = 0; cnt < length; ++cnt){
		seed		= static_cast<unsigned int>(rand_r(&seed));
		salt[cnt]	= static_cast<unsigned char>(seed & 0xFF);
	}
	return true;
}

//
// Utilities for AES256 IV
//
// [NOTE]
// Initial Vector(IV) should be generated using functions of each
// crypt library. However, it is complicated with OpenSSL(RAND_bytes)/
// gcrypt(gcry_randomize)/NSS(PK11_GenerateRandom)/nettle, and random
// functions are not found with nettle alone.
// As a result, nettle uses /dev/(u)random etc and uses random/rand
// function. Therefore, we make initial seed as random as possible,
// and make IV with this function like generating salt.
//
bool k2h_generate_iv(unsigned char* iv, size_t length)
{
	static unsigned int	seed = 0;
	static bool			init = false;
	if(!init){
		unsigned char	tid		= static_cast<unsigned char>(gettid());	// use thread id
		struct timespec	rtime	= {0, 0};
		if(-1 == clock_gettime(CLOCK_REALTIME_COARSE, &rtime)){
			WAN_K2HPRN("could not get clock time by errno(%d), so unix time is instead of it.", errno);
			seed = static_cast<unsigned int>(time(NULL));				// base is sec
		}else{
			seed = static_cast<unsigned int>(rtime.tv_nsec / 1000);		// base is us
		}
		seed = ((seed << 8) | (static_cast<unsigned int>(tid) & 0xff));	// merge
		init = true;
	}
	if(!iv || length <= 0){
		ERR_K2HPRN("parameters are wrong.");
		return false;
	}
	for(size_t cnt = 0; cnt < length; ++cnt){
		seed	= static_cast<unsigned int>(rand_r(&seed));
		iv[cnt]	= static_cast<unsigned char>(seed & 0xFF);
	}
	return true;
}

//
// Utilities for Iteration count
//
bool k2h_copy_iteration_count(unsigned char* setpos, int iter, size_t count)
{
	if(!setpos || iter <= 0 || count < 4){
		ERR_K2HPRN("parameters are wrong.");
		return false;
	}
	// force 4 bytes big endian
	int32_t	tmpiter	= static_cast<int32_t>(iter);
	tmpiter			= htobe32(tmpiter);

	// 4 byte is iteration count(big endian)
	setpos[0] = static_cast<unsigned char>((tmpiter >> 24) & 0xff);
	setpos[1] = static_cast<unsigned char>((tmpiter >> 16) & 0xff);
	setpos[2] = static_cast<unsigned char>((tmpiter >> 8) & 0xff);
	setpos[3] = static_cast<unsigned char>(tmpiter & 0xff);

	// set rest bytes as dummy
	if(4 < count && !k2h_generate_iv(&setpos[4], (count - 4))){
		return false;
	}
	return true;
}

//
// Utilities for Iteration count
//
int k2h_get_iteration_count(const unsigned char* getpos)
{
	if(!getpos){
		ERR_K2HPRN("parameters are wrong.");
		return -1;
	}
	int32_t	tmpiter = 0;
	tmpiter |= (static_cast<int32_t>(getpos[0]) << 24)	& 0xff000000;
	tmpiter |= (static_cast<int32_t>(getpos[1]) << 16)	& 0xff0000;
	tmpiter |= (static_cast<int32_t>(getpos[2]) << 8)	& 0xff00;
	tmpiter |= static_cast<int32_t>(getpos[3])			& 0xff;

	// to host byte order
	tmpiter = be32toh(tmpiter);
	return static_cast<int>(tmpiter);
}

//
// Base64
//
string to_base64(const unsigned char* data, size_t length)
{
	static const char*	composechar = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

	unsigned char	block[4];
	string			result;

	for(size_t readpos = 0; readpos < length; readpos += 3){
		block[0] = (data[readpos] & 0xfc) >> 2;
		block[1] = ((data[readpos] & 0x03) << 4) | ((((readpos + 1) < length ? data[readpos + 1] : 0x00) & 0xf0) >> 4);
		block[2] = (readpos + 1) < length ? (((data[readpos + 1] & 0x0f) << 2) | ((((readpos + 2) < length ? data[readpos + 2] : 0x00) & 0xc0) >> 6)) : 0x40;
		block[3] = (readpos + 2) < length ? (data[readpos + 2] & 0x3f) : 0x40;

		result += composechar[block[0]];
		result += composechar[block[1]];
		result += composechar[block[2]];
		result += composechar[block[3]];
	}
	return result;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
