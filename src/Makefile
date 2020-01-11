CC = g++
STD11 = --std=c++11
OPT = -O2

main:
	cd pf && g++ -c -o pf.o pf.cpp
	cd rm && make
	cd ix && make
	cd sm && g++ -o create create.cpp && g++ -o drop drop.cpp && g++ -c -o sm_manager.o sm_manager.cpp $(STD11)
	cd ql && g++ -c -o ql_manager.o ql_manager.cpp $(STD11)
	g++ -c -o use.o use.cpp $(STD11)
	g++ -o use use.o pf/pf.o rm/rm_manager.o rm/rm_filehandle.o rm/rm_filescan.o ix/ix_manager.o ix/ix_indexhandle.o ix/ix_indexscan.o sm/sm_manager.o ql/ql_manager.o $(STD11)
