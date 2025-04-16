#pragma once
#include "../db.h"
#include "../shared/standard.h"
#include "../shared/key.h"

class CWalletTx;
class uint256;

// Wallet Database class using RocksDB
class CWalletDB : public CDB {
public:
    CWalletDB(const char* pszMode = "r+", bool fTxn = false) : CDB("wallet.base", fTxn) {}

    bool ReadName(const std::string& strAddress, std::string& strName);

    bool WriteName(const std::string& strAddress, const std::string& strName);

    bool EraseName(const std::string& strAddress);

    bool ReadTx(uint256 hash, CWalletTx& wtx);

    bool WriteTx(uint256 hash, const CWalletTx& wtx);

    bool EraseTx(uint256 hash);

    bool ReadKey(const std::vector<unsigned char>& vchPubKey, CPrivKey& vchPrivKey);

    bool WriteKey(const std::vector<unsigned char>& vchPubKey, const CPrivKey& vchPrivKey);

    bool ReadDefaultKey(std::vector<unsigned char>& vchPubKey);

    bool WriteDefaultKey(const std::vector<unsigned char>& vchPubKey);

    template<typename T>
    bool ReadSetting(const std::string& strKey, T& value) {
        return Read(std::make_pair(std::string("setting"), strKey), value);
    }

    template<typename T>
    bool WriteSetting(const std::string& strKey, const T& value) {
        return Write(std::make_pair(std::string("setting"), strKey), value);
    }

    //bool LoadWallet(std::vector<unsigned char>& vchDefaultKeyRet);
};

//bool LoadWallet();

inline bool SetAddressBookName(const std::string& strAddress, const std::string& strName) {
    return CWalletDB().WriteName(strAddress, strName);
}