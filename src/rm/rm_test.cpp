#include "rm.h"
#include <bits/stdc++.h>

using namespace std;

int main() {
	FileManager *fileManager = new FileManager();
	BufPageManager *bufPageManager = new BufPageManager(fileManager);
	RM_Manager *rmm = new RM_Manager(fileManager, bufPageManager);
	rmm->CreateFile("test", 20);
	int fileID;
	rmm->OpenFile("test", fileID);
	RM_FileHandle *handle = new RM_FileHandle(fileManager, bufPageManager, fileID);
	char *data = new char[80];
	printf("%d\n", handle->GetRec(1, 1, (BufType)data));
	int pageID, slotID;
	data[0] = 'a'; data[1] = 'b'; data[2] = 'c'; data[3] = 'd'; data[4] = '\0';
	handle->InsertRec(pageID, slotID, (BufType)data);
	printf("%d %d\n", pageID, slotID);
	handle->GetRec(pageID, slotID, (BufType)data);
	printf("%s\n", (char*)data);
	int d = 12;
	handle->InsertRec(pageID, slotID, (BufType)&d);
	printf("%d %d\n", pageID, slotID);
	handle->GetRec(pageID, slotID, (BufType)data);
	printf("%d\n", *((int*)data));
	handle->DeleteRec(pageID, slotID);
	printf("%d\n", handle->GetRec(pageID, slotID, (BufType)data));
	return 0;
}
