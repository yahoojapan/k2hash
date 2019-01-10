/*
 * K2HASH
 *
 * Copyright 2015 Yahoo Japan Corporation.
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
 * AUTHOR:   Tetsuya Mochizuki
 * CREATE:   Thu Jan 15 2015
 * REVISION:
 *
 */

// -------------------------------------------------------------------
//
// k2hash touch tool
//
// -------------------------------------------------------------------
// #define DEBUG

#include <iostream>
#include <string>
#include <algorithm>
#include <cstdlib>
using namespace std ;

#include <k2hash.h>
#include <k2hshm.h>

#define K2HTOUCH_VERSION	"1.5.2.20160613"

// -------------------------------------------------------------------
// command line process framework
// -------------------------------------------------------------------

bool usage()
{
	cout << "-----------------------------------" << endl ;
	cout << "k2hash touch the k2file tool usage:" << endl ;
	cout << K2HTOUCH_VERSION << endl ;
	cout << "-----------------------------------" << endl ;
	cout << "k2htouch k2file create" << endl ;
	cout << "k2htouch k2file createmini" << endl ;
	cout << endl ;
	cout << "k2htouch k2file set ‘key’ ‘value’" << endl ;
	cout << "k2htouch k2file get ‘key’" << endl ;
	cout << "k2htouch k2file delete ‘key’" << endl ;
	cout << endl ;
	cout << "k2htouch k2file addsubkey ‘key’ 'subkey' ‘value’" << endl ;
	cout << "k2htouch k2file getsubkey ‘key’" << endl ;
	cout << endl ;
	cout << "k2htouch k2file push ‘queprefix’ 'value' [ -lifo or -fifo(default) ]" << endl ;
	cout << "k2htouch k2file pop ‘queprefix’" << endl ;
	cout << "k2htouch k2file clear ‘queprefix’" << endl ;
	cout << endl ;
	cout << "k2htouch k2file kpush ‘queprefix’ 'key' 'value' [ -lifo or -fifo(default)] " << endl ;
	cout << "k2htouch k2file kpop ‘queprefix’ " << endl ;
	cout << "k2htouch k2file kclear ‘queprefix’" << endl ;
	cout << endl ;
	cout << "k2htouch k2file getattr ‘key’" << endl ;
	cout << "k2htouch k2file list" << endl ;
	cout << "k2htouch k2file info" << endl ;
	cout << endl ;
	cout << "k2htouch k2file dtor    [on -conf dtorconffile | off]" << endl ;
	cout << "k2htouch k2file expire  [on expiretime(sec)    | off]" << endl ;
	cout << "k2htouch k2file mtime   [on | off]" << endl ;
	cout << "k2htouch k2file history [on | off]" << endl ;
	cout << endl ;
	cout << "k2htouch -h" << endl ;
	exit (EXIT_FAILURE) ;
}

// command line parameter
string argFilename, argKey, argSubkey, argValue , argQueprefix , argConffile ;
bool isFIFO ;

// minimum parameters
#define MINPARAMATERS		2

// command mode
#define MODE_HELP			0
#define MODE_SET			1
#define MODE_GET			2
#define MODE_CREATE			3
#define MODE_REMOVE			4
#define MODE_LIST			5
#define MODE_CREATEMINI		6
#define MODE_ADDSUBKEY		7
#define MODE_GETSUBKEY		8
#define MODE_INFO			9
#define MODE_PUSH			11
#define MODE_POP			12
#define MODE_CLEAR			13
#define MODE_KPUSH			14
#define MODE_KPOP			15
#define MODE_KCLEAR			16
#define MODE_DTOR			17
#define MODE_GETATTR		18
#define MODE_MTIME			19
#define MODE_HISTORY		20
#define MODE_EXPIRE			21

// dtor mode
#define K2MODE_DTOR_KEY		"K2HTOUCH_DTORMODE"
#define K2MODE_DTOR_CONF	"K2HTOUCH_DTORCONF"
#define K2MODE_DTOR_ON		"ON"
#define K2MODE_DTOR_OFF		"OFF"
#define	LIBDTOR				"libk2htpdtor.so.1"
bool DTORCOMMAND = false ;

// attr mode
#define K2MODE_ATTR_MTIME	"K2HTOUCH_ATTR_MTIME"
#define K2MODE_ATTR_EXPIRE	"K2HTOUCH_ATTR_EXPIRE"
#define K2MODE_ATTR_HISTORY	"K2HTOUCH_ATTR_HISTORY"
#define K2MODE_ON			"ON"
#define K2MODE_OFF			"OFF"

string get(K2HShm* k2hash, string Key) ;
// -------------------------------------------------------------------
// Check command line parameter
bool isExistOption(int argc, char **argv, string target) 
{
	transform(target.begin(), target.end(), target.begin(), ::tolower) ;
	bool answer = false ;
	for (int i = 1 ; i < argc; i++) {
		string tmp = argv[i] ;
		transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower) ;
		if (tmp == target) {
			answer = true ;
			break ;
		}
	}
	return answer ;
}
// -------------------------------------------------------------------
string GetOption (int argc, char **argv, string target) 
{
	string answer = "" ;

	transform(target.begin(), target.end(), target.begin(), ::tolower) ;
	for (int i = 1 ; i < argc - 1; i++) {
		string tmp = argv[i] ;
		transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower) ;
		if (tmp == target) {
			answer = argv[i + 1] ;
			break ;
		}
	}
	return answer ;
}
// -------------------------------------------------------------------
int CheckParameter(int argc, char **argv)
{
	int Answer = MODE_HELP ;
	if (argc <= MINPARAMATERS) return Answer ; 
	if (isExistOption(argc , argv , "-h"))	return Answer ;

	argFilename = argv[1] ;
	string com = argv[2] ;
	transform(com.begin(), com.end(), com.begin(), ::tolower) ;

	if (com == "set" && argc == 5) { Answer = MODE_SET ; argKey = argv[3] ; argValue = argv[4] ; }
	else if (com == "get" && argc == 4) { Answer = MODE_GET ; argKey = argv[3] ; }
	else if (com == "getattr" && argc == 4) { Answer = MODE_GETATTR ; argKey = argv[3] ; }
	else if (com == "create" && argc == 3) { Answer = MODE_CREATE ; }
	else if (com == "createmini" && argc == 3) { Answer = MODE_CREATEMINI ; }
	else if (com == "delete" && argc == 4) { Answer = MODE_REMOVE ; argKey = argv[3] ; }
	else if (com == "list" && argc == 3) { Answer = MODE_LIST ; }
	else if (com == "addsubkey" && argc == 6)
		 { Answer = MODE_ADDSUBKEY ; argKey = argv[3] ; argSubkey = argv[4] ; argValue = argv[5] ; }
	else if (com == "getsubkey" && argc == 4) { Answer = MODE_GETSUBKEY ; argKey = argv[3] ; }
	else if (com == "info" && argc == 3) { Answer = MODE_INFO ; }
	else if (com == "push" && argc >= 5) { 
		Answer = MODE_PUSH ; 
		if (isExistOption(argc, argv, "-lifo")) isFIFO = false ; else isFIFO = true ; // default = FIFO
		argQueprefix = argv[3] ; 
		argValue = argv[4] ; 
	}
	else if (com == "pop" && argc == 4) { Answer = MODE_POP ; argQueprefix = argv[3] ; }
	else if (com == "clear" && argc == 4) { Answer = MODE_CLEAR ; argQueprefix = argv[3] ; }
	else if (com == "kpush" && argc >= 6) { 
		Answer = MODE_KPUSH ; 
		if (isExistOption(argc, argv, "-lifo")) isFIFO = false ; else isFIFO = true ; // default = FIFO
		argQueprefix = argv[3] ; 
		argKey = argv[4] ; 
		argValue = argv[5] ; 
	} else if (com == "kpop" && argc == 4) { Answer = MODE_KPOP ; argQueprefix = argv[3] ; }
	else if (com == "kclear" && argc == 4) { Answer = MODE_KCLEAR ; argQueprefix = argv[3] ; }
	else if (com == "dtor" && argc >= 3) { 
		Answer = MODE_DTOR ; 
		DTORCOMMAND = true ;
		argKey   = K2MODE_DTOR_KEY ;
		argValue = "" ;
		if (isExistOption(argc, argv, "off")) argValue = K2MODE_DTOR_OFF ;
		if (isExistOption(argc, argv, "on" )) argValue = K2MODE_DTOR_ON ;
		argConffile =  GetOption(argc, argv, "-conf") ;
	}
	else if (com == "mtime" && argc >= 3) { 
		Answer = MODE_MTIME ; 
		argKey   = K2MODE_ATTR_MTIME ;
		argValue = "" ;
		if (isExistOption(argc, argv, "off")) argValue = K2MODE_OFF ;
		if (isExistOption(argc, argv, "on" )) argValue = K2MODE_ON ;
	}
	else if (com == "history" && argc >= 3) { 
		Answer = MODE_HISTORY ; 
		argKey   = K2MODE_ATTR_HISTORY ;
		argValue = "" ;
		if (isExistOption(argc, argv, "off")) argValue = K2MODE_OFF ;
		if (isExistOption(argc, argv, "on" )) argValue = K2MODE_ON ;
	}
	else if (com == "expire" && argc >= 3) { 
		Answer = MODE_EXPIRE ; 
		argKey   = K2MODE_ATTR_EXPIRE ;
		argValue = "" ;
		if (argc >= 4) argValue = argv[3] ;
		if (isExistOption(argc, argv, "off")) argValue = "0" ;
	}

	return Answer ;
}

// -------------------------------------------------------------------
// k2htouch tool 
// -------------------------------------------------------------------

// default attach parameter
#define ISREADONLY	false
#define ISCREATE	true
#define ISTEMPFILE	false
#define ISFULLMAPPING	true
#define MASK_BITCNT	10
#define CMASK_BITCNT	8
#define MAX_ELEMENT_CNT	512
#define PAGESIZE	128

// minisize attach parameter
#define SMALL_MASK_BITCNT	2
#define SMALL_CMASK_BITCNT	4
#define SMALL_MAX_ELEMENT_CNT	2

// -------------------------------------------------------------------
// attach the k2hash 
// -------------------------------------------------------------------
bool IsDTOR(K2HShm* k2hash)
{
	bool answer = false ;
	if (DTORCOMMAND) return false ;

	char* pValue = k2hash->Get(K2MODE_DTOR_KEY);
	string tmp = "" ;
	if(pValue) tmp = pValue ;
	if (tmp == K2MODE_DTOR_ON ) answer = true ;
	
	free(pValue);
	return answer ;
}
// -------------------------------------------------------------------
bool ReadFlag(K2HShm* k2hash, string FlagKey)
{
	bool answer = false ;

	char* pValue = k2hash->Get(FlagKey.c_str());
	string tmp = "" ;
	if(pValue) tmp = pValue ;
	if (tmp == K2MODE_ON ) answer = true ;
	
	free(pValue);
	return answer ;
}
// -------------------------------------------------------------------
bool AttrSet(K2HShm* k2hash)
{
	const strarr_t* pluginlibs = NULL ;
	bool is_mtime     = ReadFlag(k2hash, K2MODE_ATTR_MTIME) ;
	bool is_history   = ReadFlag(k2hash, K2MODE_ATTR_HISTORY) ;

	string expirestr  = get(k2hash, K2MODE_ATTR_EXPIRE) ;
	time_t expiretime = (long)atoi(expirestr.c_str()) ; 
	time_t *argExpire = NULL ;
	if (expiretime) argExpire = &expiretime ;

	const char*	passfile = "" ; 

	return k2hash->SetCommonAttribute(
		&is_mtime, NULL, passfile, &is_history, argExpire, pluginlibs) ;
}

bool AttrClear(K2HShm* k2hash)
{
	return k2hash->CleanCommonAttribute() ;
}
// -------------------------------------------------------------------
bool AttachK2FileMini(K2HShm* k2hash)
{
	bool answer = true ;

	if(!k2hash->Attach(argFilename.c_str(), ISREADONLY, ISCREATE, ISTEMPFILE, ISFULLMAPPING ,
	 SMALL_MASK_BITCNT, SMALL_CMASK_BITCNT, SMALL_MAX_ELEMENT_CNT, PAGESIZE)) {
		cerr << "Attach Failed. k2file is maybe readonly or have not permission." << endl ;
		answer = false ;
	}

	return answer ;
}

// -------------------------------------------------------------------
bool AttachK2File(K2HShm* k2hash)
{
	// Attach
	bool answer = true ;
	if(!k2hash->Attach(argFilename.c_str(), ISREADONLY, ISCREATE, ISTEMPFILE, ISFULLMAPPING ,
	 MASK_BITCNT, CMASK_BITCNT, MAX_ELEMENT_CNT, PAGESIZE)) {
		cerr << "Attach Failed. k2file is maybe readonly or have not permission." << endl ;
		exit(EXIT_FAILURE);
	}
	// read attr setting and set attr
	if (!AttrSet(k2hash)) {
		cerr << "Load and Set attr Failed." << endl ;
		exit(EXIT_FAILURE);
	}

	// dtor set
	if (IsDTOR(k2hash)) {
		// load transaction shared library
		string dtorlib  = LIBDTOR ;
		if(!k2h_load_transaction_library(dtorlib.c_str())){
			cerr << "Failed to load transaction shared library." << endl ;
			k2hash->Detach();
			exit(EXIT_FAILURE);
		}
		// dtor start
		char* pConf  = k2hash->Get(K2MODE_DTOR_CONF);
		string dtorconf = pConf ;
		if (!k2hash->EnableTransaction(NULL, NULL, 0,
		       (const unsigned char *)dtorconf.c_str(), dtorconf.length() + 1))
			cerr << "DTOR Start Failed. Please check your dtorinifile or chmpx process." << endl ;
	}

	return answer ;
}

// -------------------------------------------------------------------
// create database
// -------------------------------------------------------------------
bool create()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;
	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
bool createmini()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2FileMini(&k2hash)) exit(EXIT_FAILURE) ;
	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
// basic command
// -------------------------------------------------------------------
bool remove()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	if(!k2hash.Remove(argKey.c_str(), true)){
			answer = EXIT_FAILURE ;
	}

	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
static char* GetPrintableString(const unsigned char* byData, size_t length)
{
	if(!byData || 0 == length){
		length = 0;
	}
	char*	result;
	if(NULL == (result = reinterpret_cast<char*>(calloc(length + 1, sizeof(char))))){
		cerr << "Could not allocate memory." << endl ;
		return NULL;
	}
	for(size_t pos = 0; pos < length; ++pos){
		result[pos] = isprint(byData[pos]) ? byData[pos] : (0x00 != byData[pos] ? 0xFF : (pos + 1 < length ? ' ' : 0x00));
	}
	return result;
}

// -------------------------------------------------------------------
string GetPrintableDate(const unsigned char* byData, size_t length)
{
	timespec *ts = (timespec *)byData ;
	struct tm *expiretm = localtime(&ts->tv_sec) ;
	char time_str[0xff] ;
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S ", expiretm);

	return time_str ;
}
// -------------------------------------------------------------------
bool getattr()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;
	K2HAttrs* pAttr = k2hash.GetAttrs(argKey.c_str());
	if(pAttr){
		for(K2HAttrs::iterator iter = pAttr->begin(); iter != pAttr->end(); ++iter){
			char*	pattrkey = GetPrintableString(iter->pkey, iter->keylength);
			string tmpkey = pattrkey ;
			if (tmpkey == "mtime" || tmpkey =="expire") {
				string  attrtime = GetPrintableDate(iter->pval, iter->vallength);
				cout << pattrkey << ": " << attrtime << endl ;
			}
			else {
				char*	pattrval = GetPrintableString(iter->pval, iter->vallength);
				cout << pattrkey << ": " << pattrval << endl ;
				K2H_Free(pattrval);
			}
			K2H_Free(pattrkey);
		}
	}
	else {
		// cout << " no attribute " << endl ;
	}
	k2hash.Detach();
	return answer ;
}
// -------------------------------------------------------------------
bool get()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	char* pValue = k2hash.Get(argKey.c_str());
	
	if(pValue){
		cout << pValue ;
		free(pValue);
	}

	k2hash.Detach();
	return answer ;
}

string get(K2HShm* k2hash, string Key)
{
	string answer = "" ;

	char* pValue = k2hash->Get(Key.c_str());
	
	if(pValue){
		answer = pValue ;
		free(pValue);
	}
	return answer ;
}
// -------------------------------------------------------------------
bool set()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	if(!k2hash.Set(
				(const unsigned char*)argKey.c_str(), (size_t)argKey.length() +1 , 
				(const unsigned char*)argValue.c_str(), (size_t)argValue.length() +1
				)){
			answer = EXIT_FAILURE ;
	}

	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
// subkey
// -------------------------------------------------------------------
bool addsubkey()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	if(!k2hash.AddSubkey(argKey.c_str(), argSubkey.c_str(), argValue.c_str())){
			answer = EXIT_FAILURE ;
	}

	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
bool getsubkey()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	K2HSubKeys* mykeys = k2hash.GetSubKeys(argKey.c_str());
	if (mykeys != NULL) {
		strarr_t	strarr;
		mykeys->StringArray(strarr) ;
		for(strarr_t::iterator iter = strarr.begin(); iter != strarr.end(); iter++)
			cout << iter->c_str() << endl ;
	}

	k2hash.Detach();
	delete mykeys ;
	return answer ;
}
// -------------------------------------------------------------------
// utilities
// -------------------------------------------------------------------
bool info()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;
	k2hash.PrintState() ;
	cout << "DTOR      : " << get(&k2hash, K2MODE_DTOR_KEY    ) << endl ;
	cout << "Modifytime: " << get(&k2hash, K2MODE_ATTR_MTIME  ) << endl ;
	cout << "History   : " << get(&k2hash, K2MODE_ATTR_HISTORY) << endl ;
	cout << "ExpireTime: " << get(&k2hash, K2MODE_ATTR_EXPIRE ) << endl ;
	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
bool listall()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;
	for(K2HShm::iterator iter = k2hash.begin(); iter != k2hash.end(); iter++) {
		char* pKey   = k2hash.Get(*iter, K2HShm::PAGEOBJ_KEY);
		char* pValue = k2hash.Get(*iter, K2HShm::PAGEOBJ_VALUE);
		if (pKey && pValue)
			cout << pKey << "\t" << pValue << endl ;
		free(pKey);
		free(pValue);
	}    

	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
// que 
// -------------------------------------------------------------------
bool push()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	K2HQueue myqueue(&k2hash, isFIFO, (const unsigned char*)argQueprefix.c_str(), argQueprefix.length());
	if(!myqueue.Push((const unsigned char*)argValue.c_str(), argValue.length() +1))
		answer = EXIT_FAILURE ;

	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
bool pop()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;
	K2HQueue myqueue(&k2hash, isFIFO, (const unsigned char*)argQueprefix.c_str(), argQueprefix.length());

	unsigned char* pdata   = NULL;
	size_t         datalen = 0;
	string popvalue ;
	bool result = myqueue.Pop(&pdata, datalen) ;
	k2hash.Detach();

	if (!result) {
		cout << "pop false"  << endl ;
    	return EXIT_FAILURE ;
	}

	if (!datalen)  // nodata 
		popvalue = "" ;
	else 
	{
		popvalue = string((const char*)pdata, datalen) ;
		free(pdata);
	}
	cout << popvalue << endl ;

	return answer ;
}

// -------------------------------------------------------------------
bool clear()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	K2HQueue myqueue(&k2hash, isFIFO, (const unsigned char*)argQueprefix.c_str(), argQueprefix.length());
	if(!myqueue.Remove(0xffff))
		answer = EXIT_FAILURE ;

	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
// kque
// -------------------------------------------------------------------
bool kpush()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	const unsigned char *myKey  = (const unsigned char *) argKey.c_str() ;
	const unsigned char *myVal  = (const unsigned char *) argValue.c_str() ;
	int myKeylen = argKey.length() ;
	int myVallen = argValue.length() ;

	K2HKeyQueue myqueue(&k2hash, isFIFO, (unsigned char*)argQueprefix.c_str(), argQueprefix.length());
	if(!myqueue.Push(myKey, myKeylen, myVal, myVallen+1)) {
		cout << "pop false"  << endl ;
		answer = EXIT_FAILURE ;
	}

	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
bool kpop()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	K2HKeyQueue myqueue(&k2hash, isFIFO, (unsigned char*)argQueprefix.c_str(), argQueprefix.length());
	unsigned char* pkey   = NULL;
	size_t         keylen = 0;
	unsigned char* pval   = NULL;
	size_t         vallen = 0;
	string popkey, popvalue ;
	bool result = myqueue.Pop(&pkey, keylen, &pval, vallen) ;
	if(!result || !keylen){ // because error == nodata
		popkey = "" ;
		popvalue = "" ;
	}
	else 
	{
		popkey   = string((const char*)pkey, keylen) ;
		popvalue = string((const char*)pval, vallen) ;
		free(pkey);
		free(pval);
	}
	cout << popkey << "\t" << popvalue << endl ;

	k2hash.Detach();
	return answer ;
}

// -------------------------------------------------------------------
bool kclear()
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;

	K2HKeyQueue myqueue(&k2hash, isFIFO, (unsigned char*)argQueprefix.c_str(), argQueprefix.length());
	if(!myqueue.Remove(0xffff))
		answer = EXIT_FAILURE ;

	k2hash.Detach();
	return answer ;
}
// -------------------------------------------------------------------
bool dtor() // set dtor mode and dtorconffile
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;
	k2hash.DisableTransaction() ;
	AttrClear(&k2hash) ;

	if (argValue == "") // print mode
	{
		char* pMode  = k2hash.Get(K2MODE_DTOR_KEY);
		char* pConf  = k2hash.Get(K2MODE_DTOR_CONF);
		if(pMode) cout << "DTORMode = " << pMode << endl ;
		if(pConf) cout << "DTORConf = " << pConf << endl ;
		free(pMode);
		free(pConf);
		return EXIT_SUCCESS ;
	}

	// on or off
	cout << argFilename << " DTORMode Switch " << argValue << endl ;
	// set dtormode value
	if (!k2hash.Set(argKey.c_str() , argValue.c_str())) answer = EXIT_FAILURE ;
	// set dtorconf value
	if (argConffile != "") 
		if (!k2hash.Set(K2MODE_DTOR_CONF , argConffile.c_str())) answer = EXIT_FAILURE ;

	k2hash.Detach();
	return answer ;
}
// -------------------------------------------------------------------
bool mtime() // set builtin attribute mtime
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;
	k2hash.DisableTransaction() ;
	AttrClear(&k2hash) ;

	if (argValue == "") // print mode
	{
		char* pMode  = k2hash.Get(K2MODE_ATTR_MTIME);
		if(pMode) cout << "mtime = " << pMode << endl ;
		free(pMode);
		return EXIT_SUCCESS ;
	}

	// on or off
	cout << argFilename << " Modifytime Mode:" << argValue << endl ;
	// set dtormode value
	if (!k2hash.Set(argKey.c_str() , argValue.c_str())) answer = EXIT_FAILURE ;

	k2hash.Detach();
	return answer ;
}
// -------------------------------------------------------------------
bool history() // set builtin attribute mtime
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;
	k2hash.DisableTransaction() ;
	AttrClear(&k2hash) ;

	if (argValue == "") // print mode
	{
		char* pMode  = k2hash.Get(K2MODE_ATTR_HISTORY);
		if(pMode) cout << "history = " << pMode << endl ;
		free(pMode);
		return EXIT_SUCCESS ;
	}

	// on or off
	cout << argFilename << " History Mode: " << argValue << endl ;
	// set dtormode value
	if (!k2hash.Set(argKey.c_str() , argValue.c_str())) answer = EXIT_FAILURE ;

	k2hash.Detach();
	return answer ;
}
// -------------------------------------------------------------------
bool expire() // set builtin attribute mtime
{
	bool answer = EXIT_SUCCESS ;
	K2HShm    k2hash;
	if (!AttachK2File(&k2hash)) exit(EXIT_FAILURE) ;
	k2hash.DisableTransaction() ;
	AttrClear(&k2hash) ;

	if (argValue == "") // print mode
	{
		char* pMode  = k2hash.Get(K2MODE_ATTR_EXPIRE);
		if(pMode) cout << "expiretime = " << pMode << endl ;
		free(pMode);
		return EXIT_SUCCESS ;
	}

	// on or off
	cout << argFilename << " Expire Time: " << argValue << endl ;
	// set dtormode value
	if (!k2hash.Set(argKey.c_str() , argValue.c_str())) answer = EXIT_FAILURE ;

	k2hash.Detach();
	return answer ;
}
// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------
int main(int argc, char **argv)
{
	bool answer = EXIT_SUCCESS ;

	int command = CheckParameter(argc, argv) ;
	switch (command) {
		case MODE_CREATE: answer = create() ; break ;
		case MODE_CREATEMINI: answer = createmini() ; break ;

		case MODE_SET: answer = set() ; break ;
		case MODE_GET: answer = get() ; break ;
		case MODE_GETATTR: answer = getattr() ; break ;
		case MODE_REMOVE: answer = remove() ; break ;

		case MODE_ADDSUBKEY: answer = addsubkey() ; break ;
		case MODE_GETSUBKEY: answer = getsubkey() ; break ;

		case MODE_PUSH: answer = push() ; break ;
		case MODE_POP: answer = pop() ; break ;
		case MODE_CLEAR: answer = clear() ; break ;

		case MODE_KPUSH: answer = kpush() ; break ;
		case MODE_KPOP: answer = kpop() ; break ;
		case MODE_KCLEAR: answer = kclear() ; break ;

		case MODE_DTOR: answer = dtor() ; break ;

		case MODE_LIST: answer = listall() ; break ;
		case MODE_INFO: answer = info() ; break ;

		case MODE_MTIME : answer = mtime() ; break ;
		case MODE_HISTORY : answer = history() ; break ;
		case MODE_EXPIRE : answer = expire() ; break ;

		default : answer = usage() ;
		break ;
	}

	if (answer == EXIT_FAILURE ) cerr << "k2htouch command failed. Please try debugmode 'export K2HDBGMODE=ERR'" << endl ;

	exit(answer) ;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
