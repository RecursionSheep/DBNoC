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
string readIdentifierOrComma() {
	string id = "";
	char c;
	while (1) {
		c = readChar();
		if (c == '\0') return "";
		if (c == ',') return ",";
		if (c != ' ' && c != '\0' && c != '(' && c != ')') break;
	}
	id.push_back(c);
	while (1) {
		c = readChar();
		if (c != ' ' && c != '\0' && c != ',' && c != '(' && c != ')') id.push_back(c); else break;
	}
	backward();
	return id;
}


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " DBName\n";
		return 1;
	}
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
		} else if (cur == "create") {
			while ((c = readChar()) == ' ');
			cur = ""; cur.push_back(c);
			while (1) {
				c = readChar();
				if (c != ' ' && c != '\0') cur.push_back(c); else break;
			}
			if (cur == "index") {
				string tableName = readIdentifier();
				vector<string> attrs;
				while (1) {
					string attrName == readIdentifier();
					if (attrName == "") break;
					attrs.push_back(attrName);
				}
				smm->CreateIndex(tableName, attrs);
			} else if (cur == "table") {
				string tableName = readIdentifier();
				TableInfo *tableInfo = new TableInfo(); tableInfo->attrNum = tableInfo->recordSize = 0;
				while (1) {
					string attrName = readIdentifier();
					if (attrName == "") break;
					AttrInfo attrInfo;
					attrInfo.attrName = attrName;
					string attrType = readIdentifier();
					if (attrType == "INT")
						attrInfo.attrType = INTEGER;
					else if (attrType == "FLOAT")
						attrInfo.attrType = FLOAT;
					else if (attrType == "CHAR") {
						attrInfo.attrType = STRING;
						string len = readIdentifier();
						attrInfo.attrLength = atoi(len.c_str());
					}
					
				}
			}
		}
	}
	smm->CloseDB();
	delete 
	return 0;
}
