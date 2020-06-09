all:
	gcc -shared -fPIC -o ddmon.so ddmon.c -ldl
	gcc -Wno-format -o ddchck ddchck.c -pthread
	gcc -g -o test1 test1.c -pthread
	gcc -g -o test2 test2.c -pthread
	gcc -g -o test2 test2.c -pthread
	gcc -g -o test3 test3.c -pthread