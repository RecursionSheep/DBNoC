#include "ix.h"
#include <cstring>

IX_IndexScan::IX_IndexScan(FileManager *_fileManager, BufPageManager *_bufPageManager, int fileID) {
	fileManager = _fileManager; bufPageManager = _bufPageManager;
	_fileID = fileID;
	int index0;
	BufType data0 = bufPageManager->getPage(_fileID, 0, index0);
	memcpy(&_header, data0, sizeof(IX_FileHeader));
}
IX_IndexScan::~IX_IndexScan() {}

IX_TreeNode* IX_IndexScan::_getNode(int id) {
	if (id == 0) return nullptr;
	IX_TreeNode *node = new IX_TreeNode();
	BufType data = bufPageManager->getPage(_fileID, id, node->index);
	memcpy(&node->header, data, sizeof(IX_PageHeader));
	node->data = data;
	node->child = data + _header.childStart;
	node->page = data + _header.pageStart;
	node->slot = data + _header.slotStart;
	node->key = (char*)(data + _header.keyStart);
	return node;
}
void IX_IndexScan::_writeBackNode(IX_TreeNode *node) {
	memcpy(node->data, &node->header, sizeof(IX_PageHeader));
	bufPageManager->markDirty(node->index);
	//bufPageManager->writeBack(node->index);
}

bool IX_IndexScan::OpenScan(void *pData, bool lower) {
	int id = _header.rootPage;
	int pageID = lower ? -1 : 1 << 30;
	int slotID = pageID;
	while (1) {
		IX_TreeNode *node = _getNode(id);
		bufPageManager->access(node->index);
		if (!node->header.isLeaf) {
			bool flag = false;
			for (int i = node->header.keyNum - 1; i >= 0; i--) {
				//fprintf(stderr, "%.3lf ", *(double*)(node->key + i * _header.attrLen));
				if (compareLess(node->key + i * _header.attrLen, node->page[i], node->slot[i], pData, pageID, slotID, _header.attrType, _header.attrLen)) {
					id = node->child[i];
					flag = true;
					break;
				}
			}
			//fprintf(stderr, "\n");
			if (!flag) id = node->child[0];
		} else {
			_nodeID = id;
			_entry = -1;
			for (int i = 0; i < node->header.keyNum; i++) {
				if (compareLess(pData, pageID, slotID, node->key + i * _header.attrLen, node->page[i], node->slot[i], _header.attrType, _header.attrLen)) {
					//fprintf(stderr, "%.3lf %.3lf %d\n", *(double*)pData, *(double*)(node->key + i * _header.attrLen), *(double*)pData < *(double*)(node->key + i * _header.attrLen));
					_entry = i;
					break;
				}
			}
			if (_entry == -1) {
				_nodeID = node->header.nextLeaf;
				_entry = 0;
			}
			delete node;
			if (_nodeID == 0) return false;
			break;
		}
		delete node;
	}
	return true;
}
bool IX_IndexScan::GetNextEntry(int &pageID, int &slotID) {
	if (_nodeID == 0) return false;
	IX_TreeNode *node = _getNode(_nodeID);
	bufPageManager->access(node->index);
	pageID = node->page[_entry]; slotID = node->slot[_entry];
	if (_entry + 1 == node->header.keyNum) {
		if (node->header.nextLeaf == 0) {
			//fprintf(stderr, "node %d next leaf %d keynum %d\n", _nodeID, node->header.nextLeaf, node->header.keyNum);
			delete node;
			return false;
		}
		_nodeID = node->header.nextLeaf;
		_entry = 0;
	} else
		_entry++;
	delete node;
	return true;
}
bool IX_IndexScan::GetPrevEntry(int &pageID, int &slotID) {
	if (_nodeID == 0) return false;
	IX_TreeNode *node = _getNode(_nodeID);
	bufPageManager->access(node->index);
	pageID = node->page[_entry]; slotID = node->slot[_entry];
	if (_entry == 0) {
		if (node->header.prevLeaf == 0) {
			delete node;
			return true;
		}
		_nodeID = node->header.prevLeaf;
		node = _getNode(_nodeID);
		_entry = node->header.keyNum - 1;
	} else
		_entry--;
	delete node;
	return true;
}
bool IX_IndexScan::CloseScan() {}
