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
 * CREATE:   Tue Apr 1 2014
 * REVISION:
 *
 */
#ifndef	K2HSTREAM_H
#define	K2HSTREAM_H

#include <iostream>

#include "k2hshm.h"
#include "k2hdaccess.h"
#include "k2hstructure.h"
#include "k2hutil.h"
#include "k2hdbg.h"

//---------------------------------------------------------
// Template basic_k2hstreambuf
//---------------------------------------------------------
template<typename CharT, typename Traits>
class basic_k2hstreambuf : public std::basic_streambuf<CharT, Traits>
{
	public:
		typedef CharT								char_type;
		typedef Traits								traits_type;
		typedef typename traits_type::int_type		int_type;
		typedef typename traits_type::pos_type		pos_type;
		typedef typename traits_type::off_type		off_type;
		typedef std::ios_base::openmode				open_mode;
		typedef std::basic_streambuf<CharT, Traits>	streambuf_type;

	private:
		K2HDAccess*		pAccess;
		size_t			BuffSize;
		unsigned char*	pInputBuff;
		unsigned char*	pOutputBuff;
		off_t			InputBase;		// input buffer base offset from start of value data
		off_t			OutputBase;		// output buffer base offset from start of value data

	protected:
		virtual int sync(void);
		virtual int_type overflow(int_type ch = traits_type::eof());
		virtual int_type pbackfail(int_type ch = traits_type::eof());
		virtual int_type underflow(void);

		virtual pos_type seekoff(off_type offset, std::ios_base::seekdir bpostype, open_mode opmode = std::ios_base::in | std::ios_base::out);
		virtual pos_type seekpos(pos_type abspos, open_mode opmode = std::ios_base::in | std::ios_base::out);

	private:
		bool output_sync(bool check_input);
		bool input_sync(off_t offset);

	public:
		basic_k2hstreambuf(open_mode opmode = std::ios_base::in | std::ios_base::out) : pAccess(NULL), BuffSize(0UL), pInputBuff(NULL), pOutputBuff(NULL), InputBase(0L), OutputBase(0L) { }
		basic_k2hstreambuf(K2HShm* pk2hshm, const char* pkey, open_mode opmode = std::ios_base::in | std::ios_base::out);
		virtual ~basic_k2hstreambuf();

		bool reset(void);
		bool init(K2HShm* pk2hshm, const char* pkey, open_mode opmode = std::ios_base::in | std::ios_base::out);
};

//---------------------------------------------------------
// template cc
//---------------------------------------------------------
template<typename CharT, typename Traits>
basic_k2hstreambuf<CharT, Traits>::basic_k2hstreambuf(K2HShm* pk2hshm, const char* pkey, open_mode opmode) : pAccess(NULL), BuffSize(0UL), pInputBuff(NULL), pOutputBuff(NULL), InputBase(0L), OutputBase(0L)
{
	if(!pk2hshm || !pkey){
		ERR_K2HPRN("Parameters are wrong.");
		return;
	}
	if(!init(pk2hshm, pkey, opmode)){
		WAN_K2HPRN("Parameters are something wrong");
		return;
	}
}

template<typename CharT, typename Traits>
basic_k2hstreambuf<CharT, Traits>::~basic_k2hstreambuf(void)
{
	reset();
}

template<typename CharT, typename Traits>
bool basic_k2hstreambuf<CharT, Traits>::reset(void)
{
	// If there is no-flushed write buffer, flush it here.
	//
	if(pAccess && streambuf_type::pbase() < streambuf_type::pptr()){
		if(!output_sync(false)){
			WAN_K2HPRN("Could not put left write buffer into.");
		}
	}

	K2H_Delete(pAccess);
	K2H_Free(pInputBuff);
	K2H_Free(pOutputBuff);
	BuffSize	= 0UL;
	InputBase	= 0L;
	OutputBase	= 0L;

	return true;
}

template<typename CharT, typename Traits>
bool basic_k2hstreambuf<CharT, Traits>::init(K2HShm* pk2hshm, const char* pkey, open_mode opmode)
{
	reset();

	if(!pk2hshm || !pkey){
		return true;
	}

	// mode
	K2HDAccess::ACSMODE	access_mode = K2HDAccess::READ_ACCESS;
	if((opmode & (std::ios_base::in | std::ios_base::out)) == (std::ios_base::in | std::ios_base::out)){
		access_mode = K2HDAccess::RW_ACCESS;
	}else if(opmode & std::ios_base::out){
		access_mode = K2HDAccess::WRITE_ACCESS;
	}else if(opmode & std::ios_base::in){
		access_mode = K2HDAccess::READ_ACCESS;
	}else{
		ERR_K2HPRN("Parameter open_mode is unknown.");
	}

	// Get & Initialize K2HDAccess object
	if(NULL == (pAccess = pk2hshm->GetDAccessObj(pkey, access_mode))){
		ERR_K2HPRN("Could not initialize internal K2HDAccess object.");
		reset();
		return false;
	}

	// Set pagesize = buffer size & offset pointer
	BuffSize = pk2hshm->GetPageSize() - PAGEHEAD_SIZE;

	// Make buffer
	if(opmode & std::ios_base::out){
		if(NULL == (pOutputBuff = reinterpret_cast<unsigned char*>(malloc(BuffSize)))){
			ERR_K2HPRN("Could not allocate memory.");
			reset();
			return false;
		}
		streambuf_type::setp(reinterpret_cast<char_type*>(pOutputBuff), reinterpret_cast<char_type*>(reinterpret_cast<off_t>(pOutputBuff) + static_cast<off_t>(BuffSize)));
	}
	if(opmode & std::ios_base::in){
		// loading initial data
		if(!input_sync(0L)){
			ERR_K2HPRN("Could not allocate memory.");
			reset();
			return false;
		}
	}
	return true;
}

template<typename CharT, typename Traits>
bool basic_k2hstreambuf<CharT, Traits>::output_sync(bool check_input)
{
	if(!pAccess){
		ERR_K2HPRN("This object did not initialized.");
		return false;
	}

	// get size & offset in output buffer
	size_t	length		= static_cast<size_t>(streambuf_type::pptr() - streambuf_type::pbase());
	off_t	wbuffoffset	= reinterpret_cast<off_t>(streambuf_type::pbase()) - reinterpret_cast<off_t>(pOutputBuff);
	if(0UL == length){
		return true;
	}

	// set offset
	if(pAccess->GetWriteOffset() != (OutputBase + wbuffoffset)){
		if(!pAccess->SetWriteOffset(OutputBase + wbuffoffset)){
			ERR_K2HPRN("Could not set offset for writing.");
			return false;
		}
	}

	// writing
	if(!pAccess->Write(&pOutputBuff[wbuffoffset], length)){
		ERR_K2HPRN("Failed writing.");
		return false;
	}

	// check input buffer area
	if(check_input && streambuf_type::eback() != streambuf_type::egptr()){
		// If input buffer's start and end pointer is same, it means no-cached data.
		// So that case does not need reset input buffer.
		//
		if(!((OutputBase + static_cast<off_t>(BuffSize)) < InputBase || (InputBase + static_cast<off_t>(BuffSize)) < OutputBase)){
			// Input buffer area overlaps with output buffer.
			// So input buffer has cached(loaded) some datas, it is needed to clear. 
			//
			if(!input_sync(InputBase + static_cast<off_t>(streambuf_type::gptr() - streambuf_type::eback()))){
				WAN_K2HPRN("Failed resetting reading buffer and offset.");
			}
		}
	}

	// reset buffer and offset
	OutputBase += wbuffoffset + length;
	streambuf_type::setp(reinterpret_cast<char_type*>(pOutputBuff), reinterpret_cast<char_type*>(reinterpret_cast<off_t>(pOutputBuff) + static_cast<off_t>(BuffSize)));

	return true;
}

template<typename CharT, typename Traits>
int basic_k2hstreambuf<CharT, Traits>::sync(void)
{
	if(!pAccess){
		ERR_K2HPRN("This object did not initialized.");
		return -1;
	}
	if(!streambuf_type::pptr()){
		ERR_K2HPRN("There is no current put pointer.");
		return -1;
	}
	if(!output_sync(true)){
		return -1;
	}
	return 0;
}

template<typename CharT, typename Traits>
typename basic_k2hstreambuf<CharT, Traits>::int_type basic_k2hstreambuf<CharT, Traits>::overflow(int_type ch)
{
	if(!pAccess){
		ERR_K2HPRN("This object did not initialized.");
		return traits_type::eof();
	}

	// flush buffer
	if(!output_sync(true)){
		return traits_type::eof();
	}

	// write ch to new buffer
	*(streambuf_type::pptr()) = ch;
	streambuf_type::pbump(1);

	return traits_type::to_int_type(ch);
}

template<typename CharT, typename Traits>
bool basic_k2hstreambuf<CharT, Traits>::input_sync(off_t offset)
{
	if(!pAccess){
		MSG_K2HPRN("This object did not initialized or could not open key because of not existing.");
		return false;
	}

	// set offset
	if(pAccess->GetReadOffset() != offset){
		if(!pAccess->SetReadOffset(offset)){
			MSG_K2HPRN("Could not set offset for reading.");
			return false;
		}
	}

	// Check EOF
	if(EOF == pAccess->GetReadOffset()){
		K2H_Free(pInputBuff);
		streambuf_type::setg(reinterpret_cast<char_type*>(NULL), reinterpret_cast<char_type*>(NULL), reinterpret_cast<char_type*>(NULL));
		return true;
	}

	// load data
	unsigned char*	pInputTmp = NULL;
	size_t			InputSize = BuffSize;
	if(!pAccess->Read(&pInputTmp, InputSize)){
		ERR_K2HPRN("Could not load data.");
		return false;
	}

	// set offset & pointers
	if(0UL == InputSize || !pInputTmp){
		// EOF
		K2H_Free(pInputTmp);
		K2H_Free(pInputBuff);
		streambuf_type::setg(reinterpret_cast<char_type*>(NULL), reinterpret_cast<char_type*>(NULL), reinterpret_cast<char_type*>(NULL));
	}else{
		K2H_Free(pInputBuff);
		InputBase	= offset;
		pInputBuff	= pInputTmp;
		streambuf_type::setg(reinterpret_cast<char_type*>(pInputBuff), reinterpret_cast<char_type*>(pInputBuff), reinterpret_cast<char_type*>(&pInputBuff[InputSize]));
	}
	return true;
}

template<typename CharT, typename Traits>
typename basic_k2hstreambuf<CharT, Traits>::int_type basic_k2hstreambuf<CharT, Traits>::pbackfail(int_type ch)
{
	if(!pAccess){
		ERR_K2HPRN("This object did not initialized.");
		return traits_type::eof();
	}

	if(streambuf_type::eback() < streambuf_type::gptr()){
		if(!traits_type::eq(traits_type::to_char_type(ch), streambuf_type::gptr()[-1])){
			streambuf_type::gbump(-1);
			return traits_type::eof();
		}
		// decrement current
		streambuf_type::gbump(-1);

	}else if(0L == InputBase){
		// now first position in reading buffer(data)
		return traits_type::eof();

	}else{
		// load a block before now area
		off_t	offset = InputBase < static_cast<off_t>(BuffSize) ? 0L : (InputBase - static_cast<off_t>(BuffSize));
		if(!input_sync(InputBase + offset)){
			return traits_type::eof();
		}
		if(reinterpret_cast<char_type*>(NULL) == streambuf_type::eback() || reinterpret_cast<char_type*>(NULL) == streambuf_type::egptr()){
			return traits_type::eof();
		}

		// reset cur position to last pos
		streambuf_type::setg(streambuf_type::eback(), &(streambuf_type::egptr()[-1]), streambuf_type::egptr());

		// check
		if(!traits_type::eq(traits_type::to_char_type(ch), streambuf_type::gptr()[0])){
			return traits_type::eof();
		}
	}
	return traits_type::to_int_type(ch);
}

template<typename CharT, typename Traits>
typename basic_k2hstreambuf<CharT, Traits>::int_type basic_k2hstreambuf<CharT, Traits>::underflow(void)
{
	off_t	offset = static_cast<off_t>(streambuf_type::egptr() - streambuf_type::eback());
	if(!input_sync(InputBase + offset)){
		return traits_type::eof();
	}
	// Check EOF
	if(reinterpret_cast<char_type*>(NULL) == streambuf_type::eback() || reinterpret_cast<char_type*>(NULL) == streambuf_type::egptr()){
		return traits_type::eof();
	}

	int_type	result = traits_type::to_int_type(streambuf_type::gptr()[0]);

	return result;
}

template<typename CharT, typename Traits>
typename basic_k2hstreambuf<CharT, Traits>::pos_type basic_k2hstreambuf<CharT, Traits>::seekoff(off_type offset, std::ios_base::seekdir bpostype, open_mode opmode)
{
	if(!(opmode & (std::ios_base::out | std::ios_base::in))){
		ERR_K2HPRN("Parameter is wrong");
		return pos_type(off_type(-1));
	}
	if(!pAccess){
		ERR_K2HPRN("This object did not initialized.");
		return pos_type(off_type(-1));
	}

	if(opmode & std::ios_base::out){
		// make offset
		off_t	new_offset = 0L;
		if(std::ios_base::beg == bpostype){
			new_offset = OutputBase + offset;
		}else if(std::ios_base::end == bpostype){
			new_offset = static_cast<off_t>(streambuf_type::epptr() - streambuf_type::pbase()) + OutputBase + offset;
		}else if(std::ios_base::cur == bpostype){
			new_offset = static_cast<off_t>(streambuf_type::pptr() - streambuf_type::pbase()) + OutputBase + offset;
		}

		// put existing buffers
		// (If opmode has std::ios_base::in, do not sync input buffer on any case.)
		//
		if(!output_sync((opmode & std::ios_base::in) ? false : true)){
			ERR_K2HPRN("Could not put data.");
			return pos_type(off_type(-1));
		}

		// set buffers
		if(!pAccess->SetWriteOffset(new_offset)){
			ERR_K2HPRN("Could not set offset for writing.");
			return pos_type(off_type(-1));
		}
		OutputBase = new_offset;
		streambuf_type::setp(reinterpret_cast<char_type*>(pOutputBuff), reinterpret_cast<char_type*>(reinterpret_cast<off_t>(pOutputBuff) + static_cast<off_t>(BuffSize)));

	}

	if(opmode & std::ios_base::in){
		// make offset
		off_t	new_offset = 0L;
		if(std::ios_base::beg == bpostype){
			new_offset = InputBase + offset;
		}else if(std::ios_base::end == bpostype){
			new_offset = static_cast<off_t>(streambuf_type::egptr() - streambuf_type::eback()) + InputBase + offset;
		}else if(std::ios_base::cur == bpostype){
			new_offset = static_cast<off_t>(streambuf_type::gptr() - streambuf_type::eback()) + InputBase + offset;
		}

		if(!input_sync(new_offset)){
			ERR_K2HPRN("Could not put data.");
			return pos_type(off_type(-1));
		}
		// Check EOF
		if(reinterpret_cast<char_type*>(NULL) == streambuf_type::eback() || reinterpret_cast<char_type*>(NULL) == streambuf_type::egptr()){
			return pos_type(off_type(-1));
		}
	}
	return pos_type(off_type(0));
}

template<typename CharT, typename Traits>
typename basic_k2hstreambuf<CharT, Traits>::pos_type basic_k2hstreambuf<CharT, Traits>::seekpos(pos_type abspos, open_mode opmode)
{
	return seekoff(off_type(abspos), std::ios_base::beg, opmode);
}

//---------------------------------------------------------
// Template basic_ik2hstream
//---------------------------------------------------------
template<typename CharT, typename Traits>
class basic_ik2hstream : public std::basic_istream<CharT, Traits>
{
	public:
		typedef CharT										char_type;
		typedef Traits										traits_type;
		typedef typename traits_type::int_type				int_type;
		typedef typename traits_type::pos_type				pos_type;
		typedef typename traits_type::off_type				off_type;
		typedef std::ios_base::openmode						open_mode;
		typedef basic_k2hstreambuf<CharT, Traits>			k2hstreambuf_type;
		typedef std::basic_istream<char_type, traits_type>	istream_type;
		typedef std::basic_ios<char_type, traits_type>		ios_type;

	private:
		k2hstreambuf_type	k2hstreambuf;

	protected:
		basic_ik2hstream(open_mode opmode = std::ios_base::in) : istream_type(), k2hstreambuf(opmode | std::ios_base::in) { ios_type::init(&k2hstreambuf); }

	public:
		basic_ik2hstream(K2HShm* pk2hshm, const char* pkey, open_mode opmode = std::ios_base::in) : istream_type(), k2hstreambuf(pk2hshm, pkey, opmode | std::ios_base::in) { ios_type::init(&k2hstreambuf); }
		~basic_ik2hstream() { }

		k2hstreambuf_type* rdbuf() const { return const_cast<k2hstreambuf_type*>(&k2hstreambuf); }
};


//---------------------------------------------------------
// Template basic_ok2hstream
//---------------------------------------------------------
template <typename CharT, typename Traits>
class basic_ok2hstream : public std::basic_ostream<CharT, Traits>
{
	public:
		typedef CharT										char_type;
		typedef Traits										traits_type;
		typedef typename traits_type::int_type				int_type;
		typedef typename traits_type::pos_type				pos_type;
		typedef typename traits_type::off_type				off_type;
		typedef std::ios_base::openmode						open_mode;
		typedef basic_k2hstreambuf<CharT, Traits>			k2hstreambuf_type;
		typedef std::basic_ostream<char_type, traits_type>	ostream_type;
		typedef std::basic_ios<char_type, traits_type>		ios_type;

	private:
		k2hstreambuf_type	k2hstreambuf;

	protected:
		basic_ok2hstream(open_mode opmode = std::ios_base::out) : ostream_type(), k2hstreambuf(opmode | std::ios_base::out) { ios_type::init(&k2hstreambuf); }

	public:
		basic_ok2hstream(K2HShm* pk2hshm, const char* pkey, open_mode opmode = std::ios_base::out) : ostream_type(), k2hstreambuf(pk2hshm, pkey, opmode | std::ios_base::out) { ios_type::init(&k2hstreambuf); }
		~basic_ok2hstream() { }

		k2hstreambuf_type* rdbuf() const { return const_cast<k2hstreambuf_type*>(&k2hstreambuf); }
};


//---------------------------------------------------------
// Template basic_k2hstream
//---------------------------------------------------------
template <typename CharT, typename Traits>
class basic_k2hstream : public std::basic_iostream<CharT, Traits>
{
	public:
		typedef CharT										char_type;
		typedef Traits										traits_type;
		typedef typename traits_type::int_type				int_type;
		typedef typename traits_type::pos_type				pos_type;
		typedef typename traits_type::off_type				off_type;
		typedef std::ios_base::openmode						open_mode;
		typedef basic_k2hstreambuf<CharT, Traits>			k2hstreambuf_type;
		typedef std::basic_iostream<char_type, traits_type>	iostream_type;
		typedef std::basic_ios<char_type, traits_type>		ios_type;

	private:
		k2hstreambuf_type	k2hstreambuf;

	protected:
		basic_k2hstream(open_mode opmode = std::ios_base::out | std::ios_base::in) : iostream_type(), k2hstreambuf(opmode | std::ios_base::out | std::ios_base::in) { ios_type::init(&k2hstreambuf); }

	public:
		basic_k2hstream(K2HShm* pk2hshm, const char* pkey, open_mode opmode = std::ios_base::out | std::ios_base::in) : iostream_type(), k2hstreambuf(pk2hshm, pkey, opmode | std::ios_base::out | std::ios_base::in) { ios_type::init(&k2hstreambuf); }
		~basic_k2hstream() { }

		k2hstreambuf_type* rdbuf() const { return const_cast<k2hstreambuf_type*>(&k2hstreambuf); }
};

//---------------------------------------------------------
// Typedefs
//---------------------------------------------------------
template<typename CharT, typename Traits = std::char_traits<CharT> >		class basic_k2hstreambuf;
template<typename CharT, typename Traits = std::char_traits<CharT> >		class basic_ik2hstream;
template<typename CharT, typename Traits = std::char_traits<CharT> >		class basic_ok2hstream;
template<typename CharT, typename Traits = std::char_traits<CharT> >		class basic_k2hstream;

typedef basic_k2hstreambuf<char>		k2hstreambuf;
typedef basic_ik2hstream<char>			ik2hstream;
typedef basic_ok2hstream<char>			ok2hstream;
typedef basic_k2hstream<char>			k2hstream;

#ifdef	_GLIBCXX_USE_WCHAR_T
typedef basic_k2hstreambuf<wchar_t>		wk2hstreambuf;
typedef basic_ik2hstream<wchar_t>		wik2hstream;
typedef basic_ok2hstream<wchar_t>		wok2hstream;
typedef basic_k2hstream<wchar_t>		wk2hstream;
#endif	// _GLIBCXX_USE_WCHAR_T

#endif	// K2HSTREAM_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
