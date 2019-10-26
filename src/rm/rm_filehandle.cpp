#include "rm.h"

RM_FileHandle::RM_FileHandle() { _fileOpened = false; }
RM_FileHandle::~RM_FileHandle() {}

RC RM_FileHandle::_getHeader(PF_PageHandle &ph, RM_PageHeader *&header, char *&bitmap) {
	char *data;
	RC valid = ph.GetData(data);
	if (valid != RC_OK) return valid;
	header = (class RM_PageHeader *)data;
	bitmap = data + _fileHeader.bitmapStart;
	return RC_OK;
}

RC RM_FileHandle::_resetBitmap(char *bitmap, int size) {
	if (size < 0) return RM_INVALIDBITOPERATION;
	int len = (size - 1) / 8 + 1;
	if (size == 0) len = 0;
	for (int i = 0; i < len; i++) bitmap[i] = 0;
	return RC_OK;
}
RC RM_FileHandle::_setBit(char *bitmap, int size, int pos, bool bit) {
	if (pos >= size || pos < 0) return RM_INVALIDBITOPERATION;
	if (bit) bitmap[pos >> 3] |= 1 << (pos & 7); else bitmap[pos >> 3] &= ~(1 << (pos & 7));
	return RC_OK;
}
RC RM_FileHandle::_getBit(char *bitmap, int size, int pos, bool &bit) {
	if (pos >= size || pos < 0) return RM_INVALIDBITOPERATION;
	bit = (bitmap[pos >> 3] & (1 << (pos & 7))) > 0;
	return RC_OK;
}
RC RM_FileHandle::_getFirstZero(char *bitmap, int size, int &pos) {
	for (int i = 0; i < size; i++) {
		if (!(bitmap[i >> 3] & (1 << (i & 7)))) {
			pos = i;
			return RC_OK;
		}
	}
	return RM_PAGEFULL;
}
RC RM_FileHandle::_getNextOne(char *bitmap, int size, int pos, int &ret) {
	for (int i = pos; i < size; i++) {
		if (!(bitmap[i >> 3] & (1 << (i & 7)))) {
			ret = i;
			return RC_OK;
		}
	}
	return RM_ENDOFPAGE;
}

RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec) const {
	if (!_fileOpened) return RM_INVALIDFILE;
	RC valid = rid.checkValid();
	if (valid != RC_OK) return valid;
	PageNum pageNum; rid.GetPageNum(pageNum);
	slotNum slotNum; rid.GetSlotNum(slotNum);
	PF_PageHandle ph;
	valid = _pfh.GetThisPage(pageNum, ph);
	if (valid != RC_OK) return valid;
	
	RM_PageHeader *header;
	char *bitmap;
	valid = _getHeader(ph, header, bitmap);
	if (valid != RC_OK) { _pfh.UnpinPage(pageNum); return valid; }
	bool check;
	valid = _getBit(bitmap, _fileHeader.recordNumber, slotNum, check);
	if (valid != RC_OK) { _pfh.UnpinPage(pageNum); return valid; }
	if (!check) { _pfh.UnpinPage(pageNum); return RM_INVALIDRECORD; }
	valid = rec.SetData(bitmap + _fileHeader.bitmapSize + slotNum * (_fileHeader.recordSize), _fileHeader.recordSize, rid);
	if (valid != RC_OK) { _pfh.UnpinPage(pageNum); return valid; }
	
	return _pfh.UnpinPage(pageNum);
}
RC RM_FileHandle::InsertRec(const char *pData, RID &rid) {
	if (!_fileOpened) return RM_INVALIDFILE;
	if (pData == nullptr) return RM_INVALIDRECORD;
	PF_PageHandle ph;
	PageNum pageNum;
	RC valid;
	if (_fileHeader.firstFreePage == -1)
		AllocateNewPage(ph, pageNum);
	else {
		valid = _pfh.GetThisPage(_fileHeader.firstFreePage, ph);
		if (valid != RC_OK) return valid;
		pageNum = _fileHeader.firstFreePage;
	}
	RM_PageHeader *header;
	char *bitmap;
	slotNum slotNum;
	valid = _getHeader(ph, header, bitmap);
	if (valid != RC_OK) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return valid; }
	valid = _getFirstZero(bitmap, _fileHeader.recordNumber, slotNum);
	if (valid != RC_OK) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return valid; }
	valid = _setBit(bitmap, _fileHeader.recordNumber, slotNum, true);
	if (valid != RC_OK) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return valid; }
	memcpy(bitmap + (_fileHeader.bitmapSize) + slotNum * _fileHeader.recordSize, pData, _fileHeader.recordSize);
	header->recordNumber++;
	rid = RID(pageNum, slotNum);
	if (header->recordNumber == _fileHeader.recordNumber) _fileHeader.firstFreePage = header->nextFreePage;
	
	valid = _pfh.MarkDirty(pageNum);
	if (valid != RC_OK) {
		_pfh.UnpinPage(PageNum);
		return valid;
	}
	return _pfh.UnpinPage(pageNum);
}
RC RM_FileHandle::DeleteRec(const RID &rid) {
	if (!_fileOpened) return RM_INVALIDFILE;
	RC valid = rid.checkValid();
	if (valid != RC_OK) return valid;
	PageNum pageNum; rid.GetPageNum(pageNum);
	slotNum slotNum; rid.GetSlotNum(slotNum);
	PF_PageHandle ph;
	valid = _pfh.GetThisPage(pageNum, ph);
	if (valid != RC_OK) return valid;
	
	RM_PageHeader *header;
	char *bitmap;
	valid = _getHeader(ph, header, bitmap);
	if (valid != RC_OK) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return valid; }
	bool check;
	valid = _getBit(bitmap, _fileHeader.bitmapSize, slotNum, check);
	if (valid != RC_OK) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return valid; }
	if (!check) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return RM_INVALIDRECORD; }
	valid = _setBit(bitmap, _fileHeader.bitmapSize, slotNum, false);
	if (valid != RC_OK) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return valid; }
	if (header->recordNumber == _fileHeader.recordNumber) {
		header->nextFreePage = _fileHeader.firstFreePage;
		_fileHeader.firstFreePage = pageNum;
	}
	header->recordNumber--;
	
	valid = _pfh.MarkDirty(pageNum);
	if (valid != RC_OK) {
		_pfh.UnpinPage(PageNum);
		return valid;
	}
	return _pfh.UnpinPage(pageNum);
}
RC RM_FileHandle::UpdateRec(const RM_Record &rec) {
	if (!_fileOpened) return RM_INVALIDFILE;
	RID rid;
	RC valid = rec.GetRid(rid);
	if (valid != RC_OK) return valid;
	PageNum pageNum; rid.GetPageNum(pageNum);
	slotNum slotNum; rid.GetSlotNum(slotNum);
	PF_PageHandle ph;
	valid = _pfh.GetThisPage(pageNum, ph);
	if (valid != RC_OK) return valid;
	
	RM_PageHeader *header;
	char *bitmap;
	valid = _getHeader(ph, header, bitmap);
	if (valid != RC_OK) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return valid; }
	bool check;
	valid = _getBit(bitmap, _fileHeader.bitmapSize, slotNum, check);
	if (valid != RC_OK) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return valid; }
	if (!check) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return RM_INVALIDRECORD; }
	char *data;
	valid = rec.GetData(data);
	if (valid != RC_OK) { _pfh.MarkDirty(pageNum); _pfh.UnpinPage(pageNum); return valid; }
	memcpy(bitmap + (_fileHeader.bitmapSize) + slotNum * _fileHeader.recordSize, data, _fileHeader.recordSize);
	
	valid = _pfh.MarkDirty(pageNum);
	if (valid != RC_OK) {
		_pfh.UnpinPage(PageNum);
		return valid;
	}
	return _pfh.UnpinPage(pageNum);
}
RC RM_FileHandle::ForcePages(PageNum pageNum) {
	if (!_fileOpened) return RM_INVALIDFILE;
	_pfh.ForcePages(pageNum);
	return RC_OK;
}
