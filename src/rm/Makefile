CC = g++
STD11 = --std=c++11
CFLAGS = -g
DEPS = rm.h
OBJ = rm_filehandle.o rm_manager.o rm_filescan.o
OPT = -O2
main: $(OBJ)

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(STD11) $(OPT)

clean:
	rm *.o
