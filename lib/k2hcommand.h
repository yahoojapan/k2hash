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
 * CREATE:   Mon Feb 10 2014
 * REVISION:
 *
 */
#ifndef	K2HCOMMAND_H
#define	K2HCOMMAND_H

#include "k2hcommon.h"

//---------------------------------------------------------
// Symbols / Macros
//---------------------------------------------------------
// extern "C" - start
DECL_EXTERN_C_START

// Command buffer length
#define	SCOM_COMSTR_LENGTH			16

// Command string prefix
#define	SCOM_COMSTR_PREFIX			"\nK2H_"

// Command string (without prefix)
#define	SCOM_COMSTR_SET_ALL			SCOM_COMSTR_PREFIX "SET_ALL"
#define	SCOM_COMSTR_REPLACE_VAL		SCOM_COMSTR_PREFIX "REP_VAL"
#define	SCOM_COMSTR_REPLACE_SKEY	SCOM_COMSTR_PREFIX "REP_SKEY"
#define	SCOM_COMSTR_DEL_KEY			SCOM_COMSTR_PREFIX "DEL_KEY"
#define	SCOM_COMSTR_OW_VAL			SCOM_COMSTR_PREFIX "OW_VAL"
#define	SCOM_COMSTR_REPLACE_ATTRS	SCOM_COMSTR_PREFIX "REP_ATTRS"
#define	SCOM_COMSTR_RENAME			SCOM_COMSTR_PREFIX "REN_KEY"

// Command types(not enum)
#define	SCOM_UNKNOWN				-1L
#define	SCOM_TYPE_MIN				0L
#define	SCOM_SET_ALL				0L			// Both value and subkeys is over write(value and subkeys can be NULL)
#define	SCOM_REPLACE_VAL			1L			// Only value is over write(value can be NULL)
#define	SCOM_REPLACE_SKEY			2L			// Only subkeys is over write(subkeys can be NULL)
#define	SCOM_DEL_KEY				3L			// Delete key
#define	SCOM_OW_VAL					4L			// Directly over write value(use exdata(=off_t))
#define	SCOM_REPLACE_ATTRS			5L			// Only attribute is over write(attrs can be NULL)
#define	SCOM_RENAME					6L			// Rename key
#define	SCOM_TYPE_MAX				6L

// extern "C" - end
DECL_EXTERN_C_END

//---------------------------------------------------------
// Structures
//---------------------------------------------------------
// extern "C" - start
DECL_EXTERN_C_START

// Command structure
typedef struct serialize_command{
	char	szCommand[SCOM_COMSTR_LENGTH];
	long	type;
	size_t	key_length;
	size_t	val_length;
	size_t	skey_length;
	size_t	attr_length;
	size_t	exdata_length;
	off_t	key_pos;
	off_t	val_pos;
	off_t	skey_pos;
	off_t	attrs_pos;
	off_t	exdata_pos;
}K2HASH_ATTR_PACKED SCOM, *PSCOM;

// Union
typedef union binary_command{
	SCOM			scom;
	unsigned char	byData[sizeof(SCOM)];
}K2HASH_ATTR_PACKED BCOM, *PBCOM;

typedef union ow_value_exdata{
	off_t			valoffset;
	unsigned char	byData[sizeof(off_t)];
}K2HASH_ATTR_PACKED OWVAL_EXDATA, *POWVAL_EXDATA;

// extern "C" - end
DECL_EXTERN_C_END

// initialize function
inline void scom_init(SCOM& scom)
{
	scom.szCommand[0]	= '\0';
	scom.type			= SCOM_UNKNOWN;
	scom.key_length		= 0UL;
	scom.val_length		= 0UL;
	scom.skey_length	= 0UL;
	scom.attr_length	= 0UL;
	scom.exdata_length	= 0UL;
	scom.key_pos		= static_cast<off_t>(sizeof(struct serialize_command));
	scom.val_pos		= static_cast<off_t>(sizeof(struct serialize_command));
	scom.skey_pos		= static_cast<off_t>(sizeof(struct serialize_command));
	scom.attrs_pos		= static_cast<off_t>(sizeof(struct serialize_command));
	scom.exdata_pos		= static_cast<off_t>(sizeof(struct serialize_command));
}

// utilities
inline size_t scom_total_length(const SCOM& scom)
{
	return (sizeof(SCOM) + scom.key_length + scom.val_length + scom.skey_length + scom.attr_length + scom.exdata_length);
}

//---------------------------------------------------------
// K2HCommandArchive Class
//---------------------------------------------------------
class K2HCommandArchive
{
	protected:
		PBCOM	pBinCom;

	public:
		K2HCommandArchive();
		virtual ~K2HCommandArchive();

		bool SetAll(const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength, const unsigned char* bySKey, size_t skeylength, const unsigned char* byAttrs, size_t attrlength);
		bool ReplaceVal(const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength);
		bool ReplaceSKey(const unsigned char* byKey, size_t keylength, const unsigned char* bySKey, size_t skeylength);
		bool ReplaceAttrs(const unsigned char* byKey, size_t keylength, const unsigned char* byAttrs, size_t attrlength);
		bool DelKey(const unsigned char* byKey, size_t keylength);
		bool OverWriteValue(const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength, off_t valoffset);
		bool Rename(const unsigned char* byOldKey, size_t oldkeylength, const unsigned char* byNewKey, size_t newkeylength, const unsigned char* byAttrs, size_t attrlength);
		const BCOM* Get(void) const;

	protected:
		virtual bool Put(long type, const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength, const unsigned char* bySKey, size_t skeylength, const unsigned char* byAttrs, size_t attrlength, const unsigned char* byExdata, size_t exdatalength);
};

#endif	// K2HCOMMAND_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
