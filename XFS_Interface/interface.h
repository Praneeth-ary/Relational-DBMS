#ifndef NITCBASE_INTERFACE_H
#define NITCBASE_INTERFACE_H

#include <regex>

/* External File System Commands */
std::regex help("\\s*HELP\\s*;?", std::regex_constants::icase);
std::regex fdisk("\\s*FDISK\\s*;?", std::regex_constants::icase);
std::regex dump_rel("\\s*DUMP\\s+RELCAT\\s*;?", std::regex_constants::icase);
std::regex dump_attr("\\s*DUMP\\s+ATTRCAT\\s*;?", std::regex_constants::icase);
std::regex dump_bmap("\\s*DUMP\\s+BMAP\\s*;?", std::regex_constants::icase);
std::regex imp("\\s*IMPORT\\s+([a-zA-Z0-9_-]+\\.csv)\\s*;?", std::regex_constants::icase);
std::regex exprt("\\s*EXPORT\\s+([A-Za-z0-9_-]+)\\s+([a-zA-Z0-9_-]+\\.csv)\\s*;?", std::regex_constants::icase);
std::regex schema("\\s*SCHEMA\\s+([A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex list_all("\\s*LS\\s*;?", std::regex_constants::icase);
std::regex ex("\\s*EXIT\\s*\\s*;?", std::regex_constants::icase);
std::regex run("\\s*RUN\\s+([a-zA-Z0-9_/.-]+)\\s*;?", std::regex_constants::icase);
std::regex echo("\\s*ECHO\\s+([a-zA-Z0-9 _,()'?:+*.-]+)\\s*;?", std::regex_constants::icase);
std::regex bplus_tree("\\s*PRINT\\s+B\\+\\s+TREE\\s+([A-Za-z0-9_-]+)\\s*\\.\\s*([A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex print_table("\\s*PRINT\\s+TABLE\\s+([A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex bplus_blocks("\\s*EXPORT\\s+B\\+\\s+BLOCKS\\s+([A-Za-z0-9_-]+)\\s*\\.\\s*([A-Za-z0-9_-]+)\\s+([a-zA-Z0-9_-]+\\.txt)\\s*;?", std::regex_constants::icase);
// std::regex run("\\s*RUN\\s+([a-zA-Z0-9_-]+\\.txt)\\s*;?", std::regex_constants::icase); // IF WE NEED .txt in run file name

/* DDL Commands*/
std::regex create_table("\\s*CREATE\\s+TABLE\\s+([A-Za-z0-9_-]+)\\s*\\(\\s*([#A-Za-z0-9_-]+\\s+(STR|NUM),[ ]*\\s*)*([#A-Za-z0-9_-]+\\s+(STR|NUM))\\s*\\)\\s*;?", std::regex_constants::icase);
std::regex drop_table("\\s*DROP\\s+TABLE\\s+([A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex open_table("\\s*OPEN\\s+TABLE\\s+([A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex close_table("\\s*CLOSE\\s+TABLE\\s+([A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex create_index("\\s*CREATE\\s+INDEX\\s+ON\\s+([A-Za-z0-9_-]+)\\s*\\.\\s*([#A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex drop_index("\\s*DROP\\s+INDEX\\s+ON\\s+([A-Za-z0-9_-]+)\\s*\\.\\s*([#A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex rename_table("\\s*ALTER\\s+TABLE\\s+RENAME\\s+([a-zA-Z0-9_-]+)\\s+TO\\s+([a-zA-Z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex rename_column("\\s*ALTER\\s+TABLE\\s+RENAME\\s+([a-zA-Z0-9_-]+)\\s+COLUMN\\s+([#a-zA-Z0-9_-]+)\\s+TO\\s+([#a-zA-Z0-9_-]+)\\s*;?", std::regex_constants::icase);

/* DML Commands */
std::regex select_from("\\s*SELECT\\s+\\*\\s+FROM\\s+([A-Za-z0-9_-]+)\\s+INTO\\s+([A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex select_attr_from("\\s*SELECT\\s+((?:[#A-Za-z0-9_-]+\\s*,\\s*)*[#A-Za-z0-9_-]+)\\s+FROM\\s+([A-Za-z0-9_-]+)\\s+INTO\\s+([A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex select_from_where("\\s*SELECT\\s+\\*\\s+FROM\\s+([A-Za-z0-9_-]+)\\s+INTO\\s+([A-Za-z0-9_-]+)\\s+WHERE\\s+([#A-Za-z0-9_-]+)\\s*(<|<=|>|>=|=|!=)\\s*([A-Za-z0-9_-]+|([0-9]+(\\.)[0-9]+))\\s*;?", std::regex_constants::icase);
std::regex select_attr_from_where("\\s*SELECT\\s+((?:[#A-Za-z0-9_-]+\\s*,\\s*)*[#A-Za-z0-9_-]+)\\s+FROM\\s+([A-Za-z0-9_-]+)\\s+INTO\\s+([A-Za-z0-9_-]+)\\s+WHERE\\s+([#A-Za-z0-9_-]+)\\s*(<|<=|>|>=|=|!=)\\s*([A-Za-z0-9_-]+|([0-9]+(\\.)[0-9]+))\\s*;?", std::regex_constants::icase);
std::regex select_from_join("\\s*SELECT\\s+\\*\\s+FROM\\s+([A-Za-z0-9_-]+)\\s+JOIN\\s+([A-Za-z0-9_-]+)\\s+INTO\\s+([A-Za-z0-9_-]+)\\s+WHERE\\s+([A-Za-z0-9_-]+)\\s*\\.([#A-Za-z0-9_-]+)\\s*\\=\\s*([A-Za-z0-9_-]+)\\s*\\.([#A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex select_attr_from_join("\\s*SELECT\\s+((?:[#A-Za-z0-9_-]+\\s*,\\s*)*[#A-Za-z0-9_-]+)\\s+FROM\\s+([A-Za-z0-9_-]+)\\s+JOIN\\s+([A-Za-z0-9_-]+)\\s+INTO\\s+([A-Za-z0-9_-]+)\\s+WHERE\\s+([A-Za-z0-9_-]+)\\s*\\.([#A-Za-z0-9_-]+)\\s*\\=\\s*([A-Za-z0-9_-]+)\\s*\\.([#A-Za-z0-9_-]+)\\s*;?", std::regex_constants::icase);
std::regex insert_single("\\s*INSERT\\s+INTO\\s+([A-Za-z0-9_-]+)\\s+VALUES\\s*\\(\\s*(([A-Za-z0-9_-]+|[0-9]+\\.[0-9]+)\\s*,\\s*)*([A-Za-z0-9_-]+|[0-9]+\\.[0-9]+)\\s*\\)\\s*;?", std::regex_constants::icase);
std::regex insert_multiple("\\s*INSERT\\s+INTO\\s+([A-Za-z0-9_-]+)\\s+VALUES\\s+FROM\\s+([a-zA-Z0-9_-]+\\.csv)\\s*;?", std::regex_constants::icase);

std::regex temp("\\((.*)\\)");

#endif  // NITCBASE_INTERFACE_H
