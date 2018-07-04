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
 * CREATE:   Mon Feb 17 2014
 * REVISION:
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include <k2hash.h>

//---------------------------------------------------------
// Prototypes
//---------------------------------------------------------
extern "C" {

k2h_hash_t k2h_hash(const void* ptr, size_t length);
k2h_hash_t k2h_second_hash(const void* ptr, size_t length);
const char* k2h_hash_version(void);

}

//---------------------------------------------------------
// Hash macros
//---------------------------------------------------------
// [NOTICE]
// This is simple macros for making hash code.
// If you can use hash function which is licensed, you can replace
// this macros for your macros.
// (for exsample, if you know, macros name is xxx_ComputeBuffer_hcodeX).
// 
#define Test_ComputeBuffer_hcode(buffer_name, buffer_name_len, hc) \
		{\
			size_t i = 0;\
			hc = 0;\
			for(i = 0; i < buffer_name_len && i < 8; i++){\
				hc = (hc << 8) | (buffer_name)[i];\
			}\
		}

#define Test_ComputeBuffer_hcode2( buffer_name, buffer_name_len, hc ) \
		{\
			size_t i = 0;\
			int    n = 0;\
			hc = 0;\
			for(i = buffer_name_len, n = 0; 0 < i && n < 8; i--, n++){\
				hc = (hc << 8) | (buffer_name)[i - 1];\
			}\
		}

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
static const char	szVersion[] = "DSO HASH V1.0";

k2h_hash_t k2h_hash(const void* ptr, size_t length)
{
	//printf("dso hash function 1 call, %s\n", szVersion);

	if(!ptr || 1L > length){
		return 0UL;
	}
	k2h_hash_t	result = 0L;
	Test_ComputeBuffer_hcode(reinterpret_cast<const unsigned char*>(ptr), length, result);
	return result;
}

k2h_hash_t k2h_second_hash(const void* ptr, size_t length)
{
	//printf("dso hash function 2 call, %s\n", szVersion);

	if(!ptr || 1L > length){
		return 0UL;
	}
	k2h_hash_t	result = 0L;
	Test_ComputeBuffer_hcode2(reinterpret_cast<const unsigned char*>(ptr), length, result);
	return result;
}

const char* k2h_hash_version(void)
{
	return szVersion;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
