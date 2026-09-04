#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <iomanip>
#include <queue>
#include <readline/readline.h>
#include <readline/history.h>

#include "define/constants.h"
#include "define/errors.h"
#include "interface.h"
#include "schema.h"
#include "Disk.h"
#include "OpenRelTable.h"
#include "block_access.h"
#include "algebra.h"
#include "external_fs_commands.h"
#include "BPlusTree.h"

using namespace std;

void display_help();

void printErrorMsg(int ret);

vector<string> extract_tokens(string input_command);

void string_to_char_array(string x, char *a, int size);

int executeCommandsFromFile(string fileName);

bool checkValidCsvFile(string filename);

int getOperator(string op_str);

int printSchema(char relname[ATTR_SIZE]);

int printRows(char relname[ATTR_SIZE]);

int select_from_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE]);

int select_from_where_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], char attribute[ATTR_SIZE],
                              int op, char value[ATTR_SIZE]);

int select_attr_from_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], int attr_count,
                             char attrs[][ATTR_SIZE]);

int select_attr_from_where_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], int attr_count,
                                   char attrs[][ATTR_SIZE], char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]);

int select_attr_from_join_handler(char sourceRelOneName[ATTR_SIZE], char sourceRelTwoName[ATTR_SIZE],
                                  char targetRelName[ATTR_SIZE], int attrCount,
                                  char joinAttributeOne[ATTR_SIZE], char joinAttributeTwo[ATTR_SIZE],
                                  char attributeList[][ATTR_SIZE]);

void print16(char char_string_thing[ATTR_SIZE]);

void print16(char char_string_thing[ATTR_SIZE], bool newline);

int getRootBlock(char *rel_name, char *attr_name, int &attrType);

void printBPlusTree(int rootBlock, int attrType);

int exportBPlusTreeBlocks(int blockNum, int attrType, FILE *fp_export);


/* TODO: RETURN 0 here means Success, return -1 (EXIT or FAILURE) means quit XFS,
 * I have done wherever i saw, check all that you added once again Jezzy
 */
int regexMatchAndExecute(const string input_command) {
	smatch m;
	if (regex_match(input_command, help)) {
		display_help();
	} else if (regex_match(input_command, ex)) {
		return EXIT;
	} else if (regex_match(input_command, echo)) {
		regex_search(input_command, m, echo);
		string message = m[1];
		/* TODO: add bmap, relcat, attrcat check */
		cout << message << endl;
	} else if (regex_match(input_command, run)) {
		regex_search(input_command, m, run);
		string file_name = m[1];
		if (executeCommandsFromFile(file_name) == EXIT) {
			return EXIT;
		}
	} else if (regex_match(input_command, bplus_tree)) {
		regex_search(input_command, m, bplus_tree);
		string tablename = m[1];
		string attrname = m[2];
		char relname[ATTR_SIZE], attr_name[ATTR_SIZE];

		string_to_char_array(tablename, relname, ATTR_SIZE - 1);
		string_to_char_array(attrname, attr_name, ATTR_SIZE - 1);

		int attrType;
		int rootBlock = getRootBlock(relname, attr_name, attrType);
		if (rootBlock <= 0) {
			printErrorMsg(rootBlock);
			return FAILURE;
		}
		printBPlusTree(rootBlock, attrType);

	} else if (regex_match(input_command, bplus_blocks)) {
		regex_search(input_command, m, bplus_blocks);
		string tablename = m[1];
		string attrname = m[2];
		string filePath = m[3];
		filePath = OUTPUT_FILES_PATH + filePath;
		char relname[ATTR_SIZE], attr_name[ATTR_SIZE];

		string_to_char_array(tablename, relname, ATTR_SIZE - 1);
		string_to_char_array(attrname, attr_name, ATTR_SIZE - 1);

		int attrType;
		int rootBlock = getRootBlock(relname, attr_name, attrType);
		if (rootBlock <= 0) {
			printErrorMsg(rootBlock);
			return FAILURE;
		}

		FILE *fp_export = fopen(filePath.c_str(), "w");
		if (!fp_export) {
			cout << " Invalid file path" << endl;
			return FAILURE;
		}
		fputs("----- B+ TREE BLOCKS -----\n", fp_export);
		exportBPlusTreeBlocks(rootBlock, attrType, fp_export);
		fclose(fp_export);
		cout << "Exported index blocks of ";
		print16(relname, false);
		cout << " successfully to: " << filePath << endl;

	} else if (regex_match(input_command, fdisk)) {
		Disk::createDisk();
		Disk::formatDisk();
		// Re-initialize OpenRelTable
		OpenRelTable::initializeOpenRelationTable();
		cout << "Disk formatted" << endl;
 	} else if (regex_match(input_command, print_table)) {
		regex_search(input_command, m, print_table);
		string tableName = m[1];

		char relname[ATTR_SIZE];
		string_to_char_array(tableName, relname, ATTR_SIZE - 1);

		int ret = printRows(relname);

		if (ret != SUCCESS) {
			cout << "Print Command Failed" << endl;
			return FAILURE;
		}

	} else if (regex_match(input_command, dump_rel)) {
		dump_relcat();
		cout << "Dumped relation catalog to " << OUTPUT_FILES_PATH << "relation_catalog" << endl;
	} else if (regex_match(input_command, dump_attr)) {
		dump_attrcat();
		cout << "Dumped attribute catalog to " << OUTPUT_FILES_PATH << "attribute_catalog" << endl;
	} else if (regex_match(input_command, dump_bmap)) {
		dumpBlockAllocationMap();
		cout << "Dumped block allocation map to " << OUTPUT_FILES_PATH << "block_allocation_map" << endl;
	} else if (regex_match(input_command, list_all)) {
		ls();
		char rel[ATTR_SIZE], attr[ATTR_SIZE];
		string_to_char_array("numbers", rel, 15);
		string_to_char_array("key", attr, 15);
	} else if (regex_match(input_command, imp)) {
		string filepath_str;
		string complete_filepath = INPUT_FILES_PATH;

		regex_search(input_command, m, imp);
		filepath_str = m[1];
		complete_filepath = complete_filepath + filepath_str;
		char filepath[complete_filepath.length() + 1];
		string_to_char_array(complete_filepath, filepath, complete_filepath.length() + 1);
		FILE *file = fopen(filepath, "r");
		if (!file) {
			cout << "Invalid file path or file does not exist" << endl;
			return FAILURE;
		}
		fclose(file);
		string Filename = m[1];

		int ret = importRelation(filepath);
		if (ret == SUCCESS) {
			cout << "Imported from " << complete_filepath << " successfully" << endl;
		} else {
			printErrorMsg(ret);
			return FAILURE;
		}
	} else if (regex_match(input_command, exprt)) {
		regex_search(input_command, m, exprt);
		string tableName = m[1];

		string filePath = m[2];
		filePath = OUTPUT_FILES_PATH + filePath;

		char relname[ATTR_SIZE];
		string_to_char_array(tableName, relname, ATTR_SIZE - 1);
		char fileName[filePath.length() + 1];
		string_to_char_array(filePath, fileName, filePath.length() + 1);

		int ret = exportRelation(relname, fileName);

		if (ret == SUCCESS) {
			cout << "Exported ";
			print16(relname, false);
			cout << " successfully to: " << filePath << endl;
		} else {
			cout << "Export Command Failed" << endl;
			return FAILURE;
		}

	} else if (regex_match(input_command, schema)) {
		regex_search(input_command, m, schema);
		string tableName = m[1];

		char relname[ATTR_SIZE];
		string_to_char_array(tableName, relname, ATTR_SIZE - 1);

		return printSchema(relname);

	} else if (regex_match(input_command, open_table)) {

		regex_search(input_command, m, open_table);
		string tablename = m[1];
		char relname[ATTR_SIZE];
		string_to_char_array(tablename, relname, ATTR_SIZE - 1);

		int ret = openRel(relname);
		if (ret >= 0 && ret <= MAX_OPEN - 1) {
			cout << "Relation ";
			print16(relname, false);
			cout << " opened successfully\n";
		} else {
			printErrorMsg(ret);
			return FAILURE;
		}

	} else if (regex_match(input_command, close_table)) {

		regex_search(input_command, m, close_table);
		string tablename = m[1];
		char relname[ATTR_SIZE];
		string_to_char_array(tablename, relname, ATTR_SIZE - 1);

		int ret = closeRel(relname);
		if (ret == SUCCESS) {
			cout << "Relation ";
			print16(relname, false);
			cout << " closed successfully\n";
		} else {
			printErrorMsg(ret);
			return FAILURE;
		}

	} else if (regex_match(input_command, create_table)) {
		regex_search(input_command, m, create_table);

		string tablename = m[1];

        // 'temp' is used for internal purposes as of now
        if (tablename == TEMP) {
            printErrorMsg(E_CREATETEMP);
            return FAILURE;
        }

		char relname[ATTR_SIZE];
		string_to_char_array(tablename, relname, ATTR_SIZE - 1);

		regex_search(input_command, m, temp);
		string attrs = m[0];
		vector<string> words = extract_tokens(attrs);

		int no_attrs = words.size() / 2;

        if (no_attrs > 125) {
            printErrorMsg(E_MAXATTRS);
            return FAILURE;
        }

		char attribute[no_attrs][ATTR_SIZE];
		int type_attr[no_attrs];

		for (int i = 0, k = 0; i < no_attrs; i++, k += 2) {
			string_to_char_array(words[k], attribute[i], ATTR_SIZE - 1);
			if (words[k + 1] == "STR")
				type_attr[i] = STRING;
			else if (words[k + 1] == "NUM")
				type_attr[i] = NUMBER;
		}

		int ret = createRel(relname, no_attrs, attribute, type_attr);
		if (ret == SUCCESS) {
			cout << "Relation ";
			print16(relname, false);
			cout << " created successfully" << endl;
		} else {
			printErrorMsg(ret);
			return FAILURE;
		}

	} else if (regex_match(input_command, drop_table)) {
		regex_search(input_command, m, drop_table);
		string tablename = m[1];
		char relname[ATTR_SIZE];
		string_to_char_array(tablename, relname, ATTR_SIZE - 1);

		int ret = deleteRel(relname);
		if (ret == SUCCESS) {
			cout << "Relation ";
			print16(relname, false);
			cout << " deleted successfully\n";
		} else {
			printErrorMsg(ret);
			return FAILURE;
		}

	} else if (regex_match(input_command, create_index)) {

		regex_search(input_command, m, create_index);
		string tablename = m[1];
		string attrname = m[2];
		char relname[ATTR_SIZE], attr_name[ATTR_SIZE];

		string_to_char_array(tablename, relname, ATTR_SIZE - 1);
		string_to_char_array(attrname, attr_name, ATTR_SIZE - 1);

//		cout << "size of internal entry : " << sizeof(InternalEntry) << endl;
//		cout << "size of int : " << sizeof(int32_t) << endl;
//		cout << "size of attrval : " << sizeof(Attribute) << endl;
		int ret = createIndex(relname, attr_name);

		/*
		 * DEBUG
		 */
//		testBPlusTree(relname, attr_name);

		if (ret > 0)
			cout << "Index created successfully\n";
		else {
			printErrorMsg(ret);
			return FAILURE;
		}

	} else if (regex_match(input_command, drop_index)) {
		regex_search(input_command, m, drop_index);
		string tablename = m[1];
		string attrname = m[2];
		char relname[ATTR_SIZE], attr_name[ATTR_SIZE];
		string_to_char_array(tablename, relname, ATTR_SIZE - 1);
		string_to_char_array(attrname, attr_name, ATTR_SIZE - 1);

		int ret = dropIndex(relname, attr_name);
		if (ret == SUCCESS)
			cout << "Index deleted successfully\n";
		else {
			printErrorMsg(ret);
			return FAILURE;
		}
	} else if (regex_match(input_command, rename_table)) {

		regex_search(input_command, m, rename_table);
		string oldTableName = m[1];
		string newTableName = m[2];

        if (newTableName == TEMP) {
            printErrorMsg(E_RENAMETOTEMP);
            return FAILURE;
        }

        char old_relation_name[ATTR_SIZE];
		char new_relation_name[ATTR_SIZE];
        string_to_char_array(oldTableName, old_relation_name, ATTR_SIZE - 1);
		string_to_char_array(newTableName, new_relation_name, ATTR_SIZE - 1);

		int ret = renameRel(old_relation_name, new_relation_name);

		if (ret == SUCCESS) {
			cout << "Renamed Relation Successfully" << endl;
		} else {
			printErrorMsg(ret);
			return FAILURE;
		}

	} else if (regex_match(input_command, rename_column)) {

		regex_search(input_command, m, rename_column);
		string tablename = m[1];
		string oldcolumnname = m[2];
		string newcolumnname = m[3];
		char relname[ATTR_SIZE];
		char old_col[ATTR_SIZE];
		char new_col[ATTR_SIZE];
		string_to_char_array(tablename, relname, ATTR_SIZE - 1);
		string_to_char_array(oldcolumnname, old_col, ATTR_SIZE - 1);
		string_to_char_array(newcolumnname, new_col, ATTR_SIZE - 1);

		int ret = renameAtrribute(relname, old_col, new_col);

		if (ret == SUCCESS) {
			cout << "Renamed Attribute Successfully" << endl;
		} else {
			printErrorMsg(ret);
			return FAILURE;
		}

	} else if (regex_match(input_command, insert_single)) {

		regex_search(input_command, m, insert_single);
		string table_name = m[1];
		char rel_name[ATTR_SIZE];
		string_to_char_array(table_name, rel_name, ATTR_SIZE - 1);
		regex_search(input_command, m, temp);
		string attrs = m[0];
		vector<string> words = extract_tokens(attrs);

		int retValue = insert(words, rel_name);

		if (retValue == SUCCESS) {
			cout << "Inserted successfully" << endl;
		} else {
			printErrorMsg(retValue);
			return FAILURE;
		}
	} else if (regex_match(input_command, insert_multiple)) {
		regex_search(input_command, m, insert_multiple);
		string tablename = m[1];
		char relname[ATTR_SIZE];
		string p = INPUT_FILES_PATH;
		string_to_char_array(tablename, relname, ATTR_SIZE - 1);
		string t = m[2];
		p = p + t;
		char Filepath[p.length() + 1];
		string_to_char_array(p, Filepath, p.length() + 1);
		FILE *file = fopen(Filepath, "r");
		cout << Filepath << endl;
		if (!file) {
			cout << "Invalid file path or file does not exist" << endl;
			return FAILURE;
		}
		fclose(file);
		int retValue = insert(relname, Filepath);
		if (retValue == SUCCESS) {
			cout << "Inserted successfully" << endl;
		} else {
			printErrorMsg(retValue);
			return FAILURE;
		}

	} else if (regex_match(input_command, select_from)) {
		regex_search(input_command, m, select_from);
		string sourceRelName_str = m[1];
		string targetRelName_str = m[2];

        if (targetRelName_str == TEMP) {
            printErrorMsg(E_TARGETNAMETEMP);
            return FAILURE;
        }

		char sourceRelName[ATTR_SIZE];
		char targetRelName[ATTR_SIZE];

		string_to_char_array(sourceRelName_str, sourceRelName, ATTR_SIZE - 1);
		string_to_char_array(targetRelName_str, targetRelName, ATTR_SIZE - 1);

		return select_from_handler(sourceRelName, targetRelName);

	} else if (regex_match(input_command, select_from_where)) {
		regex_search(input_command, m, select_from_where);
		string sourceRel_str = m[1];
		string targetRel_str = m[2];
		string attribute_str = m[3];
		string op_str = m[4];
		string value_str = m[5];

        if (targetRel_str == TEMP) {
            printErrorMsg(E_TARGETNAMETEMP);
            return FAILURE;
        }

		char sourceRelName[ATTR_SIZE];
		char targetRelName[ATTR_SIZE];
		char attribute[ATTR_SIZE];
		char value[ATTR_SIZE];
		string_to_char_array(sourceRel_str, sourceRelName, ATTR_SIZE - 1);
		string_to_char_array(targetRel_str, targetRelName, ATTR_SIZE - 1);
		string_to_char_array(attribute_str, attribute, ATTR_SIZE - 1);
		string_to_char_array(value_str, value, ATTR_SIZE - 1);

		int op = getOperator(op_str);

		return select_from_where_handler(sourceRelName, targetRelName, attribute, op, value);

	} else if (regex_match(input_command, select_attr_from)) {
		regex_search(input_command, m, select_attr_from);

		string sourceRel_str = m[2];
		string targetRel_str = m[3];

        if (targetRel_str == TEMP) {
            printErrorMsg(E_TARGETNAMETEMP);
            return FAILURE;
        }

		char sourceRelName[ATTR_SIZE];
		char targetRelName[ATTR_SIZE];
		string_to_char_array(sourceRel_str, sourceRelName, ATTR_SIZE - 1);
		string_to_char_array(targetRel_str, targetRelName, ATTR_SIZE - 1);

		/* Get the attribute list string from the input command */
		vector<string> attr_tokens = extract_tokens(m[1]);

		int attr_count = attr_tokens.size();
		char attr_list[attr_count][ATTR_SIZE];
		for (int attr_no = 0; attr_no < attr_count; attr_no++) {
			string_to_char_array(attr_tokens[attr_no], attr_list[attr_no], ATTR_SIZE - 1);
		}


		return select_attr_from_handler(sourceRelName, targetRelName, attr_count, attr_list);

	} else if ((regex_match(input_command, select_attr_from_where))) {
		regex_search(input_command, m, select_attr_from_where);

		string sourceRel_str = m[2];
		string targetRel_str = m[3];
		string attribute_str = m[4];
		string op_str = m[5];
		string value_str = m[6];

        if (targetRel_str == TEMP) {
            printErrorMsg(E_TARGETNAMETEMP);
            return FAILURE;
        }

		char sourceRelName[ATTR_SIZE];
		char targetRelName[ATTR_SIZE];
		char attribute[ATTR_SIZE];
		char value[ATTR_SIZE];
		int op = getOperator(op_str);

		string_to_char_array(attribute_str, attribute, ATTR_SIZE - 1);
		string_to_char_array(value_str, value, ATTR_SIZE - 1);
		string_to_char_array(sourceRel_str, sourceRelName, ATTR_SIZE - 1);
		string_to_char_array(targetRel_str, targetRelName, ATTR_SIZE - 1);

		vector<string> attr_tokens = extract_tokens(m[1]);

		int attr_count = attr_tokens.size();
		char attr_list[attr_count][ATTR_SIZE];
		for (int attr_no = 0; attr_no < attr_count; attr_no++) {
			string_to_char_array(attr_tokens[attr_no], attr_list[attr_no], ATTR_SIZE - 1);
		}

		return select_attr_from_where_handler(sourceRelName, targetRelName, attr_count, attr_list, attribute, op,
		                                      value);

	} else if (regex_match(input_command, select_from_join)) {

		regex_search(input_command, m, select_from_join);

		char sourceRelOneName[ATTR_SIZE];
		char sourceRelTwoName[ATTR_SIZE];
		char targetRelName[ATTR_SIZE];
		char joinAttributeOne[ATTR_SIZE];
		char joinAttributeTwo[ATTR_SIZE];

		if (m[3] == TEMP) {
				printErrorMsg(E_TARGETNAMETEMP);
				return FAILURE;
		}

		string_to_char_array(m[1], sourceRelOneName, ATTR_SIZE - 1);
		string_to_char_array(m[2], sourceRelTwoName, ATTR_SIZE - 1);
		string_to_char_array(m[3], targetRelName, ATTR_SIZE - 1);

		if (m[1] == m[4] && m[2] == m[6]) {
			string_to_char_array(m[5], joinAttributeOne, ATTR_SIZE - 1);
			string_to_char_array(m[7], joinAttributeTwo, ATTR_SIZE - 1);

		} else if(m[1] == m[6] && m[2] == m[4]){
			string_to_char_array(m[7], joinAttributeOne, ATTR_SIZE - 1);
			string_to_char_array(m[5], joinAttributeTwo, ATTR_SIZE - 1);

		} else {
			cout << "Syntax Error: Relation names do not match" << endl;
			return FAILURE;
		}

		int ret = join(sourceRelOneName, sourceRelTwoName, targetRelName, joinAttributeOne, joinAttributeTwo);
		if (ret == SUCCESS) {
			cout << "Join successful" << endl;
		} else {
			printErrorMsg(ret);
			return FAILURE;
		}

	} else if (regex_match(input_command, select_attr_from_join)) {

		regex_search(input_command, m, select_attr_from_join);

		char sourceRelOneName[ATTR_SIZE];
		char sourceRelTwoName[ATTR_SIZE];
		char targetRelName[ATTR_SIZE];
		char joinAttributeOne[ATTR_SIZE];
		char joinAttributeTwo[ATTR_SIZE];

		if (m[4]== TEMP) {
				printErrorMsg(E_TARGETNAMETEMP);
				return FAILURE;
		}

		string_to_char_array(m[2], sourceRelOneName, ATTR_SIZE - 1);
		string_to_char_array(m[3], sourceRelTwoName, ATTR_SIZE - 1);
		string_to_char_array(m[4], targetRelName, ATTR_SIZE - 1);

		if (m[2] == m[5] && m[3] == m[7]){
			string_to_char_array(m[6], joinAttributeOne, ATTR_SIZE - 1);
			string_to_char_array(m[8], joinAttributeTwo, ATTR_SIZE - 1);
		} else if(m[2] == m[7] && m[3] == m[5]){
			string_to_char_array(m[8], joinAttributeOne, ATTR_SIZE - 1);
			string_to_char_array(m[6], joinAttributeTwo, ATTR_SIZE - 1);
		} else {
			cout << "Syntax Error: Relation names do not match" << endl;
			return FAILURE;
		}

		vector<string> attributesListAsWords = extract_tokens(m[1]);
		int attrCount = attributesListAsWords.size();
		char attributeList[attrCount][ATTR_SIZE];
		for (int i = 0; i < attrCount; i++) {
			string_to_char_array(attributesListAsWords[i], attributeList[i], ATTR_SIZE - 1);
		}

		return select_attr_from_join_handler(sourceRelOneName, sourceRelTwoName, targetRelName, attrCount,
		                                     joinAttributeOne, joinAttributeTwo, attributeList);

	} else {
		cout << "Syntax Error" << endl;
		return FAILURE;
	}
	return SUCCESS;
}

int main(int argc, char* argv[]) {

	// Initializing Open Relation Table
	OpenRelTable::initializeOpenRelationTable();

	// Taking Run Command as Command Line Argument(if provided)
	if(argc == 3 && strcmp(argv[1], "run") == 0) {
		string run_command("run ");
		run_command.append(argv[2]);
		int ret = regexMatchAndExecute(run_command);
		if (ret == EXIT) {
			return 0;
		}
	}

	char *buf;
	rl_bind_key('\t', rl_insert);
	while ((buf = readline("# ")) != nullptr) {
		if(strlen(buf) > 0){
			add_history(buf);
		}
		int ret = regexMatchAndExecute(string(buf));
		free(buf);
		if (ret == EXIT) {
			return 0;
		}
	}
}

int getOperator(string op_str) {
	int op = 0;
	if (op_str == "=")
		op = EQ;
	else if (op_str == "<")
		op = LT;
	else if (op_str == "<=")
		op = LE;
	else if (op_str == ">")
		op = GT;
	else if (op_str == ">=")
		op = GE;
	else if (op_str == "!=")
		op = NE;
	return op;
}

int getIndexOfWhereToken(vector<string> command_tokens) {
	int index_of_where;
	for (index_of_where = 0; index_of_where < command_tokens.size(); index_of_where++) {
		if (command_tokens[index_of_where] == "where" || command_tokens[index_of_where] == "WHERE")
			break;
	}
	return index_of_where;
}


int select_from_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE]) {
	/* Check if the relation is Open */
	int src_relid = OpenRelTable::getRelationId(sourceRelName);
	if (src_relid == E_RELNOTOPEN) {
		cout << "Source relation not open" << endl;
		return FAILURE;
	}

	/* Get the relation catalog entry for the source relation */
	Attribute relcatEntry[6];
	if (getRelCatEntry(src_relid, relcatEntry) != SUCCESS) {
		cout << "Command Failed: Could not get the relation catalogue entry" << endl;
		return FAILURE;
	}
	/*
	 * Array for the Attributes of the Target Relation
	 *  - target relation has same number of attributes as source relation (nAttrs)
	 * Search the Attribute Catalog to find all the attributes belonging to Source Relation
	 *  - and store them in this array (for projecting)
	 */
	int nAttrs = (int) relcatEntry[1].nval;
	char targetAttrs[nAttrs][ATTR_SIZE];
	Attribute rec_Attrcat[6];
	int recBlock_Attrcat = ATTRCAT_BLOCK; // Block Number
	HeadInfo headInfo;
	while (recBlock_Attrcat != -1) {
		headInfo = getHeader(recBlock_Attrcat);

		unsigned char slotmap[headInfo.numSlots];
		getSlotmap(slotmap, recBlock_Attrcat);

		for (int slotNum = 0; slotNum < SLOTMAP_SIZE_RELCAT_ATTRCAT; slotNum++) {
			if (slotmap[slotNum] != SLOT_UNOCCUPIED) {
				getRecord(rec_Attrcat, recBlock_Attrcat, slotNum);
				if (strcmp(rec_Attrcat[0].sval, sourceRelName) == 0) {
					// Copy the attribute name to the attribute offset index
					strcpy(targetAttrs[(int) rec_Attrcat[5].nval], rec_Attrcat[1].sval);
				}
			}
		}
		recBlock_Attrcat = headInfo.rblock;
	}

	int ret = project(sourceRelName, targetRelName, nAttrs, targetAttrs);
	if (ret == SUCCESS) {
		cout << "Selected successfully, result in relation: ";
		print16(targetRelName);
		return SUCCESS;
	} else {
		printErrorMsg(ret);
		return FAILURE;
	}
}

int select_from_where_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], char attribute[ATTR_SIZE],
                              int op, char value[ATTR_SIZE]) {
	int ret = select(sourceRelName, targetRelName, attribute, op, value);
	if (ret == SUCCESS) {
		cout << "Selected successfully, result in relation: ";
		print16(targetRelName);
	} else {
		printErrorMsg(ret);
		return FAILURE;
	}

	return SUCCESS;
}

int select_attr_from_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], int attr_count,
                             char attrs[][ATTR_SIZE]) {
	int ret = project(sourceRelName, targetRelName, attr_count, attrs);
	if (ret == SUCCESS) {
		cout << "Selected successfully, result in relation: ";
		print16(targetRelName);
	} else {
		printErrorMsg(ret);
		return FAILURE;
	}
	return SUCCESS;
}

int select_attr_from_where_handler(char sourceRelName[ATTR_SIZE], char targetRelName[ATTR_SIZE], int attr_count,
                                   char attrs[][ATTR_SIZE], char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]) {
	int ret = select(sourceRelName, TEMP, attribute, op, value);
	if (ret == SUCCESS) {
		int relid = openRel(TEMP);
		if (relid != E_RELNOTEXIST && relid != E_CACHEFULL) {
			int ret_project = project(TEMP, targetRelName, attr_count, attrs);
			if (ret_project == SUCCESS) {
				cout << "Selected successfully, result in relation: ";
				print16(targetRelName);
				closeRel(relid);
				deleteRel(TEMP);
			} else {
				closeRel(relid);
				deleteRel(TEMP);
				printErrorMsg(ret_project);
				return FAILURE;
			}
		} else {
			printErrorMsg(relid);
			return FAILURE;
		}
		return SUCCESS;
	} else {
		printErrorMsg(ret);
		return FAILURE;
	}
}

int select_attr_from_join_handler(char sourceRelOneName[ATTR_SIZE], char sourceRelTwoName[ATTR_SIZE],
                                  char targetRelName[ATTR_SIZE], int attrCount,
                                  char joinAttributeOne[ATTR_SIZE], char joinAttributeTwo[ATTR_SIZE],
                                  char attributeList[][ATTR_SIZE]) {

	int ret = join(sourceRelOneName, sourceRelTwoName, TEMP, joinAttributeOne, joinAttributeTwo);

	int relId;
	if (ret == SUCCESS) {

		relId = OpenRelTable::openRelation(TEMP);
		if (!(relId >= 0 && relId < MAX_OPEN)) {
			cout << "openRel Failed" << endl;
			deleteRel(TEMP);
			printErrorMsg(relId);
			return FAILURE;
		}

		int ret_project = project(TEMP, targetRelName, attrCount, attributeList);
		OpenRelTable::closeRelation(relId);
		deleteRel(TEMP);

		if (ret_project == SUCCESS) {
			cout << "Join successful" << endl;
			return SUCCESS;
		} else {
			printErrorMsg(ret_project);
			return FAILURE;
		}

	} else {
		printErrorMsg(ret);
		return FAILURE;
	}
}

void print16(char char_string_thing[ATTR_SIZE]) {
	for (int i = 0; i < ATTR_SIZE; i++) {
		if (char_string_thing[i] == '\0') {
			break;
		}
		cout << char_string_thing[i];
	}
	cout << endl;
}

void print16(char char_string_thing[ATTR_SIZE], bool newline) {
	for (int i = 0; i < ATTR_SIZE; i++) {
		if (char_string_thing[i] == '\0') {
			break;
		}
		cout << char_string_thing[i];
	}
	if (newline) {
		cout << endl;
	}
	return;
}

int executeCommandsFromFile(const string fileName) {
	const string filePath = BATCH_FILES_PATH;
	fstream commandsFile;
	commandsFile.open(filePath + fileName, ios::in);
	string command;
	vector<string> commands;
	if (commandsFile.is_open()) {
		while (getline(commandsFile, command)) {
			commands.push_back(command);
		}
	} else {
		cout << "The file " << fileName << " does not exist\n";
	}
	int lineNumber = 1;
	for (auto command: commands) {
		int ret = regexMatchAndExecute(command);
		if (ret == EXIT) {
			return EXIT;
		} else if (ret == FAILURE) {
			cout << "At line number " << lineNumber << endl;
			break;
		}
		lineNumber++;
	}
	return SUCCESS;
}

void display_help() {
	printf("fdisk \n\t -Format disk \n\n");
	printf("import <filename> \n\t -loads relations from the UNIX filesystem to the XFS disk. \n\n");
	printf("export <tablename> <filename>.csv \n\t -export a relation from XFS disk to UNIX file system. \n\n");
	printf("print table <tablename> \n\t-print all the rows of a relation in the XFS disk. \n\n");
	printf("ls \n\t  -list the names of all relations in the xfs disk. \n\n");
	printf("echo <any message> \n\t  -echo back the given string. \n\n");
	printf("run <filename> \n\t  -run commands from an input file in sequence. \n\n");
	printf("schema <tablename> \n\t-view the schema of a relation. \n\n");
	printf("print b+ tree tablename.attributename \n\t-print the b+ tree of an indexed attribute. \n\n");
	printf("export b+ blocks tablename.attributename <filename>.txt \n\t-export the data stored in the index blocks of an indexed attribute. \n\n");
	printf("dump bmap \n\t-dump the contents of the block allocation map.\n\n");
	printf("dump relcat \n\t-copy the contents of relation catalog to relationcatalog.txt\n \n");
	printf("dump attrcat \n\t-copy the contents of attribute catalog to an attributecatalog.txt. \n\n");
	printf("CREATE TABLE tablename(attr1_name attr1_type ,attr2_name attr2_type....); \n\t -create a relation with given attribute names\n \n");
	printf("DROP TABLE tablename;\n\t-delete the relation\n\n");
	printf("OPEN TABLE tablename;\n\t-open the relation \n\n");
	printf("CLOSE TABLE tablename;\n\t-close the relation \n\n");
	printf("CREATE INDEX ON tablename.attributename;\n\t-create an index on a given attribute.\n\n");
	printf("DROP INDEX ON tablename.attributename;\n\t-delete the index.\n\n");
	printf("ALTER TABLE RENAME tablename TO new_tablename;\n\t-rename an existing relation to a given new name.\n\n");
	printf("ALTER TABLE RENAME tablename COLUMN column_name TO new_column_name;\n\t-rename an attribute of an existing relation.\n\n");
	printf("INSERT INTO tablename VALUES ( value1,value2,value3,... );\n\t-insert a single record into the given relation. \n\n");
	printf("INSERT INTO tablename VALUES FROM filepath; \n\t-insert multiple records from a csv file \n\n");
	printf("SELECT * FROM source_relation INTO target_relation; \n\t-creates a relation with the same attributes and records as of source relation\n\n");
	printf("SELECT Attribute1,Attribute2,....FROM source_relation INTO target_relation;\n\t-creates a relation with attributes specified and all records\n\n");
	printf("SELECT * FROM source_relation INTO target_relation WHERE attrname OP value;\n\t-retrieve records based on a condition and insert them into a target relation\n\n");
	printf("SELECT Attribute1,Attribute2,....FROM source_relation INTO target_relation;\n\t-creates a relation with the attributes specified and inserts those records which satisfy the given condition.\n\n");
	printf("SELECT * FROM source_relation1 JOIN source_relation2 INTO target_relation WHERE source_relation1.attribute1 = source_relation2.attribute2;\n\t-creates a new relation with by equi-join of both the source relations\n\n");
	printf("SELECT Attribute1,Attribute2,.. FROM source_relation1 JOIN source_relation2 INTO target_relation WHERE source_relation1.attribute1 = source_relation2.attribute2;\n\t-creates a new relation by equi-join of both the source relations with the attributes specified \n\n");
	printf("exit \n\t-Exit the interface\n");
	return;
}

void printErrorMsg(int ret) {
	if (ret == FAILURE)
		cout << "Error: Command Failed" << endl;
	else if (ret == E_OUTOFBOUND)
		cout << "Error: Out of bound" << endl;
	else if (ret == E_FREESLOT)
		cout << "Error: Free slot" << endl;
	else if (ret == E_NOINDEX)
		cout << "Error: No index" << endl;
	else if (ret == E_DISKFULL)
		cout << "Error: Insufficient space in Disk" << endl;
	else if (ret == E_INVALIDBLOCK)
		cout << "Error: Invalid block" << endl;
	else if (ret == E_RELNOTEXIST)
		cout << "Error: Relation does not exist" << endl;
	else if (ret == E_RELEXIST)
		cout << "Error: Relation already exists" << endl;
	else if (ret == E_ATTRNOTEXIST)
		cout << "Error: Attribute does not exist" << endl;
	else if (ret == E_ATTREXIST)
		cout << "Error: Attribute already exists" << endl;
	else if (ret == E_CACHEFULL)
		cout << "Error: Cache is full" << endl;
	else if (ret == E_RELNOTOPEN)
		cout << "Error: Relation is not open" << endl;
	else if (ret == E_NATTRMISMATCH)
		cout << "Error: Mismatch in number of attributes" << endl;
	else if (ret == E_DUPLICATEATTR)
		cout << "Error: Duplicate attributes found" << endl;
	else if (ret == E_RELOPEN)
		cout << "Error: Relation is open" << endl;
	else if (ret == E_ATTRTYPEMISMATCH)
		cout << "Error: Mismatch in attribute type" << endl;
	else if (ret == E_INVALID)
		cout << "Error: Invalid index or argument" << endl;
	else if (ret == E_MAXRELATIONS)
		cout << "Error: Maximum number of relations already present" << endl;
    else if (ret == E_MAXATTRS)
        cout << "Error: Maximum number of attributes allowed for a relation is 125" << endl;
    else if (ret == E_RENAMETOTEMP)
        cout << "Error: Cannot rename a relation to 'temp'" << endl;
    else if (ret == E_CREATETEMP)
        cout << "Error: Cannot create relation named 'temp' as it is used for internal purposes" << endl;
    else if (ret == E_TARGETNAMETEMP)
        cout << "Error: Cannot create a target relation named 'temp' as it is used for internal purposes" << endl;

}

vector<string> extract_tokens(string input_command) {
	// tokenize with whitespace and brackets as delimiter
	vector<string> tokens;
	string token;
	for (int i = 0; i < input_command.length(); i++) {
		if (input_command[i] == '(' || input_command[i] == ')') {
			if (!token.empty()) {
				tokens.push_back(token);
			}
			token = "";
		} else if (input_command[i] == ',') {
			if (!token.empty()) {
				tokens.push_back(token);
			}
			token = "";
		} else if (input_command[i] == ' ' || input_command[i] == ';') {
			if (!token.empty()) {
				tokens.push_back(token);
			}
			token = "";
		} else {
			token += input_command[i];
		}
	}
	if (!token.empty())
		tokens.push_back(token);

	return tokens;
}

void string_to_char_array(string x, char *a, int size) {
	// Reducing size of string to the size provided
	int i;
	if (size == ATTR_SIZE - 1) {
		for (i = 0; i < x.size() && i < ATTR_SIZE - 1; i++)
			a[i] = x[i];
		a[i] = '\0';
	} else {
		for (i = 0; i < size; i++) {
			a[i] = x[i];
		}
		a[i] = '\0';
	}
}

int exportBPlusTreeBlocks(int blockNum, int attrType, FILE *fp_export) {
	HeadInfo header = getHeader(blockNum);
	int block_type = getBlockType(blockNum);
	int num_entries = header.numEntries;

	fprintf(fp_export, "BLOCK %d\n", blockNum);
	fprintf(fp_export, "Block Type: %s\n", block_type == 1 ? "IND_INTERNAL" : block_type == 2 ? "IND_LEAF" : "");
	fprintf(fp_export, "Parent Block: %d\n", header.pblock);
	fprintf(fp_export, "No of entries: %d\n", num_entries);

	if (block_type == IND_INTERNAL) {
		InternalEntry internal_entry;
		for (int iter = 0; iter < num_entries; iter++) {
			internal_entry = getInternalEntry(blockNum, iter);
			fprintf(fp_export, "lchild: %d, ", internal_entry.lChild);
			if (attrType == NUMBER) {
				fprintf(fp_export, "key_val: %.2f, ", internal_entry.attrVal.nval);
			} else {
				fprintf(fp_export, "key_val: %s, ", internal_entry.attrVal.sval);
			}
			fprintf(fp_export, "rchild: %d\n", internal_entry.rChild);
		}
	} else if (block_type == IND_LEAF) {
		fprintf(fp_export, "left node: %d, ", header.lblock);
		fprintf(fp_export, "right node: %d\n", header.rblock);
		for (int iter = 0; iter < num_entries; iter++) {
			Index index = getLeafEntry(blockNum, iter);
			if (attrType == NUMBER) {
				fprintf(fp_export, "key_val: %.2f\n", index.attrVal.nval);
			} else {
				fprintf(fp_export, "key_val: %s\n", index.attrVal.sval);
			}

		}
	}
	fputs("---------\n", fp_export);

	if (block_type == IND_INTERNAL) {
		InternalEntry internal_entry;

		int entry_num = 0;
		internal_entry = getInternalEntry(blockNum, entry_num);
		exportBPlusTreeBlocks(internal_entry.lChild, attrType, fp_export);

		for (entry_num = 0; entry_num < num_entries; entry_num++) {
			internal_entry = getInternalEntry(blockNum, entry_num);
			exportBPlusTreeBlocks(internal_entry.rChild, attrType, fp_export);
		}
	}

	return SUCCESS;
}

void printBPlusNode(int block, int attrType) {

	HeadInfo header = getHeader(block);
	int block_type = getBlockType(block);
	int num_entries = header.numEntries;

	if (block_type == IND_INTERNAL) {
		InternalEntry internal_entry;
		for (int iter = 0; iter < num_entries; iter++) {
			internal_entry = getInternalEntry(block, iter);
			if (attrType == NUMBER) {
				cout << internal_entry.attrVal.nval;
			} else {
				cout << internal_entry.attrVal.sval;
			}
			if (iter != num_entries - 1)
				cout << ",";
		}
	} else if (block_type == IND_LEAF) {
		for (int iter = 0; iter < num_entries; iter++) {
			Index index = getLeafEntry(block, iter);
			if (attrType == NUMBER) {
				cout << index.attrVal.nval;
			} else {
				cout << index.attrVal.sval;
			}
			if (iter != num_entries - 1)
				cout << ",";
		}
	}
}

void getBPlusBlocks(int rootBlock, queue<int> &bplus_blocks, vector<int> &children) {
	queue<int> blocks_tmp;
	blocks_tmp.push(rootBlock);

	while (!blocks_tmp.empty()) {
		int current_block = blocks_tmp.front();

		bplus_blocks.push(blocks_tmp.front());
		blocks_tmp.pop();

		HeadInfo header = getHeader(current_block);
		int block_type = getBlockType(current_block);
		int num_entries = header.numEntries;

		if (block_type == IND_INTERNAL) {
			children.push_back(num_entries + 1);

		}

		if (block_type == IND_INTERNAL) {
			InternalEntry internal_entry;

			int entry_num = 0;
			internal_entry = getInternalEntry(current_block, entry_num);
			blocks_tmp.push(internal_entry.lChild);

			for (entry_num = 0; entry_num < num_entries; entry_num++) {
				internal_entry = getInternalEntry(current_block, entry_num);
				blocks_tmp.push(internal_entry.rChild);
			}
		}
	}
}

void printBPlusTreeHelper(queue<int> bplus_blocks, vector<int> children, int attrType) {

	vector<int> noOfNodesInEachLevel;
	noOfNodesInEachLevel.push_back(1);
//	noOfNodesInEachLevel.push_back(children.front());
	int i = 0, k = 0;
	while (i < children.size()) {
		int nodes_in_current_level = noOfNodesInEachLevel[k];
		int nodes_in_next_level = 0;
		for (int j = 0; j < nodes_in_current_level; ++j) {
			nodes_in_next_level += children[i++];
		}
		noOfNodesInEachLevel[++k] = nodes_in_next_level;
	}

	int levels = k + 1;
	for (int current_level = 0; current_level < levels; ++current_level) {
		cout << "LEVEL " << current_level << endl;
		int nodesInCurrentLevel = noOfNodesInEachLevel[current_level];
		int nodesLeft = nodesInCurrentLevel;
		while (nodesLeft > 0) {
			int current_block = bplus_blocks.front();
			printBPlusNode(current_block, attrType);
			cout << "   ";
			bplus_blocks.pop();
			nodesLeft--;
		}
		cout << endl;
	}
}

void printBPlusTree(int rootBlock, int attrType) {
	queue<int> bplus_blocks;
	vector<int> children;
	getBPlusBlocks(rootBlock, bplus_blocks, children);
	cout << "----- B+ TREE -----" << endl;
	printBPlusTreeHelper(bplus_blocks, children, attrType);
}

int getRootBlock(char *rel_name, char *attr_name, int &attrType) {
	/* Get the Relation Catalog Entry */
	Attribute relNameAsAttribute;
	strcpy(relNameAsAttribute.sval, rel_name);
	Attribute relCatEntry[6];
	recId prev_recid, relcat_recid;
	prev_recid.block = -1;
	prev_recid.slot = -1;
	relcat_recid = linear_search(RELCAT_RELID, "RelName", relNameAsAttribute, EQ, &prev_recid);
	if (relcat_recid.block == -1 || relcat_recid.slot == -1) {
		return E_RELNOTEXIST;
	}
	getRecord(relCatEntry, relcat_recid.block, relcat_recid.slot);

	int no_of_attrs = (int) relCatEntry[RELCAT_NO_ATTRIBUTES_INDEX].nval;
	Attribute attrCatEntry[6];
	recId attrcat_recid;

	prev_recid.block = -1;
	prev_recid.slot = -1;
	int i;
	for (i = 0; i < no_of_attrs; i++) {
		attrcat_recid = linear_search(ATTRCAT_RELID, "RelName", relNameAsAttribute, EQ, &prev_recid);
		getRecord(attrCatEntry, attrcat_recid.block, attrcat_recid.slot);
		if (strcmp(attrCatEntry[ATTRCAT_ATTR_NAME_INDEX].sval, attr_name) == 0) {
			break;
		}
	}

	if (i == no_of_attrs) {
		return E_ATTRNOTEXIST;
	}

	int rootBlock = (int) attrCatEntry[ATTRCAT_ROOT_BLOCK_INDEX].nval;
	attrType = (int) attrCatEntry[ATTRCAT_ATTR_TYPE_INDEX].nval;
	if (rootBlock == -1) {
		return E_NOINDEX;
	}

	return rootBlock;
}

template <typename T>
void printTabular(T t, const int& width) {
	cout << left << setw(width) << setfill(' ') << t;
}

int printSchema(char relname[ATTR_SIZE]){
	Attribute relcat_rec[6];
	int numOfAttrs = -1;
	for (int slotNum = 0; slotNum < SLOTMAP_SIZE_RELCAT_ATTRCAT; slotNum++) {
		int retval = getRecord(relcat_rec, RELCAT_BLOCK, slotNum);
		if (retval == SUCCESS && strcmp(relcat_rec[0].sval, relname) == 0) {
			numOfAttrs = (int) relcat_rec[1].nval;
			break;
		}
	}

	if (numOfAttrs == -1) {
		cout << "The relation does not exist\n";
		return FAILURE;
	}

	Attribute rec[6];

	int recBlock_Attrcat = ATTRCAT_BLOCK;
	int nextRecBlock_Attrcat;

	// Array for attribute names and types
	int attrNo = 0;
	char attrName[numOfAttrs][ATTR_SIZE];
	int attrType[numOfAttrs];
	bool attrIndexed[numOfAttrs];

	/*
	 * Searching the Attribute Catalog Disk Blocks
	 * for finding and storing all the attributes of the given relation
	 */
	while (recBlock_Attrcat != -1) {
		HeadInfo headInfo = getHeader(recBlock_Attrcat);
		nextRecBlock_Attrcat = headInfo.rblock;
		for (int slotNum = 0; slotNum < SLOTMAP_SIZE_RELCAT_ATTRCAT; slotNum++) {
			int retval = getRecord(rec, recBlock_Attrcat, slotNum);
			if (retval == SUCCESS && strcmp(rec[0].sval, relname) == 0) {
				// Attribute belongs to this Relation - add info to array
				strcpy(attrName[attrNo], rec[1].sval);
				attrType[attrNo] = (int) rec[2].nval;
				attrIndexed[attrNo] = ((int)rec[4].nval) != -1;
				attrNo++;
			}
		}
		recBlock_Attrcat = nextRecBlock_Attrcat;
	}

	cout << "Relation: ";
	print16(relname);
	printTabular("Attribute", ATTR_SIZE + 1);
	printTabular("Type", 5);
	printTabular("Index", 5);
	cout << "\n---------------- ---- -----\n";
	for (int i = 0; i < numOfAttrs; ++i) {
		printTabular(attrName[i], ATTR_SIZE + 1);
		printTabular(attrType[i] == NUMBER ? "NUM" : "STR", 5);
		printTabular(attrIndexed[i] ? "yes" : "no", 5);
		cout << endl;
	}
	return SUCCESS;
}

int printRows(char relname[ATTR_SIZE]){
	HeadInfo headInfo;
	Attribute relcat_rec[6];

	int firstBlock, numOfAttrs;
	int slotNum;
	for (slotNum = 0; slotNum < SLOTMAP_SIZE_RELCAT_ATTRCAT; slotNum++) {
		int retval = getRecord(relcat_rec, RELCAT_BLOCK, slotNum);
		if (retval == SUCCESS && strcmp(relcat_rec[0].sval, relname) == 0) {
			firstBlock = (int) relcat_rec[3].nval;
			numOfAttrs = (int) relcat_rec[1].nval;
			break;
		}
	}

	if (slotNum == SLOTMAP_SIZE_RELCAT_ATTRCAT) {
		cout << "The relation does not exist\n";
		return FAILURE;
	}
	if (firstBlock == -1) {
		cout << "No records exist for the relation\n";
		return FAILURE;
	}

	Attribute rec[6];

	int recBlock_Attrcat = ATTRCAT_BLOCK;
	int nextRecBlock_Attrcat;

	// Array for attribute names and types
	int attrNo = 0;
	char attrName[numOfAttrs][ATTR_SIZE];
	int attrType[numOfAttrs];

	/*
	 * Searching the Attribute Catalog Disk Blocks
	 * for finding and storing all the attributes of the given relation
	 */
	while (recBlock_Attrcat != -1) {
		headInfo = getHeader(recBlock_Attrcat);
		nextRecBlock_Attrcat = headInfo.rblock;
		for (slotNum = 0; slotNum < SLOTMAP_SIZE_RELCAT_ATTRCAT; slotNum++) {
			int retval = getRecord(rec, recBlock_Attrcat, slotNum);
			if (retval == SUCCESS && strcmp(rec[0].sval, relname) == 0) {
				// Attribute belongs to this Relation - add info to array
				strcpy(attrName[attrNo], rec[1].sval);
				attrType[attrNo] = (int) rec[2].nval;
				attrNo++;
			}
		}
		recBlock_Attrcat = nextRecBlock_Attrcat;
	}

	// Write the Attribute names to console
	cout<<"| ";
	for (attrNo = 0; attrNo < numOfAttrs; attrNo++) {
		printTabular(attrName[attrNo], ATTR_SIZE-1);
		cout<<" | ";
	}

	cout << std::endl << "| ";

	for (attrNo = 0; attrNo < numOfAttrs; attrNo++) {
		printTabular("---------------", ATTR_SIZE-1);
		cout<<" | ";
	}
	cout << std::endl;

	int block_num = firstBlock;
	int num_slots;
	int num_attrs;

	/*
	 * Iterate over the record blocks of this relation
	 * Linked list traversal
	 */
	while (block_num != -1) {
		headInfo = getHeader(block_num);

		num_slots = headInfo.numSlots;
		num_attrs = headInfo.numAttrs;
		nextRecBlock_Attrcat = headInfo.rblock;

		unsigned char slotmap[num_slots];
		getSlotmap(slotmap, block_num);

		Attribute A[num_attrs];
		slotNum = 0;
		// Go through all slots and write the record entry to file
		for (slotNum = 0; slotNum < num_slots; slotNum++) {
			if (slotmap[slotNum] == SLOT_OCCUPIED) {
				getRecord(A, block_num, slotNum);

				cout << "| ";
				for (int l = 0; l < numOfAttrs; l++) {
					if (attrType[l] == NUMBER) {
						char s[ATTR_SIZE];
						sprintf(s, "%-15.2f", A[l].nval);
						printTabular(s, ATTR_SIZE - 1);

					} else if (attrType[l] == STRING) {
						printTabular(A[l].sval, ATTR_SIZE - 1);
					}
					cout << " | ";
				}

				cout << std::endl;
			}
		}

		block_num = nextRecBlock_Attrcat;
	}

	return SUCCESS;
}