#include "AttrCacheTable.h"
#include <cstring>

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];

void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS], AttrCatEntry* attrCatEntry){
    strcpy(attrCatEntry->relName,record[ATTRCAT_REL_NAME_INDEX].sVal);
    strcpy(attrCatEntry->attrName,record[ATTRCAT_ATTR_NAME_INDEX].sVal);
    attrCatEntry->attrType = record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
    attrCatEntry->primaryFlag = record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
    attrCatEntry->rootBlock = record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
    attrCatEntry->offset = record[ATTRCAT_OFFSET_INDEX].nVal;
}

void AttrCacheTable::attrCatEntryToRecord(AttrCatEntry* attrCatEntry, union Attribute record[ATTRCAT_NO_ATTRS]){
    strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal,attrCatEntry->relName);
    strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal,attrCatEntry->attrName);
    record[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrCatEntry->attrType;
    record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = attrCatEntry->primaryFlag;
    record[ATTRCAT_ROOT_BLOCK_INDEX].nVal = attrCatEntry->rootBlock;
    record[ATTRCAT_OFFSET_INDEX].nVal = attrCatEntry->offset;
}

int AttrCacheTable::getAttrCatEntry(int relId,char attrName[ATTR_SIZE],AttrCatEntry* attrCatEntry){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(attrCache[relId] == nullptr) return E_RELNOTOPEN;
    AttrCacheEntry* current = attrCache[relId];
    while(current){
        if( strcmp(current->attrCatEntry.attrName,attrName) == 0){
            *attrCatEntry = current->attrCatEntry;
            return SUCCESS; 
        }
        current = current->next;
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::getAttrCatEntry(int relId,int attrOffset,AttrCatEntry* attrCatEntry){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(attrCache[relId] == nullptr) return E_RELNOTOPEN;
    AttrCacheEntry* current = attrCache[relId];
    while(current){
        if( current->attrCatEntry.offset == attrOffset ){
            *attrCatEntry = current->attrCatEntry;
            return SUCCESS; 
        }
        current = current->next;
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatEntry){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(attrCache[relId] == nullptr) return E_RELNOTOPEN;
    AttrCacheEntry* current = attrCache[relId];
    while(current){
        if( strcmp(current->attrCatEntry.attrName,attrName) == 0){
            current->attrCatEntry = *attrCatEntry;
            current->dirty = true;
            return SUCCESS; 
        }
        current = current->next;
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatEntry){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(attrCache[relId] == nullptr) return E_RELNOTOPEN;
    AttrCacheEntry* current = attrCache[relId];
    while(current){
        if( current->attrCatEntry.offset == attrOffset ){
            current->attrCatEntry = *attrCatEntry;
            current->dirty = true;
            return SUCCESS; 
        }
        current = current->next;
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::getSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(AttrCacheTable::attrCache[relId] == nullptr) return E_RELNOTOPEN;
    AttrCacheEntry* current = attrCache[relId];
    while(current){
        if(strcmp(current->attrCatEntry.attrName,attrName) == 0){
            searchIndex->block = current->searchIndex.block;
            searchIndex->index = current->searchIndex.index;
            return SUCCESS;
        }
        current = current->next;
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::getSearchIndex(int relId, int attrOffset, IndexId *searchIndex){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(AttrCacheTable::attrCache[relId] == nullptr) return E_RELNOTOPEN;
    AttrCacheEntry* current = AttrCacheTable::attrCache[relId];
    while(current){
        if(current->attrCatEntry.offset == attrOffset){
            searchIndex->block = current->searchIndex.block;
            searchIndex->index = current->searchIndex.index;
            return SUCCESS;
        }
        current = current->next;
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(AttrCacheTable::attrCache[relId] == nullptr) return E_RELNOTOPEN;
    AttrCacheEntry* current = AttrCacheTable::attrCache[relId];
    while(current){
        if(strcmp(current->attrCatEntry.attrName,attrName) == 0){
            current->searchIndex.block = searchIndex->block;
            current->searchIndex.index = searchIndex->index;
            return SUCCESS;
        }
        current = current->next;
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId, int attrOffset, IndexId *searchIndex){
    if(relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if(AttrCacheTable::attrCache[relId] == nullptr) return E_RELNOTOPEN;
    AttrCacheEntry* current = AttrCacheTable::attrCache[relId];
    while(current){
        if(current->attrCatEntry.offset == attrOffset){
            current->searchIndex.block = searchIndex->block;
            current->searchIndex.index = searchIndex->index;
            return SUCCESS;
        }
        current = current->next;
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::resetSearchIndex(int relId, char attrName[ATTR_SIZE]){
    IndexId searchIndex{-1,-1};
    return AttrCacheTable::setSearchIndex(relId,attrName,&searchIndex);
}
int AttrCacheTable::resetSearchIndex(int relId, int attrOffset){
    IndexId searchIndex{-1,-1};
    return AttrCacheTable::setSearchIndex(relId,attrOffset,&searchIndex);
}