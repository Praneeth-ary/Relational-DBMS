#include "Algebra.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

bool isNumber(char* str){
    int len;
    float ignore;
    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]){
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId < 0) return srcRelId;

    AttrCatEntry attrCatEntry;
    int retVal = AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
    if(retVal != SUCCESS) return E_ATTRNOTEXIST;

    /*** Convert strVal to an attribute of data type NUMBER or STRING ***/
    int type = attrCatEntry.attrType;
    Attribute attrVal;
    if(type == NUMBER){
        if(isNumber(strVal)){
            attrVal.nVal = atof(strVal);
        }
        else{
            return E_ATTRTYPEMISMATCH;
        }
    }
    else if(type == STRING){
        strcpy(attrVal.sVal,strVal);
    }

    // // Print Code
    // RelCatEntry relCatEntry;
    // RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    // printf("|");
    // for (int i = 0; i < relCatEntry.numAttrs; ++i) {
    //     AttrCatEntry attrCatEntry;
    //     AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

    //     printf(" %s |", attrCatEntry.attrName);
    // }
    // printf("\n");

    // while (true) {
    //     RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

    //     if (searchRes.block != -1 && searchRes.slot != -1) {

    //         RecBuffer recBuffer(searchRes.block);

    //         Attribute record[relCatEntry.numAttrs];

    //         recBuffer.getRecord(record, searchRes.slot);

    //         printf("|");
    //         for (int i = 0; i < relCatEntry.numAttrs; ++i) {

    //             AttrCatEntry attrCatEntry;
    //             AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

    //             if (attrCatEntry.attrType == NUMBER) {
    //                 printf(" %d |", record[i].nVal);
    //             }
    //             else {
    //                 printf(" %s |", record[i].sVal);
    //             }
    //         }
    //         printf("\n");

    //     } else {
    //         break;
    //     }
    // }

    
    /*** Creating and opening the target relation ***/
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatEntry);
    
    int numAttrs = relCatEntry.numAttrs;
    char attr_names[numAttrs][ATTR_SIZE];
    int attr_types[numAttrs];

    for(int i=0;i<numAttrs;i++){
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);
        strcpy(attr_names[i],attrCatEntry.attrName);
        attr_types[i] = attrCatEntry.attrType;
    }

    retVal = Schema::createRel(targetRel,numAttrs,attr_names,attr_types);
    if(retVal != SUCCESS) return retVal;

    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0){
        Schema::deleteRel(targetRel);
        return targetRelId;
    } 

    /*** Selecting and inserting records into the target relation ***/
    RelCacheTable::resetSearchIndex(srcRelId);
    AttrCacheTable::resetSearchIndex(srcRelId,attr);
    Attribute record[numAttrs];

    while(BlockAccess::search(srcRelId,record,attr,attrVal,op) == SUCCESS){
        retVal = BlockAccess::insert(targetRelId,record);
        if(retVal != SUCCESS){
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return retVal;
        }
    }
    Schema::closeRel(targetRel);

    return SUCCESS;
}

int Algebra::insert(char relName[ATTR_SIZE], int numberOfAttributes, char record[][ATTR_SIZE]){
    if( strcmp(relName,RELCAT_RELNAME) == 0 || strcmp(relName,ATTRCAT_RELNAME) == 0){
        return E_NOTPERMITTED;
    }

    int relId = OpenRelTable::getRelId(relName);
    if(relId == E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);
    if(relCatEntry.numAttrs != numberOfAttributes){
        return E_NATTRMISMATCH;
    }

    Attribute recordValues[numberOfAttributes];
    AttrCatEntry attrCatEntry;
    for(int i=0;i<numberOfAttributes;i++){
        AttrCacheTable::getAttrCatEntry(relId,i,&attrCatEntry);
        int type = attrCatEntry.attrType;

        if(type == NUMBER){
            if(isNumber(record[i])){
                recordValues[i].nVal = atof(record[i]);
            }
            else{
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if(type == STRING){
            strcpy(recordValues[i].sVal,record[i]);
        }
    }
    
    int res = BlockAccess::insert(relId,recordValues);
    return res;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId == E_RELNOTOPEN) return E_RELNOTOPEN;

    /*** Creating and opening the target relation ***/
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatEntry);
    int numAttrs = relCatEntry.numAttrs;
    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];

    for(int i=0;i<numAttrs;i++){
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);
        strcpy(attrNames[i],attrCatEntry.attrName);
        attrTypes[i] = attrCatEntry.attrType;
    }

    int retVal = Schema::createRel(targetRel,numAttrs,attrNames,attrTypes);
    if(retVal != SUCCESS) return retVal;

    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0){
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    /*** Inserting projected records into the target relation ***/
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[numAttrs];

    while(BlockAccess::project(srcRelId,record) == SUCCESS){
        retVal = BlockAccess::insert(targetRelId,record);
        if(retVal != SUCCESS){
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return retVal;
        }
    }
    Schema::closeRel(targetRel);

    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {
    /*** Creating and opening the target relation ***/
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId == E_RELNOTOPEN) return E_RELNOTOPEN;

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatEntry);
    int srcNumAttrs = relCatEntry.numAttrs;
    int attrOffset[tar_nAttrs];
    int attrType[tar_nAttrs];
    
    for(int i=0;i<tar_nAttrs;i++){
        AttrCatEntry attrCatEntry;
        int retVal = AttrCacheTable::getAttrCatEntry(srcRelId,tar_Attrs[i],&attrCatEntry);
        if(retVal != SUCCESS) return E_ATTRNOTEXIST;
        attrOffset[i] = attrCatEntry.offset;
        attrType[i] = attrCatEntry.attrType;
    }

    int retVal = Schema::createRel(targetRel,tar_nAttrs,tar_Attrs,attrType);
    if(retVal != SUCCESS) return retVal;

    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0){
        Schema::deleteRel(targetRel);
        return targetRelId;
    }


    /*** Inserting projected records into the target relation ***/
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[srcNumAttrs];
    while(BlockAccess::project(srcRelId,record) == SUCCESS){
        Attribute targetRecord[tar_nAttrs];
        for(int i=0;i<tar_nAttrs;i++){
            targetRecord[i] = record[attrOffset[i]];
        }
        retVal = BlockAccess::insert(targetRelId,targetRecord);
        if(retVal != SUCCESS){
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return retVal;
        }
    }    
    Schema::closeRel(targetRel);
    return SUCCESS;
}

int Algebra::join(char srcRelOne[ATTR_SIZE], char srcRelTwo[ATTR_SIZE], char targetRel[ATTR_SIZE], char attrOne[ATTR_SIZE], char attrTwo[ATTR_SIZE]){
    int relId1 = OpenRelTable::getRelId(srcRelOne), relId2 = OpenRelTable::getRelId(srcRelTwo);
    if(relId1 < 0 || relId2 < 0) return E_RELNOTOPEN;
    
    AttrCatEntry attrCatEntry1, attrCatEntry2;
    int retVal = AttrCacheTable::getAttrCatEntry(relId1,attrOne,&attrCatEntry1);
    if(retVal != SUCCESS) return retVal;
    retVal = AttrCacheTable::getAttrCatEntry(relId2,attrTwo,&attrCatEntry2);
    if(retVal != SUCCESS) return retVal;

    if(attrCatEntry1.attrType != attrCatEntry2.attrType) return E_ATTRTYPEMISMATCH;

    RelCatEntry relCatEntry1,relCatEntry2;
    RelCacheTable::getRelCatEntry(relId1,&relCatEntry1);
    RelCacheTable::getRelCatEntry(relId2,&relCatEntry2);
    for(int i=0;i<relCatEntry1.numAttrs;i++){
        AttrCatEntry attr1;
        AttrCacheTable::getAttrCatEntry(relId1,i,&attr1);
        for(int j=0;j<relCatEntry2.numAttrs;j++){
            AttrCatEntry attr2;
            AttrCacheTable::getAttrCatEntry(relId2,j,&attr2);
            if(i == attrCatEntry1.offset && j == attrCatEntry2.offset) continue;
            if(strcmp(attr1.attrName,attr2.attrName) == 0) return E_DUPLICATEATTR;
        }
    }

    if(attrCatEntry2.rootBlock == -1){
        retVal = BPlusTree::bPlusCreate(relId2,attrTwo);
        if(retVal != SUCCESS) return retVal;
    }
    
    //Create Target Relation
    int numAttrs1 = relCatEntry1.numAttrs, numAttrs2 = relCatEntry2.numAttrs;
    int numAttrsTarget = numAttrs1+numAttrs2-1;
    char targetAttrNames[numAttrsTarget][ATTR_SIZE];
    int targetAttrTypes[numAttrsTarget];

    AttrCatEntry attr1;
    for(int i=0;i<numAttrs1;i++){
        AttrCacheTable::getAttrCatEntry(relId1,i,&attr1);
        strcpy(targetAttrNames[i],attr1.attrName);
        targetAttrTypes[i] = attr1.attrType;
    }
    
    int targetIdx = numAttrs1;
    AttrCatEntry attr2;
    for(int i=0;i<numAttrs2;i++){
        if(i == attrCatEntry2.offset) continue;
        AttrCacheTable::getAttrCatEntry(relId2,i,&attr2);
        strcpy(targetAttrNames[targetIdx],attr2.attrName);
        targetAttrTypes[targetIdx++] = attr2.attrType;
    }

    retVal = Schema::createRel(targetRel,numAttrsTarget,targetAttrNames,targetAttrTypes);
    if(retVal != SUCCESS) return retVal;

    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0){
        BlockAccess::deleteRelation(targetRel);
        return targetRelId;
    } 

    Attribute record1[numAttrs1], record2[numAttrs2], targetRecord[numAttrsTarget];
    while(BlockAccess::project(relId1,record1) == SUCCESS){
        RelCacheTable::resetSearchIndex(relId2);
        AttrCacheTable::resetSearchIndex(relId2,attrTwo);
        
        while(BlockAccess::search(relId2,record2,attrTwo,record1[attrCatEntry1.offset],EQ) == SUCCESS){
            int targetIdx = 0;
            for(int i=0;i<numAttrs1;i++){
                targetRecord[targetIdx++] = record1[i];
            }
            for(int i=0;i<numAttrs2;i++){
                if(i == attrCatEntry2.offset) continue;
                targetRecord[targetIdx++] = record2[i];
            }
            retVal = BlockAccess::insert(targetRelId,targetRecord);
            if(retVal == E_DISKFULL){
                OpenRelTable::closeRel(targetRelId);
                BlockAccess::deleteRelation(targetRel);
                return retVal;
            }
        }
    }

    OpenRelTable::closeRel(targetRelId);
    return SUCCESS;
}   