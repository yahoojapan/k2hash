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
#ifndef	K2HASHFUNC_H
#define	K2HASHFUNC_H

#include "k2hash.h"
#include "k2hcommon.h"

//---------------------------------------------------------
// Readme about HASH functions
//---------------------------------------------------------
// K2Hash library has three type functions for HASHING.
// 1) k2h_hash         - convert binary array to hash value(unsigned long)
// 3) k2h_second_hash  - secondary hash function like k2h_hash
// 3) k2h_hash_version - returns hash function version string
// 
// You can overwrite these three functions by follow two way.
// 1) Built'in these function in your source codes(binary)
//    You must make completely same prototype functions in your
//    codes. Thereby, your functions is given priority to over-
//    write these functions in the library, In other words you
//    can overwrite these.
// 
// 2) Load your library
//    You can load your library(so) which has these functions by
//    calling Load() member function in K2HashDynLib class.
// 
// If you use these functions for example: making hash code,
// checking function version. You can call below macros which is
// defined in this header file, so you call rightly function
// regardless of overwriting functions.
// 
//    K2H_HASH_FUNC(unsigned char* ptr, size_t length)
//    K2H_2ND_HASH_FUNC(unsigned char* ptr, size_t length)
//    K2H_HASH_VER_FUNC(void)
// 

//---------------------------------------------------------
// Global Hash function
//---------------------------------------------------------
// 
// weak function for over loading same name function.
//
DECL_EXTERN_C_START		// extern "C" - start

// First hash value returns, the value is used Key Index
extern k2h_hash_t k2h_hash(const void* ptr, size_t length) K2HASH_ATTR_WEAK;

// Second hash value returns, the value is used the Element in collision keys.
extern k2h_hash_t k2h_second_hash(const void* ptr, size_t length) K2HASH_ATTR_WEAK;

// Hash function(library) version string, the value is stamped into SHM file.
// This retuned value length must be under 32 byte.
extern const char* k2h_hash_version(void) K2HASH_ATTR_WEAK;

DECL_EXTERN_C_END		// extern "C" - end

//---------------------------------------------------------
// Prototype Hash function
//---------------------------------------------------------
DECL_EXTERN_C_START		// extern "C" - start

typedef k2h_hash_t (*Tfp_k2h_hash)(const void* ptr, size_t length);
typedef k2h_hash_t (*Tfp_k2h_second_hash)(const void* ptr, size_t length);
typedef const char* (*Tfp_k2h_hash_version)(void);

DECL_EXTERN_C_END		// extern "C" - end

//---------------------------------------------------------
// Macros
//---------------------------------------------------------
#define	CALL_K2H_HASH_FUNCTION(type, ...)	(reinterpret_cast<K2HSTRJOIN(Tfp_, type)>(NULL != K2HashDynLib::get()->K2HSTRJOIN(get_, type)() ? K2HashDynLib::get()->K2HSTRJOIN(get_, type)() : type))(__VA_ARGS__)
#define	K2H_HASH_FUNC(...)					CALL_K2H_HASH_FUNCTION(k2h_hash, __VA_ARGS__)
#define	K2H_2ND_HASH_FUNC(...)				CALL_K2H_HASH_FUNCTION(k2h_second_hash, __VA_ARGS__)
#define	K2H_HASH_VER_FUNC()					CALL_K2H_HASH_FUNCTION(k2h_hash_version)

//---------------------------------------------------------
// Load library
//---------------------------------------------------------
class K2HashDynLib
{
	private:
		void*					hDynLib;
		Tfp_k2h_hash			fp_k2h_hash;
		Tfp_k2h_second_hash		fp_k2h_second_hash;
		Tfp_k2h_hash_version	fp_k2h_hash_version;

	public:
		static K2HashDynLib* get(void);

		K2HashDynLib();
		virtual ~K2HashDynLib();

		bool Unload(void);
		bool Load(const char* path);

		Tfp_k2h_hash get_k2h_hash(void) { return fp_k2h_hash; }
		Tfp_k2h_second_hash get_k2h_second_hash(void) { return fp_k2h_second_hash; }
		Tfp_k2h_hash_version get_k2h_hash_version(void) { return fp_k2h_hash_version; }
};

#endif	// K2HASHFUNC_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
