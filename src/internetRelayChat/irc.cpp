#include "irc.h"
//#include "../globals.h"
#include "../network/net.h"
#include <netdb.h>
#include "../shared/base58.h" // globals.h come from here
#include "../database/addressdb/addressdb.h"

std::map<std::vector<unsigned char>, CAddress> mapIRCAddresses;
CCriticalSection cs_mapIRCAddresses;

#pragma pack(push, 1)  // Save current packing and set to 1-byte alignment
struct ircaddr
{
    int ip;     // 4 bytes
    short port; // 2 bytes
};
#pragma pack(pop)  // Restore previous packing setting


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

bool DecodeAddress(string str, CAddress& addr)
{
    vector<unsigned char> vch;
    //if (!DecodeBase58Check(str.substr(1), vch))
    //    return false;

    struct ircaddr tmp;
    if (vch.size() != sizeof(tmp))
        return false;
    memcpy(&tmp, &vch[0], sizeof(tmp));

    addr = CAddress(tmp.ip, tmp.port);
    return true;
}


static bool Send(int hSocket, const char* pszSend)
{
    if (strstr(pszSend, "PONG") != pszSend)
        printf("SENDING: %s\n", pszSend);
    const char* psz = pszSend;
    const char* pszEnd = psz + strlen(psz);
    while (psz < pszEnd)
    {
        int ret = send(hSocket, psz, pszEnd - psz, 0);
        if (ret < 0)
        {
            printf("send error: %s\n", strerror(errno));
            return false;
        }
        psz += ret;
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

bool RecvLineIRC(int hSocket, string& strLine)
{
    loop
    {
        bool fRet = RecvLine(hSocket, strLine);
        if (fRet)
        {
            if (fShutdown)
                return false;
            vector<string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords[0] == "PING")
            {
                strLine[1] = 'O';
                strLine += '\r';
                Send(hSocket, strLine.c_str());
                continue;
            }
        }
        return fRet;
    }
}

bool RecvUntil(int hSocket, const char* psz1, const char* psz2 = NULL, const char* psz3 = NULL)
{
    loop
    {
        string strLine;
        if (!RecvLineIRC(hSocket, strLine))
            return false;
        printf("IRC %s\n", strLine.c_str());
        if (psz1 && strLine.find(psz1) != -1)
            return true;
        if (psz2 && strLine.find(psz2) != -1)
            return true;
        if (psz3 && strLine.find(psz3) != -1)
            return true;
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

        if (!RecvUntil(hSocket, "Found your hostname", "using your IP address instead", "Couldn't look up your hostname"))
        {
            close(hSocket);
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }
        
        //sleep(500);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        Send(hSocket, "JOIN #basecoin222\r\n");
        Send(hSocket, "WHO #basecoin222\r\n");

        int64 nStart = GetTime();
        string strLine;

        while (!fShutdown && RecvLineIRC(hSocket, strLine))
        {
            if (strLine.empty() || strLine.size() > 900 || strLine[0] != ':')
                continue;
            printf("IRC %s\n", strLine.c_str());

            std::vector<std::string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords.size() < 2)
                continue;

            char pszName[10000];
            pszName[0] = '\0';

            if (vWords[1] == "352" && vWords.size() >= 8)
            {
                // index 7 is limited to 16 characters
                // could get full length name at index 10, but would be different from join messages
                strcpy(pszName, vWords[7].c_str());
                printf("GOT WHO: [%s]  ", pszName);
            }

            if (vWords[1] == "JOIN" && vWords[0].size() > 1)
            {
                // :username!username@50000007.F000000B.90000002.IP JOIN :#channelname
                strcpy(pszName, vWords[0].c_str() + 1);
                if (strchr(pszName, '!'))
                    *strchr(pszName, '!') = '\0';
                printf("GOT JOIN: [%s]  ", pszName);
            }

            if (pszName[0] == 'u')
            {
                CAddress addr;
                if (DecodeAddress(pszName, addr))
                {
                    CAddrDB addrdb;
                    if (AddAddress(addrdb, addr))
                        printf("new  ");
                    else
                    {
                        // make it try connecting again
                        CRITICAL_BLOCK(cs_mapAddresses)
                            if (mapAddresses.count(addr.GetKey()))
                                mapAddresses[addr.GetKey()].nLastFailed = 0;
                    }
                    addr.print();

                    CRITICAL_BLOCK(cs_mapIRCAddresses)
                        mapIRCAddresses.insert(std::make_pair(addr.GetKey(), addr));
                }
                else
                {
                    printf("decode failed\n");
                }
            }
        }

        close(hSocket);

        if (GetTime() - nStart > 20 * 60)
        {
            nErrorWait /= 3;
            nRetryWait /= 3;
        }

        nRetryWait = nRetryWait * 11 / 10;
        if (!Wait(nRetryWait += 60))
            return;
    }
}