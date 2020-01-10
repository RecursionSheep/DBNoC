#include "sm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
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
					_tables[i].primary.push_back(i);
				} else if (str == "DEFAULT") {
					if (_tables[i].attrs[j].attrType == INTEGER) {
						int *d = new int; metain >> *d;
						_tables[i].attrs[j].defaultValue = (BufType)d;
					} else if (_tables[i].attrs[j].attrType == STRING) {
						string d; getline(metain, d);
						_tables[i].attrs[j].defaultValue = (BufType)(new char[_tables[i].attrs[j].attrLength]);
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
		int size = table->attrs[i].attrLength;
		while (size % 4 != 0) size++;
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
	if (!_rmm->CloseFile(_tableFileID[_tables[tableID].tableName]))
		fprintf(stderr, "Error: failed to close table file!\n");
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
		_tables[tableID].attrs[attrID].haveIndex = true;
		_ixm->CreateIndex(tableName.c_str(), attr.c_str(), _tables[tableID].attrs[attrID].attrType, _tables[tableID].attrs[attrID].attrLength);
		int fileID = _tableFileID[tableName];
		RM_FileHandle *filehandle = new RM_FileHandle(fileManager, bufPageManager, fileID);
		RM_FileScan *filescan = new RM_FileScan(fileManager, bufPageManager);
		if (!filescan->OpenScan(filehandle)) return;
		int indexID;
		_ixm->OpenIndex(tableName.c_str(), attr.c_str(), indexID);
		IX_IndexHandle *indexhandle = new IX_IndexHandle(fileManager, bufPageManager, indexID);
		while (1) {
			int pageID, slotID;
			BufType data;
			bool scanNotEnd = filescan->GetNextRecord(pageID, slotID, data);
			data = data + _tables[tableID].attrs[attrID].offset;
			indexhandle->InsertEntry((void*)data, pageID, slotID);
			if (!scanNotEnd) break;
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
		memcpy(primaryKey + pos, data + _tables[tableID].attrs[i].offset, _tables[tableID].attrs[i].attrLength);
		pos += (_tables[tableID].attrs[i].attrLength >> 2);
	}
	return primaryKey;
}
