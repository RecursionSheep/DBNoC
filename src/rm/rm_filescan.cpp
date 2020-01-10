#include "rm.h"
#include <cstring>

RM_FileScan::RM_FileScan(FileManager *_fileManager, BufPageManager *_bufPageManager) {
	fileManager = _fileManager; bufPageManager = _bufPageManager;
}
RM_FileScan::~RM_FileScan() {}

bool RM_FileScan::OpenScan(RM_FileHandle *filehandle) {
	_filehandle = filehandle;
	_pageID = 1; _slotID = 0; _fileID = _filehandle->_fileID;
	while (1) {
		if (_pageID >= _filehandle->_header.pageNumber) return false;
		int index;
		BufType buf = bufPageManager->getPage(_fileID, _pageID, index);
		bufPageManager->access(index);
		BufType bitmap = buf + _filehandle->_header.bitmapStart;
		if (_filehandle->_getBit(bitmap, _filehandle->_header.recordNumPerPage, _slotID)) break;
		_slotID++;
		if (_slotID  == _filehandle->_header.recordNumPerPage) {
			_pageID++; _slotID = 0;
		}
	}
	return true;
}
bool RM_FileScan::GetNextRecord(int &pageID, int &slotID, BufType data) {
	if (_pageID >= _filehandle->_header.pageNumber) return false;
	int index;
	BufType buf = bufPageManager->getPage(_fileID, _pageID, index);
	bufPageManager->access(index);
	BufType bitmap = buf + _filehandle->_header.bitmapStart;
	pageID = _pageID; slotID = _slotID;
	buf = bitmap + _filehandle->_header.bitmapSize + slotID * _filehandle->_header.recordSize;
	memcpy(data, buf, _filehandle->_header.recordSize * sizeof(uint));
	_slotID++;
	while ((_pageID < _filehandle->_header.pageNumber) && (_filehandle->_getNextOne(bitmap, _filehandle->_header.recordNumPerPage, _slotID) == -1)) {
		_pageID++;
		_slotID = 0;
		buf = bufPageManager->getPage(_fileID, _pageID, index);
		bufPageManager->access(index);
		bitmap = buf + _filehandle->_header.bitmapStart;
	}
	//cout << _pageID << ' ' << _slotID << ' ' << _filehandle->_header.pageNumber << endl;
	if (_pageID >= _filehandle->_header.pageNumber) return false;
	return true;
}
