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
 * CREATE:   Fri Nor 31 2013
 * REVISION:
 *
 */
#ifndef	K2HASH_H
#define	K2HASH_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

#if defined(__cplusplus)
extern "C" {
#endif

//---------------------------------------------------------
// Typedefs
//---------------------------------------------------------
typedef	uint64_t	k2h_h;					// k2hash handle
typedef uint64_t	k2h_hash_t;				// for hash code
typedef uint64_t	k2h_arrpos_t;
typedef	uint64_t	k2h_find_h;				// k2hash find handle
typedef	uint64_t	k2h_da_h;				// k2hash direct access handle
typedef	uint64_t	k2h_q_h;				// k2hash queue handle
typedef	uint64_t	k2h_keyq_h;				// k2hash key queue handle

typedef enum _k2h_da_mode{					// for direct access mode
	K2H_DA_READ		= 1,
	K2H_DA_WRITE	= 2,
	K2H_DA_RW		= 3
}K2HDAMODE;

typedef enum k2h_get_trial_cb_result{
	K2HGETCB_RES_ERROR		= -1,
	K2HGETCB_RES_NOTHING	= 0,			// Nothing to do
	K2HGETCB_RES_OVERWRITE					// OVer write the value
}K2HGETCBRES;

typedef enum k2h_q_remove_trial_cb_result{
	K2HQRMCB_RES_ERROR		= -1,
	K2HQRMCB_RES_CON_RM		= 0,			// Remove and continue to do next
	K2HQRMCB_RES_CON_NOTRM,					// Not remove and continue to do next
	K2HQRMCB_RES_FIN_RM,					// Remove and finish
	K2HQRMCB_RES_FIN_NOTRM					// Not remove and finish
}K2HQRMCBRES;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	K2H_INVALID_HANDLE			0UL
#define	K2H_VERSION_LENGTH			8		// maximum version length
#define	K2H_HASH_FUNC_VER_LENGTH	32		// hash function version length

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
// for subkeys list
typedef struct k2h_key_pack{
	unsigned char*	pkey;
	size_t			length;
}K2HKEYPCK, *PK2HKEYPCK;

// for attributes list
typedef struct k2h_attr_pack{
	unsigned char*	pkey;
	size_t			keylength;
	unsigned char*	pval;
	size_t			vallength;
}K2HATTRPCK, *PK2HATTRPCK;

// for direct set/get element
typedef struct k2hash_binary_data{
	unsigned char*	byptr;
	size_t			length;
}K2HBIN, *PK2HBIN;

// for getting state
//
// [NOTE]
// This structure has all status for k2hash file(memory).
// Thus this structure includes most of the members of K2H structure, and
// has some status members made by calculation
//
typedef struct k2h_state{
	char			version[K2H_VERSION_LENGTH];			// Version string as K2hash file
	char			hash_version[K2H_HASH_FUNC_VER_LENGTH];	// Version string as Hash Function
	char			trans_version[K2H_HASH_FUNC_VER_LENGTH];// Version string as Transaction Function

	int				trans_pool_count;						// Transaction plugin thread pool count
	k2h_hash_t		max_mask;								// Muximum value for cur_mask
	k2h_hash_t		min_mask;								// Minimum value for cur_mask
	k2h_hash_t		cur_mask;								// Current mask value for hash(This value is changed automatically)
	k2h_hash_t		collision_mask;							// Mask value for collision when masked hash value by cur_mask(This value is not changed)
	unsigned long	max_element_count;						// Muximum count for elements in collision key index structure(Increasing cur_mask when this value is over)

	size_t			total_size;								// Total size of k2hash
	size_t			page_size;								// Paging size(system)
	size_t			file_size;								// k2hash file(memory) size
	size_t			total_used_size;						// Total using size in k2hash
	size_t			total_map_size;							// Total mapping size for k2hash
	size_t			total_element_size;						// Total element area size
	size_t			total_page_size;						// Total page area size

	long			total_area_count;						// Total mmap area count( = MAX_K2HAREA_COUNT)
	long			total_element_count;					// Total element count in k2hash
	long			total_page_count;						// Total page count in k2hash
	long			assigned_area_count;					// Assigned area count
	long			assigned_key_count;						// Assigned key index count
	long			assigned_ckey_count;					// Assigned collision key index count
	long			assigned_element_count;					// Assigned element count
	long			assigned_page_count;					// Assigned page count
	long			unassigned_element_count;				// Unassigned element count(free element count)
	long			unassigned_page_count;					// Unassigned page count(free page count)

	struct timeval	last_update;							// Last update of data
	struct timeval	last_area_update;						// Last update of expanding area
}__attribute__ ((packed)) K2HSTATE, *PK2HSTATE;

//---------------------------------------------------------
// Prototype Functions
//---------------------------------------------------------
typedef K2HGETCBRES (*k2h_get_trial_callback)(const unsigned char* byKey, size_t keylen, const unsigned char* byValue, size_t vallen, unsigned char** ppNewValue, size_t* pnewvallen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData);
//
// [SAMPLE] This is the sample k2h_get_trial_callback function.
//
//	This function is called from inner Get function.
//	This callback returns whichever the value will be over written
//	with new value or be to do nothing. If you need to over write the
//	value, this function must return K2HGETCB_RES_OVERWRITE and set
//	ALLOCATED new value into *ppNewValue and pnewvallen. The other you
//	do not need to do anything, this must return K2HGETCB_RES_NOTHING.
//	If something error is occurred, must return K2HGETCB_RES_ERROR.
//
//	static K2HGETCBRES GetTrialCallback(const unsigned char* byKey, size_t keylen, const unsigned char* byValue, size_t vallen, unsigned char** ppNewValue, size_t* pnewvallen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData)
//	{
//		if(!byKey || 0 == keylen || !ppNewValue || !pnewvallen){
//			return K2HGETCB_RES_ERROR;
//		}
//	
//		//
//		// Check get results which are byKey and byValue.
//		// If you need to reset value for key, you set ppNewValue and return K2HGETCB_RES_OVERWRITE.
//		// pExtData is the parameter when you call Get function.
//		//
//	
//		return K2HGETCB_RES_NOTHING;
//	}
//	

typedef K2HQRMCBRES (*k2h_q_remove_trial_callback)(const unsigned char* bydata, size_t datalen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData);
//
// [SAMPLE] This is the sample k2h_q_remove_trial_callback function.
//
//	This function is called from inner Remove function.
//	This callback returns whichever the queued data should be removed or
//	not. And whichever removing should be stopped or not. You can use
//	enum symbol in K2HQRMCBRES for the return value. The extention value
//	pExtData is the paraemter when you call Remove function. For example,
//	you can use this value for that keeping the removed keys(datas).
//
//	static K2HQRMCBRES QueueRemoveCallback(const unsigned char* bydata, size_t datalen, const PK2HATTRPCK pattrs, int attrscnt, void* pExtData)
//	{
//		if(!bydata || 0 == datalen){
//			return K2HQRMCB_RES_ERROR;
//		}
//	
//		//
//		// Check get queued data as bydata which is queued data(K2HQueue) or key(K2HKeyQueue).
//		// If you need to remove it from queue, this function must return K2HQRMCB_RES_CON_RM or K2HQRMCB_RES_FIN_RM.
//		// The other do not want to remove it, must return K2HQRMCB_RES_CON_NOTRM or K2HQRMCB_RES_FIN_NOTRM.
//		// If you want to stop removing no more, this function can return K2HQRMCB_RES_FIN_*(RM/NOTRM).
//		// pExtData is the parameter when you call Remove function.
//		//
//	
//		return K2HQRMCB_RES_CON_RM;
//	}
//	

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
// [debug]
//
// k2h_bump_debug_level				bumpup debugging level(silent -> error -> warning -> messages ->...)
// k2h_set_debug_level_silent		set silent for debugging level
// k2h_set_debug_level_error		set error for debugging level
// k2h_set_debug_level_warning		set warning for debugging level
// k2h_set_debug_level_message		set message for debugging level
// k2h_set_debug_file				set file path for debugging message
// k2h_unset_debug_file				unset file path for debugging message to stderr(default)
// k2h_load_debug_env				set debugging level and file path by loading environment.
// k2h_set_bumup_debug_signal_user1	set signal USER1 handler for bumpup debug level
//
extern void k2h_bump_debug_level(void);
extern void k2h_set_debug_level_silent(void);
extern void k2h_set_debug_level_error(void);
extern void k2h_set_debug_level_warning(void);
extern void k2h_set_debug_level_message(void);
extern bool k2h_set_debug_file(const char* filepath);
extern bool k2h_unset_debug_file(void);
extern bool k2h_load_debug_env(void);
extern bool k2h_set_bumup_debug_signal_user1(void);

// [load library]
//
// k2h_load_hash_library			load extra hash library
// k2h_unload_hash_library			unload extra hash library
// k2h_load_transaction_library		load extra transaction library
// k2h_unload_transaction_library	unload extra transaction library
// 
extern bool k2h_load_hash_library(const char* libpath);
extern bool k2h_unload_hash_library(void);
extern bool k2h_load_transaction_library(const char* libpath);
extern bool k2h_unload_transaction_library(void);

// [create / open / close]
//
// k2h_create			create and initialize k2hash file
// k2h_open				attach k2hash file or attach only memory
//						If filepath is specified, it means attach to file. If not,
//						means only memory.
//						If specified file is not existed, create and initialize it.
// k2h_open_rw			attach k2hash file for read/write, this function is simple 
//						interface to k2h_open.
// k2h_open_ro			attach k2hash file for read only, this function is simple 
//						interface to k2h_open.
// k2h_open_tempfile	attach k2hash file which tempolary file, this function is
//						simple interface to k2h_open.
// k2h_open_mem			attach k2hash on memory, this function is simple interface
//						to k2h_open.
// k2h_close			detach k2hash file(memory) immediatry
// k2h_close_wait		detach k2hash file(memory) with timeout(or blocking)
//
extern bool k2h_create(const char* filepath, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize);
extern k2h_h k2h_open(const char* filepath, bool readonly, bool removefile, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize);
extern k2h_h k2h_open_rw(const char* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize);
extern k2h_h k2h_open_ro(const char* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize);
extern k2h_h k2h_open_tempfile(const char* filepath, bool fullmap, int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize);
extern k2h_h k2h_open_mem(int maskbitcnt, int cmaskbitcnt, int maxelementcnt, size_t pagesize);
extern bool k2h_close(k2h_h handle);
extern bool k2h_close_wait(k2h_h handle, long waitms);

// [transaction / archive]
//
// k2h_transaction						enable/disable transaction
// k2h_transaction_prefix				enable/disable transaction
// k2h_transaction_param				enable/disable transaction
// k2h_transaction_param_we				enable/disable transaction
// k2h_enable_transaction				enable transaction, this function is simple interface to k2h_transaction
// k2h_enable_transaction_prefix		enable transaction, this function is simple interface to k2h_transaction
// k2h_enable_transaction_param			enable transaction, this function is simple interface to k2h_transaction
// k2h_enable_transaction_param_we		enable transaction, this function is simple interface to k2h_transaction
// k2h_disable_transaction				disable transaction, this function is simple interface to k2h_transaction
// 
// k2h_get_transaction_archive_fd		get transaction archive file discriptor, if it is set.
// 
// k2h_load_archive						load from (transaction formatted)archive file
// k2h_put_archive						put to (transaction formatted)archive file
//
// k2h_get_transaction_thread_pool		get thread pool count for transaction
// k2h_set_transaction_thread_pool		set thread pool for transaction
// k2h_unset_transaction_thread_pool	unset thread pool(no thread pool) for transaction
//
extern bool k2h_transaction(k2h_h handle, bool enable, const char* transfile);
extern bool k2h_transaction_prefix(k2h_h handle, bool enable, const char* transfile, const unsigned char* pprefix, size_t prefixlen);
extern bool k2h_transaction_param(k2h_h handle, bool enable, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen);
extern bool k2h_transaction_param_we(k2h_h handle, bool enable, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen, const time_t* expire);
extern bool k2h_enable_transaction(k2h_h handle, const char* transfile);
extern bool k2h_enable_transaction_prefix(k2h_h handle, const char* transfile, const unsigned char* pprefix, size_t prefixlen);
extern bool k2h_enable_transaction_param(k2h_h handle, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen);
extern bool k2h_enable_transaction_param_we(k2h_h handle, const char* transfile, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen, const time_t* expire);
extern bool k2h_disable_transaction(k2h_h handle);

extern int k2h_get_transaction_archive_fd(k2h_h handle);

extern bool k2h_load_archive(k2h_h handle, const char* filepath, bool errskip);
extern bool k2h_put_archive(k2h_h handle, const char* filepath, bool errskip);

extern int k2h_get_transaction_thread_pool(void);
extern bool k2h_set_transaction_thread_pool(int count);
extern bool k2h_unset_transaction_thread_pool(void);

// [attribute]
//
// k2h_set_common_attr                  set common builtin attribute parameters
// k2h_clean_common_attr                clear all common builtin attribute parameters and unload plugins
// k2h_add_attr_plugin_library          load attribute plugin library as additional
// k2h_add_attr_crypt_pass              add pass phrase for crypt in common builtin attribute
// k2h_print_attr_version               print all attribute library version information(including builtin)
// k2h_print_attr_information			print all attribute parameters
// 
extern bool k2h_set_common_attr(k2h_h handle, const bool* is_mtime, const bool* is_defenc, const char* passfile, const bool* is_history, const time_t* expire);
extern bool k2h_clean_common_attr(k2h_h handle);
extern bool k2h_add_attr_plugin_library(k2h_h handle, const char* libpath);
extern bool k2h_add_attr_crypt_pass(k2h_h handle, const char* pass, bool is_default_encrypt);
extern bool k2h_print_attr_version(k2h_h handle, FILE* stream);
extern bool k2h_print_attr_information(k2h_h handle, FILE* stream);

// [get]
//
// k2h_get_value				     get allocated binary value by key
// k2h_get_direct_value			     
// k2h_get_str_value			     get allocated string value by string key
// k2h_get_str_direct_value		     
// 
// k2h_get_value_wp                  get allocated binary value by key with manually decrypting
// k2h_get_direct_value_wp           
// k2h_get_str_value_wp              get allocated string value by string key with manually decrypting
// k2h_get_str_direct_value_wp       
// 
// k2h_get_value_np                  no attribute protect, get allocated binary value by key
// k2h_get_direct_value_np           
// k2h_get_str_value_np              no attribute protect, get allocated string value by string key
// k2h_get_str_direct_value_np       
// 
// k2h_get_value_ext			     get allocated binary value by key with callback function
// k2h_get_direct_value_ext		     
// k2h_get_str_value_ext		     get allocated string value by string key with callback function
// k2h_get_str_direct_value_ext	     
// 
// k2h_get_value_wp_ext              get allocated binary value(manually decrypted) by key with callback function
// k2h_get_direct_value_wp_ext       
// k2h_get_str_value_wp_ext          get allocated string value(manually decrypted) by string key with callback function
// k2h_get_str_direct_value_wp_ext   
// 
// k2h_get_value_np_ext              no attribute protect, get allocated binary value by key with callback function
// k2h_get_direct_value_np_ext       
// k2h_get_str_value_np_ext          no attribute protect, get allocated string value by string key with callback function
// k2h_get_str_direct_value_np_ext   
// 
// k2h_get_subkeys				     get allocated binary subkeys list packed K2HKEYPCK
// k2h_get_direct_subkeys		     
// k2h_get_str_subkeys			     get allocated string subkeys list which is null terminated string array
// k2h_get_str_direct_subkeys	     
// 
// k2h_get_subkeys_np                no attribute protect, get allocated binary subkeys list packed K2HKEYPCK
// k2h_get_direct_subkeys_np         
// k2h_get_str_subkeys_np            no attribute protect, get allocated string subkeys list which is null terminated string array
// k2h_get_str_direct_subkeys_np     
// 
// k2h_free_keypack				     free K2HKEYPCK pointer returned by k2h_get_subkeys or k2h_get_direct_subkeys
// k2h_free_keyarray			     free string array returned by k2h_get_str_subkeys or k2h_get_str_direct_subkeys
// 
// k2h_get_attrs                     get allocated binary attributes list packed K2HATTRPCK
// k2h_get_direct_attrs              
// k2h_get_str_direct_attrs          
// k2h_free_attrpack                 free K2HATTRPCK pointer returned by k2h_get_attrs or k2h_get_direct_attrs
// 
extern bool k2h_get_value(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength);
extern unsigned char* k2h_get_direct_value(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength);
extern bool k2h_get_str_value(k2h_h handle, const char* pkey, char** ppval);
extern char* k2h_get_str_direct_value(k2h_h handle, const char* pkey);

extern bool k2h_get_value_wp(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, const char* pass);
extern unsigned char* k2h_get_direct_value_wp(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, const char* pass);
extern bool k2h_get_str_value_wp(k2h_h handle, const char* pkey, char** ppval, const char* pass);
extern char* k2h_get_str_direct_value_wp(k2h_h handle, const char* pkey, const char* pass);

extern bool k2h_get_value_np(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength);
extern unsigned char* k2h_get_direct_value_np(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength);
extern bool k2h_get_str_value_np(k2h_h handle, const char* pkey, char** ppval);
extern char* k2h_get_str_direct_value_np(k2h_h handle, const char* pkey);

extern bool k2h_get_value_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData);
extern unsigned char* k2h_get_direct_value_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData);
extern bool k2h_get_str_value_ext(k2h_h handle, const char* pkey, char** ppval, k2h_get_trial_callback fp, void* pExtData);
extern char* k2h_get_str_direct_value_ext(k2h_h handle, const char* pkey, k2h_get_trial_callback fp, void* pExtData);

extern bool k2h_get_value_wp_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData, const char* pass);
extern unsigned char* k2h_get_direct_value_wp_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData, const char* pass);
extern bool k2h_get_str_value_wp_ext(k2h_h handle, const char* pkey, char** ppval, k2h_get_trial_callback fp, void* pExtData, const char* pass);
extern char* k2h_get_str_direct_value_wp_ext(k2h_h handle, const char* pkey, k2h_get_trial_callback fp, void* pExtData, const char* pass);

extern bool k2h_get_value_np_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, unsigned char** ppval, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData);
extern unsigned char* k2h_get_direct_value_np_ext(k2h_h handle, const unsigned char* pkey, size_t keylength, size_t* pvallength, k2h_get_trial_callback fp, void* pExtData);
extern bool k2h_get_str_value_np_ext(k2h_h handle, const char* pkey, char** ppval, k2h_get_trial_callback fp, void* pExtData);
extern char* k2h_get_str_direct_value_np_ext(k2h_h handle, const char* pkey, k2h_get_trial_callback fp, void* pExtData);

extern bool k2h_get_subkeys(k2h_h handle, const unsigned char* pkey, size_t keylength, PK2HKEYPCK* ppskeypck, int* pskeypckcnt);
extern PK2HKEYPCK k2h_get_direct_subkeys(k2h_h handle, const unsigned char* pkey, size_t keylength, int* pskeypckcnt);
extern int k2h_get_str_subkeys(k2h_h handle, const char* pkey, char*** ppskeyarray);
extern char** k2h_get_str_direct_subkeys(k2h_h handle, const char* pkey);

extern bool k2h_get_subkeys_np(k2h_h handle, const unsigned char* pkey, size_t keylength, PK2HKEYPCK* ppskeypck, int* pskeypckcnt);
extern PK2HKEYPCK k2h_get_direct_subkeys_np(k2h_h handle, const unsigned char* pkey, size_t keylength, int* pskeypckcnt);
extern int k2h_get_str_subkeys_np(k2h_h handle, const char* pkey, char*** ppskeyarray);
extern char** k2h_get_str_direct_subkeys_np(k2h_h handle, const char* pkey);

extern bool k2h_free_keypack(PK2HKEYPCK pkeys, int keycnt);
extern bool k2h_free_keyarray(char** pkeys);

extern bool k2h_get_attrs(k2h_h handle, const unsigned char* pkey, size_t keylength, PK2HATTRPCK* ppattrspck, int* pattrspckcnt);
extern PK2HATTRPCK k2h_get_direct_attrs(k2h_h handle, const unsigned char* pkey, size_t keylength, int* pattrspckcnt);
extern PK2HATTRPCK k2h_get_str_direct_attrs(k2h_h handle, const char* pkey, int* pattrspckcnt);
extern bool k2h_free_attrpack(PK2HATTRPCK pattrs, int attrcnt);

// [set]
//
// k2h_set_all			 set value and subkeys list to key(not make subkey as key)
// k2h_set_str_all		 
// k2h_set_value		 set value to key(not change subkeys list in key)
// k2h_set_str_value	 
// 
// k2h_set_all_wa        set value and subkeys list to key(not make subkey as key) with encrypt and expire
// k2h_set_str_all_wa    
// k2h_set_value_wa      set value to key(not change subkeys list in key) with encrypt and expire
// k2h_set_str_value_wa  
// 
// k2h_set_subkeys		 set subkeys list to key(not change value in key)
// k2h_set_str_subkeys	 
//
// k2h_add_subkey		 make subkey as key, and add subkey into key's subkeys list.
// k2h_add_str_subkey	 
//
// k2h_add_subkey_wa     make subkey as key with encrypt and expire, and add subkey into key's subkeys list,
// k2h_add_str_subkey_wa 
//
// k2h_add_attr          add attribute into key.
// k2h_add_str_attr      
//
extern bool k2h_set_all(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength, const PK2HKEYPCK pskeypck, int skeypckcnt);
extern bool k2h_set_str_all(k2h_h handle, const char* pkey, const char* pval, const char** pskeyarray);
extern bool k2h_set_value(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength);
extern bool k2h_set_str_value(k2h_h handle, const char* pkey, const char* pval);

extern bool k2h_set_all_wa(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength, const PK2HKEYPCK pskeypck, int skeypckcnt, const char* pass, const time_t* expire);
extern bool k2h_set_str_all_wa(k2h_h handle, const char* pkey, const char* pval, const char** pskeyarray, const char* pass, const time_t* expire);
extern bool k2h_set_value_wa(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pval, size_t vallength, const char* pass, const time_t* expire);
extern bool k2h_set_str_value_wa(k2h_h handle, const char* pkey, const char* pval, const char* pass, const time_t* expire);

extern bool k2h_set_subkeys(k2h_h handle, const unsigned char* pkey, size_t keylength, const PK2HKEYPCK pskeypck, int skeypckcnt);
extern bool k2h_set_str_subkeys(k2h_h handle, const char* pkey, const char** pskeyarray);

extern bool k2h_add_subkey(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* psubkey, size_t skeylength, const unsigned char* pval, size_t vallength);
extern bool k2h_add_str_subkey(k2h_h handle, const char* pkey, const char* psubkey, const char* pval);

extern bool k2h_add_subkey_wa(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* psubkey, size_t skeylength, const unsigned char* pval, size_t vallength, const char* pass, const time_t* expire);
extern bool k2h_add_str_subkey_wa(k2h_h handle, const char* pkey, const char* psubkey, const char* pval, const char* pass, const time_t* expire);

extern bool k2h_add_attr(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pattrkey, size_t attrkeylength, const unsigned char* pattrval, size_t attrvallength);
extern bool k2h_add_str_attr(k2h_h handle, const char* pkey, const char* pattrkey, const char* pattrval);

// [remove]
//
// k2h_remove_all			remove key and subkeys listed in key
// k2h_remove_str_all		
// k2h_remove				remove only key(leave subkeys)
// k2h_remove_str			
// k2h_remove_subkey		remove subkey and remove it from subkeys list in key
// k2h_remove_str_subkey	
//
extern bool k2h_remove_all(k2h_h handle, const unsigned char* pkey, size_t keylength);
extern bool k2h_remove_str_all(k2h_h handle, const char* pkey);
extern bool k2h_remove(k2h_h handle, const unsigned char* pkey, size_t keylength);
extern bool k2h_remove_str(k2h_h handle, const char* pkey);
extern bool k2h_remove_subkey(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* psubkey, size_t skeylength);
extern bool k2h_remove_str_subkey(k2h_h handle, const char* pkey, const char* psubkey);

// [rename]
//
// k2h_rename				rename key
// k2h_rename_str			
//
extern bool k2h_rename(k2h_h handle, const unsigned char* pkey, size_t keylength, const unsigned char* pnewkey, size_t newkeylength);
extern bool k2h_rename_str(k2h_h handle, const char* pkey, const char* pnewkey);

// [direct set/get]
//
// k2h_get_elements_by_hash			get binary elements from hash code
// k2h_set_element_by_binary		set element from binary element data
//
extern bool k2h_get_elements_by_hash(k2h_h handle, const k2h_hash_t starthash, const struct timespec startts, const struct timespec endts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check, k2h_hash_t* pnexthash, PK2HBIN* ppbindatas, size_t* pdatacnt);
extern bool k2h_set_element_by_binary(k2h_h handle, const PK2HBIN pbindatas, const struct timespec* pts);

// [find]
//
// k2h_find_first					get the first find handle for all keys
// k2h_find_first_subkey			get the first find handle for all subkeys in a key
// k2h_find_first_str_subkey		
// k2h_find_next					get next find handle
// k2h_find_free					free find handle
//
// k2h_find_get_key					get allocated binary key from find handle
// k2h_find_get_str_key				get allocated string key from find handle
// k2h_find_get_value				get allocated binary value from find handle
// k2h_find_get_direct_value		get allocated string value from find handle
// k2h_find_get_subkeys				get allocated binary subkeys list packed K2HKEYPCK from find handle
// k2h_find_get_direct_subkeys		
// k2h_find_get_str_subkeys			get allocated string subkeys list which is null terminated string array from find handle
// k2h_find_get_str_direct_subkeys	
//
extern k2h_find_h k2h_find_first(k2h_h handle);
extern k2h_find_h k2h_find_first_subkey(k2h_h handle, const unsigned char* pkey, size_t keylength);
extern k2h_find_h k2h_find_first_str_subkey(k2h_h handle, const char* pkey);
extern k2h_find_h k2h_find_next(k2h_find_h findhandle);
extern bool k2h_find_free(k2h_find_h findhandle);

extern bool k2h_find_get_key(k2h_find_h findhandle, unsigned char** ppkey, size_t* pkeylength);
extern char* k2h_find_get_str_key(k2h_find_h findhandle);
extern bool k2h_find_get_value(k2h_find_h findhandle, unsigned char** ppval, size_t* pvallength);
extern char* k2h_find_get_direct_value(k2h_find_h findhandle);
extern bool k2h_find_get_subkeys(k2h_find_h findhandle, PK2HKEYPCK* ppskeypck, int* pskeypckcnt);
extern PK2HKEYPCK k2h_find_get_direct_subkeys(k2h_find_h findhandle, int* pskeypckcnt);
extern int k2h_find_get_str_subkeys(k2h_find_h findhandle, char*** ppskeyarray);
extern char** k2h_find_get_str_direct_subkeys(k2h_find_h findhandle);

// [direct access]
//
// k2h_da_handle					get k2h_da_h handle for accessing value directly( for reading/writing/both )
// k2h_da_handle_read				get k2h_da_h handle for reading
// k2h_da_handle_write				get k2h_da_h handle for writing
// k2h_da_handle_rw					get k2h_da_h handle for reading and writing
// k2h_da_str_handle				get k2h_da_h handle for accessing value directly( for reading/writing/both )
// k2h_da_str_handle_read			get k2h_da_h handle for reading
// k2h_da_str_handle_write			get k2h_da_h handle for writing
// k2h_da_str_handle_rw				get k2h_da_h handle for reading and writing
// k2h_da_free						free k2h_da_h handle
// 
// k2h_da_get_length				get value length
// 
// k2h_da_get_buf_size				get internal buffer length for reading/writing
// k2h_da_set_buf_size				set internal buffer length for reading/writing
// 
// k2h_da_get_offset				get reading or writing offset in value
// k2h_da_get_read_offset			get reading offset in value
// k2h_da_get_write_offset			get writing offset in value
// k2h_da_set_offset				set reading or writing offset in value
// k2h_da_set_read_offset			set reading offset in value
// k2h_da_set_write_offset			set writing offset in value
// 
// k2h_da_get_value					read value with length from offset
// k2h_da_get_value_offset			read value with length and offset
// k2h_da_get_value_to_file			read value into file with length
// k2h_da_read						read value with length from offset
// k2h_da_read_offset				read value with length and offset
// k2h_da_read_str					read value from offset as string
// 
// k2h_da_set_value					write value with length from offset
// k2h_da_set_value_offset			write value with length and offset
// k2h_da_set_value_from_file		write value from file with length
// k2h_da_set_value_str				write string from offset
// 
extern k2h_da_h k2h_da_handle(k2h_h handle, const unsigned char* pkey, size_t keylength, K2HDAMODE mode);
extern k2h_da_h k2h_da_handle_read(k2h_h handle, const unsigned char* pkey, size_t keylength);
extern k2h_da_h k2h_da_handle_write(k2h_h handle, const unsigned char* pkey, size_t keylength);
extern k2h_da_h k2h_da_handle_rw(k2h_h handle, const unsigned char* pkey, size_t keylength);
extern k2h_da_h k2h_da_str_handle(k2h_h handle, const char* pkey, K2HDAMODE mode);
extern k2h_da_h k2h_da_str_handle_read(k2h_h handle, const char* pkey);
extern k2h_da_h k2h_da_str_handle_write(k2h_h handle, const char* pkey);
extern k2h_da_h k2h_da_str_handle_rw(k2h_h handle, const char* pkey);
extern bool k2h_da_free(k2h_da_h dahandle);

extern ssize_t k2h_da_get_length(k2h_da_h dahandle);

extern ssize_t k2h_da_get_buf_size(k2h_da_h dahandle);
extern bool k2h_da_set_buf_size(k2h_da_h dahandle, size_t bufsize);

extern off_t k2h_da_get_offset(k2h_da_h dahandle, bool is_read);
extern off_t k2h_da_get_read_offset(k2h_da_h dahandle);
extern off_t k2h_da_get_write_offset(k2h_da_h dahandle);
extern bool k2h_da_set_offset(k2h_da_h dahandle, off_t offset, bool is_read);
extern bool k2h_da_set_read_offset(k2h_da_h dahandle, off_t offset);
extern bool k2h_da_set_write_offset(k2h_da_h dahandle, off_t offset);

extern bool k2h_da_get_value(k2h_da_h dahandle, unsigned char** ppval, size_t* pvallength);
extern bool k2h_da_get_value_offset(k2h_da_h dahandle, unsigned char** ppval, size_t* pvallength, off_t offset);
extern bool k2h_da_get_value_to_file(k2h_da_h dahandle, int fd, size_t* pvallength);
extern unsigned char* k2h_da_read(k2h_da_h dahandle, size_t* pvallength);
extern unsigned char* k2h_da_read_offset(k2h_da_h dahandle, size_t* pvallength, off_t offset);
extern char* k2h_da_read_str(k2h_da_h dahandle);

extern bool k2h_da_set_value(k2h_da_h dahandle, const unsigned char* pval, size_t vallength);
extern bool k2h_da_set_value_offset(k2h_da_h dahandle, const unsigned char* pval, size_t vallength, off_t offset);
extern bool k2h_da_set_value_from_file(k2h_da_h dahandle, int fd, size_t* pvallength);
extern bool k2h_da_set_value_str(k2h_da_h dahandle, const char* pval);

// [Queue]
//
// k2h_q_handle					get k2h_q_h handle for filo or lifo queue
// k2h_q_handle_prefix			get k2h_q_h handle for filo or lifo queue with queue prefix value
// k2h_q_handle_str_prefix		get k2h_q_h handle for filo or lifo queue with string queue prefix value
// k2h_q_free					free k2h_q_h handle
// 
// k2h_q_empty					check queue empty
// k2h_q_count					get the data count in queue
// k2h_q_read					get the data from queue without removing
// k2h_q_str_read				get the string data from queue without removing
// k2h_q_push					push the data to queue
// k2h_q_str_push				push the string data to queue
// k2h_q_pop					pop the data from queue
// k2h_q_str_pop				pop the string data from queue
// k2h_q_remove					remove queue
// k2h_q_remove_ext				remove queue with calling callback function 
// k2h_q_read_wp				get the data from queue without removing with pass phrase
// k2h_q_str_read_wp			get the string data from queue without removing with pass phrase
// k2h_q_push_wa				push the data to queue with attributes
// k2h_q_str_push_wa			push the string data to queue with attributes
// k2h_q_pop_wa					pop the data and attributes from queue with pass phrase
// k2h_q_str_pop_wa				pop the string data and attributes from queue with pass phrase
// k2h_q_pop_wp					pop the data from queue with pass phrase
// k2h_q_str_pop_wp				pop the string data from queue with pass phrase
// k2h_q_remove_wp				remove queue with pass phrase
// k2h_q_remove_wp_ext			remove queue with calling callback function with pass phrase
// k2h_q_dump					dump queue for debugging
// 
// k2h_keyq_handle				get k2h_keyq_h handle for filo or lifo queue
// k2h_keyq_handle_prefix		get k2h_keyq_h handle for filo or lifo queue with queue prefix value
// k2h_keyq_handle_str_prefix	get k2h_keyq_h handle for filo or lifo queue with string queue prefix value
// k2h_keyq_free				free k2h_keyq_h handle
// 
// k2h_keyq_empty				check queue empty
// k2h_keyq_count				get the data count in queue
// k2h_keyq_read				get the data(key) from queue without removing
// k2h_keyq_read_keyval			get the data(key) and it's value from queue without removing
// k2h_keyq_str_read			get the string data(key) from queue without removing
// k2h_keyq_str_read_keyval		get the string data(key) and it's string value from queue without removing
// k2h_keyq_push				push the data(key) to queue
// k2h_keyq_push_keyval			push the key to queue and make key-value into k2hash
// k2h_keyq_str_push			push the string data(key) to queue
// k2h_keyq_str_push_keyval		push the string key to queue and make key-value into k2hash
// k2h_keyq_pop					pop the value which is pointed by queued key, the key is removed from queue and k2hash.
// k2h_keyq_pop_keyval			pop queued key and it's value, the key is removed from queue and k2hash.
// k2h_keyq_str_pop				pop the string value which is pointed by queued key, the key is removed from queue and k2hash.
// k2h_keyq_str_pop_keyval		pop queued string key and it's value, the key is removed from queue and k2hash.
// k2h_keyq_remove				remove queue and remove queued key from k2hash
// k2h_keyq_remove_ext			remove queue with calling callback function and remove queued key from k2hash
// k2h_keyq_read_wp				get the data(key with pass) from queue without removing
// k2h_keyq_read_keyval_wp		get the data(key with pass) and it's value from queue without removing
// k2h_keyq_str_read_wp			get the string data(key with pass) from queue without removing
// k2h_keyq_str_read_keyval_wp	get the string data(key with pass) and it's string value from queue without removing
// k2h_keyq_push_wa				push the data(key) to queue with attribute
// k2h_keyq_push_keyval_wa		push the key to queue and make key-value into k2hash with attribute
// k2h_keyq_str_push_wa			push the string data(key) to queue with attribute
// k2h_keyq_str_push_keyval_wa	push the string key to queue and make key-value into k2hash with attribute
// k2h_keyq_pop_wp				pop the value which is pointed by queued key with pass, the key is removed from queue and k2hash.
// k2h_keyq_pop_keyval_wp		pop queued key and it's value with pass, the key is removed from queue and k2hash.
// k2h_keyq_str_pop_wp			pop the string value which is pointed by queued key with pass, the key is removed from queue and k2hash.
// k2h_keyq_str_pop_keyval_wp	pop queued string key and it's value with pass, the key is removed from queue and k2hash.
// k2h_keyq_remove_wp			remove queue and remove queued key with pass from k2hash
// k2h_keyq_remove_wp_ext		remove queue with calling callback function and remove queued key with pass from k2hash
// k2h_keyq_dump				dump queue for debugging
// 
// [NOTES]
// k2h_q_h works only queue(fifo/lifo), but k2h_keyq_h works queue and it queued the value
// which means key name in k2hash.
// So k2h_q_h is K2HQueue class, k2h_keyq_h is K2HKeyQueue. These handle(class) is different
// on pop() and remove() functions.
// 
extern k2h_q_h k2h_q_handle(k2h_h handle, bool is_fifo);
extern k2h_q_h k2h_q_handle_prefix(k2h_h handle, bool is_fifo, const unsigned char* pref, size_t preflen);
extern k2h_q_h k2h_q_handle_str_prefix(k2h_h handle, bool is_fifo, const char* pref);
extern bool k2h_q_free(k2h_q_h qhandle);

extern bool k2h_q_empty(k2h_q_h qhandle);
extern int k2h_q_count(k2h_q_h qhandle);
extern bool k2h_q_read(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, int pos);
extern bool k2h_q_str_read(k2h_q_h qhandle, char** ppdata, int pos);
extern bool k2h_q_push(k2h_q_h qhandle, const unsigned char* bydata, size_t datalen);
extern bool k2h_q_str_push(k2h_q_h qhandle, const char* pdata);
extern bool k2h_q_pop(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen);
extern bool k2h_q_str_pop(k2h_q_h qhandle, char** ppdata);
extern bool k2h_q_remove(k2h_q_h qhandle, int count);
extern int k2h_q_remove_ext(k2h_q_h qhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata);
extern bool k2h_q_read_wp(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, int pos, const char* encpass);
extern bool k2h_q_str_read_wp(k2h_q_h qhandle, char** ppdata, int pos, const char* encpass);
extern bool k2h_q_push_wa(k2h_q_h qhandle, const unsigned char* bydata, size_t datalen, const PK2HATTRPCK pattrspck, int attrspckcnt, const char* encpass, const time_t* expire);
extern bool k2h_q_str_push_wa(k2h_q_h qhandle, const char* pdata, const PK2HATTRPCK pattrspck, int attrspckcnt, const char* encpass, const time_t* expire);
extern bool k2h_q_pop_wa(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, PK2HATTRPCK* ppattrspck, int* pattrspckcnt, const char* encpass);
extern bool k2h_q_str_pop_wa(k2h_q_h qhandle, char** ppdata, PK2HATTRPCK* ppattrspck, int* pattrspckcnt, const char* encpass);
extern bool k2h_q_pop_wp(k2h_q_h qhandle, unsigned char** ppdata, size_t* pdatalen, const char* encpass);
extern bool k2h_q_str_pop_wp(k2h_q_h qhandle, char** ppdata, const char* encpass);
extern bool k2h_q_remove_wp(k2h_q_h qhandle, int count, const char* encpass);
extern int k2h_q_remove_wp_ext(k2h_q_h qhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata, const char* encpass);
extern bool k2h_q_dump(k2h_q_h qhandle, FILE* stream);

extern k2h_keyq_h k2h_keyq_handle(k2h_h handle, bool is_fifo);
extern k2h_keyq_h k2h_keyq_handle_prefix(k2h_h handle, bool is_fifo, const unsigned char* pref, size_t preflen);
extern k2h_keyq_h k2h_keyq_handle_str_prefix(k2h_h handle, bool is_fifo, const char* pref);
extern bool k2h_keyq_free(k2h_keyq_h keyqhandle);

extern bool k2h_keyq_empty(k2h_keyq_h keyqhandle);
extern int k2h_keyq_count(k2h_keyq_h keyqhandle);
extern bool k2h_keyq_read(k2h_keyq_h keyqhandle, unsigned char** ppdata, size_t* pdatalen, int pos);
extern bool k2h_keyq_read_keyval(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen, int pos);
extern bool k2h_keyq_str_read(k2h_keyq_h keyqhandle, char** ppdata, int pos);
extern bool k2h_keyq_str_read_keyval(k2h_keyq_h keyqhandle, char** ppkey, char** ppval, int pos);
extern bool k2h_keyq_push(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen);
extern bool k2h_keyq_push_keyval(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen, const unsigned char* byval, size_t vallen);
extern bool k2h_keyq_str_push(k2h_keyq_h keyqhandle, const char* pkey);
extern bool k2h_keyq_str_push_keyval(k2h_keyq_h keyqhandle, const char* pkey, const char* pval);
extern bool k2h_keyq_pop(k2h_keyq_h keyqhandle, unsigned char** ppval, size_t* pvallen);
extern bool k2h_keyq_pop_keyval(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen);
extern bool k2h_keyq_str_pop(k2h_keyq_h keyqhandle, char** ppval);
extern bool k2h_keyq_str_pop_keyval(k2h_keyq_h keyqhandle, char** ppkey, char** ppval);
extern bool k2h_keyq_remove(k2h_keyq_h keyqhandle, int count);
extern int k2h_keyq_remove_ext(k2h_keyq_h keyqhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata);
extern bool k2h_keyq_read_wp(k2h_keyq_h keyqhandle, unsigned char** ppdata, size_t* pdatalen, int pos, const char* encpass);
extern bool k2h_keyq_read_keyval_wp(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen, int pos, const char* encpass);
extern bool k2h_keyq_str_read_wp(k2h_keyq_h keyqhandle, char** ppdata, int pos, const char* encpass);
extern bool k2h_keyq_str_read_keyval_wp(k2h_keyq_h keyqhandle, char** ppkey, char** ppval, int pos, const char* encpass);
extern bool k2h_keyq_push_wa(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen, const char* encpass, const time_t* expire);
extern bool k2h_keyq_push_keyval_wa(k2h_keyq_h keyqhandle, const unsigned char* bykey, size_t keylen, const unsigned char* byval, size_t vallen, const char* encpass, const time_t* expire);
extern bool k2h_keyq_str_push_wa(k2h_keyq_h keyqhandle, const char* pkey, const char* encpass, const time_t* expire);
extern bool k2h_keyq_str_push_keyval_wa(k2h_keyq_h keyqhandle, const char* pkey, const char* pval, const char* encpass, const time_t* expire);
extern bool k2h_keyq_pop_wp(k2h_keyq_h keyqhandle, unsigned char** ppval, size_t* pvallen, const char* encpass);
extern bool k2h_keyq_pop_keyval_wp(k2h_keyq_h keyqhandle, unsigned char** ppkey, size_t* pkeylen, unsigned char** ppval, size_t* pvallen, const char* encpass);
extern bool k2h_keyq_str_pop_wp(k2h_keyq_h keyqhandle, char** ppval, const char* encpass);
extern bool k2h_keyq_str_pop_keyval_wp(k2h_keyq_h keyqhandle, char** ppkey, char** ppval, const char* encpass);
extern bool k2h_keyq_remove_wp(k2h_keyq_h keyqhandle, int count, const char* encpass);
extern int k2h_keyq_remove_wp_ext(k2h_keyq_h keyqhandle, int count, k2h_q_remove_trial_callback fp, void* pextdata, const char* encpass);
extern bool k2h_keyq_dump(k2h_keyq_h keyqhandle, FILE* stream);

// [dump]
//
// k2h_dump_head			dump head infomation for k2hash file(memory)
// k2h_dump_keytable		dump key hash table for k2hash file(memory)
// k2h_dump_full_keytable	dump full key hash table for k2hash file(memory)
// k2h_dump_elementtable	dump elements for k2hash file(memory)
// k2h_dump_full			dump full pages for k2hash file(memory)
// k2h_print_state			print state for k2hash file(memory)
// k2h_print_version		print k2hash library version
// k2h_get_state			get state for k2hash file(memory)
//
extern bool k2h_dump_head(k2h_h handle, FILE* stream);
extern bool k2h_dump_keytable(k2h_h handle, FILE* stream);
extern bool k2h_dump_full_keytable(k2h_h handle, FILE* stream);
extern bool k2h_dump_elementtable(k2h_h handle, FILE* stream);
extern bool k2h_dump_full(k2h_h handle, FILE* stream);
extern bool k2h_print_state(k2h_h handle, FILE* stream);
extern void k2h_print_version(FILE* stream);
extern PK2HSTATE k2h_get_state(k2h_h handle);

// [utility]
//
// free_k2hbin				free PK2HBIN structure
// free_k2hbin				free PK2HBIN structure array
//
extern void free_k2hbin(PK2HBIN pk2hbin);
extern void free_k2hbins(PK2HBIN pk2hbin, size_t count);

#if defined(__cplusplus)
}
#endif	// __cplusplus

#endif	// K2HASH_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
