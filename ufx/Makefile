all:
	g++ -o libufxtraderapi.so -DNDEBUG -DUFX -shared -fpic UFXCallback.cpp UFXCallback.h UFXTraderApi.h UFXTraderApi.cpp ufx_utils.cpp -lt2sdk -lssl -lcrypto
debug:
	g++ -o libufxtraderapi.so -DUFX -shared -fpic UFXCallback.cpp UFXCallback.h UFXTraderApi.h UFXTraderApi.cpp ufx_utils.cpp -lt2sdk -lssl -lcrypto
