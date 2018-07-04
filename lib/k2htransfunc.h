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
 * CREATE:   Thu Feb 6 2014
 * REVISION:
 *
 */
#ifndef	K2HTRANSFUNC_H
#define	K2HTRANSFUNC_H

#include "k2hash.h"
#include "k2hcommon.h"
#include "k2hcommand.h"

//---------------------------------------------------------
// Readme about Transaction functions
//---------------------------------------------------------
// K2Hash library has two type functions for transaction.
// 1) k2h_trans         - transaction callback function.
//                        default buitin function does nothing.
// 2) k2h_trans_version - returns transaction function verson
//                        string
// 
// You can overwrite these functions by follow two way.
// 1) Built'in these function in your source codes(binary)
//    You must make completely same prototype functions in your
//    codes. Thereby, your functions is given priority to over-
//    write these functions in the library, In other words you
//    can overwrite these.
// 
// 2) Load your library
//    You can load your library(so) which has these functions by
//    calling Load() member function in K2HTransDynLib class.
// 
// If you use these functions for exsample: recieves transaction,
// checking function version. You can call below macros which is
// defined in this header file, so you call rightly function
// regardless of overwriting functions.
// 
//    K2H_TRANS_FUNC(k2h_h handle, PBCOM pBinCom)
//    K2H_TRANS_VER_FUNC(void)
//    K2H_TRANS_CNTL_FUNC(k2h_h handle, PTRANSOPT pOpt)
// 
// [NOTICE]
// Now libk2hash can load only one transaction library and uses one
// kind of transaction function.
// It means that the program which links libk2hash attaches many 
// k2hash file(over two k2hash file) can not load many transaction 
// functions, and that it can load only one transaction library.
// 
// Yes, we will be able to load many library and many kind of 
// functions really, but it will be a lot of costly and less benefits.
// We are pending this function because it is not needed on most case.
// And we think that it complicates the K2H_TRANS_XXXX macro calls, 
// performance will degrade. If adding this function, we need to 
// change CALL_K2H_TRANS_FUNCTION macro and K2HTransDynLib class 
// should need to take many loaded libraries handle by each k2hash 
// object like K2HTransManager class.
// 
// When a program attaches many k2hash files, one transaction function
// is used for all k2hash file. If you stop transaction for some k2hash
// file, you can disable transaction for those.
// 

//---------------------------------------------------------
// Transaction structures
//---------------------------------------------------------
DECL_EXTERN_C_START		// extern "C" - start

#define	MAX_TRANSACTION_FILEPATH	1024
#define	MAX_TRANSACTION_PREFIX		512
#define	MAX_TRANSACTION_PARAM		(1024 * 32)				// changed length from 1024 to 32K after v1.0.45

typedef struct transaction_option{
	char			szFilePath[MAX_TRANSACTION_FILEPATH];	// If isEnable is false or not use transaction file, this value can take empty.
	bool			isEnable;
	// following values supports after v1.0.8
	unsigned char	byTransPrefix[MAX_TRANSACTION_PREFIX];	// If you want to change transaction queue name, should set this value.
	size_t			PrefixLength;
	// following values supports after v1.0.14
	unsigned char	byTransParam[MAX_TRANSACTION_PARAM];	// For transaction plugin if it needs
	size_t			ParamLength;
}TRANSOPT, *PTRANSOPT;

DECL_EXTERN_C_END		// extern "C" - end

//---------------------------------------------------------
// Global Transaction function
//---------------------------------------------------------
// 
// weak function for over loading same name function.
//
DECL_EXTERN_C_START		// extern "C" - start

// transaction callback function
extern bool k2h_trans(k2h_h handle, PBCOM pBinCom) K2HASH_ATTR_WEAK;

// Transaction function(library) version string.
extern const char* k2h_trans_version(void) K2HASH_ATTR_WEAK;

// transaction control function
extern bool k2h_trans_cntl(k2h_h handle, PTRANSOPT pOpt) K2HASH_ATTR_WEAK;

DECL_EXTERN_C_END		// extern "C" - end

//---------------------------------------------------------
// Prototype Transaction function
//---------------------------------------------------------
DECL_EXTERN_C_START		// extern "C" - start

typedef bool (*Tfp_k2h_trans)(k2h_h handle, PBCOM pBinCom);
typedef const char* (*Tfp_k2h_trans_version)(void);
typedef bool (*Tfp_k2h_trans_cntl)(k2h_h handle, PTRANSOPT pOpt);

DECL_EXTERN_C_END		// extern "C" - end

//---------------------------------------------------------
// Macros
//---------------------------------------------------------
#define	CALL_K2H_TRANS_FUNCTION(type, ...)	(reinterpret_cast<K2HSTRJOIN(Tfp_, type)>(NULL != K2HTransDynLib::get()->K2HSTRJOIN(get_, type)() ? K2HTransDynLib::get()->K2HSTRJOIN(get_, type)() : type))(__VA_ARGS__)
#define	K2H_TRANS_FUNC(...)					CALL_K2H_TRANS_FUNCTION(k2h_trans, __VA_ARGS__)
#define	K2H_TRANS_VER_FUNC()				CALL_K2H_TRANS_FUNCTION(k2h_trans_version)
#define	K2H_TRANS_CNTL_FUNC(...)			CALL_K2H_TRANS_FUNCTION(k2h_trans_cntl, __VA_ARGS__)

//---------------------------------------------------------
// Load library
//---------------------------------------------------------
class K2HTransDynLib
{
	private:
		void*					hDynLib;
		Tfp_k2h_trans			fp_k2h_trans;
		Tfp_k2h_trans_version	fp_k2h_trans_version;
		Tfp_k2h_trans_cntl		fp_k2h_trans_cntl;

	private:
		K2HTransDynLib();
		virtual ~K2HTransDynLib();

	public:
		static K2HTransDynLib* get(void);

		bool Unload(void);
		bool Load(const char* path);

		Tfp_k2h_trans get_k2h_trans(void) { return fp_k2h_trans; }
		Tfp_k2h_trans_version get_k2h_trans_version(void) { return fp_k2h_trans_version; }
		Tfp_k2h_trans_cntl get_k2h_trans_cntl(void) { return fp_k2h_trans_cntl; }
};

#endif	// K2HTRANSFUNC_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
