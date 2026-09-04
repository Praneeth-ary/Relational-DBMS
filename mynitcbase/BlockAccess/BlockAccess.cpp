#include "BlockAccess.h"
#include <cstring>

RecId BlockAccess::linearSearch(int relId, char *attrName, Attribute attrVal, int op){
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    
    int block,slot;
    if(prevRecId.block == -1 && prevRecId.slot == -1){
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId,&relCatEntry);
        block = relCatEntry.firstBlk;
        slot = 0;    
    }
    else{
        block = prevRecId.block;
        slot = prevRecId.slot+1;
    }

    while(block != -1){
        RecBuffer recBuff(block);
        
        HeadInfo head;
        recBuff.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        recBuff.getSlotMap(slotMap);

        while(slot < head.numSlots && slotMap[slot] == SLOT_UNOCCUPIED) slot++;

        if(slot >= head.numSlots){
            block = head.rblock;
            slot = 0;
            continue;
        }
        
        Attribute rec[head.numAttrs];
        recBuff.getRecord(rec,slot);
        
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
        int offset = attrCatEntry.offset, attrType = attrCatEntry.attrType;
        
        int cmpVal = compareAttrs(rec[offset],attrVal,attrType);
        if( (op == EQ && cmpVal == 0) || 
            (op == NE  && cmpVal != 0) || 
            (op == LT && cmpVal < 0) || 
            (op == LE && cmpVal <= 0) ||
            (op == GT && cmpVal > 0) || 
            (op == GE && cmpVal >= 0) ){
                RecId currRecId;
                currRecId.block = block;
                currRecId.slot = slot;
                RelCacheTable::setSearchIndex(relId,&currRecId);
                return currRecId;
        }
        slot++;
    }
    return RecId{-1,-1};
}

int BlockAccess::renameRelation(char *oldName, char *newName){
    //Check if relation with newName alr exisits
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName;
    strcpy(newRelationName.sVal,newName);
    RecId newRecId = BlockAccess::linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,newRelationName,EQ);
    if(newRecId.block != -1 && newRecId.slot != -1){
        return E_RELEXIST;
    }

    //Check if relation with oldName doesn't exist
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelationName;
    strcpy(oldRelationName.sVal,oldName);
    RecId relRecId = BlockAccess::linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,oldRelationName,EQ);
    if(relRecId.block == -1 && relRecId.slot == -1){
        return E_RELNOTEXIST;
    }

    //Update Relcatalog
    RecBuffer relCatRec(RELCAT_BLOCK);
    Attribute relRecord[RELCAT_NO_ATTRS];
    relCatRec.getRecord(relRecord,relRecId.slot);
    strcpy(relRecord[RELCAT_REL_NAME_INDEX].sVal,newName);
    relCatRec.setRecord(relRecord,relRecId.slot);

    //Update AttrCatalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numAttrs = relRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    for(int i=0;i<numAttrs;i++){
        RecId attrRecId = BlockAccess::linearSearch(ATTRCAT_RELID,ATTRCAT_ATTR_RELNAME,oldRelationName,EQ);
        RecBuffer attrCatRec(attrRecId.block);
        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        attrCatRec.getRecord(attrRecord,attrRecId.slot);
        strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal,newName);
        attrCatRec.setRecord(attrRecord,attrRecId.slot);
    }
    return SUCCESS;
}

int BlockAccess::renameAttribute(char *relName, char *oldName, char *newName){
    //Check if relation exists
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relationName;
    strcpy(relationName.sVal,relName);
    RecId relRecId = BlockAccess::linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,relationName,EQ);
    if(relRecId.block == -1 && relRecId.slot == -1){
        return E_RELNOTEXIST;
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    RecId attrToRenameRecId{-1, -1};
    Attribute attrRecord[ATTRCAT_NO_ATTRS];
    
    while(true){
        //Search all Attributes of relName to check if newattrname alr exists and also find oldattr
        RecId attrRecId = BlockAccess::linearSearch(ATTRCAT_RELID,ATTRCAT_ATTR_RELNAME,relationName,EQ);
        if(attrRecId.block == -1 && attrRecId.slot == -1){
            break;
        }

        RecBuffer attrCatRec(attrRecId.block);
        attrCatRec.getRecord(attrRecord,attrRecId.slot);
        if( strcmp(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName) == 0 ){
            attrToRenameRecId = attrRecId;
        }
        if( strcmp(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName) == 0 ){
            return E_ATTREXIST;
        }
    }

    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1){
        return E_ATTRNOTEXIST;
    }

    //Update attrCatalog
    RecBuffer attrCatRec(attrToRenameRecId.block);
    attrCatRec.getRecord(attrRecord,attrToRenameRecId.slot);
    strcpy(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
    attrCatRec.setRecord(attrRecord,attrToRenameRecId.slot);

    return SUCCESS;
}

int BlockAccess::insert(int relId, union Attribute* record){
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);
    int blockNum = relCatEntry.firstBlk;
    int numSlots = relCatEntry.numSlotsPerBlk;
    int numAttrs = relCatEntry.numAttrs;
    int prevBlockNum = -1;

    // Check till u get free block or all blocks are filled
    RecId recId = {-1,-1};
    while(blockNum != -1){
        RecBuffer recBlock(blockNum);
        
        HeadInfo header;
        recBlock.getHeader(&header);
        unsigned char slotMap[numSlots];
        recBlock.getSlotMap(slotMap);

        for(int i=0;i<numSlots;i++){
            if(slotMap[i] == SLOT_UNOCCUPIED){
                recId.block = blockNum;
                recId.slot = i;
                break;
            }
        }
        if(recId.block != -1 && recId.slot != -1){
            break;
        }
        prevBlockNum = blockNum;
        blockNum = header.rblock;
    }

    //If blocks are completely filled
    if(recId.block == -1 && recId.slot == -1){
        if(relId == RELCAT_RELID){
            return E_MAXRELATIONS; //For RelCat only one block for storing relation records
        }

        RecBuffer newRec;
        int ret = newRec.getBlockNum();
        if(ret == E_DISKFULL){
            return E_DISKFULL;
        }

        recId.block = ret;
        recId.slot = 0;

        HeadInfo head;
        head.blockType = REC;
        head.pblock = -1;
        head.lblock = prevBlockNum; // Works for even no exisiting records as preBlock = -1
        head.rblock = -1;
        head.numEntries = 0;
        head.numSlots = numSlots;
        head.numAttrs = numAttrs;

        newRec.setHeader(&head); // initialize header for new block

        unsigned char slotMap[numSlots];
        for(int i=0;i<numSlots;i++){
            slotMap[i] = SLOT_UNOCCUPIED;
        }
        newRec.setSlotMap(slotMap); // intialize slotMap for new block

        if(prevBlockNum != -1){
            RecBuffer prevBlock(prevBlockNum); //If prevBlock exists set its rblock to new block
            HeadInfo prevHead;
            prevBlock.getHeader(&prevHead);
            prevHead.rblock = recId.block;
            prevBlock.setHeader(&prevHead);
        }
        else{
            RelCacheTable::getRelCatEntry(relId,&relCatEntry); // since prevBlock doesn't exist this is the first block
            relCatEntry.firstBlk = recId.block;
            RelCacheTable::setRelCatEntry(relId,&relCatEntry);
        }

        // This should be the last block
        RelCacheTable::getRelCatEntry(relId,&relCatEntry);
        relCatEntry.lastBlk = recId.block;
        RelCacheTable::setRelCatEntry(relId,&relCatEntry);
    
    }

    RecBuffer recBlk(recId.block);
    recBlk.setRecord(record,recId.slot); // insert record

    unsigned char slotMap[numSlots];
    recBlk.getSlotMap(slotMap);
    slotMap[recId.slot] = SLOT_OCCUPIED;
    recBlk.setSlotMap(slotMap); // update slotmap
    
    HeadInfo head;
    recBlk.getHeader(&head);
    head.numEntries++;
    recBlk.setHeader(&head); // update no of entries

    RelCacheTable::getRelCatEntry(relId,&relCatEntry);
    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId,&relCatEntry); // update no of entries in relcache
    
    /* B+ Tree Insertions */
    int flag = SUCCESS;
    for(int i=0;i<relCatEntry.numAttrs;i++){
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId,i,&attrCatEntry);
        if(attrCatEntry.rootBlock != -1){
            int retVal = BPlusTree::bPlusInsert(relId,attrCatEntry.attrName,record[i],recId);
            if(retVal == E_DISKFULL){
                flag = E_INDEX_BLOCKS_RELEASED;
            }
        }
    }

    return flag;
}

int BlockAccess::search(int relId, Attribute *record, char *attrName, Attribute attrVal, int op){
    
    AttrCatEntry attrCatEntry;
    int retVal = AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    if(retVal != SUCCESS) return retVal;
    
    int rootBlk = attrCatEntry.rootBlock;
    RecId recId;

    if(rootBlk == -1)
        recId = BlockAccess::linearSearch(relId,attrName,attrVal,op);
    else 
        recId = BPlusTree::bPlusSearch(relId,attrName,attrVal,op);
    
    if(recId.block == -1 && recId.slot == -1)
        return E_NOTFOUND;
        
    RecBuffer recBlk(recId.block);
    recBlk.getRecord(record,recId.slot);
    return SUCCESS;
}

int BlockAccess::deleteRelation(char* relName){
    if( strcmp(relName,RELCAT_RELNAME) == 0 || strcmp(relName,ATTRCAT_RELNAME) == 0 ){
        return E_NOTPERMITTED;
    }

    //Search for record in relCat
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    strcpy(relNameAttr.sVal,relName);
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,relNameAttr,EQ);
    if(recId.block == -1 && recId.slot == -1){
        return E_RELNOTEXIST;
    }

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer recBlk(recId.block);
    int retVal = recBlk.getRecord(relCatEntryRecord,recId.slot);
    if(retVal != SUCCESS) return retVal;

    int currBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    
    //Delete all related blocks 
    while(currBlock != -1){
        RecBuffer currRec(currBlock);
        HeadInfo head;
        retVal = currRec.getHeader(&head);
        if(retVal != SUCCESS) return retVal;
        
        currBlock = head.rblock;
        currRec.releaseBlock();
    }

    //Delete Attribute Catalog Entries of this relation
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numberOfAttributesDeleted = 0;

    while(true){
        //Search for attributes of relation in attribute catalog rel
        RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID,ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);
        if(attrCatRecId.block == -1 && attrCatRecId.slot == -1){
            break;
        }

        numberOfAttributesDeleted++;
        RecBuffer attrRecBlk(attrCatRecId.block);
        
        Attribute attrRec[ATTRCAT_NO_ATTRS];
        retVal = attrRecBlk.getRecord(attrRec,attrCatRecId.slot);
        if(retVal != SUCCESS) return retVal;
        
        int rootBlock = attrRec[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

        HeadInfo head;
        retVal = attrRecBlk.getHeader(&head); 
        if(retVal != SUCCESS) return retVal;
        
        int numSlots = head.numSlots;
        unsigned char slotMap[numSlots];
        retVal = attrRecBlk.getSlotMap(slotMap);
        if(retVal != SUCCESS) return retVal;
        
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED; //Free that slot
        retVal = attrRecBlk.setSlotMap(slotMap);
        if(retVal != SUCCESS) return retVal;
        
        head.numEntries--; //No of entries reduced
        retVal = attrRecBlk.setHeader(&head);
        if(retVal != SUCCESS) return retVal;
        
        //If no of entries is zero free that block
        if(head.numEntries == 0){
            //Set rblock of prev block to rblock of currblock
            RecBuffer leftBlk(head.lblock);
            HeadInfo leftHead;
            retVal = leftBlk.getHeader(&leftHead);
            if(retVal != SUCCESS) return retVal;
            
            leftHead.rblock = head.rblock;
            retVal = leftBlk.setHeader(&leftHead);
            if(retVal != SUCCESS) return retVal;
            
            if(head.rblock != -1){
            //Set lblock of next block to lblock of currblock
                RecBuffer rightBlk(head.rblock);
                HeadInfo rightHead;
                retVal = rightBlk.getHeader(&rightHead);
                if(retVal != SUCCESS) return retVal;

                rightHead.lblock = head.lblock;
                retVal = rightBlk.setHeader(&rightHead);
                if(retVal != SUCCESS) return retVal;
            }
            else{
                //Then this is the lastblock so update lastblock of Attribute catalog in relation Catalog
                RecBuffer relCatBlk(RELCAT_BLOCK);
                Attribute relRec[RELCAT_NO_ATTRS];
                retVal = relCatBlk.getRecord(relRec,RELCAT_SLOTNUM_FOR_ATTRCAT);
                if(retVal != SUCCESS) return retVal;

                relRec[RELCAT_LAST_BLOCK_INDEX].nVal = head.lblock;
                retVal = relCatBlk.setRecord(relRec,RELCAT_SLOTNUM_FOR_ATTRCAT);
                if(retVal != SUCCESS) return retVal;
            }

            attrRecBlk.releaseBlock();
        }

        //Delete Index
        if(rootBlock != -1){
            BPlusTree::bPlusDestroy(rootBlock);
        }
    }   
    //Delete RelCatalog entry of this Relation
    RecBuffer relCatRecBlk(recId.block);
    HeadInfo relCatHead;
    retVal = relCatRecBlk.getHeader(&relCatHead);
    if(retVal != SUCCESS) return retVal;

    relCatHead.numEntries--;
    retVal = relCatRecBlk.setHeader(&relCatHead);
    if(retVal != SUCCESS) return retVal;

    unsigned char relCatSlotMap[relCatHead.numSlots];
    retVal = relCatRecBlk.getSlotMap(relCatSlotMap);
    if(retVal != SUCCESS) return retVal;

    relCatSlotMap[recId.slot] = SLOT_UNOCCUPIED;
    retVal = relCatRecBlk.setSlotMap(relCatSlotMap);
    if(retVal != SUCCESS) return retVal;

    //Update relCat Entry in RelCacheTable
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID,&relCatEntry);
    relCatEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID,&relCatEntry);

    //Update attrCat Entry in RelCacheTable
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relCatEntry);
    relCatEntry.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID,&relCatEntry);

    return SUCCESS;
}

int BlockAccess::project(int relId, Attribute* record){
    RecId prevIdx;
    RelCacheTable::getSearchIndex(relId,&prevIdx);
    int block, slot;
    
    if(prevIdx.block == -1 && prevIdx.slot == -1){
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId,&relCatEntry);
        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else{
        block = prevIdx.block;
        slot = prevIdx.slot+1;
    }

    while(block != -1){
        RecBuffer recBlk(block);
        HeadInfo head;
        recBlk.getHeader(&head);
        unsigned char slotMap[head.numSlots];
        recBlk.getSlotMap(slotMap);

        if(slot >= head.numSlots){
            block = head.rblock;
            slot = 0;
        }
        else if(slotMap[slot] == SLOT_UNOCCUPIED){
            slot++;
        }
        else{
            break;
        }
    }
    
    if(block == -1){
        return E_NOTFOUND;
    }

    RecId nextRecId{block,slot};
    RelCacheTable::setSearchIndex(relId,&nextRecId);

    RecBuffer recBlk(block);
    recBlk.getRecord(record,slot);

    return SUCCESS;
}