#include "sm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
using namespace std;

SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) {
	_ixm = &ixm; _rmm = &rmm;
}
SM_Manager::~SM_Manager() {}

void SM_Manager::OpenDB(const string DBName) {
	_DBName.assign(DBName);
	chdir(DBName.c_str());
	ifstream metain("meta.db");
	metain >> _tableNum;
	_tables.clear();
	_tableFileID.clear();
	for (int i = 0; i < _tableNum; i++) {
		_tables.push_back(TableInfo());
		metain >> _tables[i].tableName;
		metain >> _tables[i].attrNum;
		_tables[i].attrs.clear();
		metain >> _tables[i].recordSize;
		for (int j = 0; j < _tables[i].attrNum; j++) {
			_tables[i].attrs.push_back(AttrInfo())
			_tables[i].attrs[j].notNull = _tables[i].attrs[j].primary = _tables[i].attrs[j].haveIndex = false;
			_tables[i].attrs[j].defaultValue = nullptr;
			_tables[i].attrs[j].reference = _tables[i].attrs[j].foreignKeyName = "";
			metain >> _tables[i].attrs[j].attrName;
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
				} else if (str == "DEFAULT") {
					if (_tables[i].attrs[j].attrType == INTEGER) {
						int *d = new int; metain >> *d;
						_tables[i].attrs[j].defaultValue = (BufType)d;
					} else if (_tables[i].attrs[j].attrType == STRING) {
						string d; getline(metain, d);
						_tables[i].attrs[j].defaultValue = (BufType)(new char[attr.attrLength]);
						strcpy((char*)_tables[i].attrs[j].defaultValue, d.c_str());
					} else if (_tables[i].attrs[j].attrType = FLOAT) {
						double *d = new double; metain >> d;
						_tables[i].attrs[j].defaultValue = (BufType)d;
					}
				} else if (str == "FOREIGN") {
					metain >> _tables[i].attrs[j].reference;
					metain >> _tables[i].attrs[j].foreignKeyName;
				} else if (str == "INDEX") {
					_tables[i].attrs[j].haveIndex = true;
				} else if (str == "END") {
					break;
				}
			}
		}
		int fileID;
		_rmm->OpenFile(_tables[i].tableName.c_str(), fileID);
		_tableFileID[_tables[i].tableName] = fileID;
	}
	metain.close();
}
void SM_Manager::CloseDB() {
	ofstream metaout("meta.db");
	metaout << _tableNum << "\n";
	for (int i = 0; i < _tableNum; i++) {
		metaout << _tables[i].tableName << "\n";
		metaout << _tables[i].attrNum << "\n";
		metaout << _tables[i].recordSize << "\n";
		for (int j = 0; j < _tables[i].attrNum; j++) {
			metaout << _tables[i].attrs[j].attrName << "\n";
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
		_rmm->CloseFile(_tableFileID[_tables[i].tableName]);
	}
	metain.close();
	_tables.clear();
	_tableFileID.clear();
	chdir("..");
}
void SM_Manager::CreateTable(const TableInfo* table) {
	int recordSize = 16; // bitmap for checking whether each column is NULL
	int primaryCnt = 0;
	for (int i = 0; i < table->attrNum; i++) {
		int size = table->attrs[i].attrLength;
		while (size % 4 != 0) size++;
		recordSize += size;
		if (table->attrs[i].primary) primaryCnt++;
		if (primaryCnt > 1) {
			fprintf(stderr, "Error: more than 1 primary key!\n");
			return;
		}
		if (table->attrs[i].reference != "") {
			bool ok = false;
			for (int j = 0; j < _tables.size(); j++)
				if (_tables[j].tableName == table->attrs[i].reference) {
					for (int k = 0; k < _tables[j].attrNum; k++)
						if (_tables[j].attrs[k].attrName == table->attrs[i].foreignKeyName) {
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
	if (recordSize >= PAGE_SIZE - 4) {
		fprintf(stderr, "Error: record size too large!\n");
		return;
	}
	_tables.push_back(*table);
	_tables[_tableNum].recordSize = recordSize;
	_rmm->CreateFile(table->tableName.c_str(), recordSize);
	int fileID;
	_rmm->OpenFile(table->tableName.c_str(), fileID);
	_tableFileID[table->tableName] = fileID;
	_tableNum++;
}
