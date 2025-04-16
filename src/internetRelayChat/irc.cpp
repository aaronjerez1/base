#include "irc.h"
#include "../globals.h"

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