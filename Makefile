all:	main


btree.o:btree.hpp btree.cpp
	g++ -c -g -o btree.o btree.cpp

main: 	testcase.cpp btree.o key.cpp foo
	g++ -g -Wall -o main testcase.cpp key.cpp btree.o
