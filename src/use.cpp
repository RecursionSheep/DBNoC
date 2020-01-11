#include "pf/pf.h"
#include "rm/rm.h"
#include "ix/ix.h"
#include "sm/sm.h"
#include "ql/ql.h"
#include "utils.h"

#include <string>
#include <iostream>
#include <cstring>
#include <cstdio>

using namespace std;

int pos;
string command;
char readChar() {
	if (pos == command.length()) {
		pos++;
		return '\0';
	}
	return command[pos++];
}
void backward() {
	if (pos > 0) pos--;
}
string readIdentifier() {
	string id = "";
	char c;
	while (1) {
		c = readChar();
		if (c == '\0') return "";
		if (c != ' ' && c != '\0' && c != ',' && c != '(' && c != ')') break;
	}
	id.push_back(c);
	while (1) {
		c = readChar();
		if (c != ' ' && c != '\0' && c != ',' && c != '(' && c != ')') id.push_back(c); else break;
	}
	backward();
	return id;
}
string readIdentifierOrCommaOrBracket() {
	string id = "";
	char c;
	while (1) {
		c = readChar();
		if (c == '\0' || c == ',' || c == '(' || c == ')') {
			id.push_back(c); return id;
		}
		if (c != ' ' && c != '\0') break;
	}
	id.push_back(c);
	while (1) {
		c = readChar();
		if (c != ' ' && c != '\0' && c != ',' && c != '(' && c != ')' && c != '=') id.push_back(c); else break;
	}
	backward();
	return id;
}
string readString() {
	string id = "";
	char c;
	while (1) {
		c = readChar();
		if (c == '\'') break;
	}
	while (1) {
		c = readChar();
		if (c == '\'') break; else id.push_back(c);
	}
	return id;
}
string readValue(AttrType *type) {
	string val = "";
	char c;
	while (1) {
		c = readChar();
		if (c == '\0') return "";
		if (c != ' ' && c != '\0' && c != ',' && c != '(') {
			break;
		}
	}
	if (c == ')') {
		backward();
		return ")";
	}
	if (c == '\'') {
		*type = STRING;
	} else {
		val.push_back(c);
		*type = INTEGER;
	}
	while (1) {
		c = readChar();
		if (*type == STRING) {
			if (c == '\'') break; else {
				val.push_back(c);
				continue;
			}
		}
		if (*type != STRING && (c == '.' || c == 'e')) *type = FLOAT;
		if (c != ' ' && c != '\0' && c != ',' && c != '(' && c != ')' && c != '\'') val.push_back(c); else break;
	}
	if (c != '\'') backward();
	return val;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " DBName\n";
		return 1;
	}
	MyBitMap::initConst();
	FileManager *fileManager = new FileManager();
	BufPageManager *bufPageManager = new BufPageManager(fileManager);
	RM_Manager *rmm = new RM_Manager(fileManager, bufPageManager);
	IX_Manager *ixm = new IX_Manager(fileManager, bufPageManager);
	SM_Manager *smm = new SM_Manager(ixm, rmm, fileManager, bufPageManager);
	QL_Manager *qlm = new QL_Manager(smm, ixm, rmm, fileManager, bufPageManager);
	
	smm->OpenDB(string(argv[1], argv[1] + strlen(argv[1])));
	while (1) {
		pos = 0;
		putchar('>'); putchar(' ');
		if (!getline(cin, command)) break;
		//cout << command << endl;
		string cur = readIdentifier();
		if (cur == "exit") {
			break;
		} else if (cur == "show") {
			smm->Show();
		} else if (cur == "create") {
			cur = readIdentifier();
			if (cur == "index") {
				string tableName = readIdentifier();
				vector<string> attrs;
				while (1) {
					string attrName = readIdentifier();
					if (attrName == "") break;
					attrs.push_back(attrName);
				}
				smm->CreateIndex(tableName, attrs);
			} else if (cur == "table") {
				string tableName = readIdentifier();
				TableInfo *tableInfo = new TableInfo(); tableInfo->attrNum = tableInfo->recordSize = 0;
				tableInfo->tableName = tableName;
				while (1) {
					string attrName = readIdentifier();
					if (attrName == "") break;
					AttrInfo attrInfo;
					attrInfo.attrName = attrName;
					attrInfo.notNull = attrInfo.primary = attrInfo.haveIndex = false;
					attrInfo.defaultValue = nullptr;
					string attrType = readIdentifier();
					if (attrType == "int") {
						attrInfo.attrType = INTEGER;
					} else if (attrType == "float") {
						attrInfo.attrType = FLOAT;
					} else if (attrType == "char") {
						attrInfo.attrType = STRING;
						string len = readIdentifier();
						attrInfo.attrLength = atoi(len.c_str());
						char c = readChar();
						while (c != ')') c = readChar();
					}
					string modifier;
					while (1) {
						modifier = readIdentifierOrCommaOrBracket();
						if (modifier == "," || modifier == "(" || modifier == ")") break;
						if (modifier == "not") {
							modifier = readIdentifier();
							if (modifier == "null") {
								attrInfo.notNull = true;
							}
						}
						if (modifier == "primary") {
							modifier = readIdentifier();
							if (modifier == "key") {
								attrInfo.primary = true;
							}
						}
						if (modifier == "references") {
							modifier = readIdentifier();
							attrInfo.reference = modifier;
							modifier = readIdentifier();
							attrInfo.foreignKeyName = modifier;
						}
						if (modifier == "default") {
							if (attrType == "int") {
								modifier = readIdentifier();
								int *d = new int; *d = atoi(modifier.c_str());
								attrInfo.defaultValue = (BufType)d;
							} else if (attrType == "float") {
								modifier = readIdentifier();
								double *d = new double; *d = atof(modifier.c_str());
								attrInfo.defaultValue = (BufType)d;
							} else if (attrType == "char") {
								modifier = readString();
								char *d = new char[attrInfo.attrLength];
								memset(d, 0, sizeof(char) * attrInfo.attrLength);
								memcpy(d, modifier.c_str(), min(attrInfo.attrLength - 1, (int)modifier.length()));
								attrInfo.defaultValue = (BufType)d;
							}
						}
					}
					tableInfo->attrs.push_back(attrInfo);
					tableInfo->attrNum = tableInfo->attrs.size();
				}
				smm->CreateTable(tableInfo);
				//delete tableInfo;
			}
		} else if (cur == "drop") {
			cur = readIdentifier();
			if (cur == "table") {
				cur = readIdentifier();
				smm->DropTable(cur);
			} else if (cur == "index") {
				string tableName = readIdentifier();
				vector<string> attrs;
				while (1) {
					string attrName = readIdentifier();
					if (attrName == "") break;
					attrs.push_back(attrName);
				}
				smm->DropIndex(tableName, attrs);
			}
		} else if (cur == "insert") {
			cur = readIdentifier();
			if (cur != "into") continue;
			string tableName = readIdentifier();
			vector<string> attrs;
			vector<BufType> datas;
			while (1) {
				string attr = readIdentifierOrCommaOrBracket();
				if (attr == "(") continue;
				if (attr == ")") break;
				if (attr == ",") continue;
				//cout << attr << endl;
				attrs.push_back(attr);
			}
			cur = readIdentifier();
			//cout << cur << endl;
			if (cur == "values") {
				while (1) {
					AttrType type;
					string value = readValue(&type);
					//cout << pos << endl;
					if (value == ")") break;
					//cout << value << endl;
					if (type == INTEGER) {
						int *d = new int; *d = atoi(value.c_str());
						datas.push_back((BufType)d);
					} else if (type == FLOAT) {
						double *d = new double; *d = atof(value.c_str());
						datas.push_back((BufType)d);
					} else if (type == STRING) {
						char *d = new char[value.length() + 1];
						memset(d, 0, sizeof(char) * (value.length() + 1));
						memcpy(d, value.c_str(), value.length());
						datas.push_back((BufType)d);
					}
				}
				qlm->Insert(tableName, attrs, datas);
			}
		} else if (cur == "update") {
			string tableName = readIdentifier();
			cur = readIdentifier();
			if (cur == "set") {
				string attr = readIdentifier();
				char c;
				while (1) {
					c = readChar();
					if (c == '=') break;
					if (c == '\0') break;
				}
				if (c == '=') {
					AttrType type;
					string value = readValue(&type);
					Assign assign;
					assign.table = tableName; assign.attr = attr;
					if (type == INTEGER) {
						int *d = new int; *d = atoi(value.c_str());
						assign.value = (BufType)d;
					} else if (type == FLOAT) {
						double *d = new double; *d = atof(value.c_str());
						assign.value = (BufType)d;
					} else if (type == STRING) {
						char *d = new char[value.length() + 1];
						memset(d, 0, sizeof(char) * (value.length() + 1));
						memcpy(d, value.c_str(), value.length());
						assign.value = (BufType)d;
					}
					qlm->Update(assign);
				}
			}
		} else if (cur == "delete") {
			cur = readIdentifier();
			if (cur != "from") continue;
			string tableName = readIdentifier();
			cur = readIdentifier();
			if (cur == "where") {
				vector<Relation> relations;
				while (1) {
					Relation relation;
					cur = readIdentifier();
					if (cur == "") break;
					if (cur == "and") continue;
					int pos = cur.find('.');
					if (pos == string::npos) {
						relation.table1 = tableName;
						relation.attr1 = cur;
					} else {
						relation.table1 = cur.substr(0, pos);
						relation.attr1 = cur.substr(pos + 1);
					}
					cur = readIdentifier();
					if (cur == "<") relation.op = LT_OP;
					else if (cur == ">") relation.op = GT_OP;
					else if (cur == "<=") relation.op = LE_OP;
					else if (cur == ">=") relation.op = GE_OP;
					else if (cur == "==") relation.op = EQ_OP;
					else if (cur == "<>") relation.op = NE_OP;
					char c;
					while (1) {
						c = readChar();
						if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) break;
						if (c == '\'') break;
						if (c >= '0' && c <= '9') break;
						if (c == '-') break;
					}
					backward();
					if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
						cur = readIdentifier();
						int pos = cur.find('.');
						if (pos == string::npos) {
							relation.table2 = tableName;
							relation.attr2 = cur;
						} else {
							relation.table2 = cur.substr(0, pos);
							relation.attr2 = cur.substr(pos + 1);
						}
						relation.data = nullptr;
					} else {
						AttrType type;
						string value = readValue(&type);
						if (value == ")") break;
						if (type == INTEGER) {
							int *d = new int; *d = atoi(value.c_str());
							relation.data = (BufType)d;
						} else if (type == FLOAT) {
							double *d = new double; *d = atof(value.c_str());
							relation.data = (BufType)d;
						} else if (type == STRING) {
							char *d = new char[value.length() + 1];
							memset(d, 0, sizeof(char) * (value.length() + 1));
							memcpy(d, value.c_str(), value.length());
							relation.data = (BufType)d;
						}
					}
					relations.push_back(relation);
				}
				qlm->Delete(tableName, relations);
			}
		} else if (cur == "select") {
			vector<string> attrs, tables;
			string attr;
			while (1) {
				attr = readIdentifier();
				//cout << attr << endl;
				if (attr == "from") break;
				attrs.push_back(attr);
			}
			if (attr == "from") {
				string table = readIdentifier();
				//cout << table << endl;
				tables.push_back(table);
				table = readIdentifierOrCommaOrBracket();
				//cout << table << endl;
				if (table == ",") {
					table = readIdentifier();
					//cout << table << endl;
					tables.push_back(table);
					table = readIdentifier();
					//cout << table << endl;
				}
				if (table == "where") {
					vector<Relation> relations;
					while (1) {
						Relation relation;
						cur = readIdentifier();
						//cout << cur << endl;
						if (cur == "") break;
						if (cur == "and") continue;
						int pos = cur.find('.');
						if (pos == string::npos) {
							relation.table1 = tables[0];
							relation.attr1 = cur;
						} else {
							relation.table1 = cur.substr(0, pos);
							relation.attr1 = cur.substr(pos + 1);
						}
						cur = readIdentifier();
						//cout << cur << endl;
						if (cur == "<") relation.op = LT_OP;
						else if (cur == ">") relation.op = GT_OP;
						else if (cur == "<=") relation.op = LE_OP;
						else if (cur == ">=") relation.op = GE_OP;
						else if (cur == "==") relation.op = EQ_OP;
						else if (cur == "<>") relation.op = NE_OP;
						char c;
						while (1) {
							c = readChar();
							if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) break;
							if (c == '\'') break;
							if (c >= '0' && c <= '9') break;
						}
						backward();
						if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
							cur = readIdentifier();
							//cout << cur << endl;
							int pos = cur.find('.');
							if (pos == string::npos) {
								relation.table2 = tables[0];
								relation.attr2 = cur;
							} else {
								relation.table2 = cur.substr(0, pos);
								relation.attr2 = cur.substr(pos + 1);
							}
							relation.data = nullptr;
						} else {
							AttrType type;
							string value = readValue(&type);
							//cout << value << endl;
							if (value == "") break;
							if (type == INTEGER) {
								int *d = new int; *d = atoi(value.c_str());
								relation.data = (BufType)d;
							} else if (type == FLOAT) {
								double *d = new double; *d = atof(value.c_str());
								relation.data = (BufType)d;
							} else if (type == STRING) {
								char *d = new char[value.length() + 1];
								memset(d, 0, sizeof(char) * (value.length() + 1));
								memcpy(d, value.c_str(), value.length());
								relation.data = (BufType)d;
							}
						}
						relations.push_back(relation);
					}
					if (tables.size() == 1) {
						qlm->Select(tables[0], relations, attrs);
					} else {
						qlm->Select(tables[0], tables[1], relations, attrs);
					}
				}
			}
		} else if (cur == "load") {
			string tableName = readIdentifier();
			string fileName = readIdentifier();
			qlm->Load(tableName, fileName);
		}
	}
	smm->CloseDB();
	return 0;
}
