/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <iostream>
#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "exceptions/bad_buffer_exception.h"
#include "exceptions/hash_not_found_exception.h"

namespace badgerdb { 

//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

BufMgr::BufMgr(std::uint32_t bufs)
	: numBufs(bufs) {
	bufDescTable = new BufDesc[bufs];

  for (FrameId i = 0; i < bufs; i++) 
  {
  	bufDescTable[i].frameNo = i;
  	bufDescTable[i].valid = false;
  }

  bufPool = new Page[bufs];

  int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
  hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

  clockHand = bufs - 1;
}

//flush all the frames and clear all memory
BufMgr::~BufMgr() {
}

    
void BufMgr::advanceClock()
{
    clockHand = (clockHand + 1) % numBufs;
}

//Implement the clock algorithm
//add a pinned count, if pinned count = numBufs, then throw exception
//run through each buf and check for validity following diagram.
void BufMgr::allocBuf(FrameId & frame) 
{
    //track if a frame has been found
    bool frameFound = false;
    
    std::uint32_t pinnedCount = 0;
    
    //start loop, until frame is found. Only time it will leave loop is if frame is found
    //or bufferexceededexception occurs
    while(!frameFound && (pinnedCount < numBufs)){
        
        advanceClock();
        
        //found a buffer frame that can be used, exit loop
        if(!bufDescTable[clockHand].valid){
        
            frameFound = true;
            break;
        
        } //check the refbit, if it is false then frame is found and the entry in the hashtable needs to be removed
        else if(bufDescTable[clockHand].refbit){
            bufDescTable[clockHand].refbit = false;
            continue;
        } else if(bufDescTable[clockHand].pinCnt > 0) {
            pinnedCount++;
            continue;
        }else{
            
            if(bufDescTable[clockHand].dirty){
                bufDescTable[clockHand].file->writePage(bufPool[clockHand]);
            }
            frameFound = true;
            //remove the hashtable entry
            hashTable->remove(bufDescTable[clockHand].file, bufDescTable[clockHand].pageNo);
        }
    
    }
    //All pages are pinned
    if(pinnedCount == numBufs){
        throw BufferExceededException();
    } else {
        frame = clockHand;
        
        bufDescTable[clockHand].Clear();
    }
    
}


void BufMgr::readPage(File* file, const PageId pageNo, Page*& page)
{
    /*
    // check whether page is already in buffer pool with lookup(), throws HashNotFoundExc.
    try {
        hashTable->lookup(file, pageNo, file->readPage(pageNo));

        // no exception thrown, in hash table

        // set appropriate refbit
        bufDescTable[pageNo];

        // increment pin count

        // return pointer to frame containing the page via page parameter
    }
    catch (const HashNotFoundException& e) {
        // not in buffer pool

        // allocate buffer frame
        allocBuf(file->readPage(pageNo));

        // insert page into hash table
        hashTable->insert(file, pageNo, bufDescTable[pageNo].frameNo);
    }
     */



}

//set the frame that the page is allocated to
void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty) 
{
    
}


void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page) 
{
	FrameId frameNumber = 0;
	
	//allocate page and get buffer frame pool
	Page filePage = file->allocatePage();
	allocBuf(frameNumber);
	pageNo = filePage.page_number();
	
	//insert into hashtable then set frame
	hashTable->insert(file, pageNo, frameNumber);
	bufDescTable[frameNumber].Set(file, pageNo);
	bufPool[frameNumber] = filePage;
	//passs correct pointer
	page = &bufPool[frameNumber];
	


}

//
void BufMgr::flushFile(const File* file) 
{
}

//
void BufMgr::disposePage(File* file, const PageId PageNo)
{

    // if allocated in buffer pool, free it
    try {
        hashTable->lookup(file, PageNo, bufDescTable->frameNo);
        // remove page from hash table
        hashTable->remove(file, PageNo);
    }
    catch (const HashNotFoundException& e) {
        // not in hash table, shouldn't need to do anything
    }

    file->deletePage(PageNo);
}

void BufMgr::printSelf(void) 
{
  BufDesc* tmpbuf;
	int validFrames = 0;
  
  for (std::uint32_t i = 0; i < numBufs; i++)
	{
  	tmpbuf = &(bufDescTable[i]);
		std::cout << "FrameNo:" << i << " ";
		tmpbuf->Print();

  	if (tmpbuf->valid == true)
    	validFrames++;
  }

	std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
}

}
