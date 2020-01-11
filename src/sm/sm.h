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
	int attrLength; // bytes, completed by SM for INTEGER & FLOAT
	bool notNull;
	bool primary;
	BufType defaultValue;
	string reference;
	string foreignKeyName;
	bool haveIndex;
	int offset; // 4 bytes, completed by SM
};

struct TableInfo {
	string tableName;
	int attrNum, foreignNum; // foreignNum completed by SM
	int recordSize, primarySize; // bytes, completed by SM
	vector<int> primary; // completed by SM
	vector<AttrInfo> attrs;
	vector<string> foreign; // completed by SM
	set<string> foreignSet; // completed by SM
};

class SM_Manager {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	SM_Manager(IX_Manager *ixm, RM_Manager *rmm, FileManager *_fileManager, BufPageManager *_bufPageManager);
	~SM_Manager();
	
	// each record is started with 8 bytes containing the NULL bitmap. 1 represents not null and 0 represents null.
	void OpenDB(const string DBName);
	void CloseDB();
	void Show();
	void CreateTable(TableInfo* table);
	void DropTable(const string tableName);
	void CreateIndex(const string tableName, const vector<string> attrs);
	void DropIndex(const string tableName, const vector<string> attrs);
	void AddPrimaryKey(const string tableName, const vector<string> attrs);
	void DropPrimaryKey(const string tableName);
	/*void AddForeignKey(const char *relName, const char *attrName, const char *refName, const char *refAttr, const char *conName);
	void DropForeignKey(const char *relName, const char *conName);
	void AddColumn(const char *relName, AttrInfo attr);
	void DropColumn(const char *relName, const char *attrName);
	void ChangeColumn(const char *relName, const char *attrName, AttrInfo attr);*/
	
	bool _checkForeignKeyOnTable(int tableID);
	int _fromNameToID(const string tableName);
	int _fromNameToID(const string attrName, const int tableID);
	BufType _getPrimaryKey(int tableID, BufType data);
	string _DBName;
	int _tableNum;
	std::vector<TableInfo> _tables;
	std::map<std::string, int> _tableFileID;
private:
	IX_Manager *_ixm;
	RM_Manager *_rmm;
};
