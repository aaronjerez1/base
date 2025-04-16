#pragma once
#include <map>
#include <string>
#include <vector>

class CAddress;
class CCriticalSection;

extern bool RecvLine(int hSocket, std::string& strLine);
extern void ThreadIRCSeed(void* parg);
extern bool fRestartIRCSeed;

extern std::map<std::vector<unsigned char>, CAddress> mapIRCAddresses;
extern CCriticalSection cs_mapIRCAddresses;
