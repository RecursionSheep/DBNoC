#include <bits/stdc++.h>
#include "sm.h"
using namespace std;

int main() {
	srand(time(0));
	FileManager *fileManager = new FileManager();
	BufPageManager *bufPageManager = new BufPageManager(fileManager);
	RM_Manager *rmm = new RM_Manager(fileManager, bufPageManager);
	IX_Manager *ixm = new IX_Manager(fileManager, bufPageManager);
	SM_Manager *smm = new SM_Manager(ixm, rmm, fileManager, bufPageManager);
	smm->OpenDB("abc");
	smm->DropTable("cba");
	TableInfo *table = new TableInfo();
	table->tableName = "abcd";
	table->attrNum = 2;
	table->attrs.push_back((AttrInfo){"a1", INTEGER, 4, true, true, nullptr, "", "", false, 0});
	table->attrs.push_back((AttrInfo){"a2", STRING, 5, false, false, nullptr, "", "", false, 0});
	smm->CreateTable(table);
	vector<string> t; t.push_back("a2");
	smm->CreateIndex("abcd", t);
	t.clear(); t.push_back("a1");
	smm->CreateIndex("abcd", t);
	puts("ok");
	smm->CloseDB();
	puts("ok");
	return 0;
}
