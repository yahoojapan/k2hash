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
#ifndef	K2HATTRS_H
#define	K2HATTRS_H

#include <string.h>
#include <vector>
#include "k2hutil.h"

class K2HShm;
class K2HAttrs;
class K2HAttrIterator;

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
typedef struct k2h_attr{
	size_t			keylength;
	size_t			vallength;
	unsigned char*	pkey;
	unsigned char*	pval;

	k2h_attr() : keylength(0), vallength(0), pkey(NULL), pval(NULL) {}

	// example:	memcmp(this, other)
	//			this < other ---> -1
	//			this == other --> 0
	//			this > other ---> 1
	int compare(const unsigned char* otherkey, size_t otherkeylen) const
	{
		int		result;
		if(!pkey && !otherkey){
			result = 0;
		}else if(!pkey){
			result = -1;
		}else if(!otherkey){
			result = 1;
		}else{
			size_t	cmplength = std::min(keylength, otherkeylen);
			if(0 == (result = memcmp(pkey, otherkey, cmplength))){
				if(keylength < otherkeylen){
					result = -1;
				}else if(keylength > otherkeylen){
					result = 1;
				}
			}
		}
		return result;
	}
	bool operator==(const struct k2h_attr& other) const
	{
		return (0 == compare(other.pkey, other.keylength));
	}
	bool operator!=(const struct k2h_attr& other) const
	{
		return (0 != compare(other.pkey, other.keylength));
	}
}K2HATTR, *PK2HATTR;

typedef std::vector<K2HATTR>	k2hattrarr_t;

//---------------------------------------------------------
// Class K2HAttrs
//---------------------------------------------------------
// [NOTE]
// Serialize formatted on unsigned char array.
//
//	size_t			<total serialize bytes>
//	unsigned char[] <serialize data>
//	{
//		size_t		<total attr count>
//		one attr{
//			size_t			<key length>
//			size_t			<val length>
//			unsigned char[]	<key>
//			unsigned char[]	<value>
//		}
//		*
//		*
//		*
//	}
//
class K2HAttrs
{
		friend class K2HAttrIterator;

	protected:
		k2hattrarr_t	Attrs;

	public:
		typedef K2HAttrIterator	iterator;

		K2HAttrs();
		K2HAttrs(const K2HAttrs& other);
		K2HAttrs(const char* pattrs);
		K2HAttrs(const unsigned char* pattrs, size_t attrslength);
		virtual ~K2HAttrs();

		bool Serialize(unsigned char** ppattrs, size_t& attrslen) const;
		bool Serialize(const unsigned char* pattrs, size_t attrslength);
		strarr_t::size_type KeyStringArray(strarr_t& strarr) const;

		bool operator=(const K2HAttrs& other);

		void clear(void);
		bool empty(void) const;
		size_t size(void) const;
		K2HAttrs::iterator insert(const char* pkey, const char* pval);
		K2HAttrs::iterator insert(const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength);
		K2HAttrs::iterator begin(void);
		K2HAttrs::iterator end(void);
		K2HAttrs::iterator find(const char* pkey);
		K2HAttrs::iterator find(const unsigned char* pkey, size_t keylength);
		K2HAttrs::iterator erase(K2HAttrs::iterator iter);
		bool erase(const char* pkey);
		bool erase(const unsigned char* pkey, size_t keylength);
};

//---------------------------------------------------------
// Class K2HAttrIterator
//---------------------------------------------------------
class K2HAttrIterator : public std::iterator<std::forward_iterator_tag, K2HATTR>
{
		friend class K2HAttrs;
		friend class K2HShm;
		friend class K2HIterator;

	protected:
		static const K2HATTR	dummy;

		const K2HAttrs*			pK2HAttrs;
		k2hattrarr_t::iterator	iter_pos;

	public:
		K2HAttrIterator(const K2HAttrIterator& iterator);
		virtual ~K2HAttrIterator();

	private:
		K2HAttrIterator();
		K2HAttrIterator(const K2HAttrs* pK2HAttrs, k2hattrarr_t::iterator pos);

		bool Next(void);

	public:
		K2HAttrIterator& operator++(void);
		K2HAttrIterator operator++(int);
		K2HATTR& operator*(void);
		PK2HATTR operator->(void) const;
		bool operator==(const K2HAttrIterator& iterator);
		bool operator!=(const K2HAttrIterator& iterator);
};

#endif	// K2HATTRS_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
