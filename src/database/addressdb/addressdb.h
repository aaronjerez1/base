#pragma once
#include "../db.h"

class CAddress;

class CAddrDB : public CDB {
public:
    CAddrDB(const char* pszMode = "r+", bool fTxn = false) : CDB("addr.base", fTxn) {}

    bool WriteAddress(const CAddress& addr);
    //bool LoadAddresses();
};

//bool LoadAddresses();