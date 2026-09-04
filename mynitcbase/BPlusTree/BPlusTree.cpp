#include "BPlusTree.h"

#include <cstring>

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op){
    /****** Get the next block and index to search and also validate  ******/
    IndexId searchIndex{-1,-1};
    int retVal = AttrCacheTable::getSearchIndex(relId,attrName,&searchIndex);
    if(retVal != SUCCESS) return RecId{-1,-1};

    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    int attrType = attrCatEntry.attrType;

    int block, index;
    if(searchIndex.block == -1 && searchIndex.index == -1){
        block = attrCatEntry.rootBlock;
        index = 0;
        if(block == -1) return RecId{-1,-1};
    }
    else{
        block = searchIndex.block;
        index = searchIndex.index+1;

        IndLeaf leaf(block);
        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if( index >= leafHead.numEntries){
            block = leafHead.rblock;
            index = 0;
            if(block == -1) return RecId{-1,-1};
        }
    }

    /******  Traverse through all the internal nodes according to value of attrVal and the operator op  ******/
    while(StaticBuffer::getStaticBlockType(block) == IND_INTERNAL){
        IndInternal internalBlk(block);
        HeadInfo intHead;
        internalBlk.getHeader(&intHead);
        InternalEntry intEntry;

        if( op == NE || op == LT || op == LE ){
            internalBlk.getEntry(&intEntry,0);
            block = intEntry.lChild;
        }
        else { // For EQ, GE and GT do normal search
            bool found  = false;
            for(int i=0;i<intHead.numEntries;i++){
                internalBlk.getEntry(&intEntry,i);
                int cmp = compareAttrs(intEntry.attrVal,attrVal,attrType);
                if( ( (op == EQ || op == GE ) && cmp >= 0 ) || (op == GT && cmp > 0) ){
                    block = intEntry.lChild;
                    found = true;
                    break;
                }
            }
            if(!found){ // Then attrVal is greater than all keys so go right
                internalBlk.getEntry(&intEntry,intHead.numEntries-1);
                block = intEntry.rChild;
            }
        }

    }

    /******  Identify the first leaf index entry from the current position that satisfies our condition (moving right) ******/

    while(block != -1){
        IndLeaf leafBlk(block);
        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);

        Index leafEntry;

        while(index < leafHead.numEntries){
            leafBlk.getEntry(&leafEntry,index);
            int cmpVal = compareAttrs(leafEntry.attrVal,attrVal,attrType);

            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0)
            ) {
                searchIndex = {block,index};
                AttrCacheTable::setSearchIndex(relId,attrName,&searchIndex);
                return RecId{leafEntry.block, leafEntry.slot};
            }
            else if( (op == EQ || op == LE || op == LT) && cmpVal > 0){
                return RecId{-1,-1};
            }
            index++;
        }
        if(op != NE){
            break;
        }

        block = leafHead.rblock;
        index = 0;
    }

    return RecId{-1,-1};
}
  
int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE]){
    if(relId == RELCAT_RELID || relId == ATTRCAT_RELID) return E_NOTPERMITTED;

    AttrCatEntry attrCatEntry;
    int retVal = AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    if(retVal != SUCCESS) return retVal;

    if(attrCatEntry.rootBlock != -1) return SUCCESS;

    /******Creating a new B+ Tree ******/
    IndLeaf rootBlockBuf;
    int rootBlock = rootBlockBuf.getBlockNum();
    if(rootBlock == E_DISKFULL) return E_DISKFULL;

    //Set rootBlock
    attrCatEntry.rootBlock = rootBlock;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);

    /***** Traverse all the blocks in the relation and insert them one by one into the B+ Tree *****/
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);
    int block = relCatEntry.firstBlk;
    int offset = attrCatEntry.offset;

    while(block != -1){
        RecBuffer recBlk(block);
        unsigned char slotMap[relCatEntry.numSlotsPerBlk];
        recBlk.getSlotMap(slotMap);

        for(int slot = 0; slot < relCatEntry.numSlotsPerBlk; slot++){
            if(slotMap[slot] == SLOT_OCCUPIED){
                Attribute record[relCatEntry.numAttrs];
                recBlk.getRecord(record,slot);
                
                RecId recId{block,slot};
                retVal = BPlusTree::bPlusInsert(relId,attrName,record[offset],recId);
                if(retVal == E_DISKFULL) return E_DISKFULL;
            }
        }

        HeadInfo recHead;
        recBlk.getHeader(&recHead);
        block = recHead.rblock;
    }
    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum){
    if(rootBlockNum < 0 || rootBlockNum >= DISK_BLOCKS) return E_OUTOFBOUND;
    int retVal;
    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    if(type == IND_LEAF){
        IndLeaf leafBlk(rootBlockNum);
        leafBlk.releaseBlock();
        return SUCCESS;
    }
    else if(type == IND_INTERNAL){
        IndInternal intBlk(rootBlockNum);
        HeadInfo intHead;
        intBlk.getHeader(&intHead);
        InternalEntry intEntry;

        intBlk.getEntry(&intEntry,0); // free first lchild
        retVal = BPlusTree::bPlusDestroy(intEntry.lChild);
        if(retVal != SUCCESS) return retVal;

        for(int i=0;i<intHead.numEntries;i++){ // free all rightft children
            intBlk.getEntry(&intEntry,i);
            retVal = BPlusTree::bPlusDestroy(intEntry.rChild);
            if(retVal != SUCCESS) return retVal;
        }

        intBlk.releaseBlock();
        return SUCCESS;
    }

    return E_INVALIDBLOCK;
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, RecId recordId){
    AttrCatEntry attrCatEntry;
    int retVal = AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    if(retVal != SUCCESS) return retVal;

    int blockNum = attrCatEntry.rootBlock;
    if(blockNum == -1){
        return E_NOINDEX;
    }

    int leafBlkNum = findLeafToInsert(blockNum,attrVal,attrCatEntry.attrType);
    Index entry{attrVal,recordId.block,recordId.slot};
    retVal = insertIntoLeaf(relId,attrName,leafBlkNum,entry);

    if(retVal == E_DISKFULL){
        bPlusDestroy(blockNum);
        attrCatEntry.rootBlock = -1;
        AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatEntry);
        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType){
    int blockNum = rootBlock;
    while(StaticBuffer::getStaticBlockType(blockNum) == IND_INTERNAL){
        IndInternal intBlk(blockNum);
        HeadInfo intHead;
        intBlk.getHeader(&intHead);
        
        InternalEntry intEntry;
        bool found = false;
        for(int i=0;i<intHead.numEntries;i++){
            intBlk.getEntry(&intEntry,i);
            int cmp = compareAttrs(intEntry.attrVal,attrVal,attrType);
            if(cmp >= 0){
                blockNum = intEntry.lChild;
                found = true;
                break;
            }
        }
        if(!found){
            intBlk.getEntry(&intEntry,intHead.numEntries-1);
            blockNum = intEntry.rChild;
        }
    }
    return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index entry){
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

    IndLeaf leafBlk(blockNum);
    HeadInfo leafHead;
    leafBlk.getHeader(&leafHead);

    // Insert entry to leaf
    Index indices[leafHead.numEntries+1];
    Index leafEntry;    
    int idx = 0;
    bool inserted = false;
    for(int i=0;i<leafHead.numEntries;i++){
        leafBlk.getEntry(&leafEntry,i);
        if(!inserted){
            int cmp = compareAttrs(leafEntry.attrVal,entry.attrVal,attrCatEntry.attrType);
            if(cmp >= 0){
                indices[idx++] = entry;
                inserted = true;
            }
        }
        indices[idx++] = leafEntry;
    }

    if(!inserted){
        indices[idx++] = entry;
    }

    // if numEntries is not full
    if(leafHead.numEntries != MAX_KEYS_LEAF){
        leafHead.numEntries++;
        leafBlk.setHeader(&leafHead);

        for(int i=0;i<leafHead.numEntries;i++){
            leafBlk.setEntry(&indices[i],i);
        }

        return SUCCESS;
    }

    //if numEntries is full
    int newRightBlk = splitLeaf(blockNum,indices);
    if(newRightBlk == E_DISKFULL) return E_DISKFULL;

    int retVal;
    if(leafHead.pblock != -1){
        InternalEntry newIntEntry{blockNum,indices[MIDDLE_INDEX_LEAF].attrVal,newRightBlk};
        retVal = insertIntoInternal(relId,attrName,leafHead.pblock,newIntEntry);
    }
    else{
        retVal = createNewRoot(relId,attrName,indices[MIDDLE_INDEX_LEAF].attrVal,blockNum,newRightBlk);
    }

    return retVal;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index indices[]){
    IndLeaf leftLeafBlk(leafBlockNum);
    IndLeaf rightLeafBlk;
    int leftBlkNum = leftLeafBlk.getBlockNum(), rightBlkNum = rightLeafBlk.getBlockNum();
    if(rightBlkNum == E_DISKFULL) return E_DISKFULL;
    
    HeadInfo leftHead,rightHead;
    leftLeafBlk.getHeader(&leftHead);
    
    rightLeafBlk.getHeader(&rightHead);
    rightHead.blockType = IND_LEAF;
    rightHead.numEntries = (MAX_KEYS_LEAF+1)/2;
    rightHead.lblock = leftBlkNum;
    rightHead.rblock = leftHead.rblock;
    rightHead.pblock = leftHead.pblock;
    rightLeafBlk.setHeader(&rightHead);

    leftHead.numEntries = (MAX_KEYS_LEAF+1)/2;
    leftHead.rblock = rightBlkNum;
    leftLeafBlk.setHeader(&leftHead);

    for(int i=0;i<=MIDDLE_INDEX_LEAF;i++)
        leftLeafBlk.setEntry(&indices[i],i);

    for(int i=MIDDLE_INDEX_LEAF+1;i<=MAX_KEYS_LEAF;i++)
        rightLeafBlk.setEntry(&indices[i],i-MIDDLE_INDEX_LEAF-1);

    return rightBlkNum;
}

int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry entry){
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

    IndInternal intBlk(intBlockNum);
    HeadInfo intHead;
    intBlk.getHeader(&intHead);

    InternalEntry entries[MAX_KEYS_INTERNAL+1];
    InternalEntry intEntry;
    bool inserted = false;
    int idx = 0;
    int insertPos = intHead.numEntries;

    for(int i=0; i<intHead.numEntries; i++){
        intBlk.getEntry(&intEntry, i);
        int cmp = compareAttrs(intEntry.attrVal, entry.attrVal, attrCatEntry.attrType);

        if (insertPos == intHead.numEntries && cmp >= 0) insertPos = i;
        if (i == insertPos) entries[idx++] = entry;
        entries[idx++] = intEntry;
    }

    if(insertPos == intHead.numEntries) entries[idx++] = entry;
    else entries[insertPos + 1].lChild = entry.rChild; // update lchild of next entry

    if(intHead.numEntries != MAX_KEYS_INTERNAL){
        intHead.numEntries++;
        intBlk.setHeader(&intHead);
        for(int i=0;i<intHead.numEntries;i++){
            intBlk.setEntry(&entries[i],i);
        }
        return SUCCESS;
    }

    int newRightBlk = splitInternal(intBlockNum,entries);
    if(newRightBlk == E_DISKFULL){
        BPlusTree::bPlusDestroy(entry.rChild);
        return E_DISKFULL;
    }

    int retVal;
    if(intHead.pblock != -1){
        InternalEntry parentEntry{intBlockNum,entries[MIDDLE_INDEX_INTERNAL].attrVal,newRightBlk};
        retVal = insertIntoInternal(relId,attrName,intHead.pblock,parentEntry);
    }
    else{
        retVal = createNewRoot(relId,attrName,entries[MIDDLE_INDEX_INTERNAL].attrVal,intBlockNum,newRightBlk);
    }
    return retVal;
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[]){
    IndInternal leftBlk(intBlockNum);
    IndInternal rightBlk;
    int leftBlkNum = leftBlk.getBlockNum(), rightBlkNum = rightBlk.getBlockNum();
    if(rightBlkNum == E_DISKFULL) return E_DISKFULL;

    HeadInfo leftHead, rightHead;
    leftBlk.getHeader(&leftHead);
    
    rightBlk.getHeader(&rightHead);
    rightHead.blockType = IND_INTERNAL;
    rightHead.numEntries = MAX_KEYS_INTERNAL/2;
    rightHead.pblock = leftHead.pblock;
    rightBlk.setHeader(&rightHead);

    leftHead.numEntries = MAX_KEYS_INTERNAL/2;
    leftBlk.setHeader(&leftHead);

    for(int i=0;i<MIDDLE_INDEX_INTERNAL;i++){
        leftBlk.setEntry(&internalEntries[i],i);
    }
    for(int i=MIDDLE_INDEX_INTERNAL+1;i<=MAX_KEYS_INTERNAL;i++){
        rightBlk.setEntry(&internalEntries[i],i-MIDDLE_INDEX_INTERNAL-1);
    }

    // Update pblock for children in rightBlock
    for(int i=MIDDLE_INDEX_INTERNAL+1;i<=MAX_KEYS_INTERNAL;i++){
        BlockBuffer childBlk(internalEntries[i].lChild);
        HeadInfo childHead;
        childBlk.getHeader(&childHead);
        childHead.pblock = rightBlkNum;
        childBlk.setHeader(&childHead);
    }

    BlockBuffer childBlk(internalEntries[MAX_KEYS_INTERNAL].rChild);
    HeadInfo childHead;
    childBlk.getHeader(&childHead);
    childHead.pblock = rightBlkNum;
    childBlk.setHeader(&childHead);

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild){
    //Create new Block for root
    IndInternal newRootBlk;
    int newRootBlkNum = newRootBlk.getBlockNum();
    if(newRootBlkNum == E_DISKFULL){
        BPlusTree::bPlusDestroy(rChild);
        return E_DISKFULL;
    }

    //Update header 
    HeadInfo rootHead;
    newRootBlk.getHeader(&rootHead);
    rootHead.numEntries = 1;
    newRootBlk.setHeader(&rootHead);

    //Add root entry for root Block
    InternalEntry intEntry{lChild,attrVal,rChild};
    newRootBlk.setEntry(&intEntry,0);

    // Update pblock in leftChild
    BlockBuffer lChildBlk(lChild);
    HeadInfo lChildHead;
    lChildBlk.getHeader(&lChildHead);
    lChildHead.pblock = newRootBlkNum;
    lChildBlk.setHeader(&lChildHead);

    // Update pblock in rightChild
    BlockBuffer rChildBlk(rChild);
    HeadInfo rChildHead;
    rChildBlk.getHeader(&rChildHead);
    rChildHead.pblock = newRootBlkNum;
    rChildBlk.setHeader(&rChildHead);

    // Update Attribute Cat Entry for rootBlock
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    attrCatEntry.rootBlock = newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatEntry);

    return SUCCESS;
}
