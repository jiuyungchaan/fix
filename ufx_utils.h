//
// Created by ht on 18-12-4.
//

#ifndef UFXADAPTER_TOOL_HPP
#define UFXADAPTER_TOOL_HPP

#include <stdio.h>
#include <sys/ioctl.h>
#include <t2sdk_interface.h>

int g2u(const char *inbuf, size_t inlen, char *outbuf, size_t outlen);
void ShowPacket(IF2UnPacker *lpUnPacker);

bool GetLocalMACIP(char *macAddress, char *Ip, const char *desturl);
#endif //UFXADAPTER_TOOL_HPP
