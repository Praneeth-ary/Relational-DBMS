#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

//BlockBuffer
BlockBuffer::BlockBuffer(char blockType){
    int blockTypeInt;
    if (blockType == 'R') blockTypeInt = REC;
    else if (blockType == 'I') blockTypeInt = IND_INTERNAL;
    else if (blockType == 'L') blockTypeInt = IND_LEAF;

    this->blockNum = this->getFreeBlock(blockTypeInt);
}

BlockBuffer::BlockBuffer(int blockNum){
    this->blockNum = blockNum;
}


int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char** buffPtr){
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
    if(bufferNum != E_BLOCKNOTINBUFFER){

        for(int i=0;i<BUFFER_CAPACITY;i++){
            if( StaticBuffer::metainfo[i].free == false){
                if(i == bufferNum) StaticBuffer::metainfo[i].timeStamp = 0;
                else StaticBuffer::metainfo[i].timeStamp++;
            }
        }

    }
    else{
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
        if(bufferNum == E_OUTOFBOUND){
            return bufferNum;
        }
        Disk::readBlock(StaticBuffer::blocks[bufferNum],this->blockNum);
    }
    *buffPtr = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType){
    int blockNum = -1;
    for(int i=0;i<DISK_BLOCKS;i++){
        if(StaticBuffer::blockAllocMap[i] == UNUSED_BLK){
            blockNum = i;
            break;
        }
    }
    if(blockNum == -1){
        return E_DISKFULL;
    }
    
    this->blockNum = blockNum;
    int bufferNum = StaticBuffer::getFreeBuffer(blockNum);
    if(bufferNum < 0){
        return bufferNum;
    }

    struct HeadInfo header;
    header.blockType = blockType;
    header.lblock = -1;
    header.rblock = -1;
    header.pblock = -1;
    header.numAttrs = 0;
    header.numEntries = 0;
    header.numSlots = 0;

    int res = this->setHeader(&header);
    if(res != SUCCESS){
        return res;
    }

    res = this->setBlockType(blockType);
    if(res != SUCCESS){
        return res;
    }

    return blockNum;
}

int BlockBuffer::setBlockType(int blockType){
    unsigned char * bufferPtr;
    int res = this->loadBlockAndGetBufferPtr(&bufferPtr);
    if(res != SUCCESS){
        return res;
    }
    *((int32_t*) bufferPtr) = blockType;
    StaticBuffer::blockAllocMap[this->blockNum] = blockType;
    res = StaticBuffer::setDirtyBit(this->blockNum);
    if( res != SUCCESS){
        return res;
    }
    return SUCCESS;
}

int BlockBuffer::getBlockNum(){
    return this->blockNum;
}

int BlockBuffer::getHeader(struct HeadInfo* head){
    unsigned char* bufferPtr;
    int res = this->loadBlockAndGetBufferPtr(&bufferPtr);
    if(res != SUCCESS){
        return res;    
    }
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;
    head->lblock = bufferHeader->lblock;
    head->rblock = bufferHeader->rblock;
    head->pblock = bufferHeader->pblock;
    head->numAttrs = bufferHeader->numAttrs;
    head->numEntries = bufferHeader->numEntries;
    head->numSlots = bufferHeader->numSlots;
    head->blockType = bufferHeader->blockType;

    return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo* head){
    unsigned char *bufferPtr;
    int res = this->loadBlockAndGetBufferPtr(&bufferPtr);
    if(res != SUCCESS){
        return res;
    }
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->pblock = head->pblock;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->numSlots = head->numSlots;
    bufferHeader->blockType = head->blockType;

    res = StaticBuffer::setDirtyBit(this->blockNum);
    if(res != SUCCESS){
        return res;
    }
    return SUCCESS;
}

void BlockBuffer::releaseBlock(){
    if(this->blockNum == INVALID_BLOCKNUM) return;
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
    if(bufferNum != E_BLOCKNOTINBUFFER){
        StaticBuffer::metainfo[bufferNum].free = true;
    }
    StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;
    this->blockNum = INVALID_BLOCKNUM;
}

// RecBuffer
RecBuffer::RecBuffer() : BlockBuffer('R'){}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum){}

int RecBuffer::getSlotMap(unsigned char* slotMap){
    unsigned char* buffptr;
    int ret = loadBlockAndGetBufferPtr(&buffptr);
    if(ret != SUCCESS){
        return ret;
    }
    HeadInfo head;
    getHeader(&head);
    memcpy(slotMap,buffptr+HEADER_SIZE,head.numSlots);
    
    return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char* slotMap){
    unsigned char* bufferPtr;
    int res = loadBlockAndGetBufferPtr(&bufferPtr);
    if(res != SUCCESS){
        return res;
    }

    HeadInfo header;
    getHeader(&header);
    int numSlots = header.numSlots;
    memcpy(bufferPtr+HEADER_SIZE,slotMap,numSlots);

    res = StaticBuffer::setDirtyBit(this->blockNum);
    return res;
}

int RecBuffer::getRecord(union Attribute* rec, int slotNum){
    
    struct HeadInfo head;
    this->getHeader(&head);
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;
    
    if(slotNum < 0 || slotNum >= slotCount) return E_OUTOFBOUND;

    unsigned char* buffer;
    int res = this->loadBlockAndGetBufferPtr(&buffer);
    if(res != SUCCESS) return res;    

    int recordSize = attrCount*ATTR_SIZE;
    unsigned char *slotPointer = buffer+HEADER_SIZE+slotCount+slotNum*recordSize;
    
    memcpy(rec,slotPointer,recordSize);

    return SUCCESS;
}

int RecBuffer::setRecord(union Attribute* rec, int slotNum){
    struct HeadInfo head;
    this->getHeader(&head);

    int slotCount = head.numSlots;
    int attrCount = head.numAttrs;

    if(slotNum < 0 || slotNum >= slotCount) return E_OUTOFBOUND;
    
    unsigned char* buffer;
    int res = loadBlockAndGetBufferPtr(&buffer);
    if(res != SUCCESS) return res;

    int recordSize = attrCount*ATTR_SIZE;
    unsigned char * slotPointer = buffer+HEADER_SIZE+slotCount+recordSize*slotNum;
    memcpy(slotPointer,rec,recordSize);

    StaticBuffer::setDirtyBit(this->blockNum);
    
    return SUCCESS;
}

IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum){}
IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType){}

IndInternal::IndInternal() : IndBuffer('I'){}
IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum){}

IndLeaf::IndLeaf() : IndBuffer('L'){}
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum){}

int IndInternal::getEntry(void* ptr, int indexNum){
    if(indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL) return E_OUTOFBOUND;
    
    unsigned char *bufferPtr;
    int retVal = loadBlockAndGetBufferPtr(&bufferPtr);
    if(retVal != SUCCESS) return retVal;

    InternalEntry* internalEntry = (InternalEntry*) ptr;
    unsigned char* entryPtr = bufferPtr+HEADER_SIZE+(indexNum*20);

    memcpy(&(internalEntry->lChild),entryPtr,sizeof(int32_t));
    memcpy(&(internalEntry->attrVal),entryPtr+4,sizeof(Attribute));
    memcpy(&(internalEntry->rChild),entryPtr+20,sizeof(int32_t));

    return SUCCESS;
}

int IndLeaf::getEntry(void* ptr, int indexNum){
    if(indexNum < 0 || indexNum >= MAX_KEYS_LEAF) return E_OUTOFBOUND;

    unsigned char* bufferPtr;
    int retVal = loadBlockAndGetBufferPtr(&bufferPtr);
    if(retVal != SUCCESS) return retVal;

    unsigned char* entryPtr = bufferPtr+HEADER_SIZE+indexNum*LEAF_ENTRY_SIZE;
    memcpy((Index*)ptr,entryPtr,LEAF_ENTRY_SIZE);
    
    return SUCCESS;
}

int IndInternal::setEntry(void *ptr, int indexNum) {
    if(indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL) return E_OUTOFBOUND;

    unsigned char* bufferPtr;
    int retVal = loadBlockAndGetBufferPtr(&bufferPtr);
    if( retVal != SUCCESS) return retVal;

    unsigned char *entryPtr = bufferPtr+HEADER_SIZE+indexNum*20;
    InternalEntry* intEntry = (InternalEntry*) ptr;
    
    memcpy(entryPtr,&(intEntry->lChild),sizeof(int32_t));
    memcpy(entryPtr+4,&(intEntry->attrVal),sizeof(Attribute));
    memcpy(entryPtr+20,&(intEntry->rChild),sizeof(int32_t));

    retVal = StaticBuffer::setDirtyBit(this->blockNum);
    return retVal;
}

int IndLeaf::setEntry(void *ptr, int indexNum) {
    if(indexNum < 0 || indexNum >= MAX_KEYS_LEAF) return E_OUTOFBOUND;

    unsigned char* bufferPtr;
    int retVal = loadBlockAndGetBufferPtr(&bufferPtr);
    if(retVal != SUCCESS) return retVal;

    unsigned char* entryPtr = bufferPtr+HEADER_SIZE+indexNum*LEAF_ENTRY_SIZE;
    memcpy(entryPtr,(Index*)ptr,LEAF_ENTRY_SIZE);
    retVal = StaticBuffer::setDirtyBit(this->blockNum);

    return retVal;
}


int compareAttrs(Attribute attr1, Attribute attr2, int attrType){
    if(attrType == STRING){
        return strcmp(attr1.sVal,attr2.sVal);
    }
    else{
        double diff = attr1.nVal-attr2.nVal;
        if(diff > 0) return 1;
        if(diff < 0) return -1;
        return 0;
    }
}