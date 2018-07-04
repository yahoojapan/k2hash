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
#ifndef	K2HSTRUCTURE_H
#define	K2HSTRUCTURE_H

#include "k2hash.h"
#include "k2hcommon.h"

// extern "C" - start
DECL_EXTERN_C_START

//---------------------------------------------------------
// Symbols / Macros
//---------------------------------------------------------
// For k2hash structure
#define	K2H_VERSION							2				// version string value
#define	K2H_VERSION_FORMAT					"K2H V%d"		// must be 8byte with nil
#define	MAX_K2HAREA_COUNT					2048			// muximum count for areas member in k2hash
#define	MAX_KINDEX_AREA_COUNT				32				// maximum count for key_index_area member in k2hash(this means bit count)

// For collision key index structure
#define	KINDEX_NOTASSIGNED					0L
#define	KINDEX_ASSIGNED						1L

// use long* for escaping warnning
#if defined(__cplusplus)
#define	CVT_ABS(shmbase, offset, type)		reinterpret_cast<type>(reinterpret_cast<off_t>(shmbase) + reinterpret_cast<off_t>(offset)) 	// To Absorute address
#define	CVT_REL(shmbase, address, type)		reinterpret_cast<type>(reinterpret_cast<off_t>(address) - reinterpret_cast<off_t>(shmbase))	// To Relative address
#else
#define	CVT_ABS(shmbase, offset, type)		(type)((off_t)shmbase + (off_t)offset) 		// To Absorute address
#define	CVT_REL(shmbase, address, type)		(type)((off_t)address - (off_t)shmbase)		// To Relative address
#endif

//---------------------------------------------------------
// Structures
//---------------------------------------------------------
// Page Structure
//
// This is smallest unit for key/data, this structure is a part
// of one page head.
// One page size is same as paging size. After this structure,
// a real data is set.
//
// [NOTICE]
// If accessing real data, so accessing data[1] in structure and
// after it. It means you access over this structure.
// Be careful.
//
typedef struct page_head{
	struct page_head*	prev;
	struct page_head*	next;
	size_t				length;
	unsigned char		data[1];				// Real data is after this byte.
}K2HASH_ATTR_PACKED PAGEHEAD, *PPAGEHEAD;

// For accessing by bytes array.
typedef union page_wrap{
	unsigned char		barray[sizeof(PAGEHEAD)];
	PAGEHEAD			pagehead;
}K2HASH_ATTR_PACKED PAGEWRAP, *PPAGEWRAP;

#if defined(__cplusplus)
#define	PAGEHEAD_PREV_OFFSET	0L
#define	PAGEHEAD_NEXT_OFFSET	static_cast<off_t>(sizeof(PPAGEHEAD))
#else
#define	PAGEHEAD_PREV_OFFSET	0L
#define	PAGEHEAD_NEXT_OFFSET	(off_t)(sizeof(PPAGEHEAD))
#endif

// Must PACKED for struct.
#define	PAGEHEAD_DATA_OFFSET	(sizeof(PAGEHEAD) - sizeof(unsigned char))
#define	PAGEHEAD_SIZE			(sizeof(PAGEHEAD) - sizeof(unsigned char))


//=========================================================
// Element Structure
//
// This structure has one data set which are key/data/subkeys.
// And for elements which have same hash value, this structure
// is the list of binary tree. So this member has small/big, and
// same/parent.
// This binary tree builds by subhash value. Then even if hash
// value is conflicted, this tree can be built by another hash
// value.
//
// [TODO]
// About subkeys, this value is "subkey" string lists now.
// We need to be decided which this value is "subkey" string 
// lists or "subkey" element pointer lists.
//
typedef struct element{
	struct element*	small;
	struct element*	big;
	struct element*	parent;
	struct element*	same;
	k2h_hash_t		hash;						// Main hash value
	k2h_hash_t		subhash;					// Sub(second) hash value
	PPAGEHEAD		key;
	PPAGEHEAD		value;
	PPAGEHEAD		subkeys;					// "subkey" string lists, see: TODO
	PPAGEHEAD		attrs;						// added at V2 format
	size_t			keylength;
	size_t			vallength;
	size_t			skeylength;
	size_t			attrlength;					// added at V2 format
}K2HASH_ATTR_PACKED ELEMENT, *PELEMENT;


//=========================================================
// Collision Key Index Structure
//
// This structure manages a list which the value's main hash is conflicted.
// element_list member is top of binary tree, pls see element structure
// explanation about detail.
// Important, this structure has element count which means conflicted
// elements count(children count). This value is used for checking and
// expanding area.
//
typedef struct collision_key_index{
	unsigned long	element_count;				// element count(children count)
	PELEMENT		element_list;				// element list(binary tree)
}K2HASH_ATTR_PACKED CKINDEX, *PCKINDEX;


//=========================================================
// Key Index Structure
//
// This structure has value which is same main hash value range. If there 
// are datas which are same main hash value, those datas are put into this
// structure's ckey_list.
// The range of main hash means that masked value(A) by cur_mask, value(A)
// is shifted right by collision key mask bit count. So that range of one
// of structure has range.
// ckey_list is list, this list count is set by collision key mask, this 
// means ckey_list count can not be changed. This K2Hash archtecture is 
// double array by masked hash.
// If assaign member is KINDEX_NOTASSIGNED, it means that this structure 
// is not used after increasing cur_mask yet. Because this is occurred by 
// that starting using structure is not same at increasing cur_mask.
//
// Member:
//		shifted_mask	(cur_mask << (collision mask bit count))
//		masked_mask		(this structure hash value by masking shifted_mask)
//
typedef struct key_index{
	long		assign;						// flag: used/not used this area(KINDEX_NOTASSIGN / KINDEX_ASSIGNED)
	k2h_hash_t	shifted_mask;				// Current Mask which is shifted collision mask count for this structure made.
	k2h_hash_t	masked_hash;				// masked hash value by shifted_mask for this.
	PCKINDEX	ckey_list;					// If this value is NULL, this index is not initialized.
}K2HASH_ATTR_PACKED KINDEX, *PKINDEX;


//=========================================================
// K2Hash area Structure
//
// This structure means one of area information.
// Kind of area are for K2H(head)/Key Index/Collision Key Index/
// Element(Page List)/Page.
// This structure information is used when mmapping/loading/etc.
//
// [NOTICE]
// type member is not enum, because enum size is not good for
// alignments. So this member value is symbol.
//
typedef struct k2h_area{
	long		type;
	off_t		file_offset;					// start offset in file
	size_t		length;							// size(length) by byte
}K2HASH_ATTR_PACKED K2HAREA, *PK2HAREA;

// Area type symbols
#define	K2H_AREA_UNKNOWN		0L
#define	K2H_AREA_K2H			1L
#define	K2H_AREA_KINDEX			2L
#define	K2H_AREA_CKINDEX		4L
#define	K2H_AREA_PAGELIST		8L
#define	K2H_AREA_PAGE			16L


//=========================================================
// K2Hash head Structure
//
// This structure is header information of shm(file).
// All information is very important, and k2hash library works using
// this head structure.
// 
// [NOTICE]
// About Key Index Pointer Array(key_index_area)
// 	This array posision links Key Index array for each cur_mask bit.
// 	For example, key_index_area[3] has key index array which for 0x07
// 	as cur_mask value.
// 	If cur_mask is 0x0F, key_index_area[0] to key_index_area[4] is set
// 	Key Index Array, but after key_index_area[5] is not set.
// 
// About Collision Key Index
// 	All of ckey_list in key_index is set regardless of using it, so
//	this ckey_list area assigns at expanding key index area.
//
// [TODO]
// About Compression
// 	Now do not have compress for each area and all area.
// 
typedef struct k2hash{
	char			version[K2H_VERSION_LENGTH];
	char			hash_version[K2H_HASH_FUNC_VER_LENGTH];	// Version string as Hash Function
	size_t			total_size;								// Total size for this k2hash
	size_t			page_size;								// Paging size(system)
	k2h_hash_t		max_mask;								// Muximum value for cur_mask
	k2h_hash_t		min_mask;								// Minimum value for cur_mask
	k2h_hash_t		cur_mask;								// Current mask value for hash(This value is changed automatically)
	k2h_hash_t		collision_mask;							// Mask value for collision when masked hash value by cur_mask(This value is not changed)
	unsigned long	max_element_count;						// Muximum count for elements in collision key index structure(Increasing cur_mask when this value is over)
	struct timeval	last_update;							// Last update(write data)
	struct timeval	last_area_update;						// last update(expand area)
	K2HAREA			areas[MAX_K2HAREA_COUNT];				// all of K2hash area list
	PKINDEX			key_index_area[MAX_KINDEX_AREA_COUNT];	// Key Index Pointer(List) Array(now, 0 to 31)
	long			free_element_count;						// unusing element structure count
	PELEMENT		pfree_elements;							// unusing element structure list
	long			free_page_count;						// unusing page structure count
	PPAGEHEAD		pfree_pages;							// unusing page structure list
	off_t			unassign_area;							// next expanding area offset in file(notice, this value is not alimented by paging size)
	void*			pextra;									// extra area(data) pointer(for compatibility)
}K2HASH_ATTR_PACKED K2H, *PK2H;

//---------------------------------------------------------
// Structure for Queue
//---------------------------------------------------------
typedef struct k2h_marker{
	size_t			startlen;				// start key length
	off_t			startoff;				// start key offset from this structure top
	size_t			endlen;					// end key length
	off_t			endoff;					// end key offset from this structure top
}K2HASH_ATTR_PACKED K2HMARKER, *PK2HMARKER;

// Union
typedef union binary_k2h_marker{
	unsigned char	byData[sizeof(K2HMARKER)];
	K2HMARKER		marker;
}K2HASH_ATTR_PACKED BK2HMARKER, *PBK2HMARKER;

// extern "C" - end
DECL_EXTERN_C_END

#endif	// K2HSTRUCTURE_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
