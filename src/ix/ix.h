#pragma once

#include "../pf/pf.h"
#include "../utils.h"
#include <string>

struct IX_FileHeader {
	AttrType attrType;
	int attrLen;
	int childNum;
	int childStart, pageStart, slotStart, keyStart;
	int rootPage;
	int firstLeaf;
	int nodeNum;
};

struct IX_PageHeader {
	int isLeaf;
	int keyNum;
	int prevLeaf, nextLeaf;
	int whichChild;
	int parent;
};

struct IX_TreeNode {
public:
	IX_PageHeader *header;
	BufType child, page, slot;
	char *key;
	int index;
};

class IX_Manager {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	IX_Manager(FileManager *_fileManager, BufPageManager *_bufPageManager);
	~IX_Manager();
	
	bool CreateIndex(const char *fileName, int indexID, AttrType attrType, int attrLen);
	bool DestroyIndex(const char *fileName, int indexID);
	bool OpenIndex(const char *fileName, int indexID, int &fileID);
	bool CloseIndex(int indexID, int fileID);

private:
	bool _GetIndexFileName(const char *fileName, int indexID, std::string &indexName);
};

class IX_IndexHandle {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	IX_IndexHandle(FileManager *_fileManager, BufPageManager *_bufPageManager, int indexID, int fileID);
	~IX_IndexHandle();
	
	bool InsertEntry(void *pData, int pageID, int slotID);
	bool DeleteEntry(void *pData, int pageID, int slotID);
	
private:
	IX_TreeNode* _getNode(int id);
	void _writeBackNode(IX_TreeNode* node);
	
	int _indexID, _fileID;
	IX_FileHeader _header;
};

class IX_IndexScan {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	IX_IndexScan(FileManager *_fileManager, BufPageManager *_bufPageManager, int indexID, int fileID);
	~IX_IndexScan();
	
	bool OpenScan(CompOp compOp, void *value);
	bool GetNextEntry(int &pageID, int &slotID);
	bool CloseScan();
	
private:
	int _indexID, _fileID;
};
