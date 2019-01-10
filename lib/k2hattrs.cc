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
 * CREATE:   Wed Nov 25 2015
 * REVISION:
 *
 */

#include <string.h>

#include "k2hcommon.h"
#include "k2hattrs.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// K2HAttrs Methods
//---------------------------------------------------------
K2HAttrs::K2HAttrs()
{
}

K2HAttrs::K2HAttrs(const K2HAttrs& other)
{
	(*this) = other;
}

K2HAttrs::K2HAttrs(const char* pattrs)
{
	Serialize(reinterpret_cast<const unsigned char*>(pattrs), pattrs ? strlen(pattrs) + 1 : 0UL);
}

K2HAttrs::K2HAttrs(const unsigned char* pattrs, size_t attrslength)
{
	Serialize(pattrs, attrslength);
}

K2HAttrs::~K2HAttrs()
{
	clear();
}

bool K2HAttrs::Serialize(unsigned char** ppattrs, size_t& attrslen) const
{
	if(!ppattrs){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(0 == Attrs.size()){
		*ppattrs = NULL;
		attrslen = 0UL;
		return true;
	}

	k2hattrarr_t::const_iterator	iter;

	// total length
	size_t	tlength = sizeof(size_t);	// for attrs count as first data.
	// cppcheck-suppress postfixOperator
	for(iter = Attrs.begin(); iter != Attrs.end(); iter++){
		tlength += sizeof(size_t) * 2;
		tlength += iter->keylength;
		tlength += iter->vallength;
	}

	// allocation
	unsigned char*	pSerialize;
	if(NULL == (pSerialize = (unsigned char*)malloc(tlength))){
		ERR_K2HPRN("Could not allocate memory.");
		return false;
	}

	// set subkey count
	size_t*	pCountPos = reinterpret_cast<size_t*>(pSerialize);
	*pCountPos = Attrs.size();

	// set datas.
	unsigned char*	pSetPos = pSerialize + sizeof(size_t);
	// cppcheck-suppress postfixOperator
	for(iter = Attrs.begin(); iter != Attrs.end(); iter++){
		// set length
		size_t*	pKeyLenPos	= reinterpret_cast<size_t*>(pSetPos);
		size_t*	pValLenPos	= reinterpret_cast<size_t*>(&pSetPos[sizeof(size_t)]);
		*pKeyLenPos			= iter->keylength;
		*pValLenPos			= iter->vallength;

		// set data
		if(0UL < iter->keylength){
			memcpy(&pSetPos[sizeof(size_t) * 2], iter->pkey, iter->keylength);
		}
		if(0UL < iter->vallength){
			memcpy(&pSetPos[sizeof(size_t) * 2 + iter->keylength], iter->pval, iter->vallength);
		}

		// set next position
		pSetPos	+= (sizeof(size_t) * 2 + iter->keylength + iter->vallength);
	}

	// set return value
	*ppattrs	= pSerialize;
	attrslen	= tlength;

	return true;
}

bool K2HAttrs::Serialize(const unsigned char* pattrs, size_t attrslength)
{
	clear();

	if(!pattrs || attrslength < sizeof(size_t)){
		return true;
	}

	// get subkey count
	size_t	TotalCount = 0UL;
	{
		const size_t*	pCountPos = reinterpret_cast<const size_t*>(pattrs);
		TotalCount = *pCountPos;
	}

	// load
	size_t					rest_length	= attrslength - sizeof(size_t);
	const unsigned char*	byReadPos	= &pattrs[sizeof(size_t)];

	for(size_t cnt = 0; cnt < TotalCount; cnt++){
		// check length
		if(rest_length < (sizeof(size_t) * 2)){
			ERR_K2HPRN("Not enough length for loading.");
			break;
		}
		// key & value length and position
		const size_t*			pKeyLengthPos	= reinterpret_cast<const size_t*>(byReadPos);
		const size_t*			pValLengthPos	= reinterpret_cast<const size_t*>(&byReadPos[sizeof(size_t)]);
		const unsigned char*	pKeyPos			= &byReadPos[sizeof(size_t) * 2];
		const unsigned char*	pValPos			= &byReadPos[sizeof(size_t) * 2 + (*pKeyLengthPos)];

		// re-check length
		if(rest_length < (sizeof(size_t) * 2 + (*pKeyLengthPos) + (*pValLengthPos))){
			ERR_K2HPRN("Not enough length for loading.");
			break;
		}

		// set into array(with allocation)
		insert(pKeyPos, *pKeyLengthPos, pValPos, *pValLengthPos);

		// set rest length and next position
		byReadPos	+= (sizeof(size_t) * 2 + (*pKeyLengthPos) + (*pValLengthPos));
		rest_length -= (sizeof(size_t) * 2 + (*pKeyLengthPos) + (*pValLengthPos));
	}
	return true;
}

strarr_t::size_type K2HAttrs::KeyStringArray(strarr_t& strarr) const
{
	strarr.clear();
	// cppcheck-suppress postfixOperator
	for(k2hattrarr_t::const_iterator iter = Attrs.begin(); iter != Attrs.end(); iter++){
		string	strtmp(reinterpret_cast<const char*>(iter->pkey), iter->keylength);
		strtmp += '\0';
		strarr.push_back(strtmp);
	}
	return strarr.size();
}

void K2HAttrs::clear(void)
{
	// cppcheck-suppress postfixOperator
	for(k2hattrarr_t::iterator iter = Attrs.begin(); iter != Attrs.end(); iter++){
		K2H_Free(iter->pkey);
		K2H_Free(iter->pval);
	}
	Attrs.clear();
}

bool K2HAttrs::empty(void) const
{
	return Attrs.empty();
}

size_t K2HAttrs::size(void) const
{
	return Attrs.size();
}

K2HAttrs& K2HAttrs::operator=(const K2HAttrs& other)
{
	clear();

	// cppcheck-suppress postfixOperator
	for(k2hattrarr_t::const_iterator iter = other.Attrs.begin(); iter != other.Attrs.end(); iter++){
		insert(iter->pkey, iter->keylength, iter->pval, iter->vallength);
	}
	return *this;
}

K2HAttrs::iterator K2HAttrs::begin(void)
{
	return K2HAttrIterator(this, Attrs.begin());
}

K2HAttrs::iterator K2HAttrs::end(void)
{
	return K2HAttrIterator(this, Attrs.end());
}

K2HAttrs::iterator K2HAttrs::find(const char* pkey)
{
	return find(reinterpret_cast<const unsigned char*>(pkey), pkey ? strlen(pkey) + 1 : 0UL);
}

K2HAttrs::iterator K2HAttrs::find(const unsigned char* pkey, size_t keylength)
{
	if(!pkey || 0UL == keylength){
		ERR_K2HPRN("Parameter is wrong.");
		return end();
	}

	// Search
	k2hattrarr_t::iterator	found_iter;
	size_t					StartPos= 0;
	size_t					EndPos	= Attrs.size();
	int						nResult;
	while(true){
		size_t				MidPos;

		MidPos		= (StartPos + EndPos) / 2;
		found_iter	= Attrs.begin();
		advance(found_iter, MidPos);

		if(found_iter != Attrs.end()){
			nResult	= found_iter->compare(pkey, keylength);
		}else{
			nResult	= -1;
		}
		if(nResult == 0){
			// found
			return K2HAttrIterator(this, found_iter);

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

bool K2HAttrs::erase(const char* pkey)
{
	return erase(reinterpret_cast<const unsigned char*>(pkey), pkey ? strlen(pkey) + 1 : 0UL);
}

bool K2HAttrs::erase(const unsigned char* pkey, size_t keylength)
{
	K2HAttrs::iterator iter = find(pkey, keylength);
	if(iter == end()){
		return false;
	}
	erase(iter);
	return true;
}

K2HAttrs::iterator K2HAttrs::erase(K2HAttrs::iterator iter)
{
	if(iter.pK2HAttrs != this){
		ERR_K2HPRN("iterator is not this object iterator.");
		return iter;
	}
	if(iter.iter_pos == Attrs.end()){
		WAN_K2HPRN("iterator is end(), so nothing is deleted.");
		return iter;
	}

	K2H_Free(iter.iter_pos->pkey);
	K2H_Free(iter.iter_pos->pval);
	iter.iter_pos = Attrs.erase(iter.iter_pos);
	return iter;
}

K2HAttrs::iterator K2HAttrs::insert(const char* pkey, const char* pval)
{
	return insert(reinterpret_cast<const unsigned char*>(pkey), pkey ? strlen(pkey) + 1 : 0UL, reinterpret_cast<const unsigned char*>(pval), pval ? strlen(pval) + 1 : 0UL);
}

K2HAttrs::iterator K2HAttrs::insert(const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength)
{
	if(!pkey || 0UL == keylength){
		ERR_K2HPRN("Parameter is wrong.");
		return end();
	}

	// copy(allocate) attr
	K2HATTR		attr;
	attr.keylength = keylength;
	attr.vallength = vallength;
	if(NULL == (attr.pkey = (unsigned char*)malloc(attr.keylength))){
		ERR_K2HPRN("Could not allocate memory.");
		// cppcheck-suppress unmatchedSuppression
		// cppcheck-suppress memleak
		return K2HAttrIterator(this, Attrs.end());
	}
	memcpy(attr.pkey, pkey, attr.keylength);

	if(0UL < attr.vallength){
		if(NULL == (attr.pval = (unsigned char*)malloc(attr.vallength))){
			ERR_K2HPRN("Could not allocate memory.");
			K2H_Free(attr.pkey);
			// cppcheck-suppress unmatchedSuppression
			// cppcheck-suppress memleak
			return K2HAttrIterator(this, Attrs.end());
		}
		memcpy(attr.pval, pval, attr.vallength);
	}

	// Search insert pos & insert
	k2hattrarr_t::iterator	insert_iter;
	size_t					StartPos= 0;
	size_t					EndPos	= Attrs.size();
	int						nResult;
	while(true){
		size_t				MidPos;

		MidPos		= (StartPos + EndPos) / 2;
		insert_iter	= Attrs.begin();
		advance(insert_iter, MidPos);

		if(insert_iter != Attrs.end()){
			nResult	= insert_iter->compare(pkey, keylength);
		}else{
			nResult	= -1;
		}
		if(nResult == 0){
			// same value(remove old value).
			unsigned char*	poldkey = insert_iter->pkey;
			unsigned char*	poldval = insert_iter->pval;
			insert_iter = Attrs.erase(insert_iter);
			K2H_Free(poldkey);
			K2H_Free(poldval);
			break;

		}else if(nResult < 0){
			// middle value < target value
			if(StartPos == MidPos){
				// insert after middle.
				if(Attrs.size() <= (MidPos + 1)){
					// push_back
					insert_iter = Attrs.end();
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

	insert_iter = Attrs.insert(insert_iter, attr);

	return K2HAttrIterator(this, insert_iter);
}

//---------------------------------------------------------
// K2HAttrIterator Methods
//---------------------------------------------------------
const K2HATTR	K2HAttrIterator::dummy;

K2HAttrIterator::K2HAttrIterator() : pK2HAttrs(NULL), iter_pos()
{
}

K2HAttrIterator::K2HAttrIterator(const K2HAttrs* pK2HSKeys, k2hattrarr_t::iterator pos) : pK2HAttrs(pK2HSKeys), iter_pos(pos)
{
}

K2HAttrIterator::K2HAttrIterator(const K2HAttrIterator& iterator) : pK2HAttrs(iterator.pK2HAttrs), iter_pos(iterator.iter_pos)
{
}

K2HAttrIterator::~K2HAttrIterator()
{
}

bool K2HAttrIterator::Next(void)
{
	if(!pK2HAttrs){
		ERR_K2HPRN("Not initializing this object.");
		return false;
	}
	if(iter_pos != pK2HAttrs->Attrs.end()){
		// cppcheck-suppress postfixOperator
		iter_pos++;
	}
	return true;
}

K2HAttrIterator& K2HAttrIterator::operator++(void)
{
	if(!Next()){
		WAN_K2HPRN("Something error occurred.");
	}
	return *this;
}

K2HAttrIterator K2HAttrIterator::operator++(int)
{
	K2HAttrIterator	result = *this;

	if(!Next()){
		WAN_K2HPRN("Something error occurred.");
	}
	return result;
}

K2HATTR& K2HAttrIterator::operator*(void)
{
	if(!pK2HAttrs){
		return *(const_cast<PK2HATTR>(&K2HAttrIterator::dummy));
	}
	if(iter_pos == pK2HAttrs->Attrs.end()){
		return *(const_cast<PK2HATTR>(&K2HAttrIterator::dummy));
	}
	return (*iter_pos);
}

PK2HATTR K2HAttrIterator::operator->(void) const
{
	if(!pK2HAttrs){
		return const_cast<PK2HATTR>(&K2HAttrIterator::dummy);
	}
	if(iter_pos == pK2HAttrs->Attrs.end()){
		return const_cast<PK2HATTR>(&K2HAttrIterator::dummy);
	}
	return &(*iter_pos);
}

bool K2HAttrIterator::operator==(const K2HAttrIterator& iterator)
{
	return (pK2HAttrs == iterator.pK2HAttrs && iter_pos == iterator.iter_pos);
}

bool K2HAttrIterator::operator!=(const K2HAttrIterator& iterator)
{
	return !(*this == iterator);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
