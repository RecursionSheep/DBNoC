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
	IX_PageHeader header;
	BufType data, child, page, slot;
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
	
	IX_TreeNode* _getNode(int id);
private:
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
	
	bool OpenScan(void *pData, bool lower);
	bool GetPrevEntry(int &pageID, int &slotID);
	bool GetNextEntry(int &pageID, int &slotID);
	bool CloseScan();
	
	int _nodeID, _entry;
	IX_TreeNode* _getNode(int id);
private:
	void _writeBackNode(IX_TreeNode* node);
	
	IX_FileHeader _header;
	int _indexID, _fileID;
};

bool compareLess(void* data1, int page1, int slot1, void* data2, int page2, int slot2, AttrType attrType);
