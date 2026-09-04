#ifndef NITCBASE_OPENRELTABLE_H
#define NITCBASE_OPENRELTABLE_H

#include "define/constants.h"
#include "block_access.h"

typedef struct OpenRelTableMetaInfo{
	bool free;
	char relName[ATTR_SIZE];
} OpenRelTableMetaInfo;

class OpenRelTable {
//	static char OpenRelTable[MAX_OPEN][ATTR_SIZE];
	static OpenRelTableMetaInfo tableMetaInfo[MAX_OPEN];
public:
	static void initializeOpenRelationTable();
	static int getRelationId(char relationName[ATTR_SIZE]);
	static int getRelationName(int relationId, char relationName[ATTR_SIZE]);
	static int openRelation(char relationName[ATTR_SIZE]);
	static int closeRelation(int relationId);
	static int checkIfRelationOpen(char relationName[ATTR_SIZE]);
	static int checkIfRelationOpen(int relationId);
	static int checkIfOpenRelTableHasFreeEntry();
};

#endif //NITCBASE_OPENRELTABLE_H
