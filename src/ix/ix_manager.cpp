#include "ix.h"
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>

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

bool IX_Manager::CreateIndex(const char *fileName, const char *attrName, AttrType attrType, int attrLen) {
	std::string indexName = std::string(fileName, fileName + strlen(fileName)) + "." + std::string(attrName, attrName + strlen(attrName));
	if (!fileManager->createFile(indexName.c_str())) return false;
	int fileID;
	if (!fileManager->openFile(indexName.c_str(), fileID)) return false;
	
	IX_FileHeader header;
	header.attrType = attrType;
	header.attrLen = attrLen;
	header.childNum = (PAGE_SIZE - sizeof(IX_PageHeader)) / (sizeof(int) * 3 + attrLen) - 1;
	//header.childNum = min(header.childNum, 5);
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
	bufPageManager->writeBack(index);
	
	IX_PageHeader pageHeader;
	pageHeader.isLeaf = true;
	pageHeader.keyNum = pageHeader.parent = 0;
	pageHeader.nextLeaf = pageHeader.prevLeaf = 0;
	int pageIndex;
	BufType pageBuf = bufPageManager->getPage(fileID, 1, pageIndex);
	memcpy(pageBuf, &pageHeader, sizeof(IX_PageHeader));
	bufPageManager->markDirty(pageIndex);
	bufPageManager->writeBack(pageIndex);
	
	//cout << "fileID: " << fileID << endl;
	fileManager->closeFile(fileID);
	return true;
}
bool IX_Manager::DestroyIndex(const char *fileName, const char *attrName) {
	bufPageManager->close();
	system(("rm " + std::string(fileName, fileName + strlen(fileName)) + "." + std::string(attrName, attrName + strlen(attrName))).c_str());
	return true;
}
bool IX_Manager::OpenIndex(const char *fileName, const char *attrName, int &fileID) {
	std::string indexName = string(fileName, fileName + strlen(fileName)) + "." + string(attrName, attrName + strlen(attrName));
	if (!fileManager->openFile(indexName.c_str(), fileID)) return false;
	//cout << "fileID: " << fileID << endl;
	return true;
}
bool IX_Manager::CloseIndex(int fileID) {
	bufPageManager->close();
	if (fileManager->closeFile(fileID)) return false;
	//cout << "fileID: " << fileID << endl;
	return true;
}

bool compareLess(void* data1, int page1, int slot1, void* data2, int page2, int slot2, AttrType attrType, int attrLen) {
	bool compareData1 = false, compareData2 = false;
	if (attrType == INTEGER) {
		compareData1 = *((int*)data1) < *((int*)data2);
		compareData2 = *((int*)data1) > *((int*)data2);
	} else if (attrType == FLOAT) {
		compareData1 = *((double*)data1) < *((double*)data2);
		compareData2 = *((double*)data1) > *((double*)data2);
	} else if (attrType == STRING) {
		/*for (int i = 0; i < attrLen; i++) {
			if (((char*)data1)[i] < ((char*)data2)[i]) {
				compareData1 = true; compareData2 = false;
				break;
			}
			if (((char*)data1)[i] > ((char*)data2)[i]) {
				compareData1 = false; compareData2 = true;
				break;
			}
		}*/
		//if (*((int*)data1) < *((int*)data2)) return true;
		//if (*((int*)data1) > *((int*)data2)) return false;
		int compareStr = memcmp(((char*)data1), ((char*)data2), attrLen);
		compareData1 = compareStr < 0;
		compareData2 = compareStr > 0;
	} else {
		fprintf(stderr, "Error: invalid types!\n");
	}
	if (compareData1) return true;
	if (compareData2) return false;
	if (page1 < page2) return true;
	if (page1 > page2) return false;
	return slot1 < slot2;
}
