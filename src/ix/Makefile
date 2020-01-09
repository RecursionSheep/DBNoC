CC = g++
STD11 = --std=c++11
CFLAGS = -g
DEPS = ix.h
OBJ = ix_indexhandle.o ix_indexscan.o ix_manager.o
OPT = -O2
main: $(OBJ)

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(STD11) $(OPT)

clean:
	rm *.o
