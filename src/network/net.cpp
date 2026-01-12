#include "net.h"
#include "../internetRelayChat/irc.h"
//#include "../shared/utils.h"
#include "../globals.h"
//#include "../database/walletdb/walletdb.h"
#include <netdb.h>
#include "../database/addressdb/addressdb.h"

//#define WIN32_LEAN_AND_MEAN
//#include <winsock2.h>
//#include <windows.h>
//
//#pragma comment(lib, "Ws2_32.lib")


void ThreadMessageHandler2(void* parg);
void ThreadSocketHandler2(void* parg);
void ThreadOpenConnections2(void* parg);


//
// Global state variables
//

bool fClient = false;
uint64 nLocalServices = (fClient ? 0 : NODE_NETWORK);
CAddress addrLocalHost(0, DEFAULT_PORT, nLocalServices);
CNode nodeLocalHost(-1, CAddress("127.0.0.1", nLocalServices));
CNode* pnodeLocalHost = &nodeLocalHost;
bool fShutdown = false;
CAddress addrProxy;


std::array<bool, 10> vfThreadRunning;

//std::vector<


std::vector<CNode*> vNodes; //HERE
CCriticalSection cs_vNodes; // HERE

std::vector<CAddress> vConnect; // NEW DNS address resolution.
CCriticalSection cs_vConnect;

CCriticalSection cs_mapAddresses;
std::map<std::vector<unsigned char>, CAddress> mapAddresses;


std::map<CInv, CDataStream> mapRelay;
std::deque<std::pair<int64, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
std::map<CInv, int64> mapAlreadyAskedFor;



bool Wait(int seconds)
{
	const int checkIntervalMs = 200; // check shutdown 5x per second
	int remainingMs = seconds * 1000;

	while (remainingMs > 0)
	{
		if (fShutdown)
			return false;

		int sleepMs = std::min(checkIntervalMs, remainingMs);
		std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
		remainingMs -= sleepMs;
	}

	return !fShutdown;
}




#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static bool LookupDNSSeed(const char* seed, std::vector<uint32_t>& outIPs)
{
	outIPs.clear();

	addrinfo hints{};
	hints.ai_family = AF_INET;        // IPv4 only for now
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* res = nullptr;
	int rc = getaddrinfo(seed, nullptr, &hints, &res);
	if (rc != 0 || !res)
		return false;

	for (addrinfo* p = res; p; p = p->ai_next)
	{
		if (!p->ai_addr || p->ai_addrlen < (int)sizeof(sockaddr_in))
			continue;

		sockaddr_in* sa = (sockaddr_in*)p->ai_addr;
		uint32_t ip = sa->sin_addr.s_addr;   // network order
		outIPs.push_back(ip);
	}

	freeaddrinfo(res);

	// de-dupe
	std::sort(outIPs.begin(), outIPs.end());
	outIPs.erase(std::unique(outIPs.begin(), outIPs.end()), outIPs.end());

	return !outIPs.empty();
}


bool ConnectSocket(const CAddress& addrConnect, int& hSocketRet)
{
	hSocketRet = -1;

	int hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hSocket == -1)
		return false;

	/// check if the ip is public
	bool fRoutable = !(addrConnect.GetByte(3) == 10 || (addrConnect.GetByte(3) == 192 && addrConnect.GetByte(2) == 168));
	bool fProxy = (addrProxy.ip && fRoutable);
	struct sockaddr_in sa = (fProxy ? addrProxy.GetSockAddr() : addrConnect.GetSockAddr());
	
	if (connect(hSocket, (sockaddr*)&sa, sizeof(sa)) == -1)
	{
		close(hSocket);
		return false;
	} //  this is trying to connect to an external server TODO

	printf("addrConnect.ip=%u port=%u (host order?)\n", addrConnect.ip, addrConnect.port);

	sockaddr_in sa2 = addrConnect.GetSockAddr();
	printf("sockaddr port (network order)=%u\n", (unsigned)sa2.sin_port);

	printf("ConnectSocket: connecting to %s\n", addrConnect.ToString().c_str());
	fflush(stdout);

	errno = 0;
	int rc = connect(hSocket, (sockaddr*)&sa, sizeof(sa));
	printf("ConnectSocket: connect rc=%d errno=%d (%s)\n", rc, errno, strerror(errno));
	fflush(stdout);

	if (rc == -1) {
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

static const char* STATIC_SEEDS[] = {
	"127.0.0.1:8333",      // local testing
	// "127.0.0.1:8334",   // second local node
	// "YOUR.VPS.IP:8333", // future seed
};

static const int NUM_STATIC_SEEDS =
sizeof(STATIC_SEEDS) / sizeof(STATIC_SEEDS[0]);


//extern std::vector<CAddress> vConnect;
//extern CCriticalSection cs_vConnect;
//
//static void AddStaticSeeds()
//{
//	CRITICAL_BLOCK(cs_vConnect);
//	for (int i = 0; i < NUM_STATIC_SEEDS; ++i)
//	{
//		CAddress addr(STATIC_SEEDS[i]);
//		printf("Adding static seed %s\n", addr.ToString().c_str());
//		vConnect.push_back(addr);
//	}
//}

// DNS

static const char* DNS_SEEDS[] = {
	"seed1.byte-chain.com",
	"seed0.byte-chain.com"
};
static const int NUM_DNS_SEEDS = sizeof(DNS_SEEDS) / sizeof(DNS_SEEDS[0]);



void AddDNSSeeds()
{
	for (const char* seed : DNS_SEEDS)
	{
		addrinfo hints{};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		addrinfo* res = nullptr;
		if (getaddrinfo(seed, nullptr, &hints, &res) != 0)
			continue;

		for (addrinfo* p = res; p; p = p->ai_next)
		{
			sockaddr_in* sa = (sockaddr_in*)p->ai_addr;
			CAddress addr(sa->sin_addr.s_addr, DEFAULT_PORT);

			if (!addr.IsRoutable())
				continue;

			CRITICAL_BLOCK(cs_vConnect);
			vConnect.push_back(addr);
		}

		freeaddrinfo(res);
	}
}


void ThreadDNSSeed(void* parg)
{
	// keep your thread scheduling/priority behavior
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
		bool fAnySuccess = false;
		int64 nStart = GetTime();

		for (int i = 0; i < NUM_DNS_SEEDS && !fShutdown; ++i)
		{
			const char* seed = DNS_SEEDS[i];
			printf("DNS seed: resolving %s\n", seed);

			std::vector<uint32_t> ips;
			if (!LookupDNSSeed(seed, ips))
			{
				printf("DNS seed: resolve failed for %s\n", seed);
				continue;
			}

			printf("DNS seed: %s returned %u IPs\n", seed, (unsigned)ips.size());

			// Convert to CAddress and add to addr DB (mapAddresses) like IRC thread did
			for (uint32_t ip : ips)
			{
				// Your CAddress stores port in network order.
				// DEFAULT_PORT in your codebase should match that convention.
				CAddress addr(ip, DEFAULT_PORT);

				if (!addr.IsRoutable())
					continue;

				// Add to addrman/mapAddresses through the same path you used
				CAddrDB addrdb;
				if (AddAddress(addrdb, addr))
				{
					printf("DNS seed: new ");
				}
				else
				{
					// same trick as IRC: make it try connecting again
					CRITICAL_BLOCK(cs_mapAddresses)
						if (mapAddresses.count(addr.GetKey()))
							mapAddresses[addr.GetKey()].nLastFailed = 0;
				}

				addr.print();

				// Optional: also queue some for immediate outbound dialing
				// (keeps behavior similar to "make it connect now")
				// Keep the queue bounded to avoid runaway growth.
				//{
				//    //CRITICAL_BLOCK(cs_vConnect);
				//    if (vConnect.size() < 32)
				//        vConnect.push_back(addr);
				//}
				AddDNSSeeds();
			}

			fAnySuccess = true;
		}

		// Backoff logic similar to IRC thread
		if (!fAnySuccess)
		{
			nErrorWait = nErrorWait * 11 / 10;
			printf("DNS seed: all failed, waiting %d sec\n", nErrorWait + 60);

			if (Wait(nErrorWait += 60))
				continue;
			else
				return;
		}

		// If the loop ran a long time, relax backoffs (same spirit as IRC code)
		if (GetTime() - nStart > 20 * 60)
		{
			nErrorWait /= 3;
			nRetryWait /= 3;
		}

		// Periodic refresh, but don’t hammer DNS
		nRetryWait = nRetryWait * 11 / 10;
		int waitSec = (nRetryWait += 60);

		// In practice: cap refresh so it doesn't become hours
		if (waitSec > 30 * 60) waitSec = 30 * 60;

		printf("DNS seed: refresh wait %d sec\n", waitSec);
		if (!Wait(waitSec))
			return;
	}
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

	//if (GetMyExternalIP(addrLocalHost.ip))
	//{
	//	addrIncoming = addrLocalHost;
	//	CWalletDB().WriteSetting("addrIncoming", addrIncoming);

	//}

	std::string threadError;
	// Get addresses from IRC and advertise ours
	//AddStaticSeeds();
	//if (!StartThread(ThreadIRCSeed, NULL, 1, threadError))
	//{
	//	printf("Error: StartThread(ThreadIRCSeed) failed: %s\n", threadError.c_str());
	//	return false;
	//} // TO DESTROY

	if (!StartThread(ThreadDNSSeed, NULL, 1, threadError))
	{
		printf("Error: StartThread(ThreadIRCSeed) failed: %s\n", threadError.c_str());
		return false;
	} // TODO


	// Start Threads

	if (!StartThread(ThreadSocketHandler, new int(hListenSocket), 0, threadError)) {
		strError = "Error: StartThread(ThreadSocketHandler) failed: " + threadError;
		printf("%s\n", strError.c_str());
		return false;
	}

	if (!StartThread(ThreadOpenConnections, new int(hListenSocket), 0, threadError)) {
		strError = "Error: StartThread(ThreadOpenConnections2) failed: " + threadError;
		printf("%s\n", strError.c_str());
		return false;
	}

	//if (!StartThread(ThreadMessageHandler, new int(hListenSocket), 0, threadError)) {
	//	strError = "Error: StartThread(ThreadMessageHandler) failed: " + threadError;
	//	printf("%s\n", strError.c_str());
	//	return false;
	//}



	return true;

}








CNode* FindNode(unsigned int ip)
{
	CRITICAL_BLOCK(cs_vNodes)
	{
		for(CNode * pnode: vNodes)
			if (pnode->addr.ip == ip)
				return (pnode);
	}
	return NULL;
}

CNode* FindNode(CAddress addr)
{
	CRITICAL_BLOCK(cs_vNodes)
	{
		for(CNode * pnode: vNodes)
			if (pnode->addr == addr)
				return (pnode);
	}
	return NULL;
}

CNode* ConnectNode(CAddress addrConnect, int64 nTimeout) // HERE
{
	if (addrConnect.ip == addrLocalHost.ip)
		return NULL;

	// Look for an existing connection
	CNode* pnode = FindNode(addrConnect.ip);
	if (pnode)
	{
		if (nTimeout != 0)
			pnode->AddRef(nTimeout);
		else
			pnode->AddRef();
		return pnode;
	}

	/// debug print
	printf("trying %s\n", addrConnect.ToString().c_str());

	// Connect
	int hSocket = -1;
	if (ConnectSocket(addrConnect, hSocket))
	{
		/// debug print
		printf("connected %s\n", addrConnect.ToString().c_str());

		// Set to non-blocking
		int flags = fcntl(hSocket, F_GETFL, 0);
		if (flags == -1 || fcntl(hSocket, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			printf("ConnectSocket() : fcntl nonblocking setting failed, error %d\n", errno);
		}

		// Add node
		CNode* pnode = new CNode(hSocket, addrConnect, false);
		if (nTimeout != 0)
			pnode->AddRef(nTimeout);
		else
			pnode->AddRef();
		CRITICAL_BLOCK(cs_vNodes)
			vNodes.push_back(pnode);

		CRITICAL_BLOCK(cs_mapAddresses)
			mapAddresses[addrConnect.GetKey()].nLastFailed = 0;
		return pnode;
	}
	else
	{
		CRITICAL_BLOCK(cs_mapAddresses)
			mapAddresses[addrConnect.GetKey()].nLastFailed = GetTime();
		return NULL;
	}
}


// ThreadOpenConnections

void ThreadOpenConnections(void* parg)
{
	IMPLEMENT_RANDOMIZE_STACK(ThreadOpenConnections(parg));

	loop
	{
		vfThreadRunning[1] = true;
		CheckForShutdown(1);
		try
		{
			ThreadOpenConnections2(parg);
		}
		CATCH_PRINT_EXCEPTION("ThreadOpenConnections()")
		vfThreadRunning[1] = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}
}



void ThreadOpenConnections2(void* parg)
{
	printf("ThreadOpenConnections started\n");

	// Initiate network connections
	int nTry = 0;
	bool fIRCOnly = false;
	const int nMaxConnections = 15;
	loop
	{
		// Wait
		vfThreadRunning[1] = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		while (vNodes.size() >= nMaxConnections || vNodes.size() >= mapAddresses.size())
		{
			CheckForShutdown(1);
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		}
		vfThreadRunning[1] = true;
		CheckForShutdown(1);


		//
		// The IP selection process is designed to limit vulnerability to address flooding.
		// Any class C (a.b.c.?) has an equal chance of being chosen, then an IP is
		// chosen within the class C.  An attacker may be able to allocate many IPs, but
		// they would normally be concentrated in blocks of class C's.  They can hog the
		// attention within their class C, but not the whole IP address space overall.
		// A lone node in a class C will get as much attention as someone holding all 255
		// IPs in another class C.
		//

		// Every other try is with IRC addresses only
		fIRCOnly = !fIRCOnly;
		if (mapIRCAddresses.empty())
			fIRCOnly = false;
		else if (nTry++ < 30 && vNodes.size() < nMaxConnections / 2)
			fIRCOnly = true;

		// Make a list of unique class C's
		unsigned char pchIPCMask[4] = { 0xff, 0xff, 0xff, 0x00 };
		unsigned int nIPCMask = *(unsigned int*)pchIPCMask;
		vector<unsigned int> vIPC;
		CRITICAL_BLOCK(cs_mapIRCAddresses)
		CRITICAL_BLOCK(cs_mapAddresses)
		{
			vIPC.reserve(mapAddresses.size());
			unsigned int nPrev = 0;
			for(const PAIRTYPE(vector<unsigned char>, CAddress) & item : mapAddresses)
			{
				const CAddress& addr = item.second;
				if (!addr.IsIPv4())
					continue;
				if (fIRCOnly && !mapIRCAddresses.count(item.first))
					continue;

				// Taking advantage of mapAddresses being in sorted order,
				// with IPs of the same class C grouped together.
				unsigned int ipC = addr.ip & nIPCMask;
				if (ipC != nPrev)
					vIPC.push_back(nPrev = ipC);
			}
		}
		if (vIPC.empty())
			continue;

		// Choose a random class C
		unsigned int ipC = vIPC[GetRand(vIPC.size())];

		// Organize all addresses in the class C by IP
		std::map<unsigned int, vector<CAddress> > mapIP; // OHH it gets it form the IRC seed.
		CRITICAL_BLOCK(cs_mapIRCAddresses)
		CRITICAL_BLOCK(cs_mapAddresses)
		{
			int64 nDelay = ((30 * 60) << vNodes.size());
			if (!fIRCOnly)
			{
				nDelay *= 2;
				if (vNodes.size() >= 3)
					nDelay *= 4;
				if (!mapIRCAddresses.empty())
					nDelay *= 100;
			}

			for (std::map<vector<unsigned char>, CAddress>::iterator mi = mapAddresses.lower_bound(CAddress(ipC, 0).GetKey());
				 mi != mapAddresses.upper_bound(CAddress(ipC | ~nIPCMask, 0xffff).GetKey());
				 ++mi)
			{
				const CAddress& addr = (*mi).second;
				if (fIRCOnly && !mapIRCAddresses.count((*mi).first))
					continue;

				int64 nRandomizer = (addr.nLastFailed * addr.ip * 7777U) % 20000;
				if (GetTime() - addr.nLastFailed > nDelay * nRandomizer / 10000)
					mapIP[addr.ip].push_back(addr);
			}
		}
		if (mapIP.empty())
			continue;

		// Choose a random IP in the class C
		std::map<unsigned int, vector<CAddress> >::iterator mi = mapIP.begin();
		advance(mi, GetRand(mapIP.size()));

		// Once we've chosen an IP, we'll try every given port before moving on
		for(const CAddress & addrConnect: (*mi).second)
		{
			//
			// Initiate outbound network connection
			//
			CheckForShutdown(1);
			if (addrConnect.ip == addrLocalHost.ip || !addrConnect.IsIPv4() || FindNode(addrConnect.ip))
				continue;

			vfThreadRunning[1] = false;
			CNode* pnode = ConnectNode(addrConnect);
			vfThreadRunning[1] = true;
			CheckForShutdown(1);
			if (!pnode)
				continue;
			pnode->fNetworkNode = true;

			if (addrLocalHost.IsRoutable())
			{
				// Advertise our address
				vector<CAddress> vAddrToSend;
				vAddrToSend.push_back(addrLocalHost);
				pnode->PushMessage("addr", vAddrToSend);
			}

			// Get as many addresses as we can
			pnode->PushMessage("getaddr");

			////// should the one on the receiving end do this too?
			// Subscribe our local subscription list
			const unsigned int nHops = 0;
			for (unsigned int nChannel = 0; nChannel < pnodeLocalHost->vfSubscribe.size(); nChannel++)
				if (pnodeLocalHost->vfSubscribe[nChannel])
					pnode->PushMessage("subscribe", nChannel, nHops);

			break;
		}
	}
}

// HERE. actually on the task. the unit of the blockcahin is messages and threads?

void ThreadMessageHandler(void* parg)
{
	IMPLEMENT_RANDOMIZE_STACK(ThreadMessageHandler(parg));

	loop
	{
		vfThreadRunning[2] = true;
		CheckForShutdown(2);
		try
		{
			ThreadMessageHandler2(parg);
		}
		CATCH_PRINT_EXCEPTION("ThreadMessageHandler()")
		vfThreadRunning[2] = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
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
				ProcessMessages(pnode);  //HERE

			// Send messages
			TRY_CRITICAL_BLOCK(pnode->cs_vSend)
				//SendMessages(pnode);

			pnode->Release();
		}

		// Wait and allow messages to bunch up
		vfThreadRunning[2] = false;
		//Sleep(100);
		vfThreadRunning[2] = true;
		CheckForShutdown(2);
	}
}


void CheckForShutdown(int n)
{
	if (fShutdown)
	{
		if (n != -1)
			vfThreadRunning[n] = false;
		if (n == 0)
			for (CNode* pnode : vNodes)
			{
				shutdown(pnode->hSocket, SHUT_RDWR); // unblock recv/send
				close(pnode->hSocket);
			}

		pthread_exit(nullptr);
	}
}








void ThreadSocketHandler(void* parg)
{
	IMPLEMENT_RANDOMIZE_STACK(ThreadSocketHandler(parg));

	loop
	{
		vfThreadRunning[0] = true;
		CheckForShutdown(0);
		try
		{
			ThreadSocketHandler2(parg);
		}
		CATCH_PRINT_EXCEPTION("ThreadSocketHandler()")
		vfThreadRunning[0] = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}
}
// WINDOWS version:
//void ThreadSocketHandler2(void* parg)
//{
//	printf("ThreadSocketHandler started\n");
//	SOCKET hListenSocket = *(SOCKET*)parg;
//	list<CNode*> vNodesDisconnected;
//	int nPrevNodeCount = 0;
//
//	loop
//	{
//		//
//		// Disconnect nodes
//		//
//		CRITICAL_BLOCK(cs_vNodes)
//		{
//		// Disconnect unused nodes
//		vector<CNode*> vNodesCopy = vNodes;
//		for(CNode * pnode: vNodesCopy)
//		{
//			if (pnode->ReadyToDisconnect() && pnode->vRecv.empty() && pnode->vSend.empty())
//			{
//				// remove from vNodes
//				vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());
//				pnode->Disconnect();
//
//				// hold in disconnected pool until all refs are released
//				pnode->nReleaseTime = std::max(pnode->nReleaseTime, GetTime() + 5 * 60);
//				if (pnode->fNetworkNode)
//					pnode->Release();
//				vNodesDisconnected.push_back(pnode);
//			}
//		}
//
//		// Delete disconnected nodes
//		std::list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
//		for(CNode * pnode: vNodesDisconnectedCopy)
//		{
//			// wait until threads are done using it
//			if (pnode->GetRefCount() <= 0)
//			{
//				bool fDelete = false;
//				TRY_CRITICAL_BLOCK(pnode->cs_vSend)
//				 TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
//				  TRY_CRITICAL_BLOCK(pnode->cs_mapRequests)
//				   TRY_CRITICAL_BLOCK(pnode->cs_inventory)
//					fDelete = true;
//				if (fDelete)
//				{
//					vNodesDisconnected.remove(pnode);
//					delete pnode;
//				}
//			}
//		}
//	}
//	//if (vNodes.size() != nPrevNodeCount)
//	//{
//	//	nPrevNodeCount = vNodes.size();
//	//	MainFrameRepaint();
//	//}
//
//
//	//
//	// Find which sockets have data to receive
//	//
//	struct timeval timeout;
//	timeout.tv_sec = 0;
//	timeout.tv_usec = 50000; // frequency to poll pnode->vSend
//
//	struct fd_set fdsetRecv;
//	struct fd_set fdsetSend;
//	FD_ZERO(&fdsetRecv);
//	FD_ZERO(&fdsetSend);
//	SOCKET hSocketMax = 0;
//	FD_SET(hListenSocket, &fdsetRecv);
//	hSocketMax = std::max(hSocketMax, hListenSocket);
//	CRITICAL_BLOCK(cs_vNodes)
//	{
//		for(CNode * pnode: vNodes)
//		{
//			FD_SET(pnode->hSocket, &fdsetRecv);
//			hSocketMax = std::max(hSocketMax, pnode->hSocket);
//			TRY_CRITICAL_BLOCK(pnode->cs_vSend)
//				if (!pnode->vSend.empty())
//					FD_SET(pnode->hSocket, &fdsetSend);
//		}
//	}
//
//	vfThreadRunning[0] = false;
//	int nSelect = select(hSocketMax + 1, &fdsetRecv, &fdsetSend, NULL, &timeout);
//	vfThreadRunning[0] = true;
//	CheckForShutdown(0);
//	if (nSelect == SOCKET_ERROR)
//	{
//		int nErr = WSAGetLastError();
//		printf("select failed: %d\n", nErr);
//		for (int i = 0; i <= hSocketMax; i++)
//		{
//			FD_SET(i, &fdsetRecv);
//			FD_SET(i, &fdsetSend);
//		}
//		std::this_thread::sleep_for(std::chrono::milliseconds(timeout.tv_usec / 1000));
//
//	}
//	RandAddSeed();
//
//	//// debug print
//	//foreach(CNode* pnode, vNodes)
//	//{
//	//    printf("vRecv = %-5d ", pnode->vRecv.size());
//	//    printf("vSend = %-5d    ", pnode->vSend.size());
//	//}
//	//printf("\n");
//
//
//	//
//	// Accept new connections
//	//
//	if (FD_ISSET(hListenSocket, &fdsetRecv))
//	{
//		struct sockaddr_in sockaddr;
//		int len = sizeof(sockaddr);
//		SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
//		CAddress addr(sockaddr);
//		if (hSocket == INVALID_SOCKET)
//		{
//			if (WSAGetLastError() != WSAEWOULDBLOCK)
//				printf("ERROR ThreadSocketHandler accept failed: %d\n", WSAGetLastError());
//		}
//		else
//		{
//			printf("accepted connection from %s\n", addr.ToString().c_str());
//			CNode* pnode = new CNode(hSocket, addr, true);
//			pnode->AddRef();
//			CRITICAL_BLOCK(cs_vNodes)
//				vNodes.push_back(pnode);
//		}
//	}
//
//
//	//
//	// Service each socket
//	//
//	vector<CNode*> vNodesCopy;
//	CRITICAL_BLOCK(cs_vNodes)
//		vNodesCopy = vNodes;
//	foreach(CNode * pnode, vNodesCopy)
//	{
//		CheckForShutdown(0);
//		SOCKET hSocket = pnode->hSocket;
//
//		//
//		// Receive
//		//
//		if (FD_ISSET(hSocket, &fdsetRecv))
//		{
//			TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
//			{
//				CDataStream& vRecv = pnode->vRecv;
//				unsigned int nPos = vRecv.size();
//
//				// typical socket buffer is 8K-64K
//				const unsigned int nBufSize = 0x10000;
//				vRecv.resize(nPos + nBufSize);
//				int nBytes = recv(hSocket, &vRecv[nPos], nBufSize, 0);
//				vRecv.resize(nPos + max(nBytes, 0));
//				if (nBytes == 0)
//				{
//					// socket closed gracefully
//					if (!pnode->fDisconnect)
//						printf("recv: socket closed\n");
//					pnode->fDisconnect = true;
//				}
//				else if (nBytes < 0)
//				{
//					// socket error
//					int nErr = WSAGetLastError();
//					if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
//					{
//						if (!pnode->fDisconnect)
//							printf("recv failed: %d\n", nErr);
//						pnode->fDisconnect = true;
//					}
//				}
//			}
//		}
//
//		//
//		// Send
//		//
//		if (FD_ISSET(hSocket, &fdsetSend))
//		{
//			TRY_CRITICAL_BLOCK(pnode->cs_vSend)
//			{
//				CDataStream& vSend = pnode->vSend;
//				if (!vSend.empty())
//				{
//					int nBytes = send(hSocket, &vSend[0], vSend.size(), 0);
//					if (nBytes > 0)
//					{
//						vSend.erase(vSend.begin(), vSend.begin() + nBytes);
//					}
//					else if (nBytes == 0)
//					{
//						if (pnode->ReadyToDisconnect())
//							pnode->vSend.clear();
//					}
//					else
//					{
//						printf("send error %d\n", nBytes);
//						if (pnode->ReadyToDisconnect())
//							pnode->vSend.clear();
//					}
//				}
//			}
//		}
//	}
//
//
//	Sleep(10);
//	}
//}

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <chrono>
#include <thread>

// Helpers (if you don't already have equivalents)
static inline bool IsWouldBlock(int e)
{
	return e == EWOULDBLOCK || e == EAGAIN;
}

void ThreadSocketHandler2(void* parg)
{
	printf("ThreadSocketHandler started\n");

	int hListenSocket = *(int*)parg;   // POSIX fd
	std::list<CNode*> vNodesDisconnected;
	int nPrevNodeCount = 0;

	loop
	{
		//
		// Disconnect nodes
		//
		CRITICAL_BLOCK(cs_vNodes)
		{
		// Disconnect unused nodes
		std::vector<CNode*> vNodesCopy = vNodes;
		for (CNode* pnode : vNodesCopy)
		{
			if (pnode->ReadyToDisconnect() && pnode->vRecv.empty() && pnode->vSend.empty())
			{
				// remove from vNodes
				vNodes.erase(std::remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());
				pnode->Disconnect(); // should close(pnode->hSocket) inside for Linux

				// hold in disconnected pool until all refs are released
				pnode->nReleaseTime = std::max(pnode->nReleaseTime, GetTime() + 5 * 60);
				if (pnode->fNetworkNode)
					pnode->Release();
				vNodesDisconnected.push_back(pnode);
			}
		}

		// Delete disconnected nodes
		std::list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
		for (CNode* pnode : vNodesDisconnectedCopy)
		{
			// wait until threads are done using it
			if (pnode->GetRefCount() <= 0)
			{
				bool fDelete = false;
				TRY_CRITICAL_BLOCK(pnode->cs_vSend)
					TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
						TRY_CRITICAL_BLOCK(pnode->cs_mapRequests)
							TRY_CRITICAL_BLOCK(pnode->cs_inventory)
								fDelete = true;

				if (fDelete)
				{
					vNodesDisconnected.remove(pnode);
					delete pnode;
				}
			}
		}
	}

	//
	// Find which sockets have data to receive / send
	//
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 50000; // poll frequency (50ms)

	fd_set fdsetRecv;
	fd_set fdsetSend;
	FD_ZERO(&fdsetRecv);
	FD_ZERO(&fdsetSend);

	int hSocketMax = 0;

	FD_SET(hListenSocket, &fdsetRecv);
	hSocketMax = std::max(hSocketMax, hListenSocket);

	CRITICAL_BLOCK(cs_vNodes)
	{
		for (CNode* pnode : vNodes)
		{
			int s = pnode->hSocket; // POSIX fd
			if (s < 0) continue;

			FD_SET(s, &fdsetRecv);
			hSocketMax = std::max(hSocketMax, s);

			TRY_CRITICAL_BLOCK(pnode->cs_vSend)
				if (!pnode->vSend.empty())
					FD_SET(s, &fdsetSend);
		}
	}

	vfThreadRunning[0] = false;
	int nSelect = select(hSocketMax + 1, &fdsetRecv, &fdsetSend, nullptr, &timeout);
	vfThreadRunning[0] = true;

	CheckForShutdown(0);

	if (nSelect < 0)
	{
		int nErr = errno;
		printf("select failed: %d (%s)\n", nErr, strerror(nErr));

		// If select was interrupted, just continue quickly
		if (nErr == EINTR)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		// For other errors, sleep briefly to avoid a tight loop
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		continue;
	}

	RandAddSeed(true);

	//
	// Accept new connections
	//
	if (FD_ISSET(hListenSocket, &fdsetRecv))
	{
		sockaddr_in sa;
		socklen_t len = sizeof(sa);

		int hSocket = accept(hListenSocket, (sockaddr*)&sa, &len);
		if (hSocket < 0)
		{
			int e = errno;
			if (!IsWouldBlock(e) && e != EINTR)
				printf("ERROR ThreadSocketHandler accept failed: %d (%s)\n", e, strerror(e));
		}
		else
		{
			CAddress addr(sa);
			printf("accepted connection from %s\n", addr.ToString().c_str());

			// accepted sockets inherit nonblocking if listen socket was nonblocking on Linux?:
			// Actually: accepted socket does NOT always inherit O_NONBLOCK on all POSIX.
			// Safe: explicitly set nonblocking here if your code assumes nonblocking.
			int flags = fcntl(hSocket, F_GETFL, 0);
			if (flags != -1)
				fcntl(hSocket, F_SETFL, flags | O_NONBLOCK);

			CNode* pnode = new CNode(hSocket, addr, true);
			pnode->AddRef();

			CRITICAL_BLOCK(cs_vNodes)
				vNodes.push_back(pnode);
		}
	}

	//
	// Service each socket
	//
	std::vector<CNode*> vNodesCopy;
	CRITICAL_BLOCK(cs_vNodes)
		vNodesCopy = vNodes;

	for (CNode* pnode : vNodesCopy)
	{
		CheckForShutdown(0);
		int hSocket = pnode->hSocket;

		//
		// Receive
		//
		if (hSocket >= 0 && FD_ISSET(hSocket, &fdsetRecv))
		{
			TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
			{
				CDataStream& vRecv = pnode->vRecv;
				unsigned int nPos = vRecv.size();

				const unsigned int nBufSize = 0x10000; // 64KB
				vRecv.resize(nPos + nBufSize);

				int nBytes = (int)recv(hSocket, (char*)&vRecv[nPos], nBufSize, 0);

				vRecv.resize(nPos + std::max(nBytes, 0));

				if (nBytes == 0)
				{
					if (!pnode->fDisconnect)
						printf("recv: socket closed\n");
					pnode->fDisconnect = true;
				}
				else if (nBytes < 0)
				{
					int e = errno;
					// ignore transient nonblocking conditions
					if (!IsWouldBlock(e) && e != EMSGSIZE && e != EINTR && e != EINPROGRESS)
					{
						if (!pnode->fDisconnect)
							printf("recv failed: %d (%s)\n", e, strerror(e));
						pnode->fDisconnect = true;
					}
				}
			}
		}

		//
		// Send
		//
		if (hSocket >= 0 && FD_ISSET(hSocket, &fdsetSend))
		{
			TRY_CRITICAL_BLOCK(pnode->cs_vSend)
			{
				CDataStream& vSend = pnode->vSend;
				if (!vSend.empty())
				{
					int nBytes = (int)send(hSocket, (const char*)&vSend[0], vSend.size(), 0);

					if (nBytes > 0)
					{
						vSend.erase(vSend.begin(), vSend.begin() + nBytes);
					}
					else if (nBytes == 0)
					{
						if (pnode->ReadyToDisconnect())
							pnode->vSend.clear();
					}
					else
					{
						int e = errno;
						if (!IsWouldBlock(e) && e != EINTR && e != EINPROGRESS)
						{
							printf("send failed: %d (%s)\n", e, strerror(e));
							if (pnode->ReadyToDisconnect())
								pnode->vSend.clear();
							else
								pnode->fDisconnect = true;
						}
						// else: would-block, keep data queued
					}
				}
			}
		}
	}

	// Your existing Sleep(ms) wrapper if you have it:
	//Sleep(10);
	// or:
	 std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}




void CNode::Disconnect()
{
	printf("disconnecting node %s\n", addr.ToString().c_str());

	// Close socket (Linux/POSIX)
	int s = hSocket;
	hSocket = -1; // prevent races from reusing it

	if (s >= 0)
	{
		// Best-effort shutdown; ignore errors
		shutdown(s, SHUT_RDWR);
		close(s);
	}

	// If outbound and never got version message, mark address as failed
	if (!fInbound && nVersion == 0)
	{
		CRITICAL_BLOCK(cs_mapAddresses)
		{
			auto key = addr.GetKey();
			if (mapAddresses.count(key))
				mapAddresses[key].nLastFailed = GetTime();
			// else: don't create a new entry implicitly
		}
	}

	// Tear down product adverts (legacy marketplace bits)
	//CRITICAL_BLOCK(cs_mapProducts)
	//{
	//	for (auto mi = mapProducts.begin(); mi != mapProducts.end(); )
	//		AdvertRemoveSource(this, MSG_PRODUCT, 0, (mi++)->second);
	//}

	// Cancel subscriptions
	for (unsigned int nChannel = 0; nChannel < vfSubscribe.size(); nChannel++)
		if (vfSubscribe[nChannel])
			CancelSubscribe(nChannel);
}

bool AnySubscribed(unsigned int nChannel)
{
	if (pnodeLocalHost && pnodeLocalHost->IsSubscribed(nChannel))
		return true;

	bool any = false;

	CRITICAL_BLOCK(cs_vNodes)
	{
		for (CNode* pnode : vNodes)
		{
			if (pnode && pnode->IsSubscribed(nChannel))
			{
				any = true;
				break;
			}
		}
	}

	return any;
}

bool CNode::IsSubscribed(unsigned int nChannel)
{
	if (nChannel >= vfSubscribe.size())
		return false;
	return vfSubscribe[nChannel];
}

void CNode::Subscribe(unsigned int nChannel, unsigned int nHops)
{
	if (nChannel >= vfSubscribe.size())
		return;

	// If nobody is subscribed yet, we relay a subscribe to help bootstrap the channel
	if (!AnySubscribed(nChannel))
	{
		std::vector<CNode*> nodes;
		CRITICAL_BLOCK(cs_vNodes)
		{
			nodes = vNodes;
		}

		for (CNode* pnode : nodes)
		{
			if (pnode != this)
				pnode->PushMessage("subscribe", nChannel, nHops);
		}
	}

	vfSubscribe[nChannel] = true;
}



void CNode::CancelSubscribe(unsigned int nChannel)
{
	if (nChannel >= vfSubscribe.size())
		return;

	// Prevent from relaying cancel if wasn't subscribed
	if (!vfSubscribe[nChannel])
		return;

	vfSubscribe[nChannel] = false;

	if (!AnySubscribed(nChannel))
	{
		// Take a snapshot of nodes under lock, then release lock before PushMessage
		std::vector<CNode*> nodes;
		CRITICAL_BLOCK(cs_vNodes)
		{
			nodes = vNodes;
		}

		// Relay subscription cancel
		for (CNode* pnode : nodes)
		{
			if (pnode != this)
				pnode->PushMessage("sub-cancel", nChannel);
		}

		//// Clear memory, no longer subscribed
		//if (nChannel == MSG_PRODUCT)
		//{
		//	CRITICAL_BLOCK(cs_mapProducts)
		//		mapProducts.clear();
		//}
	}
}
