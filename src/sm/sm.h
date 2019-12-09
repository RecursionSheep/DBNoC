#pragma once

#include "../rm/rm.h"
#include "../ix/ix.h"
#include <string>
#include <map>

struct AttrInfo {
	char *attrName;
	AttrType attrType;
	int attrLength;
	bool primary;
	char *defaultValue;
	char *reference;
	char *foreignKeyName;
};

class SM_Manager {
public:
	SM_Manager(IX_Manager &ixm, RM_Manager &rmm);
	~SM_Manager();
	
    void OpenDB(const char *DBName);
    void CloseDB();
    void CreateTable(const char *relName, int attrCount, AttrInfo *attr);
    void DropTable(const char *relName);
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
	RM_FileHandle *_metaFileHandle;
	char *_DBName;
	int _metaFileID;
	std::map<std::string, int> _tableFileID;
};
