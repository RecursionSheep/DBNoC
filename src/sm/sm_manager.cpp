#include "sm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <algorithm>
#include <unistd.h>
using namespace std;

SM_Manager::SM_Manager(IX_Manager *ixm, RM_Manager *rmm, FileManager *_fileManager, BufPageManager *_bufPageManager) {
	_ixm = ixm; _rmm = rmm;
	fileManager = _fileManager; bufPageManager = _bufPageManager;
}
SM_Manager::~SM_Manager() {}

void SM_Manager::Show() {
	cout << _tableNum << " table(s) in " << _DBName << endl;
	for (int tableID = 0; tableID < _tableNum; tableID++) {
		cout << "Table " << _tables[tableID].tableName << ":" << endl;
		for (int attrID = 0; attrID < _tables[tableID].attrNum; attrID++) {
			cout << _tables[tableID].attrs[attrID].attrName << " ";
			if (_tables[tableID].attrs[attrID].attrType == INTEGER) cout << "INT";
			else if (_tables[tableID].attrs[attrID].attrType == FLOAT) cout << "FLOAT";
			else if (_tables[tableID].attrs[attrID].attrType == STRING) cout << "CHAR(" << _tables[tableID].attrs[attrID].attrLength << ")";
			if (_tables[tableID].attrs[attrID].notNull) cout << " NOT NULL";
			if (_tables[tableID].attrs[attrID].primary) cout << " PRIMARY KEY";
			if (_tables[tableID].attrs[attrID].defaultValue != nullptr) {
				if (_tables[tableID].attrs[attrID].attrType == INTEGER) cout << " DEFAULT " << *(int*)_tables[tableID].attrs[attrID].defaultValue;
				else if (_tables[tableID].attrs[attrID].attrType == FLOAT) cout << " DEFAULT " << *(double*)_tables[tableID].attrs[attrID].defaultValue;
				else if (_tables[tableID].attrs[attrID].attrType == STRING) cout << " DEFAULT '" << (char*)_tables[tableID].attrs[attrID].defaultValue << "'";
			}
			if (_tables[tableID].attrs[attrID].reference != "") {
				cout << " REFERENCES " << _tables[tableID].attrs[attrID].reference << "(" << _tables[tableID].attrs[attrID].foreignKeyName << ")";
			}
			cout << endl;
		}
		cout << endl;
	}
}
void SM_Manager::OpenDB(const string DBName) {
	_DBName.assign(DBName);
	chdir(DBName.c_str());
	ifstream metain("meta.db");
	if (metain.fail()) {
		fprintf(stderr, "Error: cannot open meta-data file!\n");
		return;
	}
	metain >> _tableNum;
	_tables.clear();
	_tableFileID.clear();
	for (int i = 0; i < _tableNum; i++) {
		_tables.push_back(TableInfo());
		metain >> _tables[i].tableName;
		metain >> _tables[i].attrNum;
		metain >> _tables[i].foreignNum;
		_tables[i].attrs.clear();
		_tables[i].foreign.clear();
		_tables[i].foreignSet.clear();
		_tables[i].primary.clear();
		metain >> _tables[i].recordSize;
		metain >> _tables[i].primarySize;
		for (int j = 0; j < _tables[i].attrNum; j++) {
			_tables[i].attrs.push_back(AttrInfo());
			_tables[i].attrs[j].notNull = _tables[i].attrs[j].primary = _tables[i].attrs[j].haveIndex = false;
			_tables[i].attrs[j].defaultValue = nullptr;
			_tables[i].attrs[j].reference = _tables[i].attrs[j].foreignKeyName = "";
			metain >> _tables[i].attrs[j].attrName;
			metain >> _tables[i].attrs[j].offset;
			string str;
			metain >> str;
			if (str == "INT") {
				_tables[i].attrs[j].attrType = INTEGER;
				_tables[i].attrs[j].attrLength = 4;
			} else if (str == "STRING") {
				_tables[i].attrs[j].attrType = STRING;
				metain >> _tables[i].attrs[j].attrLength;
			} else if (str == "FLOAT") {
				_tables[i].attrs[j].attrType = FLOAT;
				_tables[i].attrs[j].attrLength = 8;
			}
			while (1) {
				metain >> str;
				if (str == "NOTNULL") {
					_tables[i].attrs[j].notNull = true;
				} else if (str == "PRIMARY") {
					_tables[i].attrs[j].primary = true;
					_tables[i].primary.push_back(j);
				} else if (str == "DEFAULT") {
					if (_tables[i].attrs[j].attrType == INTEGER) {
						int *d = new int; metain >> *d;
						_tables[i].attrs[j].defaultValue = (BufType)d;
					} else if (_tables[i].attrs[j].attrType == STRING) {
						string d; getline(metain, d);
						_tables[i].attrs[j].defaultValue = (BufType)(new char[_tables[i].attrs[j].attrLength]);
						memset(_tables[i].attrs[j].defaultValue, 0, _tables[i].attrs[j].attrLength * sizeof(char));
						strcpy((char*)_tables[i].attrs[j].defaultValue, d.c_str());
					} else if (_tables[i].attrs[j].attrType = FLOAT) {
						double *d = new double; metain >> *d;
						_tables[i].attrs[j].defaultValue = (BufType)d;
					}
				} else if (str == "FOREIGN") {
					metain >> _tables[i].attrs[j].reference;
					metain >> _tables[i].attrs[j].foreignKeyName;
					if (_tables[i].foreignSet.find(_tables[i].attrs[j].reference) == _tables[i].foreignSet.end()) {
						_tables[i].foreign.push_back(_tables[i].attrs[j].reference);
						_tables[i].foreignSet.insert(_tables[i].attrs[j].reference);
					}
				} else if (str == "INDEX") {
					_tables[i].attrs[j].haveIndex = true;
				} else if (str == "END") {
					break;
				}
			}
		}
	}
	metain.close();
	for (int i = 0; i < _tableNum; i++) {
		int fileID;
		_rmm->OpenFile(_tables[i].tableName.c_str(), fileID);
		_tableFileID[_tables[i].tableName] = fileID;
	}
}
void SM_Manager::CloseDB() {
	ofstream metaout("meta.db");
	if (metaout.fail()) {
		fprintf(stderr, "Error: cannot open meta-data file!\n");
		return;
	}
	metaout << _tableNum << "\n";
	for (int i = 0; i < _tableNum; i++) {
		metaout << _tables[i].tableName << "\n";
		metaout << _tables[i].attrNum << "\n";
		metaout << _tables[i].foreignNum << "\n";
		metaout << _tables[i].recordSize << "\n";
		metaout << _tables[i].primarySize << "\n";
		for (int j = 0; j < _tables[i].attrNum; j++) {
			metaout << _tables[i].attrs[j].attrName << "\n";
			metaout << _tables[i].attrs[j].offset << "\n";
			if (_tables[i].attrs[j].attrType == INTEGER) {
				metaout << "INT\n";
			} else if (_tables[i].attrs[j].attrType == STRING) {
				metaout << "STRING\n";
				metaout << _tables[i].attrs[j].attrLength << "\n";
			} else if (_tables[i].attrs[j].attrType == FLOAT) {
				metaout << "FLOAT\n";
			}
			if (_tables[i].attrs[j].notNull) {
				metaout << "NOTNULL\n";
			}
			if (_tables[i].attrs[j].primary) {
				metaout << "PRIMARY\n";
			}
			if (_tables[i].attrs[j].defaultValue != nullptr) {
				metaout << "DEFAULT\n";
				if (_tables[i].attrs[j].attrType == INTEGER) {
					metaout << *((int*)_tables[i].attrs[j].defaultValue) << "\n";
				} else if (_tables[i].attrs[j].attrType == STRING) {
					metaout << (char*)_tables[i].attrs[j].defaultValue << "\n";
				} else if (_tables[i].attrs[j].attrType == FLOAT) {
					metaout << *((double*)_tables[i].attrs[j].defaultValue) << "\n";
				}
			}
			if (_tables[i].attrs[j].reference != "") {
				metaout << "FOREIGN\n" << _tables[i].attrs[j].reference << "\n" << _tables[i].attrs[j].foreignKeyName << "\n";
			}
			if (_tables[i].attrs[j].haveIndex) {
				metaout << "INDEX\n";
			}
			metaout << "END\n";
		}
	}
	metaout.close();
	for (int i = 0; i < _tableNum; i++) {
		if (!_rmm->CloseFile(_tableFileID[_tables[i].tableName]))
			fprintf(stderr, "Error: failed to close table file!\n");
	}
	_tables.clear();
	_tableFileID.clear();
	chdir("..");
}
void SM_Manager::CreateTable(TableInfo* table) {
	int recordSize = 8; // bitmap for checking whether each column is NULL
	for (int i = 0; i < _tableNum; i++) {
		//cout << _tables[i].tableName << " " << table->tableName << endl;
		if (_tables[i].tableName == table->tableName) {
			fprintf(stderr, "Error: table already exists!\n");
			return;
		}
	}
	for (int i = 0; i < table->attrNum; i++) {
		table->attrs[i].offset = recordSize >> 2;
		if (table->attrs[i].attrType == INTEGER) {
			table->attrs[i].attrLength = 4;
		} else if (table->attrs[i].attrType == FLOAT) {
			table->attrs[i].attrLength = 8;
		}
		while (table->attrs[i].attrLength % 4 != 0) table->attrs[i].attrLength++;
		int size = table->attrs[i].attrLength;
		recordSize += size;
		if (table->attrs[i].primary) {
			table->attrs[i].notNull = true;
		}
		if (table->attrs[i].reference != "") {
			if (table->attrs[i].reference == table->tableName) {
				fprintf(stderr, "Error: reference cannot be the same table!\n");
				return;
			}
			bool ok = false;
			for (int j = 0; j < _tables.size(); j++)
				if (_tables[j].tableName == table->attrs[i].reference) {
					for (int k = 0; k < _tables[j].attrNum; k++)
						if (_tables[j].attrs[k].primary && _tables[j].attrs[k].attrName == table->attrs[i].foreignKeyName) {
							table->attrs[i].attrType = _tables[j].attrs[k].attrType;
							table->attrs[i].attrLength = _tables[j].attrs[k].attrLength;
							ok = true;
							break;
						}
					if (ok) break;
				}
			if (!ok) {
				fprintf(stderr, "Error: invalid foreign key!\n");
				return;
			}
		}
	}
	if (recordSize >= PAGE_SIZE - 8) {
		fprintf(stderr, "Error: record size too large!\n");
		return;
	}
	_tables.push_back(*table);
	_tables[_tableNum].recordSize = recordSize;
	_tables[_tableNum].primarySize = 0;
	_tables[_tableNum].primary.clear();
	_tables[_tableNum].foreign.clear();
	_tables[_tableNum].foreignSet.clear();
	_tables[_tableNum].foreignNum = 0;
	for (int i = 0; i < _tables[_tableNum].attrNum; i++) {
		if (_tables[_tableNum].attrs[i].primary) {
			_tables[_tableNum].primary.push_back(i);
		}
		if (_tables[_tableNum].attrs[i].reference != "") {
			if (_tables[_tableNum].foreignSet.find(_tables[_tableNum].attrs[i].reference) != _tables[_tableNum].foreignSet.end()) continue;
			_tables[_tableNum].foreignSet.insert(_tables[_tableNum].attrs[i].reference);
			_tables[_tableNum].foreign.push_back(_tables[_tableNum].attrs[i].reference);
			_tables[_tableNum].foreignNum++;
		}
	}
	_rmm->CreateFile(table->tableName.c_str(), recordSize);
	int fileID;
	_rmm->OpenFile(table->tableName.c_str(), fileID);
	_tableFileID[table->tableName] = fileID;
	if (_tables[_tableNum].primary.size() != 0) {
		int primary_size = 0;
		for (int i = 0; i < _tables[_tableNum].primary.size(); i++) {
			primary_size += _tables[_tableNum].attrs[_tables[_tableNum].primary[i]].attrLength;
		}
		_tables[_tableNum].primarySize = primary_size;
		_ixm->CreateIndex(table->tableName.c_str(), "primary", STRING, primary_size);
	}
	_tableNum++;
}
void SM_Manager::DropTable(const string tableName) {
	bool found = false;
	int tableID = _fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: such table does not exist!\n");
		return;
	}
	if (_checkForeignKeyOnTable(tableID)) {
		fprintf(stderr, "Error: foreign key on the table!\n");
		return;
	}
	if (!_rmm->CloseFile(_tableFileID[_tables[tableID].tableName])) {
		fprintf(stderr, "Error: failed to close table file!\n");
		return;
	}
	_rmm->DestroyFile(_tables[tableID].tableName.c_str());
	system(("rm " + _tables[tableID].tableName + ".*").c_str());
	_tables.erase(_tables.begin() + tableID);
	_tableNum--;
}
void SM_Manager::CreateIndex(const string tableName, const vector<string> attrs) {
	if (attrs.size() == 1) {
		string attr = attrs[0];
		int tableID, attrID;
		tableID = _fromNameToID(tableName);
		if (tableID == -1) {
			fprintf(stderr, "Error: table does not exist!\n");
			return;
		}
		bool found = false;
		for (int i = 0; i < _tables[tableID].attrNum; i++) if (_tables[tableID].attrs[i].attrName == attr) {
			found = true;
			attrID = i;
			if (!_tables[tableID].attrs[i].notNull) {
				fprintf(stderr, "Error: the column must be not null!\n");
				return;
			}
			if (_tables[tableID].attrs[i].haveIndex) {
				fprintf(stderr, "Error: index already exists!\n");
				return;
			}
			break;
		}
		if (!found) {
			fprintf(stderr, "Error: column does not exist!\n");
			return;
		}
		//cout << "create index" << endl;
		int fileID = _tableFileID[tableName];
		RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, fileID);
		RM_FileScan *filescan = new RM_FileScan(fileManager, bufPageManager);
		if (!filescan->OpenScan(filehandle)) return;
		//cout << "open file scan" << endl;
		_tables[tableID].attrs[attrID].haveIndex = true;
		_ixm->CreateIndex(tableName.c_str(), attr.c_str(), _tables[tableID].attrs[attrID].attrType, _tables[tableID].attrs[attrID].attrLength);
		int indexID;
		_ixm->OpenIndex(tableName.c_str(), attr.c_str(), indexID);
		IX_IndexHandle *indexhandle = new IX_IndexHandle(fileManager, bufPageManager, indexID);
		//cout << "open index" << endl;
		while (1) {
			int pageID, slotID;
			BufType data = new unsigned int[filehandle->_header.recordSize];
			bool scanNotEnd = filescan->GetNextRecord(pageID, slotID, data);
			BufType insertData = data + _tables[tableID].attrs[attrID].offset;
			//cout << pageID << " " << slotID << " " << *(int*)insertData << endl;
			indexhandle->InsertEntry((void*)insertData, pageID, slotID);
			delete [] data;
			if (!scanNotEnd) break;
		}
		
		IX_IndexScan *indexscan = new IX_IndexScan(fileManager, bufPageManager, indexID);
		char *sss = new char[10000];
		memset(sss, 0, 10000);
		indexscan->OpenScan(sss, true);
		while (true) {
			int page, slot;
			bool hasNext = indexscan->GetNextEntry(page, slot);
			if (!hasNext) break;
		}
		
		delete filescan, filehandle, indexhandle;
		
		_ixm->CloseIndex(indexID);
	}
}
void SM_Manager::DropIndex(const string tableName, const vector<string> attrs) {
	if (attrs.size() == 1) {
		string attr = attrs[0];
		int tableID = _fromNameToID(tableName), attrID;
		if (tableID == -1) {
			fprintf(stderr, "Error: table does not exist!\n");
			return;
		}
		bool found = false;
		for (int i = 0; i < _tables[tableID].attrNum; i++) if (_tables[tableID].attrs[i].attrName == attr) {
			found = true;
			attrID = i;
			if (!_tables[tableID].attrs[i].haveIndex) {
				fprintf(stderr, "Error: index does not exist!\n");
				return;
			}
			break;
		}
		if (!found) {
			fprintf(stderr, "Error: column does not exist!\n");
			return;
		}
		_tables[tableID].attrs[attrID].haveIndex = false;
		_ixm->DestroyIndex(tableName.c_str(), attr.c_str());
	}
}
void SM_Manager::AddPrimaryKey(const string tableName, const vector<string> attrs) {
	int tableID = _fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: invalid table!\n");
		return;
	}
	if (_tables[tableID].primary.size() != 0) {
		fprintf(stderr, "Error: primary key already exists!\n");
		return;
	}
	int primarySize = 0;
	for (int i = 0; i < attrs.size(); i++) {
		int attrID = _fromNameToID(attrs[i], tableID);
		//cout << attrID << endl;
		if (attrID == -1) {
			fprintf(stderr, "Error: invalid columns!\n");
			_tables[tableID].primary.clear();
			return;
		}
		if (_tables[tableID].attrs[attrID].reference != "") {
			fprintf(stderr, "Error: primary keys can not be foreign keys!\n");
			_tables[tableID].primary.clear();
			return;
		}
		if (!_tables[tableID].attrs[attrID].notNull) {
			fprintf(stderr, "Error: primary keys must be not null!\n");
			_tables[tableID].primary.clear();
			return;
		}
		_tables[tableID].primary.push_back(attrID);
		primarySize += _tables[tableID].attrs[attrID].attrLength;
	}
	sort(_tables[tableID].primary.begin(), _tables[tableID].primary.end());
	for (int i = 0; i < _tables[tableID].primary.size(); i++) {
		_tables[tableID].attrs[_tables[tableID].primary[i]].primary = true;
	}
	_tables[tableID].primarySize = primarySize;
	int fileID = _tableFileID[tableName];
	RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, fileID);
	RM_FileScan *filescan = new RM_FileScan(fileManager, bufPageManager);
	bool scanNotEnd = filescan->OpenScan(filehandle);
	//cout << "open file scan" << endl;
	_ixm->CreateIndex(tableName.c_str(), "primary", STRING, primarySize);
	int indexID;
	_ixm->OpenIndex(tableName.c_str(), "primary", indexID);
	IX_IndexHandle *indexhandle = new IX_IndexHandle(fileManager, bufPageManager, indexID);
	//cout << "open index" << endl;
	BufType data = new unsigned int[filehandle->_header.recordSize];
	int cnt = 0;
	while (scanNotEnd) {
		int pageID, slotID;
		scanNotEnd = filescan->GetNextRecord(pageID, slotID, data);
		BufType insertData = _getPrimaryKey(tableID, data);
		cnt++;
		/*if (cnt <= 3) {
			for (int i = 0; i < primarySize; i++) {
				printf("%d ", ((char*)insertData)[i]);
			}
			puts("");
		}*/
		//cout << *(int*)insertData << endl;
		if (indexhandle->CheckEntry((void*)insertData)) {
			fprintf(stderr, "Error: repetitive primary keys!\n");
			for (int i = 0; i < _tables[tableID].primary.size(); i++) {
				_tables[tableID].attrs[_tables[tableID].primary[i]].primary = false;
			}
			_tables[tableID].primary.clear();
			_tables[tableID].primarySize = 0;
			delete [] data;
			delete [] insertData;
			delete filescan, filehandle, indexhandle;
			_ixm->CloseIndex(indexID);
			system((string("rm ") + tableName + string(".primary")).c_str());
			return;
		}
		/*if (cnt == 50) {
			system("sleep 10000");
		}*/
		//cout << "insert new" << endl;
		indexhandle->InsertEntry((void*)insertData, pageID, slotID);
		delete [] insertData;
		
		//if (!scanNotEnd) break;
	}
	
	/*for (int i = 0; i < _tables[tableID].primary.size(); i++) {
		cout << _tables[tableID].primary[i] << endl;
	}*/
	IX_IndexScan *indexscan = new IX_IndexScan(fileManager, bufPageManager, indexID);
	char *sss = new char[10000];
	memset(sss, 0, 10000);
	indexscan->OpenScan(sss, true);
	while (true) {
		int page, slot;
		bool hasNext = indexscan->GetNextEntry(page, slot);
		if (!hasNext) break;
	}
	
	delete [] data;
	delete filescan, filehandle, indexhandle;
	_ixm->CloseIndex(indexID);
}
void SM_Manager::DropPrimaryKey(const string tableName) {
	int tableID = _fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: invalid table!\n");
		return;
	}
	if (_tables[tableID].primary.size() == 0) {
		fprintf(stderr, "Error: primary key does not exist!\n");
		return;
	}
	for (int i = 0; i < _tableNum; i++) if (_tables[i].foreignSet.find(tableName) != _tables[i].foreignSet.end()) {
		fprintf(stderr, "Error: foreign keys on the table!\n");
		return;
	}
	for (int i = 0; i < _tables[tableID].attrNum; i++) {
		_tables[tableID].attrs[i].primary = false;
	}
	_tables[tableID].primary.clear();
	_tables[tableID].primarySize = 0;
	system((string("rm ") + tableName + string(".primary")).c_str());
}
void SM_Manager::AddForeignKey(const string tableName, vector<string> attrs, const string refName, vector<string> foreigns) {
	int tableID = _fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: invalid table!\n");
		return;
	}
	int refID = _fromNameToID(refName);
	if (refID == -1) {
		fprintf(stderr, "Error: invalid reference table!\n");
		return;
	}
	if (attrs.size() != foreigns.size() || foreigns.size() != _tables[refID].primary.size()) {
		fprintf(stderr, "Error: should cover all primary keys!\n");
		return;
	}
	if (_tables[refID].foreignSet.find(tableName) != _tables[refID].foreignSet.end()) {
		fprintf(stderr, "Error: foreign keys already exists!\n");
		return;
	}
	vector<int> attrIDs, foreignIDs;
	int keyNum = _tables[refID].primary.size();
	/*for (int i = 0; i < keyNum; i++) {
		cout << _tables[refID].primary[i] << endl;
	}*/
	for (int i = 0; i < keyNum; i++) {
		attrIDs.push_back(_fromNameToID(attrs[i], tableID));
		foreignIDs.push_back(_fromNameToID(foreigns[i], refID));
		//cout << attrIDs[i] << " " << foreignIDs[i] << endl;
		if (attrIDs[i] == -1 || foreignIDs[i] == -1) {
			fprintf(stderr, "Error: invalid columns!\n");
			return;
		}
		if (!_tables[tableID].attrs[attrIDs[i]].notNull) {
			fprintf(stderr, "Error: foreign keys must be NOT-NULL!\n");
			return;
		}
	}
	
	int fileID = _tableFileID[tableName];
	RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, fileID);
	RM_FileScan *filescan = new RM_FileScan(fileManager, bufPageManager);
	bool scanNotEnd = filescan->OpenScan(filehandle);
	int indexID;
	_ixm->OpenIndex(refName.c_str(), "primary", indexID);
	IX_IndexHandle *indexhandle = new IX_IndexHandle(fileManager, bufPageManager, indexID);
	int recordSize = _tables[tableID].recordSize;
	BufType data = new unsigned int[recordSize >> 2];
	int pageID, slotID;
	int refSize = _tables[refID].primarySize;
	BufType refData = new unsigned int[refSize >> 2];
	//cout << keyNum << endl;
	while (scanNotEnd) {
		scanNotEnd = filescan->GetNextRecord(pageID, slotID, data);
		int pos = 0;
		for (int i = 0; i < _tables[refID].primary.size(); i++) {
			string primaryName = _tables[refID].attrs[_tables[refID].primary[i]].attrName;
			int primaryID = _tables[refID].primary[i];
			//cout << primaryName << " " << primaryID << endl;
			int attr = -1;
			for (int j = 0; j < keyNum; j++) if (foreignIDs[j] == primaryID) {
				attr = attrIDs[j]; break;
			}
			//cout << _tables[tableID].attrs[attr].attrName << endl;
			memcpy(refData + pos, data + _tables[tableID].attrs[attr].offset, _tables[tableID].attrs[attr].attrLength);
			pos += (_tables[tableID].attrs[attr].attrLength >> 2);
		}
		bool check = indexhandle->CheckEntry(refData);
		if (!check) {
			cout << (char*)refData << endl;
			fprintf(stderr, "Error: some foreign keys are not in the reference table!\n");
			delete [] refData;
			delete [] data;
			_ixm->CloseIndex(indexID);
			delete indexhandle, filescan, filehandle;
			return;
		}
	}
	//cout << "finish" << endl;
	for (int i = 0; i < attrIDs.size(); i++) {
		_tables[tableID].attrs[attrIDs[i]].reference = refName;
		_tables[tableID].attrs[attrIDs[i]].foreignKeyName = _tables[refID].attrs[foreignIDs[i]].attrName;
	}
	_tables[refID].foreignSet.insert(tableName);
	_tables[refID].foreign.push_back(tableName);
	delete [] refData;
	delete [] data;
	_ixm->CloseIndex(indexID);
	delete indexhandle, filescan, filehandle;
}
void SM_Manager::DropForeignKey(const string tableName, string refName) {
	int tableID = _fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: invalid table!\n");
		return;
	}
	int refID = _fromNameToID(refName);
	if (refID == -1) {
		fprintf(stderr, "Error: invalid reference table!\n");
		return;
	}
	if (_tables[refID].foreignSet.find(tableName) == _tables[refID].foreignSet.end()) {
		fprintf(stderr, "Error: table does not have the foreign keys!\n");
		return;
	}
	_tables[refID].foreignSet.erase(tableName);
	for (int i = 0; i < _tables[refID].foreign.size(); i++) if (_tables[refID].foreign[i] == tableName) {
		_tables[refID].foreign.erase(_tables[refID].foreign.begin() + i);
		break;
	}
	for (int i = 0; i < _tables[tableID].attrNum; i++) if (_tables[tableID].attrs[i].reference == refName) {
		_tables[tableID].attrs[i].reference = "";
		_tables[tableID].attrs[i].foreignKeyName = "";
	}
}
void SM_Manager::AddColumn(const string tableName, AttrInfo attr) {
	int tableID = _fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: invalid table!\n");
		return;
	}
	attr.haveIndex = false;
	attr.notNull = true;
	attr.primary = false;
	attr.reference = attr.foreignKeyName = "";
	if (attr.attrType == INTEGER) {
		attr.attrLength = 4;
	} else if (attr.attrType == FLOAT) {
		attr.attrLength = 8;
	}
	while (attr.attrLength % 4 != 0) attr.attrLength++;
	int recordSize = _tables[tableID].recordSize;
	attr.offset = recordSize >> 2;
	int newRecordSize = recordSize + attr.attrLength;
	system(("rm " + tableName + ".*").c_str());
	_rmm->CreateFile((tableName + ".backup.temp").c_str(), newRecordSize);
	int fileID = _tableFileID[tableName];
	int newFileID;
	_rmm->OpenFile((tableName + ".backup.temp").c_str(), newFileID);
	RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, fileID);
	RM_FileHandle *newfilehandle = new RM_FileHandle(fileManager, bufPageManager, newFileID);
	RM_FileScan *filescan = new RM_FileScan(fileManager, bufPageManager);
	bool hasNext = filescan->OpenScan(filehandle);
	BufType data = new unsigned int[newRecordSize >> 2];
	int attrNum = _tables[tableID].attrNum;
	while (hasNext) {
		int pageID, slotID;
		hasNext = filescan->GetNextRecord(pageID, slotID, data);
		unsigned long long *bitmap = (unsigned long long*)data;
		bitmap[0] |= (1ull << attrNum);
		memcpy((char*)data + recordSize, attr.defaultValue, attr.attrLength);
		newfilehandle->InsertRec(pageID, slotID, data);
	}
	delete [] data;
	delete newfilehandle;
	_rmm->CloseFile(newFileID);
	delete filescan, filehandle;
	_rmm->CloseFile(fileID);
	system(("rm " + tableName).c_str());
	system(("mv " + tableName + ".backup.temp " + tableName).c_str());
	_rmm->OpenFile(tableName.c_str(), newFileID);
	_tableFileID[tableName] = newFileID;
	_tables[tableID].attrNum++;
	_tables[tableID].recordSize = newRecordSize;
	_tables[tableID].attrs.push_back(attr);
	vector<string> attrs;
	for (int i = 0; i < _tables[tableID].attrNum; i++) if (_tables[tableID].attrs[i].haveIndex) {
		attrs.clear();
		attrs.push_back(_tables[tableID].attrs[i].attrName);
		_tables[tableID].attrs[i].haveIndex = false;
		CreateIndex(tableName, attrs);
	}
	if (_tables[tableID].primary.size() != 0) {
		attrs.clear();
		for (int i = 0; i < _tables[tableID].primary.size(); i++) {
			attrs.push_back(_tables[tableID].attrs[_tables[tableID].primary[i]].attrName);
		}
		_tables[tableID].primary.clear();
		_tables[tableID].primarySize = 0;
		AddPrimaryKey(tableName, attrs);
	}
}
void SM_Manager::DropColumn(const string tableName, string attrName) {
	int tableID = _fromNameToID(tableName);
	if (tableID == -1) {
		fprintf(stderr, "Error: invalid table!\n");
		return;
	}
	int attrID = _fromNameToID(attrName, tableID);
	if (attrID == -1) {
		fprintf(stderr, "Error: invalid column!\n");
		return;
	}
	if (_tables[tableID].attrs[attrID].primary || _tables[tableID].attrs[attrID].reference != "") {
		fprintf(stderr, "Error: cannot drop primary keys or foreign keys!\n");
		return;
	}
	int recordSize = _tables[tableID].recordSize;
	int newRecordSize = recordSize - _tables[tableID].attrs[attrID].attrLength;
	system(("rm " + tableName + ".*").c_str());
	_rmm->CreateFile((tableName + ".backup.temp").c_str(), newRecordSize);
	int fileID = _tableFileID[tableName];
	int newFileID;
	_rmm->OpenFile((tableName + ".backup.temp").c_str(), newFileID);
	RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, fileID);
	RM_FileHandle *newfilehandle = new RM_FileHandle(fileManager, bufPageManager, newFileID);
	RM_FileScan *filescan = new RM_FileScan(fileManager, bufPageManager);
	bool hasNext = filescan->OpenScan(filehandle);
	BufType data = new unsigned int[recordSize >> 2];
	int attrNum = _tables[tableID].attrNum;
	int pos =_tables[tableID].attrs[attrID].offset;
	int len = _tables[tableID].attrs[attrID].attrLength >> 2;
	//cout << pos << " " << len << endl;
	for (int i = attrID + 1; i < attrNum; i++) {
		_tables[tableID].attrs[i].offset -= len;
	}
	while (hasNext) {
		int pageID, slotID;
		hasNext = filescan->GetNextRecord(pageID, slotID, data);
		unsigned long long *bitmap = (unsigned long long*)data;
		cout << (char*)(data + pos) << endl;
		for (int i = pos; i < (newRecordSize >> 2); i++) {
			data[i] = data[i + len];
		}
		cout << (char*)(data + pos) << endl;
		cout << bitmap[0] << ' ';
		for (int i = attrID; i < attrNum - 1; i++) {
			if (bitmap[0] & (1ull << (i + 1))) {
				bitmap[0] |= (1ull << i);
			} else {
				bitmap[0] &= ~(1ull << i);
			}
		}
		bitmap[0] &= ~(1ull << (attrNum - 1));
		cout << bitmap[0] << endl;
		newfilehandle->InsertRec(pageID, slotID, data);
	}
	delete [] data;
	delete newfilehandle;
	_rmm->CloseFile(newFileID);
	delete filescan, filehandle;
	_rmm->CloseFile(fileID);
	system(("rm " + tableName).c_str());
	system(("mv " + tableName + ".backup.temp " + tableName).c_str());
	_rmm->OpenFile(tableName.c_str(), newFileID);
	_tableFileID[tableName] = newFileID;
	_tables[tableID].attrNum--;
	_tables[tableID].recordSize = newRecordSize;
	_tables[tableID].attrs.erase(_tables[tableID].attrs.begin() + attrID);
	vector<string> attrs;
	for (int i = 0; i < _tables[tableID].attrNum; i++) if (_tables[tableID].attrs[i].haveIndex) {
		attrs.clear();
		attrs.push_back(_tables[tableID].attrs[i].attrName);
		_tables[tableID].attrs[i].haveIndex = false;
		CreateIndex(tableName, attrs);
	}
	if (_tables[tableID].primary.size() != 0) {
		attrs.clear();
		for (int i = 0; i < _tables[tableID].attrNum; i++) if (_tables[tableID].attrs[i].primary) {
			attrs.push_back(_tables[tableID].attrs[i].attrName);
		}
		_tables[tableID].primary.clear();
		_tables[tableID].primarySize = 0;
		AddPrimaryKey(tableName, attrs);
	}
}

bool SM_Manager::_checkForeignKeyOnTable(int tableID) {
	string tableName = _tables[tableID].tableName;
	for (int i = 0; i < _tableNum; i++) if (i != tableID) {
		if (_tables[i].foreignSet.find(tableName) != _tables[i].foreignSet.end()) {
			return true;
		}
	}
	return false;
}
int SM_Manager::_fromNameToID(const string tableName) {
	for (int i = 0; i < _tableNum; i++)
		if (_tables[i].tableName == tableName)
			return i;
	return -1;
}
int SM_Manager::_fromNameToID(const string attrName, const int tableID) {
	for (int i = 0; i < _tables[tableID].attrNum; i++)
		if (_tables[tableID].attrs[i].attrName == attrName)
			return i;
	return -1;
}
BufType SM_Manager::_getPrimaryKey(int tableID, BufType data) {
	BufType primaryKey = new unsigned int[_tables[tableID].primarySize >> 2];
	int pos = 0;
	for (int i = 0; i < _tables[tableID].attrNum; i++) if (_tables[tableID].attrs[i].primary) {
		memset(primaryKey + pos, 0, sizeof(char) * _tables[tableID].attrs[i].attrLength);
		memcpy(primaryKey + pos, data + _tables[tableID].attrs[i].offset, _tables[tableID].attrs[i].attrLength);
		pos += (_tables[tableID].attrs[i].attrLength >> 2);
	}
	return primaryKey;
}
