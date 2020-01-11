#include "ql.h"
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <cassert>
using namespace std;

QL_Manager::QL_Manager(SM_Manager *smm, IX_Manager *ixm, RM_Manager *rmm, FileManager *_fileManager, BufPageManager *_bufPageManager) {
	_smm = smm; _ixm = ixm; _rmm = rmm;
	fileManager = _fileManager; bufPageManager = _bufPageManager;
}
QL_Manager::~QL_Manager() {}

void QL_Manager::Insert(const string tableName, vector<string> attrs, vector<BufType> values) {
	int tableID = _smm->_fromNameToID(tableName);
	for (int i = 0; i < _smm->_tableNum; i++) if (i != tableID) {
		if (_smm->_tables[i].foreignSet.find(tableName) != _smm->_tables[i].foreignSet.end()) {
			fprintf(stderr, "Error: foreign keys on the table!\n");
			return;
		}
	}
	int attrNum = attrs.size();
	if (attrNum != values.size()) {
		fprintf(stderr, "Error: the number of columns and values are different!\n");
		return;
	}
	BufType data = new unsigned int[_smm->_tables[tableID].recordSize >> 2];
	unsigned long long *bitmap = (unsigned long long*)data;
	bitmap[0] = 0;
	for (int i = 0; i < _smm->_tables[tableID].attrNum; i++) if (_smm->_tables[tableID].attrs[i].defaultValue != nullptr) {
		int attrID = i;
		BufType value = _smm->_tables[tableID].attrs[i].defaultValue;
		bitmap[0] |= 1ull << attrID;
		if (_smm->_tables[tableID].attrs[attrID].attrType == INTEGER) {
			assert(_smm->_tables[tableID].attrs[attrID].attrLength == 4);
			int d = *(int*)value;
			memcpy(data + _smm->_tables[tableID].attrs[attrID].offset, &d, 4);
		} else if (_smm->_tables[tableID].attrs[attrID].attrType == FLOAT) {
			assert(_smm->_tables[tableID].attrs[attrID].attrLength == 8);
			double dd = *(double*)value;
			memcpy(data + _smm->_tables[tableID].attrs[attrID].offset, &dd, 8);
		} else if (_smm->_tables[tableID].attrs[attrID].attrType == STRING) {
			char *ddd = (char*)value;
			int size = strlen(ddd);
			memset(data + _smm->_tables[tableID].attrs[attrID].offset, 0, sizeof(char) * _smm->_tables[tableID].attrs[attrID].attrLength);
			//fill(data + _smm->_tables[tableID].attrs[attrID].offset, data + _smm->_tables[tableID].attrs[attrID].offset + (_smm->_tables[tableID].attrs[attrID].attrLength >> 2), 0);
			// 把字符串长度设定为表定义的长度，注意末尾\0
			if (size >= _smm->_tables[tableID].attrs[attrID].attrLength) {
				size = _smm->_tables[tableID].attrs[attrID].attrLength;
				ddd[size - 1] = '\0';
			}
			memcpy(data + _smm->_tables[tableID].attrs[attrID].offset, ddd, size);
		}
	}
	for (int i = 0; i < attrNum; i++) {
		int attrID = _smm->_fromNameToID(attrs[i], tableID);
		if (attrID == -1) {
			fprintf(stderr, "Error: invalid column name!\n");
			delete [] data;
			return;
		}
		bitmap[0] |= 1ull << attrID;
		if (_smm->_tables[tableID].attrs[attrID].attrType == INTEGER) {
			assert(_smm->_tables[tableID].attrs[attrID].attrLength == 4);
			int d = *(int*)values[i];
			memcpy(data + _smm->_tables[tableID].attrs[attrID].offset, &d, 4);
		} else if (_smm->_tables[tableID].attrs[attrID].attrType == FLOAT) {
			assert(_smm->_tables[tableID].attrs[attrID].attrLength == 8);
			double dd = *(double*)values[i];
			memcpy(data + _smm->_tables[tableID].attrs[attrID].offset, &dd, 8);
		} else if (_smm->_tables[tableID].attrs[attrID].attrType == STRING) {
			char *ddd = (char*)values[i];
			int size = strlen(ddd);
			memset(data + _smm->_tables[tableID].attrs[attrID].offset, 0, sizeof(char) * _smm->_tables[tableID].attrs[attrID].attrLength);
			//fill(data + _smm->_tables[tableID].attrs[attrID].offset, data + _smm->_tables[tableID].attrs[attrID].offset + (_smm->_tables[tableID].attrs[attrID].attrLength >> 2), 0);
			// 把字符串长度设定为表定义的长度，注意末尾\0
			if (size >= _smm->_tables[tableID].attrs[attrID].attrLength) {
				size = _smm->_tables[tableID].attrs[attrID].attrLength;
				ddd[size - 1] = '\0';
			}
			//puts((char*)data + _smm->_tables[tableID].attrs[attrID].offset);
			memcpy(data + _smm->_tables[tableID].attrs[attrID].offset, ddd, size);
		}
	}
	for (int i = 0; i < _smm->_tables[tableID].attrNum; i++) {
		if (_smm->_tables[tableID].attrs[i].notNull && (bitmap[0] & (1ull << i) == 0)) {
			fprintf(stderr, "Error: some NOT-NULL columns are null!\n");
			delete [] data;
			return;
		}
	}
	int primary_size = _smm->_tables[tableID].primarySize;
	BufType primaryData = nullptr;
	if (primary_size != 0) {
		primaryData = _smm->_getPrimaryKey(tableID, data);
		int indexID;
		_ixm->OpenIndex(tableName.c_str(), "primary", indexID);
		IX_IndexHandle *indexhandle = new IX_IndexHandle(fileManager, bufPageManager, indexID);
		bool check = indexhandle->CheckEntry(primaryData);
		_ixm->CloseIndex(indexID);
		delete indexhandle;
		if (check) {
			fprintf(stderr, "Error: repetitive primary keys!\n");
			delete [] data;
			if (primaryData != nullptr) delete [] primaryData;
			return;
		}
	}
	// 遍历所有外键
	for (int i = 0; i < _smm->_tables[tableID].foreignNum; i++) {
		string refName = _smm->_tables[tableID].foreign[i];
		int refID = _smm->_fromNameToID(refName);
		int refSize = _smm->_tables[refID].primarySize;
		BufType refData = new unsigned int[refSize >> 2];
		// 遍历ref表的所有主键，pos记录当前主键在主键索引中的offset
		int pos = 0;
		for (int j = 0; j < _smm->_tables[refID].primary.size(); j++) {
			string primaryName = _smm->_tables[refID].attrs[_smm->_tables[refID].primary[j]].attrName;
			int attr = -1;
			// 查找tableID里的对应列
			for (int k = 0; k < _smm->_tables[tableID].attrNum; k++) {
				if (_smm->_tables[tableID].attrs[k].reference == refName && _smm->_tables[tableID].attrs[k].foreignKeyName == primaryName) {
					attr = k;
					break;
				}
			}
			if (attr == -1) {
				fprintf(stderr, "Error: foreign keys are not complete!\n");
				delete [] data;
				if (primaryData != nullptr) delete [] primaryData;
				delete [] refData;
				return;
			}
			memcpy(refData + pos, data + _smm->_tables[tableID].attrs[attr].offset, _smm->_tables[tableID].attrs[attr].attrLength);
			pos += (_smm->_tables[tableID].attrs[attr].attrLength >> 2);
		}
		int indexID;
		_ixm->OpenIndex(refName.c_str(), "primary", indexID);
		IX_IndexHandle *indexhandle = new IX_IndexHandle(fileManager, bufPageManager, indexID);
		bool check = indexhandle->CheckEntry(refData);
		_ixm->CloseIndex(indexID);
		delete indexhandle;
		delete [] refData;
		if (!check) {
			fprintf(stderr, "Error: invalid foreign keys!\n");
			delete [] data;
			if (primaryData != nullptr) delete [] primaryData;
			return;
		}
	}
	int pageID, slotID;
	int fileID = _smm->_tableFileID[tableName];
	RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, fileID);
	filehandle->InsertRec(pageID, slotID, data);
	//fprintf(stderr, "Insert record to %d %d\n", pageID, slotID);
	if (primary_size != 0) {
		int indexID;
		_ixm->OpenIndex(tableName.c_str(), "primary", indexID);
		IX_IndexHandle *indexhandle = new IX_IndexHandle(fileManager, bufPageManager, indexID);
		indexhandle->InsertEntry(primaryData, pageID, slotID);
		_ixm->CloseIndex(indexID);
		delete indexhandle;
		delete [] primaryData;
	}
	for (int i = 0; i < _smm->_tables[tableID].attrNum; i++) {
		if (_smm->_tables[tableID].attrs[i].haveIndex) {
			BufType attrData = data + _smm->_tables[tableID].attrs[i].offset;
			int indexID;
			_ixm->OpenIndex(tableName.c_str(), _smm->_tables[tableID].attrs[i].attrName.c_str(), indexID);
			IX_IndexHandle *indexhandle = new IX_IndexHandle(fileManager, bufPageManager, indexID);
			indexhandle->InsertEntry(attrData, pageID, slotID);
			_ixm->CloseIndex(indexID);
			delete indexhandle;
		}
	}
	delete [] data;
}

void QL_Manager::Update(const Assign assign) {
	string tableName = assign.table;
	string attrName = assign.attr;
	int tableID = _smm->_fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: such table does not exist!\n");
		return;
	}
	for (int i = 0; i < _smm->_tableNum; i++) if (i != tableID) {
		if (_smm->_tables[i].foreignSet.find(tableName) != _smm->_tables[i].foreignSet.end()) {
			fprintf(stderr, "Error: foreign keys on the table!\n");
			return;
		}
	}
	int attrID = _smm->_fromNameToID(assign.attr, tableID);
	if (attrID == -1) {
		fprintf(stderr, "Error: such column does not exist!\n");
		return;
	}
	if (_smm->_tables[tableID].attrs[attrID].primary) {
		fprintf(stderr, "Error: cannot update primary keys!\n");
		return;
	}
	if (_smm->_tables[tableID].attrs[attrID].reference != "") {
		fprintf(stderr, "Error: cannot update foreign keys!\n");
		return;
	}
	int attrLength = _smm->_tables[tableID].attrs[attrID].attrLength;
	AttrType attrType = _smm->_tables[tableID].attrs[attrID].attrType;
	if (assign.value == nullptr && _smm->_tables[tableID].attrs[attrID].notNull) {
		fprintf(stderr, "Error: set NOT-NULL columns to be null!\n");
		return;
	}
	int index = -1;
	IX_IndexHandle *indexhandle;
	int recordSize = _smm->_tables[tableID].recordSize;
	int offset = _smm->_tables[tableID].attrs[attrID].offset;
	if (_smm->_tables[tableID].attrs[attrID].haveIndex) {
		_ixm->OpenIndex(tableName.c_str(), attrName.c_str(), index);
		indexhandle = new IX_IndexHandle(fileManager, bufPageManager, index);
	}
	RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, _smm->_tableFileID[tableName]);
	RM_FileScan *filescan = new RM_FileScan(fileManager, bufPageManager);
	if (!filescan->OpenScan(filehandle)) {
		delete filescan, filehandle;
		return;
	}
	int pageID, slotID;
	while (1) {
		BufType data = new unsigned int[recordSize >> 2];
		bool hasNext = filescan->GetNextRecord(pageID, slotID, data);
		if (index != -1) {
			indexhandle->DeleteEntry((void*)(data + offset), pageID, slotID);
		}
		fill(data + offset, data + offset + (attrLength >> 2), 0);
		if (attrType == STRING) {
			int strLen = strlen((char*)assign.value);
			if (strLen >= attrLength) {
				strLen = attrLength;
				((char*)assign.value)[strLen - 1] = '\0';
			}
			memcpy(data + offset, assign.value, strLen);
		} else {
			memcpy(data + offset, assign.value, attrLength);
		}
		filehandle->UpdateRec(pageID, slotID, data);
		if (index != -1) {
			indexhandle->InsertEntry((void*)(data + offset), pageID, slotID);
		}
		if (!hasNext) break;
		delete data;
	}
	if (index != -1) {
		_ixm->CloseIndex(index);
		delete indexhandle;
	}
	delete filescan, filehandle;
}

void QL_Manager::Delete(const string tableName, vector<Relation> relations) {
	int tableID = _smm->_fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: such table does not exist!\n");
		return;
	}
	for (int i = 0; i < _smm->_tableNum; i++) if (i != tableID) {
		if (_smm->_tables[i].foreignSet.find(tableName) != _smm->_tables[i].foreignSet.end()) {
			fprintf(stderr, "Error: foreign keys on the table!\n");
			return;
		}
	}
	int fileID = _smm->_tableFileID[tableName];
	RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, fileID);
	RM_FileScan *filescan = new RM_FileScan(fileManager, bufPageManager);
	if (!filescan->OpenScan(filehandle)) {
		delete filescan, filehandle;
		return;
	}
	int pageID, slotID;
	vector<int> attrID1, attrID2;
	for (int i = 0; i < relations.size(); i++) {
		int attr = _smm->_fromNameToID(relations[i].attr1, tableID);
		if (attr == -1) {
			delete filescan, filehandle;
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
		attrID1.push_back(attr);
		if (relations[i].data != nullptr) {
			attrID2.push_back(-1);
			continue;
		}
		attr = _smm->_fromNameToID(relations[i].attr2, tableID);
		if (attr == -1) {
			delete filescan, filehandle;
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
		attrID2.push_back(attr);
		if (_smm->_tables[tableID].attrs[attrID1[i]].attrType != _smm->_tables[tableID].attrs[attrID2[i]].attrType) {
			delete filescan, filehandle;
			fprintf(stderr, "Error: invalid comparison!\n");
			return;
		}
	}
	int primaryIndex = -1;
	IX_IndexHandle *primaryhandle;
	vector<int> attrs, indexes;
	vector<IX_IndexHandle*> handles;
	if (_smm->_tables[tableID].primary.size() != 0) {
		_ixm->OpenIndex(tableName.c_str(), "primary", primaryIndex);
		primaryhandle = new IX_IndexHandle(fileManager, bufPageManager, primaryIndex);
	}
	for (int i = 0; i < _smm->_tables[tableID].attrNum; i++) if (_smm->_tables[tableID].attrs[i].haveIndex) {
		attrs.push_back(i);
		int fileID;
		_ixm->OpenIndex(tableName.c_str(), _smm->_tables[tableID].attrs[i].attrName.c_str(), fileID);
		indexes.push_back(fileID);
		handles.push_back(new IX_IndexHandle(fileManager, bufPageManager, fileID));
	}
	int recordSize = _smm->_tables[tableID].recordSize;
	while (1) {
		BufType data = new unsigned int[recordSize >> 2];
		bool hasNext = filescan->GetNextRecord(pageID, slotID, data);
		bool ok = true;
		for (int i = 0; i < relations.size(); i++) {
			BufType data1 = data + _smm->_tables[tableID].attrs[attrID1[i]].offset;
			BufType data2 = relations[i].data;
			if (data2 == nullptr) {
				data2 = data + _smm->_tables[tableID].attrs[attrID2[i]].offset;
			}
			ok = _compare(data1, data2, relations[i].op, _smm->_tables[tableID].attrs[attrID1[i]].attrType);
			if (!ok) break;
		}
		if (ok) {
			if (primaryIndex != -1) {
				BufType primaryData = _smm->_getPrimaryKey(tableID, data);
				primaryhandle->DeleteEntry(primaryData, pageID, slotID);
				delete primaryData;
			}
			for (int i = 0; i < handles.size(); i++) {
				BufType attrData = data + _smm->_tables[tableID].attrs[attrs[i]].offset;
				handles[i]->DeleteEntry(attrData, pageID, slotID);
			}
			if (!filehandle->DeleteRec(pageID, slotID)) cout << "failed" << endl;
		}
		delete data;
		if (!hasNext) break;
	}
	if (primaryIndex != -1) {
		_ixm->CloseIndex(primaryIndex);
		delete primaryhandle;
	}
	for (int i = 0; i < handles.size(); i++) {
		_ixm->CloseIndex(indexes[i]);
		delete handles[i];
	}
	delete filescan, filehandle;
}

void QL_Manager::Select(const string tableName, vector<Relation> relations, vector<string> attrNames) {
	int tableID = _smm->_fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: such table does not exist!\n");
		return;
	}
	vector<int> attrIDs;
	for (int i = 0; i < attrNames.size(); i++) {
		int attrID = _smm->_fromNameToID(attrNames[i], tableID);
		if (attrID == -1) {
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
		attrIDs.push_back(attrID);
	}
	int fileID = _smm->_tableFileID[tableName];
	RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, fileID);
	RM_FileScan *filescan = new RM_FileScan(fileManager, bufPageManager);
	if (!filescan->OpenScan(filehandle)) {
		delete filescan, filehandle;
		return;
	}
	int pageID, slotID;
	vector<int> attrID1, attrID2;
	for (int i = 0; i < relations.size(); i++) {
		int attr = _smm->_fromNameToID(relations[i].attr1, tableID);
		if (attr == -1) {
			delete filescan, filehandle;
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
		attrID1.push_back(attr);
		if (relations[i].data != nullptr) {
			attrID2.push_back(-1);
			continue;
		}
		attr = _smm->_fromNameToID(relations[i].attr2, tableID);
		if (attr == -1) {
			delete filescan, filehandle;
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
		attrID2.push_back(attr);
		if (_smm->_tables[tableID].attrs[attrID1[i]].attrType != _smm->_tables[tableID].attrs[attrID2[i]].attrType) {
			delete filescan, filehandle;
			fprintf(stderr, "Error: invalid comparison!\n");
			return;
		}
	}
	bool found = false;
	int indexAttr = -1, indexRel = -1, indexID = -1;
	IX_IndexScan *indexscan = nullptr;
	for (int i = 0; i < relations.size(); i++) {
		if (attrID2[i] != -1) continue;
		if (relations[i].op != EQ_OP) continue;
		if (!_smm->_tables[tableID].attrs[attrID1[i]].haveIndex) continue;
		found = true;
		indexAttr = attrID1[i]; indexRel = i;
		_ixm->OpenIndex(tableName.c_str(), _smm->_tables[tableID].attrs[indexAttr].attrName.c_str(), indexID);
		indexscan = new IX_IndexScan(fileManager, bufPageManager, indexID);
		if (!indexscan->OpenScan(relations[i].data, true)) {
			delete indexscan;
			_ixm->CloseIndex(indexID);
			delete filescan, filehandle; 
			return;
		}
		break;
	}
	int recordSize = _smm->_tables[tableID].recordSize;
	int outcnt = 0;
	while (1) {
		BufType data = new unsigned int[recordSize >> 2];
		bool hasNext;
		if (found) {
			hasNext = indexscan->GetNextEntry(pageID, slotID);
			filehandle->GetRec(pageID, slotID, data);
		} else {
			hasNext = filescan->GetNextRecord(pageID, slotID, data);
		}
		bool ok = true;
		unsigned long long *bitmap = (unsigned long long*)data;
		for (int i = 0; i < relations.size(); i++) {
			BufType data1 = data + _smm->_tables[tableID].attrs[attrID1[i]].offset;
			BufType data2 = relations[i].data;
			if (data2 == nullptr) {
				data2 = data + _smm->_tables[tableID].attrs[attrID2[i]].offset;
			}
			if (bitmap[0] & (1ull << attrID1[i]) == 0) {
				ok = false; break;
			}
			if (bitmap[0] & (1ull << attrID2[i]) == 0) {
				ok = false; break;
			}
			ok = _compare(data1, data2, relations[i].op, _smm->_tables[tableID].attrs[attrID1[i]].attrType);
			if (i == indexRel && !ok) hasNext = false;
			if (!ok) break;
		}
		if (ok) {
			outcnt++;
			if (outcnt <= 100) {
				putchar('|');
				//cout << " bitmap: " << bitmap[0] << " |";
				for (int i = 0; i < attrIDs.size(); i++) {
					BufType out = data + _smm->_tables[tableID].attrs[attrIDs[i]].offset;
					//cout << " " << (bitmap[0] & (1ull << attrIDs[i])) << " ";
					if ((bitmap[0] & (1ull << attrIDs[i])) == 0) {
						printf(" NULL |");
						continue;
					}
					if (_smm->_tables[tableID].attrs[attrIDs[i]].attrType == INTEGER) {
						printf(" INT %d ", *(int*)out);
					} else if (_smm->_tables[tableID].attrs[attrIDs[i]].attrType == FLOAT) {
						printf(" FLOAT %.6lf ", *(double*)out);
					} else if (_smm->_tables[tableID].attrs[attrIDs[i]].attrType == STRING) {
						printf(" STRING %s ", (char*)out);
						/*for (int j = 0; j < _smm->_tables[tableID].attrs[attrIDs[i]].attrLength; j++) {
							printf("%d ", ((char*)out)[j]);
						}*/
					}
					putchar('|');
				}
				putchar('\n');
			}
		}
		delete [] data;
		if (!hasNext) break;
	}
	if (outcnt > 100) {
		puts("...");
		printf("Altogether %d records.\n", outcnt);
	}
	if (found) {
		delete indexscan;
		_ixm->CloseIndex(indexID);
	}
	delete filescan, filehandle;
}

void QL_Manager::Select(string tableName1, string tableName2, vector<Relation> relations, vector<string> attrNames) {
	int tableID1 = _smm->_fromNameToID(tableName1);
	if (tableID1 == -1) {
		fprintf(stderr, "Error: such table does not exist!\n");
		return;
	}
	int tableID2 = _smm->_fromNameToID(tableName2);
	if (tableID2 == -1) {
		fprintf(stderr, "Error: such table does not exist!\n");
		return;
	}
	if (tableID1 == tableID2) {
		fprintf(stderr, "Error: cannot join same tables!\n");
		return;
	}
	vector<pair<int, int> > attrIDs;
	for (int i = 0; i < attrNames.size(); i++) {
		int pos = attrNames[i].find('.');
		if (pos == string::npos) {
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
		string table = attrNames[i].substr(0, pos);
		string attr = attrNames[i].substr(pos + 1);
		int attrID = -1;
		if (table == tableName1) {
			attrID = _smm->_fromNameToID(attr, tableID1);
			attrIDs.push_back(make_pair(tableID1, attrID));
		} else if (table == tableName2) {
			attrID = _smm->_fromNameToID(attr, tableID2);
			attrIDs.push_back(make_pair(tableID2, attrID));
		}
		if (attrID == -1) {
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
	}
	vector<pair<int, int> > attrID1, attrID2;
	for (int i = 0; i < relations.size(); i++) {
		int tableID = _smm->_fromNameToID(relations[i].table1);
		if (tableID == -1) {
			fprintf(stderr, "Error: invalid tables!\n");
			return;
		}
		int attr = _smm->_fromNameToID(relations[i].attr1, tableID);
		if (attr == -1) {
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
		attrID1.push_back(make_pair(tableID, attr));
		if (relations[i].data != nullptr) {
			attrID2.push_back(make_pair(-1, -1));
			continue;
		}
		tableID = _smm->_fromNameToID(relations[i].table2);
		if (tableID == -1) {
			fprintf(stderr, "Error: invalid tables!\n");
			return;
		}
		attr = _smm->_fromNameToID(relations[i].attr2, tableID);
		if (attr == -1) {
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
		attrID2.push_back(make_pair(tableID, attr));
		if (_smm->_tables[attrID1[i].first].attrs[attrID1[i].second].attrType != _smm->_tables[attrID2[i].first].attrs[attrID2[i].second].attrType) {
			fprintf(stderr, "Error: invalid comparison!\n");
			return;
		}
	}
	bool found = false;
	int indexAttr = -1, indexRel = -1, indexID = -1;
	IX_IndexScan *indexscan = nullptr;
	for (int i = 0; i < relations.size(); i++) {
		if (attrID2[i].first == -1) continue;
		if (attrID1[i].first == attrID2[i].first) continue;
		if (relations[i].op != EQ_OP) continue;
		if (_smm->_tables[attrID1[i].first].attrs[attrID1[i].second].haveIndex) {
			swap(attrID1[i], attrID2[i]);
		}
		if (!_smm->_tables[attrID2[i].first].attrs[attrID2[i].second].haveIndex) continue;
		if (attrID1[i].first != tableID1) {
			swap(tableID1, tableID2);
			tableName1.swap(tableName2);
		}
		found = true;
		indexAttr = attrID2[i].second; indexRel = i;
		_ixm->OpenIndex(_smm->_tables[attrID2[i].first].tableName.c_str(), _smm->_tables[attrID2[i].first].attrs[attrID2[i].second].attrName.c_str(), indexID);
		indexscan = new IX_IndexScan(fileManager, bufPageManager, indexID);
		break;
	}
	int fileID1 = _smm->_tableFileID[tableName1];
	RM_FileHandle *filehandle1 = new RM_FileHandle(fileManager, bufPageManager, fileID1);
	RM_FileScan *filescan1 = new RM_FileScan(fileManager, bufPageManager);
	if (!filescan1->OpenScan(filehandle1)) {
		delete filescan1, filehandle1;
		if (found) {
			_ixm->CloseIndex(indexID);
			delete indexscan;
		}
		return;
	}
	int fileID2 = _smm->_tableFileID[tableName2];
	RM_FileHandle *filehandle2 = new RM_FileHandle(fileManager, bufPageManager, fileID2);
	RM_FileScan *filescan2 = new RM_FileScan(fileManager, bufPageManager);
	if (!filescan2->OpenScan(filehandle2)) {
		delete filescan1, filehandle1, filescan2, filehandle2;
		if (found) {
			_ixm->CloseIndex(indexID);
			delete indexscan;
		}
		return;
	}
	int recordSize1 = _smm->_tables[tableID1].recordSize, recordSize2 = _smm->_tables[tableID2].recordSize;
	int pageID, slotID;
	int outcnt = 0;
	while (1) {
		BufType data1 = new unsigned int[recordSize1 >> 2], data2 = new unsigned int[recordSize2 >> 2];
		bool hasNext = filescan1->GetNextRecord(pageID, slotID, data1);
		if (found) {
			BufType searchData = data1 + _smm->_tables[attrID1[indexRel].first].attrs[attrID1[indexRel].second].offset;
			if (!indexscan->OpenScan(searchData, true)) {
				delete [] data1;
				delete [] data2;
				if (!hasNext) break;
				continue;
			}
		}
		if (!filescan2->OpenScan(filehandle2)) {
			delete [] data1;
			delete [] data2;
			if (!hasNext) break;
			continue;
		}
		while (1) {
			bool hasNext2;
			if (found) {
				hasNext2 = indexscan->GetNextEntry(pageID, slotID);
				filehandle2->GetRec(pageID, slotID, data2);
			} else {
				hasNext2 = filescan2->GetNextRecord(pageID, slotID, data2);
			}
			bool ok = true;
			unsigned long long *bitmap1 = (unsigned long long*)data1;
			unsigned long long *bitmap2 = (unsigned long long*)data2;
			for (int i = 0; i < relations.size(); i++) {
				BufType attr1;
				//cout << recordSize1 << " " << recordSize2 << endl;
				//cout << _smm->_tables[attrID1[i].first].attrs[attrID1[i].second].offset << " " << _smm->_tables[attrID2[i].first].attrs[attrID2[i].second].offset << endl;
				if (attrID1[i].first == tableID1) {
					attr1 = data1 + _smm->_tables[tableID1].attrs[attrID1[i].second].offset;
					if ((bitmap1[0] & (1ull << attrID1[i].second)) == 0) {
						ok = false; break;
					}
				} else {
					attr1 = data2 + _smm->_tables[tableID2].attrs[attrID1[i].second].offset;
					if ((bitmap2[0] & (1ull << attrID1[i].second)) == 0) {
						ok = false; break;
					}
				}
				//cout << *(int*)attr1 << endl;
				BufType attr2 = relations[i].data;
				if (attr2 == nullptr) {
					if (attrID2[i].first == tableID1) {
						attr2 = data1 + _smm->_tables[tableID1].attrs[attrID2[i].second].offset;
						if ((bitmap1[0] & (1ull << attrID2[i].second)) == 0) {
							ok = false; break;
						}
					} else {
						attr2 = data2 + _smm->_tables[tableID2].attrs[attrID2[i].second].offset;
						if ((bitmap2[0] & (1ull << attrID2[i].second)) == 0) {
							ok = false; break;
						}
					}
				}
				//cout << *(int*)attr2 << endl;
				ok = _compare(attr1, attr2, relations[i].op, _smm->_tables[attrID1[i].first].attrs[attrID1[i].second].attrType);
				if (i == indexRel && !ok) hasNext2 = false;
				if (!ok) break;
			}
			if (ok) {
				outcnt++;
				if (outcnt <= 100) {
					putchar('|');
					//cout << " bitmap: " << bitmap[0] << " |";
					for (int i = 0; i < attrIDs.size(); i++) {
						BufType out;
						if (attrIDs[i].first == tableID1) {
							out = data1 + _smm->_tables[tableID1].attrs[attrIDs[i].second].offset;
							if ((bitmap1[0] & (1ull << attrIDs[i].second)) == 0) {
								printf(" NULL |");
								continue;
							}
						} else {
							out = data2 + _smm->_tables[tableID2].attrs[attrIDs[i].second].offset;
							if ((bitmap2[0] & (1ull << attrIDs[i].second)) == 0) {
								printf(" NULL |");
								continue;
							}
						}
						//cout << " " << (bitmap[0] & (1ull << attrIDs[i])) << " ";
						if (_smm->_tables[attrIDs[i].first].attrs[attrIDs[i].second].attrType == INTEGER) {
							printf(" INT %d ", *(int*)out);
						} else if (_smm->_tables[attrIDs[i].first].attrs[attrIDs[i].second].attrType == FLOAT) {
							printf(" FLOAT %.6lf ", *(double*)out);
						} else if (_smm->_tables[attrIDs[i].first].attrs[attrIDs[i].second].attrType == STRING) {
							printf(" STRING %s ", (char*)out);
						}
						putchar('|');
					}
					putchar('\n');
				}
			}
			if (!hasNext2) break;
		}
		
		delete [] data1;
		delete [] data2;
		if (!hasNext) break;
	}
	if (outcnt > 100) {
		puts("...");
		printf("Altogether %d records.\n", outcnt);
	}
	if (found) {
		delete indexscan;
		_ixm->CloseIndex(indexID);
	}
	delete filescan1, filehandle1, filescan2, filehandle2;
}

void QL_Manager::Load(const string tableName, const string fileName) {
	int tableID = _smm->_fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: such table does not exist!\n");
		return;
	}
	ifstream load(fileName);
	string str;
	vector<string> attrs;
	for (int i = 0; i < _smm->_tables[tableID].attrNum; i++) {
		attrs.push_back(_smm->_tables[tableID].attrs[i].attrName);
	}
	//cout << tableName << endl;
	cout << "loading ..." << endl;
	//bool flag = false;
	while (getline(load, str)) {
		vector<BufType> values;
		string buf = "";
		int attrCnt = 0;
		for (int i = 0; i < str.length(); i++) {
			if (str[i] == '|') {
				//cout << buf << endl;
				if (_smm->_tables[tableID].attrs[attrCnt].attrType == INTEGER) {
					int *d = new int; *d = atoi(buf.c_str());
					values.push_back((BufType)d);
				} else if (_smm->_tables[tableID].attrs[attrCnt].attrType == FLOAT) {
					double *d = new double; *d = atof(buf.c_str());
					values.push_back((BufType)d);
				} else if (_smm->_tables[tableID].attrs[attrCnt].attrType == STRING) {
					char *d = new char[_smm->_tables[tableID].attrs[attrCnt].attrLength];
					memset(d, 0, sizeof(char) * _smm->_tables[tableID].attrs[attrCnt].attrLength);
					//cout << "char: " << _smm->_tables[tableID].attrs[attrCnt].attrLength << ' ' << buf.length() << endl;
					memcpy(d, buf.c_str(), min(_smm->_tables[tableID].attrs[attrCnt].attrLength, (int)buf.length()));
					values.push_back((BufType)d);
				}
				buf = "";
				attrCnt++;
			} else {
				buf.push_back(str[i]);
			}
		}
		//cout << endl;
		/*if (!flag) {
			cout << tableName << endl;
			cout << *(int*)values[0] << endl;
			printf("%s\n", (char*)values[1]);
			printf("%s\n", (char*)values[2]);
			flag = true;
		}*/
		Insert(tableName, attrs, values);
	}
	load.close();
	cout << "done." << endl;
}

bool QL_Manager::_compare(BufType data1, BufType data2, CompOp op, AttrType type) {
	if (type == STRING) {
		int result = strcmp((char*)data1, (char*)data2);
		if (op == EQ_OP) return result == 0;
		if (op == NE_OP) return result != 0;
		if (op == LT_OP) return result < 0;
		if (op == GT_OP) return result > 0;
		if (op == LE_OP) return result <= 0;
		if (op == GE_OP) return result >= 0;
	} else if (type == INTEGER) {
		if (op == EQ_OP) return *(int*)data1 == *(int*)data2;
		if (op == NE_OP) return *(int*)data1 != *(int*)data2;
		if (op == LT_OP) return *(int*)data1 < *(int*)data2;
		if (op == GT_OP) return *(int*)data1 > *(int*)data2;
		if (op == LE_OP) return *(int*)data1 <= *(int*)data2;
		if (op == GE_OP) return *(int*)data1 >= *(int*)data2;
	} else if (type == FLOAT) {
		if (op == EQ_OP) return *(double*)data1 == *(double*)data2;
		if (op == NE_OP) return *(double*)data1 != *(double*)data2;
		if (op == LT_OP) return *(double*)data1 < *(double*)data2;
		if (op == GT_OP) return *(double*)data1 > *(double*)data2;
		if (op == LE_OP) return *(double*)data1 <= *(double*)data2;
		if (op == GE_OP) return *(double*)data1 >= *(double*)data2;
	}
}
