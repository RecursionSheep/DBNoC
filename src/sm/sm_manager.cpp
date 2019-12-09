/*
 * 由于这两周有组成原理的造机DDL，实在是没有时间完成整个系统管理模块，只实现了定义接口和两个函数。
 * 之后会补上剩下的接口实现和Parser部分。
 */

#include "sm.h"
#include <iostream>

SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) {
	_ixm = &ixm; _rmm = &rmm;
}
SM_Manager::~SM_Manager() {}

void SM_Manager::OpenDB(const char *DBName) {
	_DBName = DBName;
	system(strcat("cd ", DBName));
	_rmm->OpenFile("meta", _metaFileID);
	_metaFileHandle = new RM_FileHandle(_metaFileID);
	for (int i = 1; i < _metaFileHandle->_header.pageNumber; i++) {
		BufType buf;
		_metaFileHandle->GetRec(i, 0, buf);
		char *name = (char*)buf;
		std::string tableName(name, name + strlen(name));
		int fileID;
		_rmm->OpenFile(name, fileID);
		_tableFileID[tableName] = fileID;
	}
}
void SM_Manager::CloseDB() {
	_rmm->CloseFile(_metaFileID);
	for (int i = 1; i < _metaFileHandle->_header.pageNumber; i++) {
		BufType buf;
		_metaFileHandle->GetRec(i, 0, buf);
		char *name = (char*)buf;
		std::string tableName(name, name + strlen(name));
		_rmm->CloseFile(_tableFileID[tableName]);
	}
	system("cd ..");
}
void SM_Manager::CreateTable(const char *relName, int attrCount, AttrInfo *attr) {
	
}
// to be finished