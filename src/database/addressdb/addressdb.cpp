#include "addressdb.h"
#include "../globals.h"

CCriticalSection cs_mapIRCAddresses; // this is very convoluted thing. this was in irc
//CCriticalSection cs_mapAddresses; // and this in net
//
// took a two day break.
// we left it on, a part of the requirements is in irc, and another in net.
// but this file is required by net, hence a circular dependecy
// 
// we need then to specifyu what of this file is needed from net
// 
// and what of net is needed of this file, and the problems thereof.
// 
// 
// or orignal requirement was the address database for final use on net.c (for final working networking and savign of the ipaddrssses of the nodes i think)
// 
bool CAddrDB::WriteAddress(const CAddress& addr) {
    // Serialize the address key and the address object
    return Write(std::make_pair(std::string("addr"), addr.GetKey()), addr);
}

//bool CAddrDB::LoadAddresses() {
//    // Locks for critical sections can be handled with std::mutex if necessary
//    CRITICAL_BLOCK(cs_mapIRCAddresses)
//        CRITICAL_BLOCK(cs_mapAddresses)
//    {
//
//        // Load user-provided addresses from a file (addr.txt)
//        std::ifstream filein("addr.txt");
//        if (filein.is_open()) {
//            std::string line;
//            while (std::getline(filein, line)) {
//                CAddress addr(line.c_str(), NODE_NETWORK);
//                if (addr.ip != 0) {
//                    AddAddress(*this, addr);
//                    mapIRCAddresses.insert(std::make_pair(addr.GetKey(), addr));
//                }
//            }
//            filein.close();
//        }
//
//        // Create a RocksDB iterator to read the data from the database
//        rocksdb::Iterator* it = pdb->NewIterator(rocksdb::ReadOptions());
//
//        // We want to find all keys starting with "addr"
//        std::string prefix = "addr";
//        for (it->Seek(prefix); it->Valid() && it->key().ToString().compare(0, prefix.size(), prefix) == 0; it->Next()) {
//            // Convert the RocksDB key and value into a byte stream
//            std::string strKey = it->key().ToString();
//            std::string strValue = it->value().ToString();
//
//            // Construct CDataStream objects using the appropriate constructor
//            CDataStream ssKey((const char*)strKey.data(), (const char*)strKey.data() + strKey.size(), SER_DISK, VERSION);
//            CDataStream ssValue((const char*)strValue.data(), (const char*)strValue.data() + strValue.size(), SER_DISK, VERSION);
//
//            std::string strType;
//            ssKey >> strType;
//
//            if (strType == "addr") {
//                CAddress addr;
//                ssValue >> addr;
//                mapAddresses.insert(std::make_pair(addr.GetKey(), addr));
//            }
//        }
//
//
//
//        // Clean up the iterator
//        delete it;
//
//        // Debug print
//        printf("mapAddresses:\n");
//        for (const auto& item : mapAddresses) {
//            item.second.print();
//        }
//        printf("-----\n");
//
//    }
//
//    return true;
//}