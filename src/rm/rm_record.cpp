#include "rm.h"

RM_Record::RM_Record() { _data = nullptr; _size = -1; }
RM_Record::~RM_Record() { if (_data != nullptr) delete [] _data; }

RC RM_Record::GetData(char *&pData) const {
	if (_data == nullptr || _size < 0) return RM_INVALIDRECORD;
	pData = _data;
	return RC_OK;
}
RC RM_Record::GetRid(RID &rid) const {
	RC valid = _rid.checkValid();
	if (valid == RC_OK) rid = _rid;
	return valid;
}
RC RM_Record::SetData(char *data, int size, RID rid) {
	RC valid = rid.checkValid();
	if (valid == RC_OK) {
		if (size <= 0 || size > PF_PAGE_SIZE) return RM_BADRECORDSIZE;
		if (data == nullptr) return RM_INVALIDRECORD;
		if (_data != nullptr) delete [] _data;
		_size = size;
		_data = new char[size];
		memcpy(_data, data, size);
		_rid = rid;
		return RC_OK;
	}
	return RM_INVALIDRID;
}
