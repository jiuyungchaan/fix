#include "x1trader.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

char ParseToOrderType(string order_type) {
  char ret = kOrderTypeLimit;
  if (strcasecmp(order_type.c_str(), "limit") == 0) {
    ret = kOrderTypeLimit;
  } else if (strcasecmp(order_type.c_str(), "market") == 0) {
    ret = kOrderTypeMarket;
  } else if (strcasecmp(order_type.c_str(), "market_limit") == 0) {
    ret = kOrderTypeMarketLimit;
  } else if (strcasecmp(order_type.c_str(), "stop") == 0) {
    ret = kOrderTypeStop;
  } else if (strcasecmp(order_type.c_str(), "stop_limit") == 0) {
    ret = kOrderTypeStopLimit;
  }
  return ret;
}

char ParseToDirection(string direction) {
  char ret = kDirectionBuy;
  if (strcasecmp(direction.c_str(), "buy") == 0) {
    ret = kDirectionBuy;
  } else if (strcasecmp(direction.c_str(), "sell") == 0){
    ret = kDirectionSell;
  }
  return ret;
}

char ParseToOffset(string offset) {
  char ret = kOffsetOpen;
  if (strcasecmp(offset.c_str(), "open") == 0) {
    ret = kOffsetOpen;
  } else if (strcasecmp(offset.c_str(), "close") == 0){
    ret = kOffsetClose;
  } else if (strcasecmp(offset.c_str(), "closetoday") == 0){
    ret = kOffsetCloseToday;
  }
  return ret;
}

char ParseToTimeInForce(string time_in_force) {
  char ret = kTimeInForceDay;
  if (strcasecmp(time_in_force.c_str(), "day") == 0) {
    ret = kTimeInForceDay;
  } else if (strcasecmp(time_in_force.c_str(), "gtc") == 0) {
    ret = kTimeInForceGTC;
  } else if (strcasecmp(time_in_force.c_str(), "fak") == 0) {
    ret = kTimeInForceFAK;
  } else if (strcasecmp(time_in_force.c_str(), "gtd") == 0) {
    ret = kTimeInForceGTD;
  }
  return ret;
}

int main(int argc, char **argv) {

  int arg_pos = 1;
  string cmd_file_name;
  string front = "tcp://203.187.171.248:50001";
  string account = "000100000119";
  string password = "200372";
  while (arg_pos < argc) {
    if (strcmp(argv[arg_pos], "-f") == 0) {
      front = argv[++arg_pos];
    }
    if (strcmp(argv[arg_pos], "-c") == 0) {
      cmd_file_name = argv[++arg_pos];
    }
    if (strcmp(argv[arg_pos], "-a") == 0) {
      account = argv[++arg_pos];
    }
    if (strcmp(argv[arg_pos], "-p") == 0) {
      password = argv[++arg_pos];
    }
    ++arg_pos;
  }

  X1Trader trader;
  trader.user_id_ = account;
  trader.password_ = password;
  trader.Logon(const_cast<char*>(front.c_str()));
  sleep(5);

  fstream cmd_file;
  cmd_file.open(cmd_file_name, fstream::in | fstream::app);
  while(!cmd_file.eof()) {
    char line[512];
    cmd_file.getline(line, 512);
    if (line[0] == '#') {
      cout << "\033[32mComment:[" << line << "]\033[0m" << endl;
      continue;
    } else if (strcmp(line, "") == 0) {
      cout << "Passing blank line..." << endl;
      continue;
    }
    cout << "\033[31mCommand:[" << line << "]\033[0m" << endl;
    char cmd;
    while(true) {
      cout << "\033[33mInput command-[E-Execute P-Pass Q-Quit] :\033[0m";
      scanf("%c", &cmd);
      getchar();
      if (cmd == 'E' || cmd == 'e' || cmd == 'P' || cmd == 'p' ||
          cmd == 'Q' || cmd == 'q') {
        break;
      } else {
        cout << "Invalid command : " << cmd << endl;
      }
    }
    if (cmd == 'E' || cmd == 'e') {
      istringstream line_stream(line);
      string request;
      line_stream >> request;
      if (strcasecmp(request.c_str(), "insert") == 0) {
        string symbol, instrument, price, stop_price, volume, direction, 
               offset, order_type, time_in_force;
        line_stream >> symbol >> instrument >> price >> stop_price >> volume 
                    >> direction >> offset >> order_type >> time_in_force;
        Order *order = new Order();
        snprintf(order->account, sizeof(order->account), "%s", account.c_str());
        snprintf(order->symbol, sizeof(order->symbol), "%s", symbol.c_str());
        snprintf(order->instrument_id, sizeof(order->instrument_id), "%s", instrument.c_str());
        order->limit_price = atof(price.c_str());
        order->stop_price = atof(stop_price.c_str());
        order->volume = atoi(volume.c_str());
        order->direction = ParseToDirection(direction);
        order->offset = ParseToOffset(offset);
        order->order_type = ParseToOrderType(order_type);
        order->time_in_force = ParseToTimeInForce(time_in_force);
        trader.ReqInsertOrder(order);
      } else if (strcasecmp(request.c_str(), "cancel") == 0) {
        string order_id;
        line_stream >> order_id;
        if (order_id == "-1") {
          string symbol, instrument, side, local_id, sys_id;
          cout << "\033[33mInput order info to cancel [LocalID SysID InstrumentID]:\033[0m" << endl;
          cin >> local_id >> sys_id >> instrument;
          getchar();
          trader.ReqCancelOrder(atoi(local_id.c_str()), atoi(sys_id.c_str()), instrument.c_str());
        } else {
          Order *order = new Order();
          order->orig_order_id = atoi(order_id.c_str());
          trader.ReqCancelOrder(order);
        }
      } else if (strcasecmp(request.c_str(), "position") == 0) {
        trader.ReqQryPosition();
      }
      sleep(3);
    } else if (cmd == 'P' || cmd == 'p') {
      continue;
    } else if (cmd == 'Q' || cmd == 'q') {
      cout << "Quit ..." << endl;
      break;
    }
  }

  char q;
  cout << "Confirm to quit ? y/n :";
  scanf("%c", &q);
  getchar();
  trader.Logout();
  sleep(3);

  // while(true) {
  //   sleep(1);
  // }
  return 0;
}
