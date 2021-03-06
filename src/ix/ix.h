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
	//int whichChild;
	int parent;
};

struct IX_TreeNode {
public:
	IX_PageHeader header;
	BufType data, child, page, slot;
	unsigned char *key;
	int index;
};

class IX_Manager {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	IX_Manager(FileManager *_fileManager, BufPageManager *_bufPageManager);
	~IX_Manager();
	
	// attrLen Bytes
	bool CreateIndex(const char *fileName, const char *attrName, AttrType attrType, int attrLen);
	bool DestroyIndex(const char *fileName, const char *attrName);
	bool OpenIndex(const char *fileName, const char *attrName, int &fileID);
	bool CloseIndex(int fileID);

private:
	bool _GetIndexFileName(const char *fileName, int indexID, std::string &indexName);
};

class IX_IndexHandle {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	IX_IndexHandle(FileManager *_fileManager, BufPageManager *_bufPageManager, int fileID);
	~IX_IndexHandle();
	
	bool InsertEntry(void *pData, int pageID, int slotID);
	bool DeleteEntry(void *pData, int pageID, int slotID);
	bool CheckEntry(void *pData);
	
	IX_TreeNode* _getNode(int id);
private:
	void _writeBackNode(IX_TreeNode* node);
	int _whichChild(int child, IX_TreeNode* parent);
	
	int _fileID;
	IX_FileHeader _header;
};

class IX_IndexScan {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	IX_IndexScan(FileManager *_fileManager, BufPageManager *_bufPageManager, int fileID);
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
	int _fileID;
};

bool compareLess(void* data1, int page1, int slot1, void* data2, int page2, int slot2, AttrType attrType, int attrLen);
