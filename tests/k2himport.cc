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
 * CREATE:   Thu Jan 26 2015
 * REVISION:
 *
 */

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <k2hash.h>
#include <k2hshm.h>
using namespace std ;

#define MAX 10

#define MODE_TSV	0
#define MODE_MDBM	1

// -----------------------------------------
// Processing for command line
// -----------------------------------------
void usage()
{
    cout << "\nusage: \n" << endl ;
    cout << "k2himport inputfile outputfile [-mdbm]" << endl ;
    cout << "k2himport -h\n" << endl ;
	exit(EXIT_FAILURE) ;
}

bool isExistOption(int argc, const char **argv, const string& target)
{
	bool answer = false ;
	for (int i = 1 ; i < argc; i++)
	{
		string tmp = argv[i] ;
		if (tmp == target) {
			answer = true ;
			break ;
		}
	}
	return answer ;
}

#define MINPARAMATERS	3
int CheckParamater(int argc, const char **argv)
{

	int Answer = MODE_TSV ;
	if (argc < MINPARAMATERS) usage() ;

	if (isExistOption(argc , argv , string("-mdbm")))	Answer = MODE_MDBM ;
	return Answer ;
}

// -----------------------------------------
// Converting TSV mode
// -----------------------------------------
//
int ConvertfromTsv(ifstream *ifs, K2HShm *k2hash)
{
	int readlines = 0 ;
	string readkey ;
	string readvalue ;

	// processing by each line
	while (getline(*ifs, readkey, '\t')) {	// read till tab
		if (ifs->eof()) break ;
		getline(*ifs, readvalue) ;			// read from tab to eol
		k2hash->Set(readkey.c_str(), readvalue.c_str()) ;
		readlines++ ;
	}

	return readlines ;
}

// -----------------------------------------
// Converting mdbm_export mode
// -----------------------------------------
//
int ConvertfromMdbm(ifstream *ifs, K2HShm *k2hash)
{
	int readlines = 0 ;
	string mdbmheader[5] ;
	string readkey ;
	string readvalue ;

	// skip head of file, so mdbm_exports puts five line for head.
	for (int i = 0 ; i < 5; i ++) getline(*ifs, mdbmheader[i]) ;
	if (mdbmheader[4] != "HEADER=END") { // check line at 5
		cout << "error: not a mdbm file." << endl ;
		exit(EXIT_FAILURE) ;
	}

	// loop for key and value by each line after skip head
	while (getline(*ifs, readkey)) {
		getline(*ifs, readvalue) ;
		k2hash->Set(readkey.c_str(), readvalue.c_str()) ;
		readlines++ ;
	}

	return readlines ;
}

// -----------------------------------------
// Main
// -----------------------------------------
int main(int argc, const char **argv)
{
	int convertmode = CheckParamater(argc, argv) ;
	string modename ;
	string inpfile = argv[1] ;
	string outfile = argv[2] ;

	// import base file
	ifstream ifs(inpfile.c_str()) ;
	if (ifs.fail()) {
		cout << "error: can't open inputfile" << endl ;
		exit(EXIT_FAILURE);
	}

	// attach k2hash file
	K2HShm    k2hash;
	if(!k2hash.Attach(outfile.c_str(), false, true, false, true, 10, 8, 512, 128)){
		cout << "error: can't attach k2file" << endl ;
		exit(EXIT_FAILURE);
	}

	// converting
	int readlines = 0 ;
	switch(convertmode)
	{
		case MODE_TSV:
			readlines = ConvertfromTsv(&ifs, &k2hash) ;
			modename = "(tsv)" ;
			break ;
		case MODE_MDBM:
			readlines = ConvertfromMdbm(&ifs, &k2hash) ;
			modename = "(mdbm)" ;
			break ;
	}

	// Post-processing
	ifs.close() ;
	k2hash.Detach() ;
	cout << readlines << " items import success" << modename << endl ;
	return EXIT_SUCCESS ;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
