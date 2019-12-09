#include "ix.h"

IX_IndexScan::IX_IndexScan(int indexID, int fileID) {
	_indexID = indexID; _fileID = fileID;
}
IX_IndexScan::~IX_IndexScan() {}
