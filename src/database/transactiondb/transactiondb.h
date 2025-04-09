#include "../db.h"
////
// Header moved to globals.h to avoiud circular dependecy
// Implementation is still in transaction.cpp
/////

//#include "../globals.h"
// if i foward declare
// if i 

//class CTxDB : public CDB {
//public:
//    CTxDB(const char* pszMode = "r+", bool fTxn = false) : CDB(!fClient ? "blkindex.base" : nullptr, fTxn) {}
//
//    bool ReadTxIndex(uint256 hash, CTxIndex& txindex);
//    bool UpdateTxIndex(uint256 hash, const CTxIndex& txindex);
//    bool AddTxIndex(const CTransaction& tx, const CDiskTxPos& pos, int nHeight);
//    bool EraseTxIndex(const CTransaction& tx);
//    bool ContainsTx(uint256 hash);
//    bool ReadOwnerTxes(uint160 hash160, int nHeight, vector<CTransaction>& vtx);
//    bool ReadDiskTx(uint256 hash, CTransaction& tx, CTxIndex& txindex);
//    bool ReadDiskTx(uint256 hash, CTransaction& tx);
//    bool ReadDiskTx(COutPoint outpoint, CTransaction& tx, CTxIndex& txindex);
//    bool ReadDiskTx(COutPoint outpoint, CTransaction& tx);
//    bool WriteBlockIndex(const CDiskBlockIndex& blockindex);
//    bool EraseBlockIndex(uint256 hash);
//    bool ReadHashBestChain(uint256& hashBestChain);
//    bool WriteHashBestChain(uint256 hashBestChain);
// /*   bool WriteModelHeader(const GPT2* model);
//    bool WriteModelParameters(const GPT2* model);*/
//    bool LoadBlockIndex();
//    //bool LoadGPT2(GPT2* model);
//};
