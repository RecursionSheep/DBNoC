#include "rm.h"
#include <bits/stdc++.h>

using namespace std;

set<pair<double, pair<int, int> > > rec;

int main() {
	srand(time(0));
	FileManager *fileManager = new FileManager();
	BufPageManager *bufPageManager = new BufPageManager(fileManager);
	RM_Manager *rmm = new RM_Manager(fileManager, bufPageManager);
	rmm->CreateFile("test", 8);
	int fileID;
	if (!rmm->OpenFile("test", fileID)) return 0;
	RM_FileHandle *handle = new RM_FileHandle(fileManager, bufPageManager, fileID);
	int page, slot;
	double *t = new double(0.0);
	//if (!handle->GetRec(1, 0, (BufType)t)) return 0;
	//fprintf(stderr, "%.4lf\n", *t);
	//return 0;
	for (int i = 1; i <= 500000; i++) {
		int page, slot;
		double *d = new double(rand() / 1000.0);
		handle->InsertRec(page, slot, (BufType)d);
		//fprintf(stderr, "Insert %.4lf\n", *d);
		rec.insert(make_pair(*d, make_pair(page, slot)));
		if (i > 100 && rand() % 10 < 3) {
			double t = rand() / 1000.0;
			auto it = rec.lower_bound(make_pair(t, make_pair(-1, -1)));
			if (it == rec.end()) continue;
			//fprintf(stderr, "Delete %.4lf\n", t);
			handle->DeleteRec((*it).second.first, (*it).second.second);
			rec.erase(it);
		}
		if (i > 50 && rand() % 10 < 7) {
			double t = rand() / 1000.0;
			auto it = rec.lower_bound(make_pair(t, make_pair(-1, -1)));
			if (it == rec.end()) continue;
			page = (*it).second.first; slot = (*it).second.second;
			t = rand() / 1000.0;
			//fprintf(stderr, "Update %.4lf to %.4lf\n", (*it).first, t);
			handle->UpdateRec(page, slot, (BufType)&t);
			rec.erase(it);
			rec.insert(make_pair(t, make_pair(page, slot)));
		}
		/*RM_FileScan *scan = new RM_FileScan(fileManager, bufPageManager);
		scan->OpenScan(handle);
		double *t = new double;
		while (scan->GetNextRecord(page, slot, (BufType)t)) {
			fprintf(stderr, "%d %d %.4lf\n", page, slot, *t);
			auto it = rec.find(make_pair(t, make_pair(page, slot)));
			if (it == rec.end()) {
				puts("failed");
				break;
			}
			rec.erase(it);
		}
		fprintf(stderr, "\n");*/
	}
	RM_FileScan *scan = new RM_FileScan(fileManager, bufPageManager);
	scan->OpenScan(handle);
	while (scan->GetNextRecord(page, slot, (BufType)t)) {
		//fprintf(stderr, "%d %d %.4lf\n", page, slot, *t);
		auto it = rec.find(make_pair(*t, make_pair(page, slot)));
		if (it == rec.end()) {
			puts("failed");
			break;
		}
		/*if ((*it).second.first != page || (*it).second.second != slot) {
			fprintf(stderr, "%d %d %.4lf\n", (*it).second.first, (*it).second.second, *t);
			puts("failed");
			break;
		}*/
		rec.erase(it);
	}
	if (rec.size() > 0) puts("failed");
	fprintf(stderr, "%d\n", handle->_header.pageNumber);
	rmm->CloseFile(fileID);
	system("del test");
	return 0;
}
