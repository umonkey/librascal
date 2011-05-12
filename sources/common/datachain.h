// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: datachain.h 3 2006-12-16 00:47:17Z justin.forest $

#ifndef __common_datachain_h
#define __common_datachain_h

class datachainitem {
	friend class datachain;
	friend class packetchain;
	DECLARE_ALLOCATOR;
public:
	// The number of bytes used in this slot.
	unsigned int used;
	// The number of peeked bytes.
	unsigned int dead;
	// The data itself.
	char data[2048];
protected:
	// The next element in chain.
	datachainitem *next;
public:
	DECLARE_ALLOCATORS;
	// Initialization.
	datachainitem();
	~datachainitem();
};


// Data chain maintenance.  Stream oriented version.
class datachain {
protected:
	datachainitem *head, *tail;
	DECLARE_ALLOCATOR;
public:
	datachainitem tmp;
public:
	DECLARE_ALLOCATORS;
	// Initialization.
	datachain();
	// Destruction.
	virtual ~datachain();
	// Appends data to the chain.
	virtual bool append(const char *src, unsigned int size);
	// Retreives and removes data from the chain.
	virtual void extract(char *dst, unsigned int &size);
	// Retreives data from the chain, does not remove.
	virtual void peek(char *dst, unsigned int &size);
	// Reads a line of text from the chain, removes.
	virtual rrid_t gets(char *dst, unsigned int &size, int flags);
	// Returns the number of bytes available for extraction.
	unsigned int get_size() const;
	// Removes a number of bytes from the queue.
	void remove(unsigned int size);
	// Checks whether the chain has data.
	bool has_data() const;
};


// Packet chain.  The difference between this class and the original
// datachain is that the data is appended and dequeueed by packets,
// each up to sizeof(datachainitem::data) bytes.  The caller will not
// be able to retreive two chain items by a single call to the extraction
// function; adding blocks of data larger than that will result in
// truncation.  The gets function is unavailable.
class packetchain : public datachain {
public:
	DECLARE_ALLOCATORS;
	// Appends data to the chain.
	bool append(const char *src, unsigned int size);
	// Retreives and removes data from the chain.
	void extract(char *dst, unsigned int &size);
	// Retreives data from the chain, does not remove.
	void peek(char *dst, unsigned int &size);
	// Reads a line of text from the chain, removes.
	rrid_t gets(char *dst, unsigned int &size);
};

#endif // __common_datachain_h
