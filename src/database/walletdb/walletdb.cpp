#include "walletdb.h"

#include "../globals.h"
#include "../../shared/base58.h"


bool CWalletDB::ReadName(const std::string& strAddress, std::string& strName) {
    strName = "";
    return Read(std::make_pair(std::string("name"), strAddress), strName);
}

bool CWalletDB::WriteName(const std::string& strAddress, const std::string& strName) {
    mapAddressBook[strAddress] = strName;
    return Write(std::make_pair(std::string("name"), strAddress), strName);
}

bool CWalletDB::EraseName(const std::string& strAddress) {
    mapAddressBook.erase(strAddress);
    return Erase(std::make_pair(std::string("name"), strAddress));
}

bool CWalletDB::ReadTx(uint256 hash, CWalletTx& wtx) {
    return Read(std::make_pair(std::string("tx"), hash), wtx);
}

bool CWalletDB::WriteTx(uint256 hash, const CWalletTx& wtx) {
    return Write(std::make_pair(std::string("tx"), hash), wtx);
}

bool CWalletDB::EraseTx(uint256 hash) {
    return Erase(std::make_pair(std::string("tx"), hash));
}

bool CWalletDB::ReadKey(const std::vector<unsigned char>& vchPubKey, CPrivKey& vchPrivKey) {
    vchPrivKey.clear();
    return Read(std::make_pair(std::string("key"), vchPubKey), vchPrivKey);
}

bool CWalletDB::WriteKey(const std::vector<unsigned char>& vchPubKey, const CPrivKey& vchPrivKey) {
    return Write(std::make_pair(std::string("key"), vchPubKey), vchPrivKey, false);
}

bool CWalletDB::ReadDefaultKey(std::vector<unsigned char>& vchPubKey) {
    vchPubKey.clear();
    return Read(std::string("defaultkey"), vchPubKey);
}

bool CWalletDB::WriteDefaultKey(const std::vector<unsigned char>& vchPubKey) {
    return Write(std::string("defaultkey"), vchPubKey);
}












bool CWalletDB::LoadWallet(std::vector<unsigned char>& vchDefaultKeyRet)
{
    vchDefaultKeyRet.clear();

    // Use the old CRITICAL_BLOCK for thread safety
    CRITICAL_BLOCK(cs_mapKeys)
    CRITICAL_BLOCK(cs_mapWallet)
    {
        // Create a RocksDB iterator
        rocksdb::Iterator* it = pdb->NewIterator(rocksdb::ReadOptions());

        // Iterate over all the entries in the wallet
        for (it->SeekToFirst(); it->Valid(); it->Next())
        {
            // Convert the RocksDB key and value into strings
            std::string strKey = it->key().ToString();
            std::string strValue = it->value().ToString();

            // Create CDataStream objects to deserialize the key-value pairs
            CDataStream ssKey(strKey.data(), strKey.data() + strKey.size(), SER_DISK, VERSION);
            CDataStream ssValue(strValue.data(), strValue.data() + strValue.size(), SER_DISK, VERSION);

            // Unserialize key type
            std::string strType;
            ssKey >> strType;

            // Handle each type of entry in the wallet
            if (strType == "name")
            {
                std::string strAddress;
                ssKey >> strAddress;
                ssValue >> mapAddressBook[strAddress];
            }
            else if (strType == "tx")
            {
                uint256 hash;
                ssKey >> hash;
                CWalletTx& wtx = mapWallet[hash];
                ssValue >> wtx;

                if (wtx.GetHash() != hash)
                    printf("Error in wallet.dat, hash mismatch\n");
            }
            else if (strType == "key")
            {
                std::vector<unsigned char> vchPubKey;
                ssKey >> vchPubKey;
                CPrivKey vchPrivKey;
                ssValue >> vchPrivKey;

                mapKeys[vchPubKey] = vchPrivKey;
                mapPubKeys[Hash160(vchPubKey)] = vchPubKey;
            }
            else if (strType == "defaultkey")
            {
                ssValue >> vchDefaultKeyRet;
            }
            else if (strType == "setting")
            {
                std::string strSettingKey;
                ssKey >> strSettingKey;

                if (strSettingKey == "fGenerateBasecoins")  ssValue >> fGenerateBasecoins;
                if (strSettingKey == "nTransactionFee")    ssValue >> nTransactionFee;
                if (strSettingKey == "addrIncoming")       ssValue >> addrIncoming;
                /// UI possibly params
                //if (strSettingKey == "minimizeToTray")     ssValue >> minimizeToTray;
                //if (strSettingKey == "closeToTray")        ssValue >> closeToTray;
                //if (strSettingKey == "startOnSysBoot")     ssValue >> startOnSysBoot;
                //if (strSettingKey == "askBeforeClosing")   ssValue >> askBeforeClosing;
                //if (strSettingKey == "alwaysShowTrayIcon") ssValue >> alwaysShowTrayIcon;
            }
        }

        // Clean up the iterator
        delete it;

        // Debug print
        printf("fGenerateCoins = %d\n", fGenerateBasecoins);
        printf("nTransactionFee = %I64d\n", nTransactionFee);
        printf("addrIncoming = %s\n", addrIncoming.ToString().c_str());
    }

    return true;
}

bool LoadWallet()
{
    std::vector<unsigned char> vchDefaultKey;

    // Read the wallet from RocksDB (similar to CWalletDB("cr").LoadWallet(vchDefaultKey))
    if (!CWalletDB("cr").LoadWallet(vchDefaultKey)) {
        return false;
    }

    // Check if the default key is already present in the mapKeys
    if (mapKeys.count(vchDefaultKey)) {
        // Set the public and private keys for keyUser
        keyUser.SetPubKey(vchDefaultKey);
        keyUser.SetPrivKey(mapKeys[vchDefaultKey]);
    }
    else {
        // Generate a new keyUser as the default key
        RandAddSeed(true);  // Adding entropy for random number generation
        keyUser.MakeNewKey();  // Create a new key

        // Add the new key to the wallet
        if (!AddKey(keyUser)) {
            return false;
        }

        // Set the address book name for the default key
        if (!SetAddressBookName(PubKeyToAddress(keyUser.GetPubKey()), "Your Address")) {
            return false;
        }

        // Write the new default key to the wallet database
        if (!CWalletDB().WriteDefaultKey(keyUser.GetPubKey())) {
            return false;
        }
    }

    return true;
}