
#include <iostream>
#include <string>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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
  FIX::SessionSettings settings(config_file_name);
  FIX::FileStoreFactory store_factory(settings);
  FIX::ScreenLogFactory log_factory(settings);
  FIX::SocketInitiator initiator(fix_trader, store_factory, settings, 
                                 log_factory);

  initiator.start();
  fix_trader.run();
  initiator.stop();

  return 0;
}
