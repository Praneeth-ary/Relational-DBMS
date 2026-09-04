#include "OpenRelTable.h"
#include <cstring>
#include<cstdlib>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable(){
    for(int i=0;i<MAX_OPEN;i++){
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        tableMetaInfo[i].free = true;
    }
    //Set up RelCache Entry for relCat and attrCat
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_RELCAT);

    RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord,&(relCacheEntry.relCatEntry));
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
    relCacheEntry.dirty = false;
    relCacheEntry.searchIndex = {-1, -1};

    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*) malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);
    RelCacheTable::recordToRelCatEntry(relCatRecord,&(relCacheEntry.relCatEntry));
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*) malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;


    //Set up AttrCache Entry for relCat and attrCat
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    AttrCacheEntry *head = nullptr, *prev = nullptr;
    for(int i=0;i<RELCAT_NO_ATTRS;i++){
        attrCatBlock.getRecord(attrCatRecord,i);

        AttrCacheEntry* attrCacheEntry = (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        attrCacheEntry->recId.slot = i;
        attrCacheEntry->next = nullptr;

        if(head == nullptr) head = attrCacheEntry;
        else prev->next = attrCacheEntry;

        prev = attrCacheEntry;
    }
    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    head = nullptr; prev = nullptr;
    for(int i=RELCAT_NO_ATTRS;i<RELCAT_NO_ATTRS+ATTRCAT_NO_ATTRS;i++){
        attrCatBlock.getRecord(attrCatRecord,i);
        
        AttrCacheEntry* attrCacheEntry = (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        attrCacheEntry->recId.slot = i;
        attrCacheEntry->next = nullptr;

        if(head == nullptr) head = attrCacheEntry;
        else prev->next = attrCacheEntry;

        prev = attrCacheEntry;
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

     /************ Setting up tableMetaInfo entries ************/
    
     tableMetaInfo[RELCAT_RELID].free = false;
     tableMetaInfo[ATTRCAT_RELID].free = false;

     strcpy(tableMetaInfo[RELCAT_RELID].relName,RELCAT_RELNAME);
     strcpy(tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);
}


OpenRelTable::~OpenRelTable(){

    for(int i=2;i<MAX_OPEN;i++){
        if(!tableMetaInfo[i].free){
            OpenRelTable::closeRel(i);
        }
    }
    
    //If relCat in relCache is modified
    if(RelCacheTable::relCache[RELCAT_RELID]->dirty){
        RelCatEntry relCatEntry = RelCacheTable::relCache[RELCAT_RELID]->relCatEntry;
        Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry,record);
        RecBuffer relRec(RELCAT_BLOCK);
        relRec.setRecord(record,RELCAT_SLOTNUM_FOR_RELCAT);
    }
    free(RelCacheTable::relCache[RELCAT_RELID]);
    RelCacheTable::relCache[RELCAT_RELID] = nullptr;

    //if attrCat in relCache is modified
    if(RelCacheTable::relCache[ATTRCAT_RELID]->dirty){
        RelCatEntry relCatEntry = RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry;
        Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry,record);
        RecBuffer relRec(RELCAT_BLOCK);
        relRec.setRecord(record,RELCAT_SLOTNUM_FOR_ATTRCAT);
    }
    free(RelCacheTable::relCache[ATTRCAT_RELID]);
    RelCacheTable::relCache[ATTRCAT_RELID] = nullptr;

    //Free relCat entries in attrCat
    AttrCacheEntry* curr = AttrCacheTable::attrCache[RELCAT_RELID];
    while(curr){
        AttrCacheEntry* next = curr->next;
        free(curr);
        curr = next;
    }
    AttrCacheTable::attrCache[RELCAT_RELID] = nullptr;

    //Free attrCat entries in attrCat
    curr = AttrCacheTable::attrCache[ATTRCAT_RELID];
    while(curr){
        AttrCacheEntry* next = curr->next;
        free(curr);
        curr = next;
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = nullptr;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]){
    for(int i=0;i<MAX_OPEN;i++){
        if( !tableMetaInfo[i].free && strcmp(tableMetaInfo[i].relName,relName) == 0 ){
            return i;
        }
    }
    return E_RELNOTOPEN;
}

int OpenRelTable::getFreeOpenRelTableEntry(){
    for(int i=0;i<MAX_OPEN;i++){
        if(tableMetaInfo[i].free) return i;
    }
    return E_CACHEFULL;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]){
    int relId = OpenRelTable::getRelId(relName);
    if( relId >= 0 ) return relId;
    
    relId = OpenRelTable::getFreeOpenRelTableEntry();
    if(relId == E_CACHEFULL) return E_CACHEFULL;

    // Add it to relCache
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute attrVal;
    strcpy(attrVal.sVal,relName);

    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,attrVal,EQ);

    if(relcatRecId.block == -1 && relcatRecId.slot == -1) return E_RELNOTEXIST;

    RecBuffer recBlock(relcatRecId.block);
    Attribute relRec[RELCAT_NO_ATTRS];
    recBlock.getRecord(relRec,relcatRecId.slot);

    RelCacheEntry* relCacheEntry = (RelCacheEntry*) malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relRec,&(relCacheEntry->relCatEntry));
    relCacheEntry->dirty = false;
    relCacheEntry->recId = relcatRecId;
    relCacheEntry->searchIndex = {-1, -1};
    
    RelCacheTable::relCache[relId] = relCacheEntry;

    //Add it to AttrCache
    AttrCacheEntry *head = nullptr, *prev = nullptr;
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    while(true){
        Attribute attrVal;
        strcpy(attrVal.sVal,relName);
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID,ATTRCAT_ATTR_RELNAME,attrVal,EQ);

        if(attrcatRecId.block == -1 && attrcatRecId.slot == -1){
            break;
        }

        RecBuffer attrBlock(attrcatRecId.block);
        Attribute attrRec[ATTRCAT_NO_ATTRS];
        attrBlock.getRecord(attrRec,attrcatRecId.slot);

        AttrCacheEntry* curr = (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrRec,&(curr->attrCatEntry));
        curr->dirty = false;
        curr->recId = attrcatRecId;
        curr->next = nullptr;

        if(head == nullptr) head = curr;
        else prev->next = curr;

        prev = curr;
    }
    AttrCacheTable::attrCache[relId] = head;
    
    //Update openRelTable
    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName,relName);

    return relId;
}

int OpenRelTable::closeRel(int relId){
    if(relId == RELCAT_RELID || relId == ATTRCAT_RELID) return E_NOTPERMITTED;
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(tableMetaInfo[relId].free) return E_RELNOTOPEN;

    //  Write Back for relCat entry
    if(RelCacheTable::relCache[relId]->dirty){
        Attribute relCatRec[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry), relCatRec);
        
        RecId recId = RelCacheTable::relCache[relId]->recId;
        RecBuffer relCatBlock(recId.block);

        relCatBlock.setRecord(relCatRec,recId.slot);
    }

    //Free RelCache Entry
    free(RelCacheTable::relCache[relId]);
    RelCacheTable::relCache[relId] = nullptr;
    
    //  Write Back for attrCat entry and Free AttrCacheEntry
    AttrCacheEntry* curr = AttrCacheTable::attrCache[relId];
    while(curr){
        if(curr->dirty){
            Attribute attrCatRec[ATTRCAT_NO_ATTRS];
            AttrCacheTable::attrCatEntryToRecord(&(curr->attrCatEntry),attrCatRec);
            
            RecBuffer attrCatBlock(curr->recId.block);
            attrCatBlock.setRecord(attrCatRec,curr->recId.slot);
        }
        AttrCacheEntry* next = curr->next;
        free(curr);
        curr = next;
    }
    AttrCacheTable::attrCache[relId] = nullptr;

    tableMetaInfo[relId].free = true;
    
    return SUCCESS;
}
