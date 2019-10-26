#include "rm.h"
#include <iostream>

static char *RM_Warning[] = {
	(char*)"invalid RID",
	(char*)"bad record size",
	(char*)"invalid record object",
	(char*)"invalid bit operation",
	(char*)"page is full",
	(char*)"invalid file",
	(char*)"invalid file handle",
	(char*)"invalid file scan",
	(char*)"end of page",
	(char*)"end of file",
	(char*)"invalid filename"
};

void RM_PrintError(RC rc) {
	if (rc >= START_RM_WARN && rc <= RM_LASTWARN)
		std::cerr << "RM warning: " << RM_Warning[rc - START_RM_WARN] << endl;
	else if (rc >= START_RM_ERR && rc <= RM_LASTERROR)
		std::cerr << "RM error" << endl;
}
