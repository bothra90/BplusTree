all:	main


btree.o:btree.hpp btree.cpp
	g++ -c -g -o btree.o btree.cpp

main: 	main.cpp btree.o key.cpp
	g++ -g -o main main.cpp key.cpp btree.o