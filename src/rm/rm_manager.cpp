#include "rm.h"
#include <cstring>
#include <cstdlib>
#include <string>

RM_Manager::RM_Manager(FileManager *_fileManager, BufPageManager *_bufPageManager) {
	fileManager = _fileManager; bufPageManager = _bufPageManager;
}
RM_Manager::~RM_Manager() {}

bool RM_Manager::CreateFile(const char *fileName, int recordSize) {
	if (!fileManager->createFile(fileName)) return false;
	int fileID;
	if (!fileManager->openFile(fileName, fileID)) return false;
	RM_FileHeader header;
	while (recordSize % 4 != 0) recordSize++;
	header.recordSize = recordSize >> 2;
	header.recordNumPerPage = (PAGE_SIZE - sizeof(RM_PageHeader)) * 8 / (recordSize * 32 + 1) - 1;
	header.firstFreePage = 0;
	header.pageNumber = 1;
	header.bitmapSize = (header.recordNumPerPage - 1) / 32 + 1;
	header.bitmapStart = sizeof(RM_PageHeader) / sizeof(int);
	int index;
	BufType buf = bufPageManager->getPage(fileID, 0, index);
	memcpy(buf, &header, sizeof(RM_FileHeader));
	bufPageManager->markDirty(index);
	bufPageManager->writeBack(index);
	
	//cout << "fileID: " << fileID << endl;
	fileManager->closeFile(fileID);
	return true;
}
bool RM_Manager::DestroyFile(const char *fileName) {
	bufPageManager->close();
	system(("rm " + std::string(fileName, fileName + strlen(fileName))).c_str());
	return true;
}
bool RM_Manager::OpenFile(const char *fileName, int &fileID) {
	if (!fileManager->openFile(fileName, fileID)) return false;
	//cout << "fileID: " << fileID << endl;
	return true;
}
bool RM_Manager::CloseFile(int fileID) {
	bufPageManager->close();
	//cout << "fileID: " << fileID << endl;
	if (fileManager->closeFile(fileID)) return false;
	return true;
}
