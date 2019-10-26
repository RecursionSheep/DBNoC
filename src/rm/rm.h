#pragma once

#include "../pf/pf.h"
#include "../constants.h"

typedef int SlotNum;

class RM_FileHeader {
public:
	int recordSize, recordNumber;
	PageNum firstFreePage;
	int pageNumber;
	int bitmapStart, bitmapSize;
};

class RM_PageHeader {
public:
	PageNum nextFreePage;
	int recordNumber;
};

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
};

class RM_FileHandle {
public:
	RM_FileHandle();
	~RM_FileHandle();
	
	RC GetRec(const RID &rid, RM_Record &rec) const;
	RC InsertRec(const char *pData, RID &rid);
	RC DeleteRec(const RID &rid);
	RC UpdateRec(const RM_Record &rec);
	RC ForcePages(PageNum pageNum = ALL_PAGES) const;

private:
	class RM_FileHeader _fileHeader;
	PF_FileHandle _pfh;
	bool _fileOpened, _headerDirty;
	RC _getHeader(PF_PageHandle &ph, RM_PageHeader *&header, char *&bitmap);
	
	RC _resetBitmap(char *bitmap, int size);
	RC _setBit(char *bitmap, int size, int pos, bool bit);
	RC _getBit(char *bitmap, int size, int pos, bool &bit);
	RC _getFirstZero(char *bitmap, int size, int &pos);
	RC _getNextOne(char *bitmap, int size, int pos, int &ret);
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

private:
	bool _scanOpened;
	RM_FileHandle *_fileHandle;
	
};

class RM_Record {
public:
	RM_Record();
	~RM_Record();
	
	RC GetData(char *&pData) const;
	RC GetRid(RID &rid) const;
	RC SetData(char *data, int size, RID rid);
	
private:
	RID _rid;
	char *_data;
	int _size;
};

class RID {
public:
	RID();
	~RID();
	
	RID(PageNum pageNum, SlotNum slotNum): _pageNum(pageNum), _slotNum(slotNum);
	RC GetPageNum(PageNum &pageNum) const { pageNum = _pageNum; return OK_RC; };
	RC GetSlotNum(SlotNum &slotNum) const { slotNum = _slotNum; return OK_RC; };
	
	RC checkValid() const { return _pageNum > 0 && _slotNum >= 0 ? RC_OK & RM_INVALIDRID; }
	
	RID& operator =(const RID &rid) {
		if (this != &rid) {
			_pageNum = rid._pageNum;
			_slotNum = rid._slotNum;
		}
		return *this;
	}
	bool operator ==(const RID &rid) {
		return _pageNum == rid._pageNum && _slotNum == rid._slotNum;
	}
	
private:
	PageNum _pageNum = -1;
	SlotNum _slotNum = -1;
};

void RM_PrintError(RC rc);

#define RM_INVALIDRID           (START_RM_WARN + 0) // invalid RID
#define RM_BADRECORDSIZE        (START_RM_WARN + 1) // record size is invalid
#define RM_INVALIDRECORD        (START_RM_WARN + 2) // invalid record
#define RM_INVALIDBITOPERATION  (START_RM_WARN + 3) // invalid page header bit ops
#define RM_PAGEFULL             (START_RM_WARN + 4) // no more free slots on page
#define RM_INVALIDFILE          (START_RM_WARN + 5) // file is corrupt/not there
#define RM_INVALIDFILEHANDLE    (START_RM_WARN + 6) // filehandle is improperly set up
#define RM_INVALIDSCAN          (START_RM_WARN + 7) // scan is improperly set up
#define RM_ENDOFPAGE            (START_RM_WARN + 8) // end of a page
#define RM_EOF                  (START_RM_WARN + 9) // end of file 
#define RM_BADFILENAME          (START_RM_WARN + 10)
#define RM_LASTWARN             RM_BADFILENAME

#define RM_ERROR                (START_RM_ERR - 0) // error
#define RM_LASTERROR            RM_ERROR
