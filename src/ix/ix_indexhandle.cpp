#include "ix.h"
#include <cstring>
#include <cassert>
#include <iostream>

//int cnt = 0;

IX_IndexHandle::IX_IndexHandle(FileManager *_fileManager, BufPageManager *_bufPageManager, int fileID) {
	fileManager = _fileManager; bufPageManager = _bufPageManager;
	_fileID = fileID;
	int index0;
	BufType data0 = bufPageManager->getPage(_fileID, 0, index0);
	memcpy(&_header, data0, sizeof(IX_FileHeader));
}
IX_IndexHandle::~IX_IndexHandle() {}

IX_TreeNode* IX_IndexHandle::_getNode(int id) {
	if (id == 0) return nullptr;
	IX_TreeNode *node = new IX_TreeNode();
	BufType data = bufPageManager->getPage(_fileID, id, node->index);
	bufPageManager->access(node->index);
	memcpy(&node->header, data, sizeof(IX_PageHeader));
	node->data = data;
	node->child = data + _header.childStart;
	node->page = data + _header.pageStart;
	node->slot = data + _header.slotStart;
	node->key = (unsigned char*)(data + _header.keyStart);
	return node;
}
void IX_IndexHandle::_writeBackNode(IX_TreeNode *node) {
	memcpy(node->data, &node->header, sizeof(IX_PageHeader));
	bufPageManager->markDirty(node->index);
	//bufPageManager->writeBack(node->index);
}

bool IX_IndexHandle::InsertEntry(void *pData, int pageID, int slotID) {
	int id = _header.rootPage;
	while (1) {
		IX_TreeNode *node = _getNode(id);
		//fprintf(stderr, "node %d\n", id);
		if (!node->header.isLeaf) {
			bufPageManager->access(node->index);
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
					node->child[i + 1] = 0;
					break;
				}
				node->page[i + 1] = node->page[i]; node->slot[i + 1] = node->slot[i];
				memcpy(node->key + (i + 1) * _header.attrLen, node->key + i * _header.attrLen, _header.attrLen);
			}
			if (!flag) {
				node->page[0] = pageID; node->slot[0] = slotID;
				node->child[0] = 0;
				memcpy(node->key, pData, _header.attrLen);
			}
			node->header.keyNum++;
			//if (node->header.isLeaf) fprintf(stderr, "next leaf %d\n", node->header.nextLeaf);
			
			if (node->header.isLeaf && node->header.nextLeaf != 0) {
				IX_TreeNode *nextLeaf = _getNode(node->header.nextLeaf);
				void *data1 = (node->key + (node->header.keyNum - 1) * _header.attrLen);
				void *data2 = nextLeaf->key;
				if (*(int*)data1 > *(int*)data2) {
					cout << *(int*)data1 << " " << *(int*)data2 << endl;
					cout << node->header.keyNum << " " << _header.childNum << endl;
					cout << node->header.parent << " " << nextLeaf->header.parent << endl;
					IX_TreeNode *parentNode = _getNode(nextLeaf->header.parent);
					int whichChild = _whichChild(node->header.nextLeaf, parentNode);
					cout << *(int*)(parentNode->key + whichChild * _header.attrLen) << " " << *(int*)data2 << endl;
					assert(*(int*)data1 <= *(int*)data2);
				}
				delete nextLeaf;
			}
			
			flag = false;
			while (node->header.keyNum == _header.childNum) {
				//fprintf(stderr, "new node\n");
				flag = true;
				IX_TreeNode *parentNode;
				int parent = node->header.parent;
				//cout << "node: " << id << " parent: " << node->header.parent << endl;
				if (parent == 0) {
					parent = _header.nodeNum++;
					_header.rootPage = parent;
					parentNode = _getNode(parent);
					parentNode->header.keyNum = 1; parentNode->header.isLeaf = false; parentNode->header.parent = 0;
					parentNode->page[0] = node->page[0]; parentNode->slot[0] = node->slot[0];
					parentNode->child[0] = id;
					memcpy(parentNode->key, node->key, _header.attrLen);
					node->header.parent = parent;
				} else
					parentNode = _getNode(parent);
				int whichChild = _whichChild(id, parentNode);
				if (whichChild == -1) {
					cout << id << " " << parent << endl;
					for (int i = 0; i < parentNode->header.keyNum; i++)
						cout << parentNode->child[i] << " ";
					cout << endl;
					assert(false);
				}
				for (int i = parentNode->header.keyNum; i > whichChild + 1; i--) {
					parentNode->child[i] = parentNode->child[i - 1];
					parentNode->page[i] = parentNode->page[i - 1]; parentNode->slot[i] = parentNode->slot[i - 1];
					memcpy(parentNode->key + i * _header.attrLen, parentNode->key + (i - 1) * _header.attrLen, _header.attrLen);
				}
				
				/*cout << id << endl;
				for (int i = 0; i < node->header.keyNum; i++) {
					cout << ((int*)node->key)[i] << " ";
				}
				cout << endl;*/
				
				parentNode->header.keyNum++;
				parentNode->child[whichChild + 1] = _header.nodeNum++;
				int newID = parentNode->child[whichChild + 1];
				//cout << "new node: " << newID << endl;
				IX_TreeNode *newNode = _getNode(newID);
				newNode->header.isLeaf = node->header.isLeaf; newNode->header.keyNum = node->header.keyNum - node->header.keyNum / 2;
				newNode->header.parent = parent;
				node->header.keyNum /= 2;
				for (int i = 0; i < newNode->header.keyNum; i++) {
					newNode->child[i] = node->child[i + node->header.keyNum];
					if (!newNode->header.isLeaf && newNode->child[i] != 0) {
						//cout << "child: " << newNode->child[i] << endl;
						IX_TreeNode *childNode = _getNode(newNode->child[i]);
						childNode->header.parent = newID;
						_writeBackNode(childNode);
						delete childNode;
					}
					newNode->page[i] = node->page[i + node->header.keyNum];
					newNode->slot[i] = node->slot[i + node->header.keyNum];
					memcpy(newNode->key + i * _header.attrLen, node->key + (i + node->header.keyNum) * _header.attrLen, _header.attrLen);
				}
				if (node->header.isLeaf) {
					newNode->header.nextLeaf = node->header.nextLeaf;
					newNode->header.prevLeaf = id;
					if (node->header.nextLeaf != 0) {
						IX_TreeNode *nextLeaf = _getNode(node->header.nextLeaf);
						//void *data1 = (newNode->key + (newNode->header.keyNum - 1) * _header.attrLen);
						//void *data2 = nextLeaf->key;
						//assert(*(int*)data1 <= *(int*)data2);
						nextLeaf->header.prevLeaf = newID;
						_writeBackNode(nextLeaf);
						delete nextLeaf;
					}
					node->header.nextLeaf = newID;
				}
				memcpy(parentNode->key + whichChild * _header.attrLen, node->key, _header.attrLen);
				memcpy(parentNode->key + (whichChild + 1) * _header.attrLen, newNode->key, _header.attrLen);
				parentNode->page[whichChild] = node->page[0];
				parentNode->page[whichChild + 1] = newNode->page[0];
				parentNode->slot[whichChild] = node->slot[0];
				parentNode->slot[whichChild + 1] = newNode->slot[0];
				_writeBackNode(node);
				_writeBackNode(newNode);
				
				/*cout << parent << endl;
				for (int i = 0; i < parentNode->header.keyNum; i++) {
					cout << ((int*)parentNode->key)[i] << " ";
				}
				cout << endl;*/
				
				id = parent;
				delete node, newNode;
				node = parentNode;
			}
			_writeBackNode(node);
			
			if (flag) {
				int index0;
				BufType data0 = bufPageManager->getPage(_fileID, 0, index0);
				memcpy(data0, &_header, sizeof(IX_FileHeader));
				bufPageManager->markDirty(index0);
				//bufPageManager->writeBack(index0);
			}
			while (id != _header.rootPage) {
				IX_TreeNode *parentNode = _getNode(node->header.parent);
				int whichChild = _whichChild(id, parentNode);
				if (whichChild == -1) {
					cout << id << " " << node->header.parent << endl;
					for (int i = 0; i < parentNode->header.keyNum; i++)
						cout << parentNode->child[i] << " ";
					cout << endl;
					assert(false);
				}
				parentNode->page[whichChild] = node->page[0]; parentNode->slot[whichChild] = node->slot[0];
				memcpy(parentNode->key + whichChild * _header.attrLen, node->key, _header.attrLen);
				_writeBackNode(parentNode);
				id = node->header.parent;
				delete node;
				node = parentNode;
			}
			delete node;
			break;
		}
		delete node;
	}
	return true;
}
bool IX_IndexHandle::DeleteEntry(void *pData, int pageID, int slotID) {
	int id = _header.rootPage;
	while (1) {
		IX_TreeNode *node = _getNode(id);
		if (!node->header.isLeaf) {
			bufPageManager->access(node->index);
			for (int i = node->header.keyNum - 1; i >= 0; i--)
				if (i == 0 || !compareLess(pData, pageID, slotID, node->key + i * _header.attrLen, node->page[i], node->slot[i], _header.attrType, _header.attrType)) {
					id = node->child[i];
					break;
				}
		} else {
			int pos = -1;
			for (int i = node->header.keyNum - 1; i >= 0; i--) {
				if (!compareLess(pData, pageID, slotID, node->key + i * _header.attrLen, node->page[i], node->slot[i], _header.attrType, _header.attrLen)) {
					pos = i;
					break;
				}
			}
			if (pos == -1) {
				bufPageManager->access(node->index);
				delete node;
				return false;
			}
			node->header.keyNum--;
			for (int i = pos; i < node->header.keyNum; i++) {
				node->page[i] = node->page[i + 1]; node->slot[i] = node->slot[i + 1];
				memcpy(node->key + i * _header.attrLen, node->key + (i + 1) * _header.attrLen, _header.attrLen);
			}
			if (node->header.isLeaf && node->header.keyNum == 0) {
				int prevLeaf = node->header.prevLeaf, nextLeaf = node->header.nextLeaf;
				if (prevLeaf) {
					IX_TreeNode *prevNode = _getNode(prevLeaf);
					prevNode->header.nextLeaf = nextLeaf;
					_writeBackNode(prevNode);
					delete prevNode;
				}
				if (nextLeaf) {
					IX_TreeNode *nextNode = _getNode(nextLeaf);
					nextNode->header.prevLeaf = prevLeaf;
					_writeBackNode(nextNode);
					delete nextNode;
				}
			}
			while (node->header.keyNum == 0) {
				int parent = node->header.parent;
				IX_TreeNode *parentNode = _getNode(parent);
				int pos = _whichChild(id, parentNode);
				for (int i = pos + 1; i < parentNode->header.keyNum; i++) {
					parentNode->child[i - 1] = parentNode->child[i];
					parentNode->page[i - 1] = parentNode->page[i];
					parentNode->slot[i - 1] = parentNode->slot[i];
					memcpy(parentNode->key + (i - 1) * _header.attrLen, parentNode->key + i * _header.attrLen, _header.attrLen);
				}
				parentNode->header.keyNum--;
				_writeBackNode(node);
				delete node;
				node = parentNode;
				id = parent;
				if (parent == _header.rootPage) break;
			}
			_writeBackNode(node);
			delete node;
			break;
		}
		delete node;
	}
	return true;
}
bool IX_IndexHandle::CheckEntry(void *pData) {
	int id = _header.rootPage;
	int pageID = -1;
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
			int _entry = -1;
			for (int i = 0; i < node->header.keyNum; i++) {
				if (compareLess(pData, pageID, slotID, node->key + i * _header.attrLen, node->page[i], node->slot[i], _header.attrType, _header.attrLen)) {
					//fprintf(stderr, "%.3lf %.3lf %d\n", *(double*)pData, *(double*)(node->key + i * _header.attrLen), *(double*)pData < *(double*)(node->key + i * _header.attrLen));
					_entry = i;
					break;
				}
			}
			//cnt++;
			if (_entry == -1) {
				id = node->header.nextLeaf;
				_entry = 0;
				delete node;
				if (id == 0) return false;
				node = _getNode(id);
			}
			//for (int i = 0; i < _header.attrLen; i++) printf("%d %d ", ((char*)pData)[i], ((char*)node->key + _entry * _header.attrLen)[i]);
			//puts("");
			/*if (cnt > 943744) {
				cout << _header.attrLen << endl;
				cout << *(int*)pData << " " << (char*)((int*)pData + 1) << endl;
				for (int t = 0; t < node->header.keyNum; t++) {
					void* entry = node->key + t * _header.attrLen;
					cout << *(int*)entry << " " << (char*)((int*)entry + 1) << endl;
				}
			}*/
			if (memcmp(pData, node->key + _entry * _header.attrLen, _header.attrLen) == 0) return true;
			delete node;
			break;
		}
		delete node;
	}
	return false;
}

int IX_IndexHandle::_whichChild(int child, IX_TreeNode* parent) {
	for (int i = 0; i < parent->header.keyNum; i++) {
		if (parent->child[i] == child) return i;
	}
	return -1;
	//cout << "failed" << child << endl;
}