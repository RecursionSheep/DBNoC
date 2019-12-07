#include "rm.h"
#include <cstring>

RM_FileHandle::RM_FileHandle(FileManager *_fileManager, BufPageManager *_bufPageManager, int fileID) {
	fileManager = _fileManager; bufPageManager = _bufPageManager;
	_fileID = fileID;
	int index;
	BufType data = bufPageManager->getPage(fileID, 0, index);
	memcpy(&_header, data, sizeof(RM_FileHeader));
}
RM_FileHandle::~RM_FileHandle() {}

bool RM_FileHandle::GetRec(int pageID, int slotID, BufType data) const {
	if (pageID >= _header.pageNumber) return false;
	int index;
	BufType buf = bufPageManager->getPage(_fileID, pageID, index);
	buf += _header.bitmapStart;
	if (!_getBit(buf, _header.recordNumPerPage, slotID)) return false;
	buf += _header.bitmapSize + slotID * _header.recordSize;
	memcpy(data, buf, _header.recordSize * sizeof(uint));
	//printf("get: %d\n", data[0]);
	return true;
}
bool RM_FileHandle::InsertRec(int &pageID, int &slotID, const BufType data) {
	//printf("insert: %d\n", data[0]);
	if (_header.firstFreePage == 0) {
		_header.firstFreePage = _header.pageNumber++;
		int index0;
		BufType page0 = bufPageManager->getPage(_fileID, 0, index0);
		memcpy(page0, &_header, sizeof(RM_FileHeader));
		bufPageManager->markDirty(index0);
		bufPageManager->writeBack(index0);
		page0 = bufPageManager->getPage(_fileID, _header.firstFreePage, index0);
		memset(page0, 0, PAGE_SIZE);
		bufPageManager->markDirty(index0);
		bufPageManager->writeBack(index0);
	}
	int index;
	pageID = _header.firstFreePage;
	BufType buf = bufPageManager->getPage(_fileID, pageID, index);
	BufType bitmap = buf + _header.bitmapStart;
	slotID = _getFirstZero(bitmap, _header.recordNumPerPage);
	BufType record = bitmap + _header.bitmapSize + slotID * _header.recordSize;
	memcpy(record, data, _header.recordSize * sizeof(uint));
	_setBit(bitmap, _header.recordNumPerPage, slotID, 1);
	if (++buf[1] == _header.recordNumPerPage) _header.firstFreePage = buf[0];
	bufPageManager->markDirty(index);
	bufPageManager->writeBack(index);
	int index0;
	BufType page0 = bufPageManager->getPage(_fileID, 0, index0);
	memcpy(page0, &_header, sizeof(RM_FileHeader));
	bufPageManager->markDirty(index0);
	bufPageManager->writeBack(index0);
	return true;
}
bool RM_FileHandle::DeleteRec(int pageID, int slotID) {
	int index;
	BufType buf = bufPageManager->getPage(_fileID, pageID, index);
	BufType bitmap = buf + _header.bitmapStart;
	if (!_getBit(bitmap, _header.recordNumPerPage, slotID)) return false;
	_setBit(bitmap, _header.recordNumPerPage, slotID, 0);
	if (buf[1]-- == _header.recordNumPerPage) {
		buf[0] = _header.firstFreePage;
		_header.firstFreePage = pageID;
		int index0;
		BufType page0 = bufPageManager->getPage(_fileID, 0, index0);
		memcpy(page0, &_header, sizeof(RM_FileHeader));
		bufPageManager->markDirty(index0);
		bufPageManager->writeBack(index0);
	}
	bufPageManager->markDirty(index);
	bufPageManager->writeBack(index);
	return true;
}
bool RM_FileHandle::UpdateRec(int pageID, int slotID, const BufType data) {
	int index;
	BufType buf = bufPageManager->getPage(_fileID, pageID, index);
	BufType bitmap = buf + _header.bitmapStart;
	if (!_getBit(bitmap, _header.recordNumPerPage, slotID)) return false;
	BufType record = bitmap + _header.bitmapSize + slotID * _header.recordSize;
	memcpy(record, data, _header.recordSize * sizeof(uint));
	bufPageManager->markDirty(index);
	bufPageManager->writeBack(index);
	return true;
}

void RM_FileHandle::_resetBitmap(uint *bitmap, int size) {
	int len = (size - 1) / 32 + 1;
	if (size == 0) len = 0;
	for (int i = 0; i < len; i++) bitmap[i] = 0;
}
void RM_FileHandle::_setBit(uint *bitmap, int size, int pos, bool bit) {
	if (bit) bitmap[pos >> 5] |= 1 << (pos & 31); else bitmap[pos >> 5] &= ~(1 << (pos & 31));
}
bool RM_FileHandle::_getBit(uint *bitmap, int size, int pos) const {
	return (bitmap[pos >> 5] & (1 << (pos & 31))) > 0;
}
int RM_FileHandle::_getFirstZero(uint *bitmap, int size) const {
	for (int i = 0; i < size; i++)
		if (!(bitmap[i >> 5] & (1 << (i & 31)))) return i;
	return -1;
}
int RM_FileHandle::_getNextOne(uint *bitmap, int size, int pos) const {
	for (int i = pos; i < size; i++)
		if (bitmap[i >> 5] & (1 << (i & 31))) return i;
	return -1;
}
