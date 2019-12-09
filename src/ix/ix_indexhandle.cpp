#include "ix.h"
#include <cstring>

IX_IndexHandle::IX_IndexHandle(FileManager *_fileManager, BufPageManager *_bufPageManager, int indexID, int fileID) {
	fileManager = _fileManager; bufPageManager = _bufPageManager;
	_indexID = indexID; _fileID = fileID;
	int index0;
	BufType data0 = bufPageManager->getPage(_fileID, 0, index0);
	memcpy(&_header, data0, sizeof(IX_FileHeader));
}
IX_IndexHandle::~IX_IndexHandle() {}

IX_TreeNode* IX_IndexHandle::_getNode(int id) {
	IX_TreeNode *node = new IX_TreeNode();
	BufType data = bufPageManager->getPage(_fileID, id, node->index);
	node->pageHeader = (IX_PageHeader*)data;
	node->child = data + _header.childStart;
	node->page = data + _header.pageStart;
	node->slot = data + _header.slotStart;
	node->key = (char*)(data + _header.keyStart);
	return node;
}
void IX_IndexHandle::_writeBackNode(IX_TreeNode *node) {
	bufPageManager->markDirty(node->index);
	bufPageManager->writeBack(node->index);
}

bool IX_IndexHandle::InsertEntry(void *pData, int pageID, int slotID) {
	int id = _header.rootPage;
	while (1) {
		IX_TreeNode *node = _getNode(id);
		if (!node->header.isLeaf) {
			for (int i = node->header.keyNum - 1; i >= 0; i--)
				if (i == 0 || compareLess(node->key + i * _header.attrLen, node->page[i], node->slot[i], pData, pageID, slotID, _header.attrType, _header.attrLen)) {
					id = node->child[i];
					break;
				}
		} else {
			bool flag = false;
			for (int i = node->header.keyNum - 1; i >= 0; i--) {
				if (compareLess(node->key + i * _header.attrLen, node->page[i], node->slot[i], pData, pageID, slotID, _header.attrType, _header.attrLen)) {
					flag = true;
					node->page[i + 1] = pageID; node->slot[i + 1] = slotID;
					memcpy(node->key + (i + 1) * _header.attrLen, pData, _header.attrLen);
					break;
				}
				node->page[i + 1] = page[i]; node->slot[i + 1] = slot[i];
				memcpy(node->key + (i + 1) * _header.attrLen, node->key + i * _header.attrLen, _header.attrLen);
			}
			if (!flag) {
				node->page[0] = pageID; node->slot[0] = slotID;
				memcpy(node->key, pData, _header.attrLen);
			}
			node->header.key++;
			flag = false;
			while (node->header.keyNum == _header.childNum) {
				flag = true;
				IX_TreeNode *parentNode;
				int parent = node->header.parent;
				if (parent == 0) {
					parent = _header.nodeNum++;
					_header.rootPage = parent;
					parentNode = _getNode(parent);
					parentNode->header.keyNum = 1; parentNode->header.isLeaf = false; parenNode->parent = 0;
					parentNode->page[0] = node->page[0]; parentNode->slot[0] = node->slot[0];
					memcpy(parentNode->key, node->key, _header.attrLen);
					node->header.parent = parent; node->header.whichChild = 0;
				} else
					parentNode = _getNode(parent);
				int whichChild = node->header.whichChild;
				for (int i = parentNode->header.keyNum; i > whichChild + 1; i++) {
					parentNode->child[i] = parentNode->child[i - 1];
					parentNode->page[i] = parentNode->page[i - 1]; parentNode->slot[i] = parentNode->slot[i - 1];
					memcpy(parentNode->key + i * _header.attrLen, parentNode->key + (i - 1) * _header.attrLen, _header.attrLen);
				}
				
				parentNode->header.keyNum++;
				parentNode->child[whichChild + 1] = _header.nodeNum++;
				int newID = parentChild[whichChild + 1];
				IX_TreeNode *newNode = _getNode(newID);
				newNode->header.isLeaf = node->header.isLeaf; newNode->header.keyNum = node->header.keyNum - node->header.keyNum / 2;
				newNode->header.parent = parent; newNode->header.whichChild = whichChild + 1;
				node->header.keyNum /= 2;
				for (int i = 0; i < newNode->header.keyNum; i++) {
					newNode->child[i] = node->child[i + node->header.keyNum];
					newNode->page[i] = node->page[i + node->header.keyNum];
					newNode->slot[i] = node->slot[i + node->header.keyNum];
					memcpy(newNode->key + i * _header.attrLen, node->key + (i + node->header.keyNum) * _header.attrLen, _header.attrLen);
				}
				if (node->header.isLeaf) {
					newNode->header.nextLeaf = node->header.nextLeaf;
					newNode->header.prevLeaf = id;
					if (node->header.nextLeaf != 0) {
						IX_TreeNode *nextLeaf = _getNode(node->header.nextLeaf);
						nextLeaf->header.prevLeaf = newID;
						_writeBackNode(nextLeaf);
					}
					node->header.nextLeaf = newID;
				}
				for (int i = whichChild + 2; i < parentNode->header.keyNum; i++) {
					IX_TreeNode *child = _getNode(parentNode->child[i]);
					child->header.whichChild++;
					_writeBackNode(child);
				}
				memcpy(parentNode->key + whichChild * _header.attrLen, node->key, _header.attrLen);
				memcpy(parentNode->key + (whichChild + 1) * _header.attrLen, newNode->key, _header.attrLen);
				parentNode->page[whichChild] = node->page[0];
				parentNode->page[whichChild + 1] = newNode->page[0];
				parentNode->slot[whichChild] = node->slot[0];
				parentNode->slot[whichChild + 1] = newNode->slot[0];
				_writeBackNode(node);
				_writeBackNode(newNode);
				id = parent;
				node = parentNode;
			}
			_writeBackNode(node);
			if (flag) {
				int index0;
				BufType data0 = bufPageManager->getPage(_fileID, 0, index0);
				memcpy(data0, &_header, sizeof(IX_FileHeader));
				bufPageManager->markDirty(index0);
				bufPageManager->writeBack(index0);
			}
			while (id != _header.rootPage) {
				IX_TreeNode *parentNode = _getNode(node->header.parent);
				int whichChild = node->header.whichChild;
				parentNode->page[whichChild] = node->page[0]; parentNode->slot[whichChild] = node->slot[0];
				memcpy(parentNode->key + whichChild * _header.attrLen, node->key, _header.attrLen);
				_writeBackNode(parentNode);
				id = node->header.parent;
				node = parentNode;
			}
			break;
		}
	}
	return true;
}
bool IX_IndexHandle::DeleteEntry(void *pData, int pageID, int slotID) {
	int id = _header.rootPage;
	while (1) {
		IX_TreeNode *node = _getNode(id);
		if (!node->header.isLeaf) {
			for (int i = node->header.keyNum - 1; i >= 0; i--)
				if (i == 0 || !compareLess(pData, pageID, slotID, node->key + i * _header.attrLen, node->page[i], node->slot[i], _header.attrType, _header.attrLen)) {
					id = node->child[i];
					break;
				}
		} else {
			int pos;
			for (int i = node->header.keyNum - 1; i >= 0; i--) {
				if (!compareLess(pData, pageID, slotID, node->key + i * _header.attrLen, node->page[i], node->slot[i], _header.attrType, _header.attrLen)) {
					pos = i;
					break;
				}
			}
			node->header.keyNum--;
			for (int i = pos; i < node->header.keyNum; i++) {
				node->page[i] = node->page[i + 1]; node->slot[i] = node->slot[i + 1];
				memcpy(node->key + i * _header.attrLen, node->key + (i + 1) * _header.attrLen, _header.attrLen);
			}
			while (node->header.keyNum == 0) {
				
			}
		}
	}
}
