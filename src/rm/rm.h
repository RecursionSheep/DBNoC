#pragma once

#include "../pf/pf.h"
#include "../Constants.h"

class RM_Manager {
public:
	RM_Manager(PF_Manager &pfm);
	~RM_Manager();
	
	RC CreateFile(const char *fileName, int recordSize);
	RC DestroyFile(const char *fileName);
	RC OpenFile(const char *fileName, RM_FileHandle &fileHandle);
	RC CloseFile(RM_FileHandle &fileHandle);
private:
	PF_Manager *_pfm;
}

class RM_FileHandle {
public:
	RM_FileHandle();
	~RM_FileHandle();
	
	RC GetRec(const RID &rid, RM_Record &rec) const;
	RC InsertRec(const char *pData, RID &rid);
	RC DeleteRec(const RID &rid);
	RC UpdateRec(const RM_Record &rec);
	RC ForcePages(PageNum pageNum = ALL_PAGES) const;
};

class RM_FileScan {
public:
	RM_FileScan();
	~RM_FileScan();
	
	RC OpenScan(const RM_FileHandle &fileHandle,
		AttrType attrType,
		int attrLength,
		int attrOffset,
		CompOp compOp,
		void *value,
		ClientHint pinHint = NO_HINT);
	RC GetNextRec(RM_Record &rec);
	RC CloseScan();
};


class RM_Record {
public:
	RM_Record();
	~RM_Record();
	
	RC GetData(char *&pData) const;
	RC GetRid(RID &rid) const;
};

class RID {
public:
	RID();
	~RID();
	
	RID(PageNum pageNum, SlotNum slotNum);
	RC GetPageNum(PageNum &pageNum) const;
	RC GetSlotNum(SlotNum &slotNum) const;
};
