#include "net.h"


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
vector<CNode*> vNodes;
CCriticalSection cs_vNodes;
std::map<vector<unsigned char>, CAddress> mapAddresses;
CCriticalSection cs_mapAddresses;
std::map<CInv, CDataStream> mapRelay;
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