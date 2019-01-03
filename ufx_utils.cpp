//
// Created by ht on 18-12-4.
//

#ifndef UFXADAPTER_TOOL_HPP
#define UFXADAPTER_TOOL_HPP

#include <stdio.h>
#include <ctype.h>
#include <iconv.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <t2sdk_interface.h>

int code_convert(const char *from_charset, const char *to_charset, const char *inbuf, size_t inlen,
                 char *outbuf, size_t outlen) {
    iconv_t cd;
    char **pin = (char **) &inbuf;
    char **pout = &outbuf;

    cd = iconv_open(to_charset, from_charset);
    if (cd == 0)
        return -1;
    memset(outbuf, 0, outlen);
    if (iconv(cd, pin, &inlen, pout, &outlen) == -1)
        return -1;
    iconv_close(cd);
//    *pout = '\0';

    return 0;
}

int u2g(char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
    return code_convert("utf-8", "gb2312", inbuf, inlen, outbuf, outlen);
}

int g2u(const char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
    return code_convert("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);
}
void ShowPacket(IF2UnPacker *lpUnPacker)
{
    int i = 0, t = 0, j = 0, k = 0;

    for (i = 0; i < lpUnPacker->GetDatasetCount(); ++i)
    {
        // 设置当前结果集

        printf("记录集：%d/%d\r\n", i+1, lpUnPacker->GetDatasetCount());
        lpUnPacker->SetCurrentDatasetByIndex(i);

        // 打印所有记录
        for (j = 0; j < (int)lpUnPacker->GetRowCount(); ++j)
        {
            printf("\t第%d/%d条记录：\r\n", j+1,lpUnPacker->GetRowCount());
            // 打印每条记录
            for (k = 0; k < lpUnPacker->GetColCount(); ++k)
            {
                switch (lpUnPacker->GetColType(k))
                {
                    case 'I':
                        printf("\t【整数】%20s = %35d\r\n", lpUnPacker->GetColName(k), lpUnPacker->GetIntByIndex(k));
                        break;

                    case 'C':
                        printf("\t【字符】%20s = %35c\r\n", lpUnPacker->GetColName(k), lpUnPacker->GetCharByIndex(k));
                        break;

                    case 'S':

                        if (NULL != strstr((char *)lpUnPacker->GetColName(k),"password"))
                        {
                            printf("\t【字串】%20s = %35s\r\n", lpUnPacker->GetColName(k), "******");
                        } else {
                            char buff[128];
                            const char *gb2312_str = lpUnPacker->GetStrByIndex(k);
                            g2u(gb2312_str, strlen(gb2312_str), buff, sizeof(buff));
                            printf("\t【字串】%20s = %35s\r\n", lpUnPacker->GetColName(k), buff);
                        }
                        break;

                    case 'F':
                        printf("\t【数值】%20s = %35f\r\n", lpUnPacker->GetColName(k), lpUnPacker->GetDoubleByIndex(k));
                        break;

                    case 'R':
                    {
                        int nLength = 0;
                        void *lpData = lpUnPacker->GetRawByIndex(k, &nLength);
                        switch(nLength){
                            case 0:
                                printf("\t【数据】%20s = %35s\r\n", lpUnPacker->GetColName(k), "(N/A)");
                                break;
                            default:
                                printf("\t【数据】%20s = 0x", lpUnPacker->GetColName(k));
                                for(t = nLength; t < 11; t++){
                                    printf("   ");
                                }
                                unsigned char *p = (unsigned char *)lpData;
                                for(t = 0; t < nLength; t++){
                                    printf("%3x", *p++);
                                }
                                printf("\r\n");
                                break;
                        }
                        // 对2进制数据进行处理
                        break;
                    }

                    default:
                        // 未知数据类型
                        printf("未知数据类型。\n");
                        break;
                }
            }

            putchar('\n');

            lpUnPacker->Next();
        }

        putchar('\n');
    }
}

bool GetIpAddressByUrl(char *ip, const char *inurl) {
    const int allocsize = strlen(inurl) + 1;
    char *name = (char *) malloc(allocsize);
    memset(name, 0, allocsize);
    const char *url = strchr(inurl, ':');
    struct in_addr addr;

    if (url <= 0) {
        strcpy(name, url);
    }
    if (url[1] == '/' || url[1] == '\\') {//tcp:\\... or http:\\...
        url += 3;
        const char *end = strchr(url, ':');
        if (end <= 0) {
            strcpy(name, url);
        } else {
            strncpy(name, url, end - url);
        }
    } else {
        strncpy(name, inurl, url - inurl);
    }
    struct hostent *remoteHost;
    if (isalpha(name[0])) {
        remoteHost = gethostbyname(name);
        if (remoteHost == NULL)
            return false;
        addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];
        sprintf(ip, "%s", inet_ntoa(addr));
    } else {
        addr.s_addr = inet_addr(name);
        if (addr.s_addr == INADDR_NONE) {
            return false;
        } else {
            //remoteHost = gethostbyaddr((char *) &addr, 4, AF_INET);
            sprintf(ip, "%s", inet_ntoa(addr));
        }
    }
    return true;
}
//bool GetLocalMACIP(char* macAddress, char* Ip,const char* desturl)
//{
//    char szDestIp[64];
//    if( GetIpAddressByUrl(szDestIp, desturl) == false)
//    {
//        strcpy(macAddress, "Unknown");
//        strcpy(Ip,"Unknown");
//        return false;
//    }
//
//    #define SDKDebug(x) (std::cout<<x)
//	register int fd, intrface = 0;
//	ifreq buf[16];
//	struct ifconf ifc;
//
//	if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) >= 0)
//	{
//		ifc.ifc_len = sizeof buf;
//		ifc.ifc_buf = (caddr_t) buf;
//
//		if(!ioctl (fd, SIOCGIFCONF, (char *) &ifc))
//		{
//			intrface = ifc.ifc_len / sizeof (struct ifreq);
//			while (intrface-- > 0)
//			{
//
//				//get flags
//				if (!(ioctl (fd, SIOCGIFFLAGS, (char *) &buf[intrface])))
//				{
//					if (buf[intrface].ifr_flags & IFF_PROMISC)
//					{
//					}
//				}
//
//				//Jugde whether the net card status is up
//				if (buf[intrface].ifr_flags & IFF_UP)
//				{
//					//get ip
//					if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))
//					{
//						sprintf(Ip,"%s", inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
//					}
//
//					//get mac
//					if (!(ioctl (fd, SIOCGIFHWADDR, (char *) &buf[intrface])))
//					{
//						//是否为空
//						unsigned char result = 0xFF;
//						for (int t=0;t<6;t++)
//						{
//							result += ((unsigned char)buf[intrface].ifr_hwaddr.sa_data[t]);
//						}
//						if(result != 0)
//						{
//							sprintf(macAddress,"%02X%02X%02X%02X%02X%02x",
//								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[0],
//								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[1],
//								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[2],
//								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[3],
//								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[4],
//								(unsigned char)buf[intrface].ifr_hwaddr.sa_data[5]);
//							break;
//						}
//						else
//						{
//						}
//					}
//
//				}
//			}
//		}
//	}
//	close(fd);
//	return true;
//}
#endif //UFXADAPTER_TOOL_HPP
