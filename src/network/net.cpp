#include "net.h"
#include "../internetRelayChat/irc.h"
//#include "../shared/utils.h"

#include <netdb.h>
//
// Global state variables
//

bool fClient = false;
uint64 nLocalServices = (fClient ? 0 : NODE_NETWORK);
CAddress addrLocalHost(0, DEFAULT_PORT, nLocalServices);
//CNode nodeLocalHost(-1, CAddress("127.0.0.1", nLocalServices));
//CNode* pnodeLocalHost = &nodeLocalHost;
bool fShutdown = false;
std::array<bool, 10> vfThreadRunning;
std::vector<CNode*> vNodes;
CCriticalSection cs_vNodes;
std::map<std::vector<unsigned char>, CAddress> mapAddresses;
CCriticalSection cs_mapAddresses;
//std::map<CInv, CDataStream> mapRelay;
std::deque<std::pair<int64, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
std::map<CInv, int64> mapAlreadyAskedFor;


CAddress addrProxy;

bool ConnectSocket(const CAddress& addrConnect, int hSocketRet)
{
	hSocketRet = -1;

	int hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hSocket == -1)
		return false;

	/// check if the ip is public
	bool fRoutable = !(addrConnect.GetByte(3) == 10 || (addrConnect.GetByte(3) == 192 && addrConnect.GetByte(2) == 168));
	bool fProxy = (addrProxy.ip && fRoutable);
	struct sockaddr_in sockaddr = (fProxy ? addrProxy.GetSockAddr() : addrConnect.GetSockAddr());
	
	if (connect(hSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1)
	{
		close(hSocket);
		return false;
	}

	if (fProxy)
	{
		printf("Proxy connecting to %s\n", addrConnect.ToString().c_str());
		char pszSocks4IP[] = "\4\1\0\0\0\0\0\0user";
		memcpy(pszSocks4IP + 2, &addrConnect.port, 2);
		memcpy(pszSocks4IP + 4, &addrConnect.ip, 4);
		char* pszSocks4 = pszSocks4IP;
		int nSize = sizeof(pszSocks4IP);

		int ret = send(hSocket, pszSocks4, nSize, 0);
		if (ret != nSize)
		{
			close(hSocket);
			return error("Error sending to proxy\n");
		}
		char pchRet[8];

		if (recv(hSocket, pchRet, 8, 0) != 8)
		{
			close(hSocket);
			return error("Error reading proxy response\n");
		}

		if (pchRet[1] != 0x5a)
		{
			close(hSocket);
			return error("Proxy returned error %d\n", pchRet[1]);
		}
		printf("Proxy connection established %s\n", addrConnect.ToString().c_str());
	}
	hSocketRet = hSocket;
	return true;
}

bool GetMyExternalIP2(const CAddress& addrConnect, const char* pszGet, const char* pszKeyword, unsigned int& ipRet)
{
	int hSocket;
	if (!ConnectSocket(addrConnect, hSocket))
		return error("GetMyExternalIP() : connection to %s failed\n", addrConnect.ToString().c_str());

	send(hSocket, pszGet, strlen(pszGet), 0);


	string strLine;
	while (RecvLine(hSocket, strLine))
	{
		if (strLine.empty())
		{
			for (;;)
			{
				if (!RecvLine(hSocket, strLine))
				{
					close(hSocket);
					return false;
				}
				if (strLine.find(pszKeyword) != -1)
				{
					strLine = strLine.substr(strLine.find(pszKeyword) + strlen(pszKeyword));
					break;
				}
			}
			close(hSocket);
			if (strLine.find("<"))
				strLine = strLine.substr(0, strLine.find("<"));
            strLine = strLine.substr(strspn(strLine.c_str(), " \t\n\r"));
			strLine = TrimString(strLine);
			CAddress addr(strLine.c_str());
            printf("GetMyExternalIP() received [%s] %s\n", strLine.c_str(), addr.ToString().c_str());
            if (addr.ip == 0 || !addr.IsRoutable())
                return false;
            ipRet = addr.ip;
            return true;
		}
		close(hSocket);
		return error("GetMyExternalIP() : connection closed\n");
	}

}

bool GetMyExternalIP(unsigned int& ipRet)
{
	CAddress addrConnect;
	const char* pszGet;
	const char* pszKeyword;

	for (int nLookup = 0; nLookup <= 1; nLookup++)
		for (int nHost = 1; nHost <= 2; nHost++)
		{
			if (nHost == 1)
			{
				addrConnect = CAddress("70.86.96.218:80"); // www.ipaddressworld.com

				if (nLookup == 1)
				{
					struct hostent* phostent = gethostbyname("www.ipaddressworld.com");
					if (phostent && phostent->h_addr_list && phostent->h_addr_list[0])
						addrConnect = CAddress(*(uint32_t*)phostent->h_addr_list[0], htons(80));
				}

				pszGet = "GET /ip.php HTTP/1.1\r\n"
					"Host: www.ipaddressworld.com\r\n"
					"User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
					"Connection: close\r\n"
					"\r\n";

				pszKeyword = "IP:";
			}
			else if (nHost == 2)
			{
				addrConnect = CAddress("208.78.68.70:80"); // checkip.dyndns.org

				if (nLookup == 1)
				{
					struct hostent* phostent = gethostbyname("checkip.dyndns.org");
					if (phostent && phostent->h_addr_list && phostent->h_addr_list[0])
						addrConnect = CAddress(*(uint32_t*)phostent->h_addr_list[0], htons(80));
				}

				pszGet = "GET / HTTP/1.1\r\n"
					//"Host: checkip.dyndns.org\r\n"
					//"User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
					"Connection: close\r\n"
					"\r\n";

				pszKeyword = "Address:";
			}

			if (GetMyExternalIP2(addrConnect, pszGet, pszKeyword, ipRet))
				return true;
		}

	return false;
}
#include "../database/addressdb/addressdb.h"
bool AddAddress(CAddrDB& addrdb, const CAddress& addr)
{
	if (!addr.IsRoutable())
	{
		return false;
	}
	if (addr.ip == addrLocalHost.ip)
	{ 
		return false;
	}
	CRITICAL_BLOCK(cs_mapAddresses)
	{
		std::map<std::vector<unsigned char>, CAddress>::iterator it = mapAddresses.find(addr.GetKey());
		if (it == mapAddresses.end())
		{
			// New Address
			mapAddresses.insert(std::make_pair(addr.GetKey(), addr));
			addrdb.WriteAddress(addr);
			return true;
		}
		else
		{
			CAddress& addrFound = (*it).second;
			if ((addrFound.nServices | addr.nServices) != addrFound.nServices)
			{
				// Services have been added
				addrFound.nServices |= addr.nServices;
				addrdb.WriteAddress(addrFound);
				return true;
			}
		}
	}
	return false;
}

void AbandonRequests(void (*fn)(void*, CDataStream&), void* param1)
{
	// if the dialog might get closed before the reply comes back,
	// call this in the destructor so it doesn't get called after it's deleted.
	CRITICAL_BLOCK(cs_vNodes)
	{
		for (CNode* pnode : vNodes)
		{
			CRITICAL_BLOCK(pnode->cs_mapRequests)
			{
				for (std::map<uint256, CRequestTracker>::iterator mi = pnode->mapRequests.begin(); mi != pnode->mapRequests.end(); )
				{
					CRequestTracker& tracker = (*mi).second;
					if (tracker.fn == fn && tracker.param1 == param1)
					{
						pnode->mapRequests.erase(mi++);
					}
					else
					{
						mi++;
					}
				}
			}
		}
	}
}

//
// Subscription methods for the broadcast and subscription system.
// channel numbers are message numbers, i.e. MSG_TABLE and MSG_PRODUC