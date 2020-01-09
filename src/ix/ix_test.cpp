#include "ix.h"
#include <bits/stdc++.h>

using namespace std;

set<pair<double, pair<int, int> > > s;

int main() {
	srand(time(0));
	FileManager *fileManager = new FileManager();
	BufPageManager *bufPageManager = new BufPageManager(fileManager);
	IX_Manager *ixm = new IX_Manager(fileManager, bufPageManager);
	ixm->CreateIndex("test", "a", FLOAT, 8);
	int fileID;
	fprintf(stderr, "ok\n");
	ixm->OpenIndex("test", "a", fileID);
	IX_IndexHandle *index = new IX_IndexHandle(fileManager, bufPageManager, fileID);
	
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
		IX_IndexScan *scan = new IX_IndexScan(fileManager, bufPageManager, fileID);
		scan->OpenScan(&x, true);
		//fprintf(stderr, "set %.3lf\n", (*it).first);
		while (it != s.end()) {
			int page = -1, slot = -1;
			scan->GetNextEntry(page, slot);
			//fprintf(stderr, "%d %.3lf\n", (*it).second.first, (*it).first);
			assert((*it).second.first == page);
			it++;
		}
		scan->CloseScan();
		delete scan;
		//fprintf(stderr, "\n");
	}
	bufPageManager->close();
	ixm->CloseIndex(fileID);
	fprintf(stderr, "close\n");
	return 0;
}
