all:
	g++ -c -o utils.o utils.cpp --std=c++0x
	g++ -c -o sequence_serialization.o sequence_serialization.cpp --std=c++0x
	g++ -c -o audit_trail.o audit_trail.cpp --std=c++0x
	g++ -c -o fix_trader.o fix_trader.cpp --std=c++0x
	g++ -o fix_engine fix_engine.cpp utils.o fix_trader.o audit_trail.o sequence_serialization.o -lquickfix --std=c++0x
	g++ -o fix_interface fix_interface.cpp utils.o -lquickfix -lfixtraderapi --std=c++0x
