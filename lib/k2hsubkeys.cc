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

#include <string.h>

#include "k2hcommon.h"
#include "k2hsubkeys.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// K2HSubKeys Methods
//---------------------------------------------------------
K2HSubKeys::K2HSubKeys()
{
}

K2HSubKeys::K2HSubKeys(const K2HSubKeys& other)
{
	(*this) = other;
}

K2HSubKeys::K2HSubKeys(const char* pSubkeys)
{
	Serialize(reinterpret_cast<const unsigned char*>(pSubkeys), pSubkeys ? strlen(pSubkeys) + 1 : 0UL);
}

K2HSubKeys::K2HSubKeys(const unsigned char* pSubkeys, size_t length)
{
	Serialize(pSubkeys, length);
}

K2HSubKeys::~K2HSubKeys()
{
	clear();
}

bool K2HSubKeys::Serialize(unsigned char** ppSubkeys, size_t& length) const
{
	if(!ppSubkeys){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(0 == SubKeys.size()){
		*ppSubkeys	= NULL;
		length		= 0UL;
		return true;
	}

	skeyarr_t::const_iterator	iter;

	// total length
	size_t	tlength = sizeof(size_t);	// for subkey count as first data.
	for(iter = SubKeys.begin(); iter != SubKeys.end(); iter++){
		tlength += sizeof(size_t);
		tlength += iter->length;
	}

	// allocation
	unsigned char*	bySerialize;
	if(NULL == (bySerialize = (unsigned char*)malloc(tlength))){
		ERR_K2HPRN("Could not allocate memory.");
		return false;
	}

	// set subkey count
	size_t*	pCountPos = reinterpret_cast<size_t*>(bySerialize);
	*pCountPos = SubKeys.size();

	// set datas.
	size_t*			pLengthPos;
	unsigned char*	bySetPos = bySerialize + sizeof(size_t);
	for(skeyarr_t::const_iterator iter = SubKeys.begin(); iter != SubKeys.end(); iter++){
		pLengthPos	= reinterpret_cast<size_t*>(bySetPos);
		*pLengthPos	= iter->length;

		bySetPos	= bySetPos + sizeof(size_t);
		memcpy(bySetPos, iter->pSubKey, iter->length);

		bySetPos	+= iter->length;
	}

	// set return value
	*ppSubkeys	= bySerialize;
	length		= tlength;

	return true;
}

bool K2HSubKeys::Serialize(const unsigned char* pSubkeys, size_t length)
{
	clear();

	if(!pSubkeys || 0UL == length){
		return true;
	}

	// get subkey count
	size_t	TotalCount = 0UL;
	{
		const size_t*	pCountPos = reinterpret_cast<const size_t*>(pSubkeys);
		TotalCount = *pCountPos;
	}

	// load
	const size_t*			pLengthPos;
	size_t					rest_length	= length;
	const unsigned char*	byReadPos	= reinterpret_cast<const unsigned char*>(reinterpret_cast<size_t>(pSubkeys) + sizeof(size_t));

	for(size_t cnt = 0; cnt < TotalCount; cnt++){
		// each length
		if(rest_length < sizeof(size_t)){
			ERR_K2HPRN("Not enough length for loading.");
			return false;
		}
		pLengthPos		= reinterpret_cast<const size_t*>(byReadPos);
		byReadPos		+= sizeof(size_t);
		rest_length		-= sizeof(size_t);

		if(rest_length < *pLengthPos){
			ERR_K2HPRN("Not enough length for loading.");
			return false;
		}

		// set into array
		insert(byReadPos, *pLengthPos);

		byReadPos		+= *pLengthPos;
		rest_length		-= *pLengthPos;
	}
	return true;
}

strarr_t::size_type K2HSubKeys::StringArray(strarr_t& strarr) const
{
	strarr.clear();
	for(skeyarr_t::const_iterator iter = SubKeys.begin(); iter != SubKeys.end(); iter++){
		string	strtmp(reinterpret_cast<const char*>(iter->pSubKey), iter->length);
		strtmp += '\0';
		strarr.push_back(strtmp);
	}
	return strarr.size();
}

void K2HSubKeys::clear(void)
{
	for(skeyarr_t::iterator iter = SubKeys.begin(); iter != SubKeys.end(); iter++){
		K2H_Free(iter->pSubKey);
	}
	SubKeys.clear();
}

bool K2HSubKeys::empty(void) const
{
	return SubKeys.empty();
}

size_t K2HSubKeys::size(void) const
{
	return SubKeys.size();
}

bool K2HSubKeys::operator=(const K2HSubKeys& other)
{
	clear();

	for(skeyarr_t::const_iterator iter = other.SubKeys.begin(); iter != other.SubKeys.end(); iter++){
		insert(iter->pSubKey, iter->length);
	}
	return true;
}

K2HSubKeys::iterator K2HSubKeys::begin(void)
{
	return K2HSKIterator(this, SubKeys.begin());
}

K2HSubKeys::iterator K2HSubKeys::end(void)
{
	return K2HSKIterator(this, SubKeys.end());
}

//
// Binary Search
//
K2HSubKeys::iterator K2HSubKeys::find(const unsigned char* bySubkey, size_t length)
{
	if(!bySubkey || 0UL == length){
		ERR_K2HPRN("Parameter is wrong.");
		return end();
	}

	// Search
	skeyarr_t::iterator	found_iter;
	size_t	StartPos= 0;
	size_t	EndPos	= SubKeys.size();
	size_t	MidPos;
	int		nResult;
	while(true){
		MidPos		= (StartPos + EndPos) / 2;
		found_iter	= SubKeys.begin();
		advance(found_iter, MidPos);

		if(found_iter != SubKeys.end()){
			nResult	= found_iter->compare(bySubkey, length);
		}else{
			nResult	= -1;
		}
		if(nResult == 0){
			// found
			return K2HSKIterator(this, found_iter);

		}else if(nResult < 0){
			// middle value < target value
			if(StartPos == MidPos){
				// not found
				break;
			}
			StartPos = MidPos;

		}else{	// 0 < nResult
			// middle value > target value
			if(EndPos == MidPos){
				// not found
				break;
			}
			EndPos = MidPos;
		}
	}
	return end();
}

bool K2HSubKeys::erase(const char* pSubkey)
{
	return erase(reinterpret_cast<const unsigned char*>(pSubkey), pSubkey ? strlen(pSubkey) + 1 : 0UL);
}

bool K2HSubKeys::erase(const unsigned char* bySubkey, size_t length)
{
	K2HSubKeys::iterator iter = find(bySubkey, length);
	if(iter == end()){
		return false;
	}
	erase(iter);
	return true;
}

K2HSubKeys::iterator K2HSubKeys::erase(K2HSubKeys::iterator iter)
{
	if(iter.pK2HSubKeys != this){
		ERR_K2HPRN("iterator is not this object iterator.");
		return iter;
	}
	if(iter.iter_pos == SubKeys.end()){
		WAN_K2HPRN("iterator is end(), so nothing is deleted.");
		return iter;
	}

	K2H_Free(iter.iter_pos->pSubKey);
	iter.iter_pos = SubKeys.erase(iter.iter_pos);
	return iter;
}

K2HSubKeys::iterator K2HSubKeys::insert(const char* pSubkey)
{
	return insert(reinterpret_cast<const unsigned char*>(pSubkey), pSubkey ? strlen(pSubkey) + 1 : 0UL);
}

//
// Insert for Binary Search
//
K2HSubKeys::iterator K2HSubKeys::insert(const unsigned char* bySubkey, size_t length)
{
	if(!bySubkey || 0UL == length){
		ERR_K2HPRN("Parameter is wrong.");
		return end();
	}

	// make subkey
	SUBKEY	subkey;
	subkey.length = length;
	if(NULL == (subkey.pSubKey = (unsigned char*)malloc(subkey.length))){
		ERR_K2HPRN("Could not allocate memory.");
		return K2HSKIterator(this, SubKeys.end());
	}
	memcpy(subkey.pSubKey, bySubkey, subkey.length);

	// Search insert pos & insert
	skeyarr_t::iterator	insert_iter;
	size_t	StartPos= 0;
	size_t	EndPos	= SubKeys.size();
	size_t	MidPos;
	int		nResult;
	while(true){
		MidPos		= (StartPos + EndPos) / 2;
		insert_iter	= SubKeys.begin();
		advance(insert_iter, MidPos);

		if(insert_iter != SubKeys.end()){
			nResult	= insert_iter->compare(bySubkey, length);
		}else{
			nResult	= -1;
		}
		if(nResult == 0){
			// same value(remove old value).
			unsigned char*	pSubKey = insert_iter->pSubKey;
			insert_iter = SubKeys.erase(insert_iter);
			K2H_Free(pSubKey);
			break;

		}else if(nResult < 0){
			// middle value < target value
			if(StartPos == MidPos){
				// insert after middle.
				if(SubKeys.size() <= (MidPos + 1)){
					// push_back
					insert_iter = SubKeys.end();
				}else{
					// insert before middle
					advance(insert_iter, 1);
				}
				break;
			}
			StartPos = MidPos;

		}else{	// 0 < nResult
			// middle value > target value
			if(EndPos == MidPos){
				// insert before middle.
				break;
			}
			EndPos = MidPos;
		}
	}

	insert_iter = SubKeys.insert(insert_iter, subkey);

	return K2HSKIterator(this, insert_iter);
}

//---------------------------------------------------------
// K2HSKIterator Methods
//---------------------------------------------------------
K2HSKIterator::K2HSKIterator() : pK2HSubKeys(NULL), iter_pos(), dummy()
{
}

K2HSKIterator::K2HSKIterator(const K2HSubKeys* pK2HSKeys, skeyarr_t::iterator pos) : pK2HSubKeys(pK2HSKeys), iter_pos(pos), dummy()
{
}

K2HSKIterator::K2HSKIterator(const K2HSKIterator& iterator)
{
	pK2HSubKeys		= iterator.pK2HSubKeys;
	iter_pos		= iterator.iter_pos;
}

K2HSKIterator::~K2HSKIterator()
{
}

bool K2HSKIterator::Next(void)
{
	if(!pK2HSubKeys){
		ERR_K2HPRN("Not initializing this object.");
		return false;
	}
	if(iter_pos != pK2HSubKeys->SubKeys.end()){
		iter_pos++;
	}
	return true;
}

K2HSKIterator& K2HSKIterator::operator++(void)
{
	if(!Next()){
		WAN_K2HPRN("Something error occurred.");
	}
	return *this;
}

K2HSKIterator K2HSKIterator::operator++(int)
{
	K2HSKIterator	result = *this;

	if(!Next()){
		WAN_K2HPRN("Something error occurred.");
	}
	return result;
}

SUBKEY& K2HSKIterator::operator*(void)
{
	if(!pK2HSubKeys){
		return dummy;
	}
	if(iter_pos == pK2HSubKeys->SubKeys.end()){
		return dummy;
	}
	return (*iter_pos);
}

PSUBKEY K2HSKIterator::operator->(void) const
{
	if(!pK2HSubKeys){
		return const_cast<PSUBKEY>(&dummy);
	}
	if(iter_pos == pK2HSubKeys->SubKeys.end()){
		return const_cast<PSUBKEY>(&dummy);
	}
	return &(*iter_pos);
}

bool K2HSKIterator::operator==(const K2HSKIterator& iterator)
{
	return (pK2HSubKeys == iterator.pK2HSubKeys && iter_pos == iterator.iter_pos);
}

bool K2HSKIterator::operator!=(const K2HSKIterator& iterator)
{
	return !(*this == iterator);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
