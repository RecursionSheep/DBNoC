#pragma once

#include "../rm/rm.h"
#include "../ix/ix.h"
#include <string>
#include <map>
#include <vector>

struct AttrInfo {
	std::string attrName;
	AttrType attrType;
	int attrLength;
	bool notNull;
	bool primary;
	BufType defaultValue;
	std::string reference;
	std::string foreignKeyName;
	bool haveIndex;
};

struct TableInfo {
	std::string tableName;
	int attrNum, recordSize;
	vector<AttrInfo> attrs;
};

class SM_Manager {
public:
	SM_Manager(IX_Manager &ixm, RM_Manager &rmm);
	~SM_Manager();
	
	void OpenDB(const string DBName);
	void CloseDB();
	void CreateTable(const TableInfo* table);
	void DropTable(const string tableName);
	void CreateIndex(const char *relName, const char *attrName);
	void DropIndex(const char *relName, const char *attrName);
	void AddPrimaryKey(const char *relName, const char *attrName);
	void DropPrimaryKey(const char *relName);
	void AddForeignKey(const char *relName, const char *attrName, const char *refName, const char *refAttr, const char *conName);
	void DropForeignKey(const char *relName, const char *conName);
	void AddColumn(const char *relName, AttrInfo attr);
	void DropColumn(const char *relName, const char *attrName);
	void ChangeColumn(const char *relName, const char *attrName, AttrInfo attr);
	void Load(const char *relName, const char *fileName);
	void Print(const char *relName);
	void Set(const char *paramName, const char *value);
	
private:
	IX_Manager *_ixm;
	RM_Manager *_rmm;
	string _DBName;
	int _tableNum;
	std::vector<TableInfo> _tables;
	std::map<std::string, int> _tableFileID;
};
