#pragma once

#include "../pf/pf.h"
#include "../utils.h"

struct RM_FileHeader {
	int recordSize, recordNumPerPage;
	int firstFreePage;
	int pageNumber;
	int bitmapStart, bitmapSize;
	int recordStart;
};

struct RM_PageHeader {
	int nextFreePage;
	int recordNum;
};

class RM_Manager {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	RM_Manager(FileManager *_fileManager, BufPageManager *_bufPageManager);
	~RM_Manager();
	
	bool CreateFile(const char *fileName, int recordSize);
	bool DestroyFile(const char *fileName);
	bool OpenFile(const char *fileName, int &fileID);
	bool CloseFile(int fileID);
};

class RM_FileHandle {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	RM_FileHandle(FileManager *_fileManager, BufPageManager *_bufPageManager, int fileID);
	~RM_FileHandle();
	
	bool GetRec(int pageID, int slotID, BufType data) const;
	bool InsertRec(int &pageID, int &slotID, const BufType data);
	bool DeleteRec(int pageID, int slotID);
	bool UpdateRec(int pageID, int slotID, const BufType data);
	
	int _fileID;
	RM_FileHeader _header;
	
	void _resetBitmap(uint *bitmap, int size);
	void _setBit(uint *bitmap, int size, int pos, bool bit);
	bool _getBit(uint *bitmap, int size, int pos) const;
	int _getFirstZero(uint *bitmap, int size) const;
	int _getNextOne(uint *bitmap, int size, int pos) const;
};

class RM_FileScan {
public:
	FileManager *fileManager;
	BufPageManager *bufPageManager;
	
	RM_FileScan(FileManager *_fileManager, BufPageManager *_bufPageManager);
	~RM_FileScan();
	
	bool OpenScan(RM_FileHandle *filehandle);
	bool GetNextRecord(int &pageID, int &slotID, BufType data);

private:
	int _pageID, _slotID, _fileID;
	RM_FileHandle *_filehandle;
};
