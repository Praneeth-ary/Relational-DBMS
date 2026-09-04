#ifndef NITCBASE_EXTERNAL_FS_COMMANDS_H
#define NITCBASE_EXTERNAL_FS_COMMANDS_H

void dump_relcat();
void dump_attrcat();
void dumpBlockAllocationMap();
void ls();
int importRelation(char *fileName);
int exportRelation(char *relname, char *filename);
bool checkIfInvalidCharacter(char character);

#endif //NITCBASE_EXTERNAL_FS_COMMANDS_H
