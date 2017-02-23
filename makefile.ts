
CXX=g++
SRC=TsSecurityFtdcTraderApi.cpp
INCLUDE=-I../include
CXXFLAGS=-Wall -shared -fpic --std=c++0x
TARGET=libtssecuritytraderapi.so
LD_FLAGS=-lpthread

all:$(TARGET)

clean:
	rm libtssecuritytraderapi.so

$(TARGET):
	$(CXX) $(SRC) $(INCLUDE) -o $(TARGET) $(CXXFLAGS) $(LD_FLAGS)
	#scp -P 1022 libfixtraderapi.so root@192.168.91.100:/usr/local/lib/
    #cp libtstraderapi.so /home/amanoenko/root/usr/local/lib/
