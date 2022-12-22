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
#ifndef	K2HSUBKEYS_H
#define	K2HSUBKEYS_H

#include <string.h>
#include <vector>
#include "k2hutil.h"

class K2HShm;
class K2HSubKeys;
class K2HSKIterator;

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
typedef struct subkey{
	unsigned char*	pSubKey;
	size_t			length;

	subkey() : pSubKey(NULL), length(0UL) {}

	// example:	memcmp(this, other)
	//			this < other ---> -1
	//			this == other --> 0
	//			this > other ---> 1
	int compare(const unsigned char* bySubkey, size_t SKLength) const
	{
		int		result;
		if(!pSubKey && !bySubkey){
			result = 0;
		}else if(!pSubKey){
			result = -1;
		}else if(!bySubkey){
			result = 1;
		}else{
			size_t	cmplength = std::min(length, SKLength);
			if(0 == (result = memcmp(pSubKey, bySubkey, cmplength))){
				if(length < SKLength){
					result = -1;
				}else if(length > SKLength){
					result = 1;
				}
			}
		}
		return result;
	}
	bool operator==(const struct subkey& other) const
	{
		return (0 == compare(other.pSubKey, other.length));
	}
	bool operator!=(const struct subkey& other) const
	{
		return (0 != compare(other.pSubKey, other.length));
	}
}SUBKEY, *PSUBKEY;

typedef std::vector<SUBKEY>	skeyarr_t;

//---------------------------------------------------------
// Class K2HSubKeys
//---------------------------------------------------------
class K2HSubKeys
{
		friend class K2HSKIterator;

	protected:
		skeyarr_t	SubKeys;

	public:
		typedef K2HSKIterator	iterator;

		K2HSubKeys();
		K2HSubKeys(const K2HSubKeys& other);
		explicit K2HSubKeys(const char* pSubkeys);
		K2HSubKeys(const unsigned char* pSubkeys, size_t length);
		virtual ~K2HSubKeys();

		bool Serialize(unsigned char** ppSubkeys, size_t& length) const;
		bool Serialize(const unsigned char* pSubkeys, size_t length);
		strarr_t::size_type StringArray(strarr_t& strarr) const;

		K2HSubKeys& operator=(const K2HSubKeys& other);

		void clear(void);
		bool empty(void) const;
		size_t size(void) const;
		K2HSubKeys::iterator insert(const char* pSubkey);
		K2HSubKeys::iterator insert(const unsigned char* bySubkey, size_t length);
		K2HSubKeys::iterator begin(void);
		K2HSubKeys::iterator end(void);
		K2HSubKeys::iterator find(const unsigned char* bySubkey, size_t length);
		K2HSubKeys::iterator erase(K2HSubKeys::iterator iter);
		bool erase(const char* pSubkey);
		bool erase(const unsigned char* bySubkey, size_t length);
};

//---------------------------------------------------------
// Class K2HSKIterator
//---------------------------------------------------------
// [NOTE]
// Branched for CentOS7 support.
//
#if __GNUC__ > 5
// cppcheck-suppress copyCtorAndEqOperator
class K2HSKIterator
#else
// cppcheck-suppress copyCtorAndEqOperator
class K2HSKIterator : public std::iterator<std::forward_iterator_tag, SUBKEY>
#endif
{
		friend class K2HSubKeys;
		friend class K2HShm;
		friend class K2HIterator;

	protected:
		const K2HSubKeys*	pK2HSubKeys;
		skeyarr_t::iterator	iter_pos;
		SUBKEY				dummy;

	public:
		#if __GNUC__ > 5
		using iterator_category	= std::random_access_iterator_tag;
		using value_type		= SUBKEY;
		using difference_type	= std::ptrdiff_t;
		using pointer			= value_type*;
		using reference			= value_type&;
		#endif

	public:
		K2HSKIterator(const K2HSKIterator& iterator);
		virtual ~K2HSKIterator();

	private:
		K2HSKIterator();
		K2HSKIterator(const K2HSubKeys* pK2HSKeys, skeyarr_t::iterator pos);

		bool Next(void);

	public:
		K2HSKIterator& operator++(void);
		K2HSKIterator operator++(int);
		SUBKEY& operator*(void);
		PSUBKEY operator->(void) const;
		bool operator==(const K2HSKIterator& iterator);
		bool operator!=(const K2HSKIterator& iterator);
};

#endif	// K2HSUBKEYS_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
