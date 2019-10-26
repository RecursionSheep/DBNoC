#include "rm.h"

RM_Manager::RM_Manager(PF_Manager &pfm) { _pfm = &pfm; }
RM_Manager::~RM_Manager() {}

RC CreateFile(const char *fileName, int recordSize) {
	int createStatus = _pfm->CreateFile(fileName);
	if (createStatus != OK_RC) return createStatus;
	if (recordSize <= 0 || recordSize > PF_PAGE_SIZE) return RM_BADRECORDSIZE;
	
}
RC DestroyFile(const char *fileName) {
	_pfm->DestroyFile(fileName);
}
RC OpenFile(const char *fileName, RM_FileHandle &fileHandle) {
}
RC CloseFile(RM_FileHandle &fileHandle) {
}
