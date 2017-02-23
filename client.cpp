#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>

#include <string>
#include <map>
#include <fstream>
#include <cctype>
#include <algorithm>
#include <vector>


using namespace std;


int server_fd_;
struct sockaddr_in server_addr_;

void split(const string& str, const string& del, vector<string>& v) {
  string::size_type start, end;
  start = 0;
  end = str.find(del);
  while(end != string::npos) {
    v.push_back(str.substr(start, end - start));
    start = end + del.size();
    end = str.find(del, start);
  }
  if (start != str.length()) {
    v.push_back(str.substr(start));
  }
}

void* recv_thread(void *arg) {
  int recv_len;
  char buffer[512];
  while((recv_len = recv(server_fd_, buffer, 512, 0)) > 0) {
    buffer[recv_len] = '\0';
    printf("Data Received: %s\n", buffer);
  }
  printf("Failed to receive data! Exit thread...\n");  
}

int main() {
  // connect server
  char front_addr_[64] = "tcp://112.74.17.91:5354";
  vector<string> vals;
  split(front_addr_, "//", vals);
  if (vals.size() == 2) { 
    vector<string> addr;
    split(vals[1], ":", addr);
    if (addr.size() == 2) {
      int port = atoi(addr[1].c_str());
      server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
      if (server_fd_ == -1) {
        printf("Failed to create socket: %s-%d\n", strerror(errno), errno);
        exit(0);
      }
      char ip_addr[32];
      snprintf(ip_addr, sizeof(ip_addr), "%s", addr[0].c_str());
      printf("parse ip:%s:%d\n", ip_addr, port);
      memset(&server_addr_, 0, sizeof(server_addr_));
      server_addr_.sin_family = AF_INET;
      server_addr_.sin_port = htons(port);
      if (inet_pton(AF_INET, ip_addr, &server_addr_.sin_addr) <= 0) {
        printf("Failed to parse ip address: %s-%s-%d\n", ip_addr, 
               strerror(errno), errno);
        // trader_spi_->OnFrontDisconnected(1001);
        return 0;
      }
      if (connect(server_fd_, (struct sockaddr*)&server_addr_, 
                  sizeof(server_addr_)) < 0) {
        printf("Failed to connect: [%s:%d]-%s-%d\n", ip_addr, port, 
               strerror(errno), errno);
        // trader_spi_->OnFrontDisconnected(1001);
        return 0;
      }
      // trader_spi_->OnFrontConnected();

      pthread_t socket_thread_;
      int ret = pthread_create(&socket_thread_, NULL, recv_thread, NULL);
      if (ret != 0) {
        printf("Failed to create socket thread!\n");
        exit(0);
      }
      printf("Socket thread create successfully!\n");

      sleep(3);

      char message[512] = "COMMAND=SENDORDER;ACCOUNT=410082065696;ACTION=BUY;SYMBOL=000060.SZ;QUANTITY=100;TYPE=MARKET;DURATION=IC5;CLIENTID=kdbkkk";
      send(server_fd_, message, strlen(message), 0);

      sleep(5);
      printf("Data sent\n");      
    }
  } else {
    printf("ERROR: Invalid Front Address: %s\n", front_addr_);
    // trader_spi_->OnFrontDisconnected(1001);   
  }

  return 0;
}
