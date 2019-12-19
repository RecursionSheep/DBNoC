#include "ix.h"
#include <bits/stdc++.h>

using namespace std;

set<pair<double, pair<int, int> > > s;

int main() {
	srand(233);
	FileManager *fileManager = new FileManager();
	BufPageManager *bufPageManager = new BufPageManager(fileManager);
	IX_Manager *ixm = new IX_Manager(fileManager, bufPageManager);
	ixm->CreateIndex("test", 1, FLOAT, 8);
	int fileID;
	fprintf(stderr, "ok\n");
	ixm->OpenIndex("test", 1, fileID);
	IX_IndexHandle *index = new IX_IndexHandle(fileManager, bufPageManager, 1, fileID);
	
	for (int i = 1; i <= 10000; i++) {
		double x = (rand() % 2000) / 1000.0;
		fprintf(stderr, "Insert %.3lf %d\n", x, i);
		index->InsertEntry(&x, i, i);
		s.insert(make_pair(x, make_pair(i, i)));
		if (i > 100 && (rand() % 100 < 15)) {
			int k = rand() % s.size();
			auto it = s.begin();
			for (int j = 0; j < k; j++) it++;
			fprintf(stderr, "Delete %.3lf %d\n", (*it).first, (*it).second.first);
			x = (*it).first;
			index->DeleteEntry(&x, (*it).second.first, (*it).second.second);
			s.erase(it);
		}
		x = (rand() % 2000) / 1000.0;
		fprintf(stderr, "Query %.3lf\n", x);
		auto it = s.lower_bound(make_pair(x, make_pair(-1, -1)));
		IX_IndexScan *scan = new IX_IndexScan(fileManager, bufPageManager, 1, fileID);
		scan->OpenScan(&x, true);
		fprintf(stderr, "ok\n");
		while (it != s.end()) {
			int page = -1, slot = -1;
			assert(scan->GetNextEntry(page, slot));
			if ((*it).second.first != page) fprintf(stderr, "%d %d\n", (*it).second.first, page);
			assert((*it).second.first == page);
			it++;
		}
		fprintf(stderr, "ok\n");
		scan->CloseScan();
		delete scan;
		//fprintf(stderr, "\n");
	}
	ixm->CloseIndex(1, fileID);
	bufPageManager->close();
	return 0;
}
