#include "RelCacheTable.h"
#include <cstring>

RelCacheEntry* RelCacheTable::relCache[MAX_OPEN];

void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS],RelCatEntry* relCatEntry){
    strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    relCatEntry->numAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relCatEntry->numRecs = record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk = record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk = record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk = record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;
}

void RelCacheTable::relCatEntryToRecord(RelCatEntry* relCatEntry, union Attribute record[RELCAT_NO_ATTRS]){
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal,relCatEntry->relName);
    record[RELCAT_NO_ATTRIBUTES_INDEX].nVal = relCatEntry->numAttrs;
    record[RELCAT_NO_RECORDS_INDEX].nVal = relCatEntry->numRecs;
    record[RELCAT_FIRST_BLOCK_INDEX].nVal = relCatEntry->firstBlk;
    record[RELCAT_LAST_BLOCK_INDEX].nVal = relCatEntry->lastBlk;
    record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = relCatEntry->numSlotsPerBlk;
}

int RelCacheTable::getRelCatEntry(int relId, RelCatEntry* relCatEntry){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(relCache[relId] == nullptr) return E_RELNOTOPEN;

    *relCatEntry = relCache[relId]->relCatEntry;
    return SUCCESS;
}

int RelCacheTable::setRelCatEntry(int relId, RelCatEntry* relCatEntry){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(relCache[relId] == nullptr) return E_RELNOTOPEN;
    relCache[relId]->relCatEntry = *relCatEntry;
    relCache[relId]->dirty = true;
    return SUCCESS;
}

int RelCacheTable::getSearchIndex(int relId, RecId* searchIdx){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(relCache[relId] == nullptr) return E_RELNOTOPEN;
    *searchIdx = relCache[relId]->searchIndex;
    return SUCCESS;
}

int RelCacheTable::setSearchIndex(int relId, RecId* searchIdx){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(relCache[relId] == nullptr) return E_RELNOTOPEN;
    relCache[relId]->searchIndex = *searchIdx;
    return SUCCESS;
}

int RelCacheTable::resetSearchIndex(int relId){
    RecId searchIdx = {-1,-1};
    return setSearchIndex(relId,&searchIdx);
}