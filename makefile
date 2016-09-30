all:
	g++ -c -o fix_trader.o fix_trader.cpp --std=c++0x
	g++ -o fix_engine fix_engine.cpp fix_trader.o -lquickfix --std=c++0x
