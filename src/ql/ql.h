#pragma once

#include "../pf/pf.h"
#include "../rm/rm.h"
#include "../ix/ix.h"
#include "../sm/sm.h"
#include "../utils.h"
#include <vector>
#include <string>

struct Relation {
	string table1;
	string attr1;
	string table2;
	string attr2;
	BufType data;
	CompOp op;
};

struct Assign {
	string table;
	string attr;
	BufType value;
};

class QL_Manager {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	QL_Manager(SM_Manager *smm, IX_Manager *ixm, RM_Manager *rmm, FileManager *_fileManager, BufPageManager *_bufPageManager);
	~QL_Manager();
	
	void Insert(const string tableName, vector<string> attrs, vector<BufType> values);
	void Update(const Assign assign);
	void Delete(const string tableName, vector<Relation> relations);
	void Select(const string tableName, vector<Relation> relations, vector<string> attrNames);
	void Load(const string tableName, const string fileName);

	bool _compare(BufType data1, BufType data2, CompOp op, AttrType type);
private:
	SM_Manager *_smm;
	IX_Manager *_ixm;
	RM_Manager *_rmm;
};
