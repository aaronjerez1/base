#pragma once
#include <rocksdb/db.h>
#include <sstream>

extern rocksdb::DB* dbenv;



// Serialization functions for RocksDB
template <typename T>
std::string Serialize(const T& obj) {
    std::ostringstream oss;
    oss.write(reinterpret_cast<const char*>(&obj), sizeof(T));
    return oss.str();
}

template <typename T>
void Deserialize(const std::string& data, T& obj) {
    std::istringstream iss(data);
    iss.read(reinterpret_cast<char*>(&obj), sizeof(T));
}


// Modernized `CDB` class using RocksDB
class CDB {
protected:
    rocksdb::DB* pdb;  // RocksDB instance
    std::string strFile;
    std::vector<rocksdb::WriteBatch> vTxn;  // Transaction batches

    explicit CDB(const char* pszFile, bool fTxn = false);
    ~CDB() { Close(); }

public:
    void Close();

    // Delete copy constructor and assignment operator to avoid copying the database handle
    CDB(const CDB&) = delete;
    void operator=(const CDB&) = delete;

protected:
    // Generic read/write/erase/exists functions for RocksDB
    template<typename K, typename T>
    bool Read(const K& key, T& value) {
        if (!pdb)
            return false;

        std::string strKey = Serialize(key);
        std::string strValue;

        // RocksDB read operation
        rocksdb::Status s = pdb->Get(rocksdb::ReadOptions(), strKey, &strValue);
        if (!s.ok())
            return false;

        Deserialize(strValue, value);
        return true;
    }

    template<typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite = true) {
        if (!pdb)
            return false;

        std::string strKey = Serialize(key);
        std::string strValue = Serialize(value);

        // Write to RocksDB
        rocksdb::WriteOptions write_options;
        rocksdb::Status s = pdb->Put(write_options, strKey, strValue);
        return s.ok();
    }

    template<typename K>
    bool Erase(const K& key) {
        if (!pdb)
            return false;

        std::string strKey = Serialize(key);

        // Erase key-value pair from RocksDB
        rocksdb::WriteOptions write_options;
        rocksdb::Status s = pdb->Delete(write_options, strKey);
        return s.ok();
    }

    template<typename K>
    bool Exists(const K& key) {
        if (!pdb)
            return false;

        std::string strKey = Serialize(key);
        std::string strValue;

        // Check if key exists in RocksDB
        rocksdb::Status s = pdb->Get(rocksdb::ReadOptions(), strKey, &strValue);
        return s.ok();
    }

    // RocksDB transactions
    rocksdb::WriteBatch GetWriteBatch() {
        return rocksdb::WriteBatch();
    }

public:
    bool TxnBegin();
    bool TxnCommit();
    bool TxnAbort();
    bool ReadVersion(int& nVersion);
    bool WriteVersion(int nVersion);
};

