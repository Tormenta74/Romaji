
all: st_test

st_test: st_test.o symtable.o
	g++ -o $@ $^ -g -gdwarf-2 -O0

%.o : %.cpp
	g++ -o $@ -c $<

clean:
	rm -r *.o st_test
