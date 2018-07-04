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
 * CREATE:   Tue Feb 3 2015
 * REVISION:
 *
 */
#ifndef	K2HQUEUE_H
#define	K2HQUEUE_H

#include "k2hattropsman.h"

class K2HShm;

//---------------------------------------------------------
// K2HQueue Class
//---------------------------------------------------------
class K2HQueue
{
	protected:
		static const unsigned char	builtin_pref[];

		K2HShm*						pK2HShm;
		bool						isFIFO;
		unsigned char*				prefix;
		size_t						prefix_len;
		unsigned char*				marker;
		size_t						marker_len;
		pid_t						tid;			// uses for building uniq key name
		K2hAttrOpsMan::ATTRINITTYPE	attrtype;

	protected:
		static unsigned char* GetMarkerName(const unsigned char* pref, size_t preflen, size_t& markerlength);
		static unsigned char* GetUniqKeyName(const unsigned char* pref, size_t preflen, pid_t tid, size_t& keylength);

		bool IsInit(void) const { return (NULL != pK2HShm); }
		bool IsSafe(void) const { return (NULL != pK2HShm && NULL != prefix && NULL != marker); }
		bool Clear(void);

		unsigned char* GetUniqKey(size_t& keylen);

	public:
		K2HQueue(K2HShm* pk2h = NULL, bool is_fifo = true, const unsigned char* pref = NULL, size_t preflen = 0L, K2hAttrOpsMan::ATTRINITTYPE type = K2hAttrOpsMan::OPSMAN_MASK_QUEUEKEY);
		virtual ~K2HQueue();

		bool Init(K2HShm* pk2h, bool is_fifo, const unsigned char* pref = NULL, size_t preflen = 0L, K2hAttrOpsMan::ATTRINITTYPE type = K2hAttrOpsMan::OPSMAN_MASK_QUEUEKEY);
		bool Init(const unsigned char* pref = NULL, size_t preflen = 0L, K2hAttrOpsMan::ATTRINITTYPE type = K2hAttrOpsMan::OPSMAN_MASK_QUEUEKEY);

		virtual bool IsEmpty(void) const;
		virtual int GetCount(void) const;
		virtual bool Read(unsigned char** ppdata, size_t& datalen, int pos = 0, const char* encpass = NULL) const;
		virtual bool Push(const unsigned char* bydata, size_t datalen, K2HAttrs* pAttrs = NULL, const char* encpass = NULL, const time_t* expire = NULL);
		virtual bool Pop(unsigned char** ppdata, size_t& datalen, K2HAttrs** ppAttrs = NULL, const char* encpass = NULL);
		virtual int Remove(int count, k2h_q_remove_trial_callback fp = NULL, void* pExtData = NULL, const char* encpass = NULL);
		virtual bool Dump(FILE* stream);
};

//---------------------------------------------------------
// K2HKeyQueue Class
//---------------------------------------------------------
// This class saves key-name into Queue, and writes key and value into k2hash.
// The difference between K2HQueue and this class is that this class writes key
// and value at the same time. When this class pops key-name and it's value(or
// only value) from Queue, removes key and value from k2hash.
// 
// Read(unsigned char** ppkey, size_t& keylen, unsigned char** ppval, size_t& vallen, int pos)
// 	This method returns the key and value in Queue, but not remove the key.
// 
// Read(unsigned char** ppdata, size_t& datalen, int pos)
//	This virtual method returns only value which is pointed queued key-name in Queue, but not remove the key.
// 
// Push(const unsigned char* bydata, size_t datalen)
// 	This virtual method does not write any key and value into k2hash, but saves bydata as key-name into Queue.
// 	(K2HQueue class defined this method)
// 
// Push(const unsigned char* pkey, size_t keylen, const unsigned char* byval, size_t vallen)
// 	This method writes the key and value into k2hash, and saves that key as key-name into Queue.
// 
// Pop(unsigned char** ppkey, size_t& keylen, unsigned char** ppval, size_t& vallen)
// 	This method returns the key and value in Queue, and removes that key from k2hash.
// 
// Pop(unsigned char** ppdata, size_t& datalen)
// 	This virtual method returns only value which is pointed queued key-name in Queue, and remove that key from k2hash.
// 
// Remove(int count)
// 	Removes the key names from Queue, and removes those key from k2hash.
// 
// [NOTICE]
// The base class's method K2HQueue::Push has "bydata" parameter, which means "key-name"
// in this class. This value(byval) is stored into Queue.
// 
class K2HKeyQueue : public K2HQueue
{
	public:
		K2HKeyQueue(K2HShm* pk2h = NULL, bool is_fifo = true, const unsigned char* pref = NULL, size_t preflen = 0L) : K2HQueue(pk2h, is_fifo, pref, preflen, K2hAttrOpsMan::OPSMAN_MASK_KEYQUEUEKEY) {}
		virtual ~K2HKeyQueue() {}

		using K2HQueue::Push;

		bool Read(unsigned char** ppkey, size_t& keylen, unsigned char** ppval, size_t& vallen, int pos = 0, const char* encpass = NULL) const;
		virtual bool Read(unsigned char** ppdata, size_t& datalen, int pos = 0, const char* encpass = NULL) const;
		bool Push(const unsigned char* pkey, size_t keylen, const unsigned char* byval, size_t vallen, const char* encpass = NULL, const time_t* expire = NULL);
		virtual bool Push(const unsigned char* bydata, size_t datalen, K2HAttrs* pAttrs = NULL, const char* encpass = NULL, const time_t* expire = NULL);
		bool Pop(unsigned char** ppkey, size_t& keylen, unsigned char** ppval, size_t& vallen, const char* encpass = NULL);
		virtual bool Pop(unsigned char** ppdata, size_t& datalen, K2HAttrs** ppAttrs = NULL, const char* encpass = NULL);
		virtual int Remove(int count, k2h_q_remove_trial_callback fp = NULL, void* pExtData = NULL, const char* encpass = NULL);
};

//---------------------------------------------------------
// K2HLowOpsQueue Class
//---------------------------------------------------------
// This class opelates only queue key names in queue marker, and supports
// those operation manually.
// This class operates no queue key which is listed in queue list too.
// 
// If you use this class, you must set/create/remove queue key before
// modifying queue marker. So that You can use this class for only
// modifying/reading queue marker.
// 
// We make this class for distributed k2hash claster, because the queue
// marker and queue keys is not same k2hash file(machine) on distributed
// k2hash environment.
// 
class K2HLowOpsQueue : public K2HQueue
{
	protected:
		virtual bool Read(unsigned char** ppdata, size_t& datalen, int pos = 0, const char* encpass = NULL) const;
		virtual bool Push(const unsigned char* bydata, size_t datalen, K2HAttrs* pAttrs, const char* encpass, const time_t* expire);

		bool ReplaceTopQueueKey(const unsigned char* preplacekey, size_t replacekeylen);
		unsigned char* GetEdgeQueueKey(size_t& keylen, bool is_top) const;

	public:
		K2HLowOpsQueue(K2HShm* pk2h = NULL, bool is_fifo = true, const unsigned char* pref = NULL, size_t preflen = 0L) : K2HQueue(pk2h, is_fifo, pref, preflen) {}
		virtual ~K2HLowOpsQueue() {}

		//
		// Push/Pop
		//
		// Push							: push new queue key name into queue, decide FIFO/LIFO by this method
		// Pop							: pop one queue key name from queue, and replace new queue key in marker which is planned next popping, decide FIFO/LIFO by this method
		//
		bool Push(const unsigned char* bykey, size_t keylen) { return Push(bykey, keylen, NULL, NULL, NULL); }
		bool Pop(const unsigned char* preplacekey, size_t replacekeylen) { return ReplaceTopQueueKey(preplacekey, replacekeylen); }

		//
		// get queue key names methods
		// 
		// GetTopQueueKey				: get top key name of queue list, for inserting LIFO. this key name is set into new queued key' subkey list before inserting it to queue.
		// GetBottomQueueKey			: get bottom key name of queue list, for inserting FIFO. new queued key name is set into this key's subkey list before inserting it to queue.
		// GetPlannedPopQueueKey		: get queued key name which is planned next popping from the queue.
		// 
		unsigned char* GetTopQueueKey(size_t& keylen) const { return GetEdgeQueueKey(keylen, true); }
		unsigned char* GetBottomQueueKey(size_t& keylen) const { return GetEdgeQueueKey(keylen, false); }
		unsigned char* GetPlannedPopQueueKey(size_t& keylen) const;

		//
		// utilities
		//
		// K2HQueue::GetMarkerName		: get marker name. this class method is inherited from base class.
		// K2HQueue::GetUniqKey			: get uniq queue key name which can be used new queue key name. this method is inherited from base class.
		//
		static unsigned char* GetMarkerName(const unsigned char* pref, size_t preflen, size_t& markerlength) { return K2HQueue::GetMarkerName(pref, preflen, markerlength); }
		unsigned char* GetUniqKey(size_t& keylen) { return K2HQueue::GetUniqKey(keylen); }
};

#endif	// K2HQUEUE_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
