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
	if (pos == command.length()) return '\0';
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
		if (c == '\0') return "";
		if (c == ',') return ",";
		if (c == '(') return "(";
		if (c == ')') return ")";
		if (c != ' ' && c != '\0') break;
	}
	id.push_back(c);
	while (1) {
		c = readChar();
		if (c != ' ' && c != '\0' && c != ',' && c != '(' && c != ')') id.push_back(c); else break;
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
		getline(cin, command);
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
							} else {
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
		}
	}
	smm->CloseDB();
	return 0;
}
