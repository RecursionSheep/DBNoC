#include "ix.h"
#include <sstream>
#include <cstring>

IX_Manager::IX_Manager(FileManager *_fileManager, BufPageManager *_bufPageManager) {
	fileManager = _fileManager; bufPageManager = _bufPageManager;
}
IX_Manager::~IX_Manager() {}

bool IX_Manager::_GetIndexFileName(const char *fileName, int indexID, std::string &indexName) {
	std::stringstream convert;
	convert << indexID;
	std::string idx_num = convert.str();
	indexName = std::string(fileName);
	indexName.append(".");
	indexName.append(idx_num);
	return true;
}

bool IX_Manager::CreateIndex(const char *fileName, int indexID, AttrType attrType, int attrLen) {
	std::string indexName;
	_GetIndexFileName(fileName, indexID, indexName);
	if (!fileManager->createFile(indexName.c_str())) return false;
	int fileID;
	if (!fileManager->openFile(indexName.c_str(), fileID)) return false;
	
	IX_FileHeader header;
	header.attrType = attrType;
	header.attrLen = attrLen;
	header.childNum = (PAGE_SIZE - sizeof(IX_PageHeader)) / (sizeof(int) * 3 + attrLen) - 1;
	header.rootPage = 1;
	header.nodeNum = 2;
	header.childStart = sizeof(IX_PageHeader) / sizeof(int);
	header.pageStart = header.childStart + header.childNum;
	header.slotStart = header.pageStart + header.childNum;
	header.keyStart = header.slotStart + header.childNum;
	header.childNum--;
	int index;
	BufType buf = bufPageManager->getPage(fileID, 0, index);
	memcpy(buf, &header, sizeof(IX_FileHeader));
	bufPageManager->markDirty(index);
	//bufPageManager->writeBack(index);
	
	IX_PageHeader pageHeader;
	pageHeader.isLeaf = true;
	pageHeader.keyNum = pageHeader.parent = pageHeader.whichChild = 0;
	pageHeader.nextLeaf = pageHeader.prevLeaf = 0;
	int pageIndex;
	BufType pageBuf = bufPageManager->getPage(fileID, 1, pageIndex);
	memcpy(pageBuf, &pageHeader, sizeof(IX_PageHeader));
	bufPageManager->markDirty(pageIndex);
	//bufPageManager->writeBack(pageIndex);
	return true;
}
bool IX_Manager::DestroyIndex(const char *fileName, int indexID) {
	// todo
}
bool IX_Manager::OpenIndex(const char *fileName, int indexID, int &fileID) {
	std::string indexName;
	_GetIndexFileName(fileName, indexID, indexName);
	if (!fileManager->openFile(indexName.c_str(), fileID)) return false;
	return true;
}
bool IX_Manager::CloseIndex(int indexID, int fileID) {
	if (fileManager->closeFile(fileID)) return false;
	return true;
}

bool compareLess(void* data1, int page1, int slot1, void* data2, int page2, int slot2, AttrType attrType) {
	bool compareData1, compareData2;
	if (attrType == INTEGER) {
		compareData1 = *((int*)data1) < *((int*)data2);
		compareData2 = *((int*)data1) > *((int*)data2);
	} else if (attrType == FLOAT) {
		compareData1 = *((double*)data1) < *((double*)data2);
		compareData2 = *((double*)data1) > *((double*)data2);
	} else if (attrType == STRING) {
		int compareStr = strcmp((char*)data1, (char*)data2);
		compareData1 = compareStr < 0;
		compareData2 = compareStr > 0;
	}
	if (compareData1) return true;
	if (compareData2) return false;
	if (page1 < page2) return true;
	if (page1 > page2) return false;
	return slot1 < slot2;
}
