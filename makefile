all:
	g++ -c -o utils.o utils.cpp --std=c++0x
	g++ -c -o sequence_serialization.o sequence_serialization.cpp --std=c++0x
	g++ -c -o audit_trail.o audit_trail.cpp --std=c++0x
	g++ -c -o fix_trader.o fix_trader.cpp --std=c++0x
	#g++ -o fix_engine fix_engine.cpp utils.o fix_trader.o audit_trail.o sequence_serialization.o -lquickfix --std=c++0x
	#g++ -o fix_interface fix_interface.cpp utils.o -lquickfix -lfixtraderapi --std=c++0x
	#g++ -o dropcopy_interface fix_interface.cpp utils.o -lquickfix -lfixqueryapi --std=c++0x
	#g++ -o ts_interface ts_interface.cpp utils.o libtssecuritytraderapi.so libtssecurityqueryapi.so --std=c++0x
	#g++ -o cox_interface cox_interface.cpp utils.o ./libcoxtraderapi.so ./libcoxqueryapi.so --std=c++0x
	g++ -o bo_interface bo_interface.cpp utils.o ./libbotraderapi.so ./libboqueryapi.so --std=c++0x
