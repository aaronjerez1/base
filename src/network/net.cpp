#include "net.h"
#include "../internetRelayChat/irc.h"
//#include "../shared/utils.h"
#include "../globals.h"
//#include "../database/walletdb/walletdb.h"
#include <netdb.h>
#include "../database/addressdb/addressdb.h"
//
// Global state variables
//

bool fClient = false;
uint64 nLocalServices = (fClient ? 0 : NODE_NETWORK);
CAddress addrLocalHost(0, DEFAULT_PORT, nLocalServices);
//CNode nodeLocalHost(-1, CAddress("127.0.0.1", nLocalServices));
//CNode* pnodeLocalHost = &nodeLocalHost;
bool fShutdown = false;
CAddress addrProxy;


std::array<bool, 10> vfThreadRunning;
std::vector<CNode*> vNodes;
CCriticalSection cs_vNodes;
CCriticalSection cs_mapAddresses;
std::map<std::vector<unsigned char>, CAddress> mapAddresses;
//std::map<CInv, CDataStream> mapRelay;
std::deque<std::pair<int64, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
std::map<CInv, int64> mapAlreadyAskedFor;


void ThreadMessageHandler2(void* parg);




bool ConnectSocket(const CAddress& addrConnect, int hSocketRet)
{
	hSocketRet = -1;

	int hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hSocket == -1)
		return false;

	/// check if the ip is public
	bool fRoutable = !(addrConnect.GetByte(3) == 10 || (addrConnect.GetByte(3) == 192 && addrConnect.GetByte(2) == 168));
	bool fProxy = (addrProxy.ip && fRoutable);
	struct sockaddr_in sa = (fProxy ? addrProxy.GetSockAddr() : addrConnect.GetSockAddr());
	
	//if (connect(hSocket, (sockaddr*)&sa, sizeof(sa)) == -1)
	//{
	//	close(hSocket);
	//	return false;
	//} //  this is trying to connect to an external server TODO

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
					"Host: checkip.dyndns.org\r\n"
					"User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
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
#include <fcntl.h>
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

// TODO: 
// Subscription methods for the broadcast and subscription system.
// channel numbers are message numbers, i.e. MSG_TABLE and MSG_PRODUC

/// <summary>
/// Thread handler
/// </summary>
struct ThreadArgs {
	void (*func)(void*);
	void* arg;
	int index;
};

//bool vfThreadRunning[4] = { false, false, false, false };

// Simple thread wrapper that sets/clears the running flag
void* ThreadWrapperFunction(void* args)
{
	// Unpack the arguments
	ThreadArgs* threadArgs = (ThreadArgs*)args;

	int index = threadArgs->index;
	void (*func)(void*) = threadArgs->func;
	void* arg = threadArgs->arg;

	// Free the arguments structure
	free(threadArgs);

	// Set the thread running flag
	vfThreadRunning[index] = true;

	// Call the actual thread function
	func(arg);

	// Clear the thread running flag
	vfThreadRunning[index] = false;

	return NULL;
}

// Modified thread creation function for void return type
bool StartThread(void (*func)(void*), void* arg, int index, string& strError)
{
	// Allocate and prepare arguments
	ThreadArgs* threadArgs = (ThreadArgs*)malloc(sizeof(ThreadArgs));
	if (!threadArgs) {
		strError = "Error: malloc failed";
		return false;
	}

	threadArgs->func = func;
	threadArgs->arg = arg;
	threadArgs->index = index;

	// Create the thread
	pthread_t thread;
	int result = pthread_create(&thread, NULL, ThreadWrapperFunction, threadArgs);

	if (result != 0) {
		free(threadArgs);
		strError = strerror(result);
		return false;
	}

	// Detach thread so it cleans up automatically
	pthread_detach(thread);
	return true;
}

#include "../database/walletdb/walletdb.h"
bool StartNode(string& strError)
{
	strError = "";

	// Get local host ip
	char pszHostName[255];
	if (gethostname(pszHostName, 255) == -1)
	{
		strError = strprintf("Error: Unable to get IP address of this computer (gethostname returned error %d)", errno);
		printf("%s\n", strError.c_str());
		return false;
	}

	struct hostent* phostent = gethostbyname(pszHostName);
	if (!phostent)
	{
		strError = strprintf("Error: Unable to get IP address of this computer (gethostbyname returned error %d)", errno);
		printf("%s\n", strError.c_str());
		return false;
	}
	
	addrLocalHost = CAddress(*(long*)(phostent->h_addr_list[0]),
		DEFAULT_PORT,
		nLocalServices);
	printf("addrLocalHost = %s\n", addrLocalHost.ToString().c_str());

	int hListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hListenSocket == -1)
	{
		strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %d)", errno);
		printf("%s\n", strError.c_str());
		return false;
	}

	// Set to nonblocking, incoming connections will also inherit this
	int flags = fcntl(hListenSocket, F_GETFL, 0);
	if (flags == -1)
	{
		strError = strprintf("Error: Couldn't get socket flags (fcntl returned error %d)", errno);
		printf("%s\n", strError.c_str());
		return false;
	}

	if (fcntl(hListenSocket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		strError = strprintf("Error: Couldn't set socket to non-blocking mode (fcntl returned error %d)", errno);
		printf("%s\n", strError.c_str());
		return false;
	}

	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound
	int nRetryLimit = 15;
	struct sockaddr_in sockaddr = addrLocalHost.GetSockAddr();
	if (bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1)
	{
		int nErr = errno;
		if (nErr == EADDRINUSE)
			strError = strprintf("Error: Unable to bind to port %s on this computer. The program is probably already running.", addrLocalHost.ToString().c_str());
		else
			strError = strprintf("Error: Unable to bind to port %s on this computer (bind returned error %d)", addrLocalHost.ToString().c_str(), nErr);
		printf("%s\n", strError.c_str());
		return false;
	}
	printf("bound to addrLocalHost = %s\n\n", addrLocalHost.ToString().c_str());


	// Listen for incoming connections
	if (listen(hListenSocket, SOMAXCONN) == -1)
	{
		strError = strprintf("Error: Listening for incoming connections failed (listen returned error %d)", errno);
		printf("%s\n", strError.c_str());
		return false;
	}

	// Get our external IP address for incoming connections
	if (addrIncoming.ip)
		addrLocalHost.ip = addrIncoming.ip;\

	if (GetMyExternalIP(addrLocalHost.ip))
	{
		addrIncoming = addrLocalHost;
		CWalletDB().WriteSetting("addrIncoming", addrIncoming);

	}

	std::string threadError;
	// Get addresses from IRC and advertise ours
	if (!StartThread(ThreadIRCSeed, NULL, 1, threadError))
	{
		printf("Error: StartThread(ThreadIRCSeed) failed: %s\n", threadError.c_str());
		return false;
	}

	// Start Threads

	//if (!StartThread(ThreadSocketHandler, new int(hListenSocket), 0, threadError)) {
	//	strError = "Error: StartThread(ThreadSocketHandler) failed: " + threadError;
	//	printf("%s\n", strError.c_str());
	//	return false;
	//}


	return true;

}

// HERE. actually on the task. the unit of the blockcahin is messages

void ThreadMessageHandler(void* parg)
{
	IMPLEMENT_RANDOMIZE_STACK(ThreadMessageHandler(parg));

	loop
	{
		vfThreadRunning[2] = true;
		//CheckForShutdown(2);
		//try
		//{
		//	ThreadMessageHandler2(parg);
		//}
		//CATCH_PRINT_EXCEPTION("ThreadMessageHandler()")
		//vfThreadRunning[2] = false;
		//Sleep(5000);
	}
}

void ThreadMessageHandler2(void* parg)
{
	printf("ThreadMessageHandler started\n");
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	loop
	{
		// Poll the connected nodes for messages
		vector<CNode*> vNodesCopy;
		CRITICAL_BLOCK(cs_vNodes)
			vNodesCopy = vNodes;
		for(CNode * pnode: vNodesCopy)
		{
			pnode->AddRef();

			// Receive messages
			TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
				//ProcessMessages(pnode);

			// Send messages
			TRY_CRITICAL_BLOCK(pnode->cs_vSend)
				//SendMessages(pnode);

			pnode->Release();
		}

		// Wait and allow messages to bunch up
		vfThreadRunning[2] = false;
		//Sleep(100);
		vfThreadRunning[2] = true;
		//CheckForShutdown(2);
	}
}
