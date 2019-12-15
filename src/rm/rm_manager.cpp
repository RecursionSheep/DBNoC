#include "rm.h"
#include <cstring>

RM_Manager::RM_Manager(FileManager *_fileManager, BufPageManager *_bufPageManager) {
	fileManager = _fileManager; bufPageManager = _bufPageManager;
}
RM_Manager::~RM_Manager() {}

bool RM_Manager::CreateFile(const char *fileName, int recordSize) {
	if (!fileManager->createFile(fileName)) return false;
	int fileID;
	if (!fileManager->openFile(fileName, fileID)) return false;
	RM_FileHeader header;
	header.recordSize = recordSize;
	header.recordNumPerPage = (PAGE_SIZE - sizeof(RM_PageHeader)) * 8 / (recordSize * 32 + 1) - 1;
	header.firstFreePage = 0;
	header.pageNumber = 1;
	header.bitmapSize = (header.recordNumPerPage - 1) / 32 + 1;
	header.bitmapStart = sizeof(RM_PageHeader) / sizeof(int);
	int index;
	BufType buf = bufPageManager->getPage(fileID, 0, index);
	memcpy(buf, &header, sizeof(RM_FileHeader));
	bufPageManager->markDirty(index);
	//bufPageManager->writeBack(index);
	fileManager->closeFile(fileID);
	return true;
}
bool RM_Manager::DestroyFile(const char *fileName) {
	// todo
}
bool RM_Manager::OpenFile(const char *fileName, int &fileID) {
	if (!fileManager->openFile(fileName, fileID)) return false;
	return true;
}
bool RM_Manager::CloseFile(int fileID) {
	if (fileManager->closeFile(fileID)) return false;
	return true;
}
