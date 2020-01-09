#pragma once

#include "../pf/pf.h"
#include "../rm/rm.h"
#include "../ix/ix.h"
#include <string>
#include <map>
#include <vector>
#include <set>

struct AttrInfo {
	string attrName;
	AttrType attrType;
	int attrLength;
	bool notNull;
	bool primary;
	BufType defaultValue;
	string reference;
	string foreignKeyName;
	bool haveIndex;
	int offset; //4 bytes
};

struct TableInfo {
	string tableName;
	int attrNum, foreignNum, recordSize;
	vector<int> primary;
	vector<AttrInfo> attrs;
	vector<string> foreign;
	set<string> foreignSet;
};

class SM_Manager {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	SM_Manager(IX_Manager *ixm, RM_Manager *rmm, FileManager *_fileManager, BufPageManager *_bufPageManager);
	~SM_Manager();
	
	void OpenDB(const string DBName);
	void CloseDB();
	void CreateTable(TableInfo* table);
	void DropTable(const string tableName);
	void CreateIndex(const string tableName, const vector<string> attrs);
	void DropIndex(const string tableName, const vector<string> attrs);
	/*void AddPrimaryKey(const char *relName, const char *attrName);
	void DropPrimaryKey(const char *relName);
	void AddForeignKey(const char *relName, const char *attrName, const char *refName, const char *refAttr, const char *conName);
	void DropForeignKey(const char *relName, const char *conName);
	void AddColumn(const char *relName, AttrInfo attr);
	void DropColumn(const char *relName, const char *attrName);
	void ChangeColumn(const char *relName, const char *attrName, AttrInfo attr);
	void Load(const char *relName, const char *fileName);
	void Print(const char *relName);
	void Set(const char *paramName, const char *value);*/
	
private:
	IX_Manager *_ixm;
	RM_Manager *_rmm;
	string _DBName;
	int _tableNum;
	std::vector<TableInfo> _tables;
	std::map<std::string, int> _tableFileID;
	
	bool _checkForeignKeyOnTable(int tableID);
	int _fromNameToID(const string tableName);
};
