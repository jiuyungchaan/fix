all:
	g++ -o libUFXTraderApi.so -DUFX -shared -fpic UFXCallback.cpp UFXCallback.h UFXTraderApi.h UFXTraderApi.cpp ufx_utils.cpp -lt2sdk -lssl -lcrypto
