#include "../globals.h"


//bool CTransaction::ConnectInputs(CTxDB& txdb, std::map<uint256, CTxIndex>& mapTestPool, CDiskTxPos posThisTx, int nHeight, int64& nFees, bool fBlock, bool fMiner, int64 nMinFee)
//{
//    // Take over previous transactions' spent pointers
//    if (!IsCoinBase())
//    {
//        int64 nValueIn = 0;
//        for (int i = 0; i < vin.size(); i++)
//        {
//            COutPoint prevout = vin[i].prevout;
//
//            // Read txindex
//            CTxIndex txindex;
//            bool fFound = true;
//            if (fMiner && mapTestPool.count(prevout.hash))
//            {
//                // Get txindex from current proposed changes
//                txindex = mapTestPool[prevout.hash];
//            }
//            else
//            {
//                // Read txindex from txdb
//                fFound = txdb.ReadTxIndex(prevout.hash, txindex);
//            }
//            if (!fFound && (fBlock || fMiner))
//                return fMiner ? false : error("ConnectInputs() : %s prev tx %s index entry not found", GetHash().ToString().substr(0, 6).c_str(), prevout.hash.ToString().substr(0, 6).c_str());
//
//            // Read txPrev
//            CTransaction txPrev;
//            if (!fFound || txindex.pos == CDiskTxPos(1, 1, 1))
//            {
//                // Get prev tx from single transactions in memory
//                CRITICAL_BLOCK(cs_mapTransactions)
//                {
//                    if (!mapTransactions.count(prevout.hash))
//                        return error("ConnectInputs() : %s mapTransactions prev not found %s", GetHash().ToString().substr(0, 6).c_str(), prevout.hash.ToString().substr(0, 6).c_str());
//                    txPrev = mapTransactions[prevout.hash];
//                }
//                if (!fFound)
//                    txindex.vSpent.resize(txPrev.vout.size());
//            }
//            else
//            {
//                // Get prev tx from disk
//                if (!txPrev.ReadFromDisk(txindex.pos))
//                    return error("ConnectInputs() : %s ReadFromDisk prev tx %s failed", GetHash().ToString().substr(0, 6).c_str(), prevout.hash.ToString().substr(0, 6).c_str());
//            }
//
//            if (prevout.n >= txPrev.vout.size() || prevout.n >= txindex.vSpent.size())
//                return error("ConnectInputs() : %s prevout.n out of range %d %d %d", GetHash().ToString().substr(0, 6).c_str(), prevout.n, txPrev.vout.size(), txindex.vSpent.size());
//
//            // If prev is coinbase, check that it's matured
//            if (txPrev.IsCoinBase())
//                for (CBlockIndex* pindex = pindexBest; pindex && nBestHeight - pindex->nHeight < COINBASE_MATURITY - 1; pindex = pindex->pprev)
//                    if (pindex->nBlockPos == txindex.pos.nBlockPos && pindex->nFile == txindex.pos.nFile)
//                        return error("ConnectInputs() : tried to spend coinbase at depth %d", nBestHeight - pindex->nHeight);
//
//            // Verify signature TODO:
//            //if (!VerifySignature(txPrev, *this, i))
//            //    return error("ConnectInputs() : %s VerifySignature failed", GetHash().ToString().substr(0, 6).c_str());
//
//            // Check for conflicts
//            if (!txindex.vSpent[prevout.n].IsNull())
//                return fMiner ? false : error("ConnectInputs() : %s prev tx already used at %s", GetHash().ToString().substr(0, 6).c_str(), txindex.vSpent[prevout.n].ToString().c_str());
//
//            // Mark outpoints as spent
//            txindex.vSpent[prevout.n] = posThisTx;
//
//            // Write back
//            if (fBlock)
//                txdb.UpdateTxIndex(prevout.hash, txindex);
//            else if (fMiner)
//                mapTestPool[prevout.hash] = txindex;
//
//            nValueIn += txPrev.vout[prevout.n].nValue;
//        }
//
//        // Tally transaction fees
//        int64 nTxFee = nValueIn - GetValueOut();
//        if (nTxFee < 0)
//            return error("ConnectInputs() : %s nTxFee < 0", GetHash().ToString().substr(0, 6).c_str());
//        if (nTxFee < nMinFee)
//            return false;
//        nFees += nTxFee;
//    }
//
//    if (fBlock)
//    {
//        // Add transaction to disk index
//        if (!txdb.AddTxIndex(*this, posThisTx, nHeight))
//            return error("ConnectInputs() : AddTxPos failed");
//    }
//    else if (fMiner)
//    {
//        // Add transaction to test pool
//        mapTestPool[GetHash()] = CTxIndex(CDiskTxPos(1, 1, 1), vout.size());
//    }
//
//    return true;
//}
