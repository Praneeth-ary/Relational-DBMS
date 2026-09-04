#ifndef NITCBASE_BPLUSTREE_H
#define NITCBASE_BPLUSTREE_H

#include "define/constants.h"
#include "define/errors.h"
#include "disk_structures.h"

class BPlusTree{
private :
	int rootBlock;
	int relId;
	char attrName[ATTR_SIZE];

public:
	BPlusTree(int relid, char attrName[ATTR_SIZE]);
	int getRootBlock();
	int bPlusInsert(union Attribute attrVal, recId recordId);
	recId BPlusSearch(union Attribute attrVal, int op, recId *prev_indexId);
	static int bPlusDestroy(int blockNum);
};

#endif //NITCBASE_BPLUSTREE_H
