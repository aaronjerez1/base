#include "irc.h"
#include "../globals.h"
#include "../network/net.h"
#include <netdb.h>


/// CONNECTSOCKET
//bool ConnectSocket(const CAddress& addrConnect, int hSocketRet)
//{
//    hSocketRet = -1;
//    int hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//    if (hSocket == -1)
//    {
//        return false;
//    }
//
//    bool fRoutable = !(addrConnect.GetByte(3) == 10 || (addrConnect.GetByte(3) == 192 && addrConnect.GetByte(2) == 168));
//    bool fProxy = (addrProxy.ip && fRoutable);
//    struct sockaddr_in sockaddr = (fProxy ? addrProxy.GetSockAddr() : addrConnect.GetSockAddr());
//
//    if (connect(hSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1)
//    {
//        close(hSocket);
//        return false;
//    }
//
//    if (fProxy)
//    {
//        printf("Proxy connecting to %s\n", addrConnect.ToString().c_str());
//        char pszSocks4IP[] = "\4\1\0\0\0\0\0\0user";
//        memcpy(pszSocks4IP + 2, &addrConnect.port, 2);
//        memcpy(pszSocks4IP + 4, &addrConnect.ip, 4);
//        char* pszSocks4 = pszSocks4IP;
//        int nSize = sizeof(pszSocks4IP);
//
//        int ret = send(hSocket, pszSocks4, nSize, 0);
//        if (ret != nSize)
//        {
//            close(hSocket);
//            return error("Error sending to proxy\n");
//        }
//        char pchRet[8];
//        if (recv(hSocket, pchRet, 8, 0) != 8)
//        {
//            close(hSocket);
//            return error("Error reading proxy response\n");
//        }
//        if (pchRet[1] != 0x5a)
//        {
//            close(hSocket);
//            return error("Proxy returned error %d\n", pchRet[1]);
//        }
//        printf("Proxy connection established %s\n", addrConnect.ToString().c_str());
//    }
//
//    hSocketRet = hSocket;
//    return true;
//}
bool Wait(int nSeconds)
{
    if (fShutdown)
        return false;
    printf("Waiting %d seconds to reconnect to IRC\n", nSeconds);
    for (int i = 0; i < nSeconds; i++)
    {
        if (fShutdown)
            return false;
        usleep(1000);
    }
    return true;
}


/// <summary>
///
/// </summary>
/// <param name="hSocket"></param>
/// <param name="strLine"></param>
/// <returns></returns>

bool RecvLine(int hSocket, string& strLine)
{
    strLine = "";
    for (;;)  // Standard infinite loop instead of 'loop'
    {
        char c;
        int nBytes = recv(hSocket, &c, 1, 0);
        if (nBytes > 0)
        {
            if (c == '\n')
                continue;
            if (c == '\r')
                return true;
            strLine += c;
        }
        else if (nBytes <= 0)
        {
            if (!strLine.empty())
                return true;
            // socket closed
            printf("IRC socket closed\n");
            return false;
        }
        else
        {
            // socket error
            int nErr = errno;  // Use errno instead of WSAGetLastError()
            if (nErr != EMSGSIZE && nErr != EINTR && nErr != EINPROGRESS)
            {
                printf("IRC recv failed: %d\n", nErr);
                return false;
            }
        }
    }
}

void ThreadIRCSeed(void* parg)
{
    pthread_t current_thread = pthread_self();
    struct sched_param param;
    int policy;

    pthread_getschedparam(current_thread, &policy, &param);

    param.sched_priority = 0;
    pthread_setschedparam(current_thread, SCHED_OTHER, &param);

    int nErrorWait = 10;
    int nRetryWait = 10;

    while (!fShutdown)
    {
        // TODO find libera chat ip?
        CAddress addrConnect;
        struct hostent* phostent = gethostbyname("irc.libera.chat");
        if (phostent && phostent->h_addr_list && phostent->h_addr_list[0])
            addrConnect = CAddress(*(uint32_t*)phostent->h_addr_list[0], htons(6697)); 
        else
            addrConnect = CAddress("irc.libera.chat:6697");

        int hSocket;
        if (!ConnectSocket(addrConnect, hSocket))
        {
            printf("IRC connect failed\n");
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }
        //
        /// pointer
    }
}