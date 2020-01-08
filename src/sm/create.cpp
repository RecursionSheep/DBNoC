#include <iostream>
#include <cstring>
#include <unistd.h>

using namespace std;

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " DBName\n";
		return 1;
	}
	char command[100] = "mkdir ";
	system(strcat(command, argv[1]));
	chdir(argv[1]);
	freopen("meta.db", "w", stdout);
	printf("0\n");
	fclose(stdout);
	return 0;
}
