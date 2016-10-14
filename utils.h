
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

const char* time_now() {
  static char timestamp_str[32];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  
  struct timeval tv;
  gettimeofday(&tv, NULL);
  
  snprintf(timestamp_str, sizeof(timestamp_str), 
           "%d%02d%02d-%02d:%02d:%02d.%03ld\n", 1900 + timeinfo->tm_year,
           1 + timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour+8,
           timeinfo->tm_min, timeinfo->tm_sec, tv.tv_usec / 1000);
  return timestamp_str;
}

