#include "transactiondb.h"
#include "../globals.h"

bool CTxDB::ReadTxIndex(uint256 hash, CTxIndex& txindex)
{
    txindex.SetNull();
    return Read(std::make_pair(std::string("tx"), hash), txindex);
}

bool CTxDB::UpdateTxIndex(uint256 hash, const CTxIndex& txindex)
{
    return Write(std::make_pair(std::string("tx"), hash), txindex);
}

bool CTxDB::AddTxIndex(const CTransaction& tx, const CDiskTxPos& pos, int nHeight)
{
    uint256 hash = tx.GetHash();
    CTxIndex txindex(pos, tx.vout.size());
    return Write(std::make_pair(std::string("tx"), hash), txindex);
}

bool CTxDB::EraseTxIndex(const CTransaction& tx)
{
    uint256 hash = tx.GetHash();
    return Erase(std::make_pair(std::string("tx"), hash));
}

bool CTxDB::ContainsTx(uint256 hash)
{
    return Exists(std::make_pair(std::string("tx"), hash));
}

bool CTxDB::ReadOwnerTxes(uint160 hash160, int nMinHeight, std::vector<CTransaction>& vtx)
{
    vtx.clear();

    rocksdb::Iterator* it = pdb->NewIterator(rocksdb::ReadOptions());
    std::string prefix = Serialize(std::make_pair(std::string("owner"), hash160));

    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        // Convert the RocksDB key and value from string to a vector of unsigned chars
        std::vector<unsigned char> keyData(it->key().data(), it->key().data() + it->key().size());
        std::vector<unsigned char> valueData(it->value().data(), it->value().data() + it->value().size());

        // Create CDataStream objects from these vectors
        CDataStream ssKey(keyData, SER_DISK, VERSION);
        CDataStream ssValue(valueData, SER_DISK, VERSION);

        std::string strType;
        uint160 hashItem;
        CDiskTxPos pos;
        ssKey >> strType >> hashItem >> pos;

        int nItemHeight;
        ssValue >> nItemHeight;

        if (nItemHeight >= nMinHeight)
        {
            vtx.resize(vtx.size() + 1);
            if (!vtx.back().ReadFromDisk(pos))
                return false;
        }
    }

    delete it;
    return true;
}


bool CTxDB::ReadDiskTx(uint256 hash, CTransaction& tx, CTxIndex& txindex)
{
    tx.SetNull();
    if (!ReadTxIndex(hash, txindex))
        return false;
    return (tx.ReadFromDisk(txindex.pos));
}

bool CTxDB::ReadDiskTx(uint256 hash, CTransaction& tx)
{
    CTxIndex txindex;
    return ReadDiskTx(hash, tx, txindex);
}

bool CTxDB::ReadDiskTx(COutPoint outpoint, CTransaction& tx, CTxIndex& txindex)
{
    return ReadDiskTx(outpoint.hash, tx, txindex);
}

bool CTxDB::ReadDiskTx(COutPoint outpoint, CTransaction& tx)
{
    CTxIndex txindex;
    return ReadDiskTx(outpoint.hash, tx, txindex);
}



bool CTxDB::WriteBlockIndex(const CDiskBlockIndex& blockindex)
{
    return Write(std::make_pair(std::string("blockindex"), blockindex.GetBlockHash()), blockindex);
}

bool CTxDB::EraseBlockIndex(uint256 hash)
{
    return Erase(std::make_pair(std::string("blockindex"), hash));
}

bool CTxDB::ReadHashBestChain(uint256& hashBestChain)
{
    return Read(std::string("hashBestChain"), hashBestChain);
}

bool CTxDB::WriteHashBestChain(uint256 hashBestChain)
{
    return Write(std::string("hashBestChain"), hashBestChain);
}






CBlockIndex* InsertBlockIndex(uint256 hash)
{
    if (hash == 0)
        return NULL;

    auto mi = mapBlockIndex.find(hash);
    if (mi != mapBlockIndex.end())
        return (*mi).second;

    CBlockIndex* pindexNew = new CBlockIndex();
    if (!pindexNew)
        throw std::runtime_error("LoadBlockIndex() : new CBlockIndex failed");

    mi = mapBlockIndex.insert(std::make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);

    return pindexNew;
}


bool CTxDB::LoadBlockIndex() {
    // Create a RocksDB iterator
    rocksdb::Iterator* it = pdb->NewIterator(rocksdb::ReadOptions());

    // Prefix for block index entries
    std::string prefix = Serialize(std::make_pair(std::string("blockindex"), uint256(0)));

    // Use the iterator to seek and load block index records
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        // Read the key and value from the RocksDB iterator
        std::vector<unsigned char> keyData(it->key().data(), it->key().data() + it->key().size());
        std::vector<unsigned char> valueData(it->value().data(), it->value().data() + it->value().size());

        // Ensure that keyData and valueData are not empty before deserialization
        if (keyData.empty() || valueData.empty()) {
            std::cerr << "Error: Empty key or value data, skipping..." << std::endl;
            continue;
        }

        try {
            // Construct CDataStream objects using raw pointers
            CDataStream ssKey((const char*)keyData.data(), (const char*)keyData.data() + keyData.size(), SER_DISK, VERSION);
            CDataStream ssValue((const char*)valueData.data(), (const char*)valueData.data() + valueData.size(), SER_DISK, VERSION);

            std::string strType;
            ssKey >> strType;  // Deserialize the string that represents the type of data

            // Check if the key is for a block index
            if (strType == "blockindex") {
                CDiskBlockIndex diskindex;
                ssValue >> diskindex;  // Deserialize the disk index

                // Insert the new block index into the map
                CBlockIndex* pindexNew = InsertBlockIndex(diskindex.GetBlockHash());
                pindexNew->pprev = InsertBlockIndex(diskindex.hashPrev);
                pindexNew->pnext = InsertBlockIndex(diskindex.hashNext);
                pindexNew->nFile = diskindex.nFile;
                pindexNew->nBlockPos = diskindex.nBlockPos;
                pindexNew->nHeight = diskindex.nHeight;
                pindexNew->nVersion = diskindex.nVersion;
                pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                pindexNew->nTime = diskindex.nTime;
                pindexNew->nBits = diskindex.nBits;
                pindexNew->nNonce = diskindex.nNonce;

                // Check for the genesis block
                if (pindexGenesisBlock == NULL && diskindex.GetBlockHash() == hashGenesisBlock) { // TODO: the hash for the genesis block  has to be manually created
                    pindexGenesisBlock = pindexNew;
                }
            }
            else {
                break;  // If we encounter a non-blockindex key, exit the loop
            }
        }
        catch (const std::exception& e) {
            // Handle any deserialization errors
            std::cerr << "Error deserializing block index: " << e.what() << std::endl;
            continue;
        }
    }

    // Clean up the iterator
    delete it;

    // Read the best chain hash from the database
    if (!ReadHashBestChain(hashBestChain)) {
        if (pindexGenesisBlock == NULL)
            return true;  // If there's no genesis block, just return true
        throw std::runtime_error("CTxDB::LoadBlockIndex() : hashBestChain not found");
    }

    // Ensure the best chain block index exists
    if (!mapBlockIndex.count(hashBestChain)) {
        throw std::runtime_error("CTxDB::LoadBlockIndex() : blockindex for hashBestChain not found");
    }

    // Set the best block
    pindexBest = mapBlockIndex[hashBestChain];
    nBestHeight = pindexBest->nHeight;

    // Log the results
    std::cout << "LoadBlockIndex(): hashBestChain=" << hashBestChain.ToString().substr(0, 14)
        << "  height=" << nBestHeight << std::endl;

    return true;
}
