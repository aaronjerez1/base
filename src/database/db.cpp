#include "db.h"
#include "../globals.h"
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

rocksdb::DB* dbenv;
// Constructor for CDB class/
CDB::CDB(const char* pszFile, bool fTxn) : pdb(nullptr) {
    if (pszFile == nullptr) {
        return;
    }

    bool fCreate = true;  // RocksDB always allows creating a new DB if it doesn't exist

    CRITICAL_BLOCK(cs_db) {
        if (!fDbEnvInit) {
            std::string strAppDir = "database";
            if (!fs::exists(strAppDir)) {
                fs::create_directory(strAppDir);
            }
            std::cout << "dbenv.open strAppDir=" << strAppDir << std::endl;

            rocksdb::Options options;
            options.create_if_missing = true;
            options.create_missing_column_families = true;

            rocksdb::Status status = rocksdb::DB::Open(options, strAppDir, &dbenv);
            if (!status.ok()) {
                throw std::runtime_error("CDB() : error opening database environment: " + status.ToString());
            }
            fDbEnvInit = true;
        }

        strFile = pszFile;
        ++mapFileUseCount[strFile];
    }

    rocksdb::Options options;
    options.create_if_missing = true;

    rocksdb::Status status = rocksdb::DB::Open(options, pszFile, &pdb);
    if (!status.ok()) {
        std::cerr << "Error opening RocksDB: " << status.ToString() << std::endl;
        pdb = nullptr;
        CRITICAL_BLOCK(cs_db)
            --mapFileUseCount[strFile];
        throw std::runtime_error("CDB() : can't open database file " + strFile + ", error: " + status.ToString());
    }

    // Ensure that version is written to the database
    if (fCreate && !Exists(std::string("version"))) {
        WriteVersion(1);
    }
}

// Method to close the database
void CDB::Close() {
    if (!pdb) {
        return;
    }

    delete pdb;
    pdb = nullptr;

    CRITICAL_BLOCK(cs_db)
        --mapFileUseCount[strFile];

    // Flush remaining changes to disk
    if (dbenv) {
        dbenv->FlushWAL(true);
    }
}

bool CDB::TxnBegin() {
    if (!pdb)
        return false;
    // Create a new WriteBatch transaction
    vTxn.push_back(rocksdb::WriteBatch());
    return true;
}

bool CDB::TxnAbort() {
    if (!pdb || vTxn.empty())
        return false;

    // We simply discard the last transaction by popping it
    vTxn.pop_back();
    return true;
}

bool CDB::TxnCommit() {
    if (!pdb || vTxn.empty())
        return false;

    rocksdb::WriteBatch& batch = vTxn.back();  // Get the last transaction batch
    rocksdb::WriteOptions write_options;
    rocksdb::Status s = pdb->Write(write_options, &batch);  // Commit the transaction
    vTxn.pop_back();  // Remove the last transaction batch

    return s.ok();
}

// Implementation of ReadVersion
bool CDB::ReadVersion(int& nVersion) {
    nVersion = 0;
    return Read(std::string("version"), nVersion);
}

// Implementation of WriteVersion
bool CDB::WriteVersion(int nVersion) {
    return Write(std::string("version"), nVersion);
}