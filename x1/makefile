all:
	g++ -c -o x1trader.o x1trader.cpp -I./include -std=c++0x 
	g++ -o trader x1.cpp -I./include --std=c++0x x1trader.o libx1_dfitc_api.so -ldl /lib/x86_64-linux-gnu/libuuid.so.1 
