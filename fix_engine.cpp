
#include <iostream>
#include <string>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <quickfix/FileStore.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/Log.h>

#include "fix_trader.h"

using namespace std;

int main(int argc, char **argv) {
  int arg_pos = 1;
  string config_file_name = "./test.cfg";
  while (arg_pos < argc) {
    if (strcmp(argv[arg_pos], "-f") == 0) {
      config_file_name = argv[++arg_pos];
    }
    ++arg_pos;
  }

  FixTrader fix_trader;
  cout << "initialize setting" << endl;
  FIX::SessionSettings settings(config_file_name);
  cout << "initialize store factory" << endl;
  FIX::FileStoreFactory store_factory(settings);
  cout << "initialize log factory" << endl;
  FIX::ScreenLogFactory log_factory(settings);
  cout << "initialize initiator" << endl;
  FIX::SocketInitiator initiator(fix_trader, store_factory, settings, 
                                 log_factory);

  cout << "initiator start" << endl;
  initiator.start();
  cout << "initiator run" << endl;
  fix_trader.run();
  while(true) {
    sleep(1);
  }
  initiator.stop();

  return 0;
}
