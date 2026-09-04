#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer(){
    for(int i=0;i<BLOCK_ALLOCATION_MAP_SIZE;i++){
        Disk::readBlock(blockAllocMap+i*BLOCK_SIZE,i);
    }
    for(int i=0;i<BUFFER_CAPACITY;i++){
        metainfo[i].free = true;
        metainfo[i].dirty = false;
        metainfo[i].blockNum = INVALID_BLOCKNUM;
        metainfo[i].timeStamp = -1;
    }
}

StaticBuffer::~StaticBuffer(){
    for(int i=0;i<BLOCK_ALLOCATION_MAP_SIZE;i++){
        Disk::writeBlock(blockAllocMap+i*BLOCK_SIZE,i);
    }
    for(int i=0;i<BUFFER_CAPACITY;i++){
        if(!metainfo[i].free && metainfo[i].dirty){
            Disk::writeBlock(blocks[i],metainfo[i].blockNum);
        }
    }
}

int StaticBuffer::getBufferNum(int blockNum){
    if(blockNum < 0 || blockNum >= DISK_BLOCKS) return E_OUTOFBOUND;
    for(int i=0;i<BUFFER_CAPACITY;i++){
        if(metainfo[i].blockNum == blockNum) return i;
    }
    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::getFreeBuffer(int blockNum){
    if(blockNum < 0 || blockNum >= DISK_BLOCKS) return E_OUTOFBOUND;
    for(int i=0;i<BUFFER_CAPACITY;i++){
        if(metainfo[i].free==false)
            metainfo[i].timeStamp++;
    }
    int bufferNum = -1;
    for(int i=0;i<BUFFER_CAPACITY;i++){
        if(metainfo[i].free == true){
            bufferNum = i;
            break;
        }
    }

    if(bufferNum == -1){
        int maxTime = -1;
        for(int i=0;i<BUFFER_CAPACITY;i++){
            if(maxTime < metainfo[i].timeStamp){
                maxTime = metainfo[i].timeStamp;
                bufferNum = i;
            }
        }
        if(metainfo[bufferNum].dirty){
            Disk::writeBlock(blocks[bufferNum],metainfo[bufferNum].blockNum);
        }
    }
    metainfo[bufferNum].free = false;
    metainfo[bufferNum].dirty = false;
    metainfo[bufferNum].blockNum = blockNum;
    metainfo[bufferNum].timeStamp = 0;
    
    return bufferNum;
}

int StaticBuffer::setDirtyBit(int blockNum){
    int bufferNum = StaticBuffer::getBufferNum(blockNum);
    if(bufferNum == E_BLOCKNOTINBUFFER) return bufferNum;
    if(bufferNum == E_OUTOFBOUND) return bufferNum;

    metainfo[bufferNum].dirty = true;
    return SUCCESS;
}

int StaticBuffer::getStaticBlockType(int blockNum){
    if(blockNum <= 0 || blockNum >= DISK_BLOCKS) return E_OUTOFBOUND;
    return (int) blockAllocMap[blockNum];
}