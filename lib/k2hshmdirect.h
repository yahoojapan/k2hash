/*
 * K2HASH
 *
 * Copyright 2016 Yahoo Japan Corporation.
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
 * CREATE:   Fri Jul 01 2016
 * REVISION:
 *
 */

#ifndef	K2HSHMDIRECT_H
#define	K2HSHMDIRECT_H

#include "k2hcommon.h"

//---------------------------------------------------------
// Structures
//---------------------------------------------------------
// extern "C" - start
DECL_EXTERN_C_START

//
// Data structure for one element data
//
typedef struct raw_all_element_data{
	k2h_hash_t	hash;					// hash code
	k2h_hash_t	subhash;				// subhash code
	size_t		key_length;
	size_t		val_length;
	size_t		skey_length;
	size_t		attrs_length;
	off_t		key_pos;				// offset from this structure top
	off_t		val_pos;				//
	off_t		skey_pos;				//
	off_t		attrs_pos;				//
}K2HASH_ATTR_PACKED RALLEDATA, *PRALLEDATA;

//
// Binary array union for one element data
//
typedef union binary_all_element_data{
	RALLEDATA		rawdata;
	unsigned char	byData[sizeof(RALLEDATA)];
}K2HASH_ATTR_PACKED BALLEDATA, *PBALLEDATA;

// extern "C" - end
DECL_EXTERN_C_END

//
// initialize RALLEDATA structure
//
inline void ralledata_init(RALLEDATA& rawdata)
{
	rawdata.hash		= 0UL;
	rawdata.subhash		= 0UL;
	rawdata.key_length	= 0UL;
	rawdata.val_length	= 0UL;
	rawdata.skey_length	= 0UL;
	rawdata.attrs_length= 0UL;
	rawdata.key_pos		= static_cast<off_t>(sizeof(struct raw_all_element_data));
	rawdata.val_pos		= static_cast<off_t>(sizeof(struct raw_all_element_data));
	rawdata.skey_pos	= static_cast<off_t>(sizeof(struct raw_all_element_data));
	rawdata.attrs_pos	= static_cast<off_t>(sizeof(struct raw_all_element_data));
}

//
// utilities for RALLEDATA structure
//
inline size_t calc_ralledata_datas_length(const RALLEDATA& rawdata)
{
	return (rawdata.key_length + rawdata.val_length + rawdata.skey_length + rawdata.attrs_length);
}

inline size_t calc_ralledata_length(const RALLEDATA& rawdata)
{
	return (sizeof(RALLEDATA) + calc_ralledata_datas_length(rawdata));
}

#endif	// K2HSHMDIRECT_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
