#include "globals.h"
#include <sys/stat.h>
#include "shared/key.h"
#include "database/walletdb/walletdb.h"
#include "network/net.h"
#include <algorithm> 


// Settings
int fGenerateBasecoins = 1; // hehe
int64 nTransactionFee = 0;
CAddress addrIncoming;


unsigned int nTransactionsUpdated = 0;
bool fDbEnvInit = false;
std::map<uint256, CTransaction> mapTransactions;
std::map<COutPoint, CInPoint> mapNextTx;


const unsigned int MAX_SIZE = 0x02000000;
const int64 COIN = 100000000;
const int64 CENT = 1000000;
const int COINBASE_MATURITY = 100;

CBlockIndex* pindexBest = NULL;
CBlockIndex* pindexGenesisBlock = NULL;
uint256 hashBestChain = 0;

std::map<uint256, CBlock*> mapOrphanBlocks;
std::multimap<uint256, CBlock*> mapOrphanBlocksByPrev;

std::map<uint256, CDataStream*> mapOrphanTransactions;
std::multimap<uint256, CDataStream*> mapOrphanTransactionsByPrev;

uint256 myvalue(0);

const CBigNum bnProofOfWorkLimit(~uint256(0) >> 32);
const uint256 hashGenesisBlock("f1111738a22917f78aa841427dc870905efd8e6027d4468a4ebabbd5b394651e"); // TODO: find the proper genesis block manually since this is hardcoded.

int nBestHeight = -1;
std::map<uint256, CBlockIndex*> mapBlockIndex; // organize these guys


CCriticalSection cs_main;
CCriticalSection cs_mapTransactions;
CCriticalSection cs_db;

CKey keyUser;
std::map<std::vector<unsigned char>, CPrivKey> mapKeys; //in shared/key.h
std::map<uint160, std::vector<unsigned char> > mapPubKeys;
CCriticalSection cs_mapKeys;

//string strSetDataDir;
int nDropMessagesTest = 0;


//////////////////////////////////////////////////////////////////////////////
//
// mapKeys
//

bool AddKey(const CKey& key)
{
    CRITICAL_BLOCK(cs_mapKeys)
    {
        mapKeys[key.GetPubKey()] = key.GetPrivKey();
        mapPubKeys[Hash160(key.GetPubKey())] = key.GetPubKey();
    }
    return CWalletDB().WriteKey(key.GetPubKey(), key.GetPrivKey());
}

std::vector<unsigned char> GenerateNewKey()
{
    CKey key;
    key.MakeNewKey();
    if (!AddKey(key))
        throw std::runtime_error("GenerateNewKey() : AddKey failed\n");
    return key.GetPubKey();
}


std::map<uint256, CWalletTx> mapWallet;
std::vector<std::pair<uint256, bool> > vWalletUpdated;
CCriticalSection cs_mapWallet;

std::string strSetDataDir;

std::map<std::string, int> mapFileUseCount;
std::map<string, string> mapAddressBook;


bool CBlock::AddToBlockIndex(unsigned int nFile, unsigned int nBlockPos)
{
    return true;
}


std::string GetAppDir()
{
    std::string strDir;
    if (!strSetDataDir.empty())
    {
        strDir = strSetDataDir;
    }
    else if (getenv("HOME"))
    {
        // Linux/Unix standard: ~/.base
        strDir = strprintf("%s/.base", getenv("HOME"));
    }
    else
    {
        // Fallback to current directory
        return ".";
    }

    static bool fMkdirDone;
    if (!fMkdirDone)
    {
        fMkdirDone = true;
        mkdir(strDir.c_str(), 0755);
    }

    return strDir;
}

FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode)
{
    if (nFile == -1)
    {
        return NULL;
    }
    FILE* file = fopen(strprintf("%s\\blk%04d.base", GetAppDir().c_str(), nFile).c_str(), pszMode);
    if (!file)
    {
        return NULL;
    }
    if (nBlockPos != 0 && !strchr(pszMode, 'a') && !strchr(pszMode, 'w'))
    {
        if (fseek(file, nBlockPos, SEEK_SET) != 0)
        {
            fclose(file);
            return NULL;
        }
    }
    return file;
}

static unsigned int nCurrentBlockFile = 1;

FILE* AppendBlockFile(unsigned int& nFileRet)
{
    nFileRet = 0;
    loop
    {
        FILE * file = OpenBlockFile(nCurrentBlockFile, 0, "ab");
        if (!file)
        {
            return NULL;
        }
        if (fseek(file, 0, SEEK_END) != 0)
        {
            return NULL;
        }
        // Old 32 bit system requirements:  FAT32 filesize max 4GB, fseek and freel max 2gb, so we must stay under 2 gb
        // we are going to keep this requirements for the first version
        // TODO: decide when to modernize this file requiremnts to 64 bit system
        if (ftell(file) < 0x7F000000 - MAX_SIZE)
        {
            nFileRet = nCurrentBlockFile;
            return file;
        }
        fclose(file);
        nCurrentBlockFile++;

    }
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast)
{
    const unsigned int nTargetTimespan = 14 * 24 * 60 * 60; // two weeks
    const unsigned int nTargetSpacing = 10 * 60;
    const unsigned int nInterval = nTargetTimespan / nTargetSpacing;

     //Genesis Block
    if (pindexLast == NULL)
    {
        return bnProofOfWorkLimit.GetCompact();
    }

    //  Only change once per interval
    if ((pindexLast->nHeight + 1) % nInterval != 0)
    {
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < nInterval - 1; i++)
    {
        pindexFirst = pindexFirst->pprev;
    }
    assert(pindexFirst);

    // Limit adjustment step
    unsigned int nActualTimespan = pindexLast->nTime - pindexFirst->nTime;
    printf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
    if (nActualTimespan < nTargetTimespan / 4)
        nActualTimespan = nTargetTimespan / 4;
    if (nActualTimespan > nTargetTimespan * 4)
        nActualTimespan = nTargetTimespan * 4;

    // Retarget
    CBigNum bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > bnProofOfWorkLimit)
        bnNew = bnProofOfWorkLimit;


    /// debug print
    printf("\n\n\nGetNextWorkRequired RETARGET *****\n");
    printf("nTargetTimespan = %d    nActualTimespan = %d\n", nTargetTimespan, nActualTimespan);
    printf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString().c_str());
    printf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());

    return bnNew.GetCompact();

}








///
/// CMerkleTx
/// 
int CMerkleTx::GetBlocksToMaturity() const {
    std::runtime_error("NOT IMPLEMENTEd");
    return 1;
}

int CMerkleTx::GetDepthInMainChain() const
{
    std::runtime_error("NOT IMPLEMENTEd");
    return 1;
    //if (hashBlock == 0 || nIndex == -1)
    //    return 0;

    //// Find the block it claims to be in
    //std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
    //if (mi == mapBlockIndex.end())
    //    return 0;
    //CBlockIndex* pindex = (*mi).second;
    //if (!pindex || !pindex->IsInMainChain())
    //    return 0;

    //// Make sure the merkle branch connects to this block
    //if (!fMerkleVerified)
    //{
    //    if (CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
    //        return 0;
    //    fMerkleVerified = true;
    //}

    //return pindexBest->nHeight - pindex->nHeight + 1;
}


bool CMerkleTx::AcceptTransaction(CTxDB& txdb, bool fCheckInputs)
{
    if (fClient)
    {
        if (!IsInMainChain() && !ClientConnectInputs())
            return false;
        return CTransaction::AcceptTransaction(txdb, false);
    }
    else
    {
        return CTransaction::AcceptTransaction(txdb, fCheckInputs);
    }
}




///
/// CWalletTx
/// 
bool CWalletTx::AcceptWalletTransaction(CTxDB& txdb, bool fCheckInputs)
{
    CRITICAL_BLOCK(cs_mapTransactions)
    {
        for(CMerkleTx & tx: vtxPrev)
        {
            if (!tx.IsCoinBase())
            {
                uint256 hash = tx.GetHash();
                if (!mapTransactions.count(hash) && !txdb.ContainsTx(hash))
                    tx.AcceptTransaction(txdb, fCheckInputs);
            }
        }
        if (!IsCoinBase())
            return AcceptTransaction(txdb, fCheckInputs);
    }
    return true;
}

///
/// METTHODS
/// 

void ReacceptWalletTransactions()
{
    // Reaccept any txes of ours that aren't already in a block
    CTxDB txdb("r");
    CRITICAL_BLOCK(cs_mapWallet)
    {
        for(PAIRTYPE(const uint256, CWalletTx) & item: mapWallet)
        {
            CWalletTx& wtx = item.second;
            if (!wtx.IsCoinBase() && !txdb.ContainsTx(wtx.GetHash()))
                wtx.AcceptWalletTransaction(txdb, false);
        }
    }
}

bool CheckDiskSpace(int64 nAdditionalBytes)
{
    //uint64 nFreeBytesAvailable = 0;     // bytes available to caller
    //uint64 nTotalNumberOfBytes = 0;     // bytes on disk
    //uint64 nTotalNumberOfFreeBytes = 0; // free bytes on disk

    //if (!GetDiskFreeSpaceEx(GetAppDir().c_str(),
    //    (PULARGE_INTEGER)&nFreeBytesAvailable,
    //    (PULARGE_INTEGER)&nTotalNumberOfBytes,
    //    (PULARGE_INTEGER)&nTotalNumberOfFreeBytes))
    //{
    //    printf("ERROR: GetDiskFreeSpaceEx() failed\n");
    //    return true;
    //}

    //// Check for 15MB because database could create another 10MB log file at any time
    //if ((int64)nFreeBytesAvailable < 15000000 + nAdditionalBytes)
    //{
    //    fShutdown = true;
    //    wxMessageBox("Warning: Your disk space is low  ", "Bitcoin", wxICON_EXCLAMATION);
    //    _beginthread(Shutdown, 0, NULL);
    //    return false;
    //}
    std::runtime_error("NOT IMPLMENETED");

    return true;
}


bool CTransaction::AddToMemoryPool()
{
    // Add to memory pool without checking anything.  Don't call this directly,
    // call AcceptTransaction to properly check the transaction first.
    CRITICAL_BLOCK(cs_mapTransactions)
    {
        uint256 hash = GetHash();
        mapTransactions[hash] = *this;
        for (int i = 0; i < vin.size(); i++)
            mapNextTx[vin[i].prevout] = CInPoint(&mapTransactions[hash], i);
        nTransactionsUpdated++;
    }
    return true;
}

bool CTransaction::RemoveFromMemoryPool()
{
    // Remove transaction from memory pool
    CRITICAL_BLOCK(cs_mapTransactions)
    {
        for(const CTxIn & txin: vin)
            mapNextTx.erase(txin.prevout);
        mapTransactions.erase(GetHash());
        nTransactionsUpdated++;
    }
    return true;
}

bool AddToWalletIfMine(const CTransaction& tx, const CBlock* pblock)
{
    //if (tx.IsMine() || mapWallet.count(tx.GetHash()))
    //{
    //    CWalletTx wtx(tx);
    //    // Get merkle branch if transaction was found in a block
    //    if (pblock)
    //        wtx.SetMerkleBranch(pblock);
    //    return AddToWallet(wtx);
    //}
    return true;
}

bool EraseFromWallet(uint256 hash)
{
    CRITICAL_BLOCK(cs_mapWallet)
    {
        if (mapWallet.erase(hash))
            CWalletDB().EraseTx(hash);
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////////
//
// mapOrphanTransactions
//

void AddOrphanTx(const CDataStream& vMsg)
{
    CTransaction tx;
    CDataStream(vMsg) >> tx;
    uint256 hash = tx.GetHash();
    if (mapOrphanTransactions.count(hash))
        return;
    CDataStream* pvMsg = mapOrphanTransactions[hash] = new CDataStream(vMsg);
    for(const CTxIn & txin: tx.vin)
        mapOrphanTransactionsByPrev.insert(std::make_pair(txin.prevout.hash, pvMsg));
}

void EraseOrphanTx(uint256 hash)
{
    if (!mapOrphanTransactions.count(hash))
        return;
    const CDataStream* pvMsg = mapOrphanTransactions[hash];
    CTransaction tx;
    CDataStream(*pvMsg) >> tx;
    for(const CTxIn & txin: tx.vin)
    {
        for (std::multimap<uint256, CDataStream*>::iterator mi = mapOrphanTransactionsByPrev.lower_bound(txin.prevout.hash);
            mi != mapOrphanTransactionsByPrev.upper_bound(txin.prevout.hash);)
        {
            if ((*mi).second == pvMsg)
                mapOrphanTransactionsByPrev.erase(mi++);
            else
                mi++;
        }
    }
    delete pvMsg;
    mapOrphanTransactions.erase(hash);
}

bool ProcessMessages(CNode* pfrom)
{
    CDataStream& vRecv = pfrom->vRecv;
    if (vRecv.empty())
        return true;
    printf("ProcessMessages(%d bytes)\n", vRecv.size());

    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (x) data
    //

    loop
    {
        // Scan for message start
        CDataStream::iterator pstart = search(vRecv.begin(), vRecv.end(), BEGIN(pchMessageStart), END(pchMessageStart));
        if (vRecv.end() - pstart < sizeof(CMessageHeader))
        {
            if (vRecv.size() > sizeof(CMessageHeader))
            {
                printf("\n\nPROCESSMESSAGE MESSAGESTART NOT FOUND\n\n");
                vRecv.erase(vRecv.begin(), vRecv.end() - sizeof(CMessageHeader));
            }
            break;
        }
        if (pstart - vRecv.begin() > 0)
            printf("\n\nPROCESSMESSAGE SKIPPED %d BYTES\n\n", pstart - vRecv.begin());
        vRecv.erase(vRecv.begin(), pstart);

        // Read header
        CMessageHeader hdr;
        vRecv >> hdr;
        if (!hdr.IsValid())
        {
            printf("\n\nPROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n", hdr.GetCommand().c_str());
            continue;
        }
        string strCommand = hdr.GetCommand();

        // Message size
        unsigned int nMessageSize = hdr.nMessageSize;
        if (nMessageSize > vRecv.size())
        {
            // Rewind and wait for rest of message
            ///// need a mechanism to give up waiting for overlong message size error
            printf("MESSAGE-BREAK\n");
            vRecv.insert(vRecv.begin(), BEGIN(hdr), END(hdr));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            break;
        }

        // Copy message to its own buffer
        CDataStream vMsg(vRecv.begin(), vRecv.begin() + nMessageSize, vRecv.nType, vRecv.nVersion);
        vRecv.ignore(nMessageSize);

        // Process message
        bool fRet = false;
        try
        {
            CheckForShutdown(2);
            CRITICAL_BLOCK(cs_main)
                fRet = ProcessMessage(pfrom, strCommand, vMsg);
            CheckForShutdown(2);
        }
        CATCH_PRINT_EXCEPTION("ProcessMessage()")
        if (!fRet)
            printf("ProcessMessage(%s, %d bytes) from %s to %s FAILED\n", strCommand.c_str(), nMessageSize, pfrom->addr.ToString().c_str(), addrLocalHost.ToString().c_str());
    }

    vRecv.Compact();
    return true;
}

bool ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv)
{
    static std::map<unsigned int, vector<unsigned char> > mapReuseKey;
    printf("received: %-12s (%zu bytes)  ", strCommand.c_str(), vRecv.size());

    for (size_t i = 0; i < std::min(vRecv.size(), size_t(20)); ++i)
        printf("%02x ", vRecv[i] & 0xff);

    printf("\n");

    if (nDropMessagesTest > 0 && GetRand(nDropMessagesTest) == 0)
    {
        printf("dropmessages DROPPING RECV MESSAGE\n");
        return true;
    }
    if (strCommand == "version")
    {
    }

    else if (pfrom->nVersion == 0)
    {
        // Must have a version message before anything else
        return false;
    }

    else if (strCommand == "addr")
    {
    }


    else if (strCommand == "inv")
    {
    }

    else if (strCommand == "getdata")
    {
    }

    else if (strCommand == "getblocks")
    {
    }

    else if (strCommand == "tx")
    {
        vector<uint256> vWorkQueue;
        CDataStream vMsg(vRecv);
        CTransaction tx;
        vRecv >> tx; /// HERE

        CInv inv(MSG_TX, tx.GetHash());
        pfrom->AddInventoryKnown(inv);

        bool fMissingInputs = false;
        if (tx.AcceptTransaction(true, &fMissingInputs))
        {
            AddToWalletIfMine(tx, NULL);  // TODO
            RelayMessage(inv, vMsg);
            mapAlreadyAskedFor.erase(inv);
            vWorkQueue.push_back(inv.hash);

            // Recursively process any orphan transactions that depended on this one
            for (int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hashPrev = vWorkQueue[i];
                for (std::multimap<uint256, CDataStream*>::iterator mi = mapOrphanTransactionsByPrev.lower_bound(hashPrev); /// TODO mapOrphanTransactionsByPrev
                    mi != mapOrphanTransactionsByPrev.upper_bound(hashPrev);
                    ++mi)
                {
                    const CDataStream& vMsg = *((*mi).second);
                    CTransaction tx;
                    CDataStream(vMsg) >> tx;
                    CInv inv(MSG_TX, tx.GetHash());

                    if (tx.AcceptTransaction(true))
                    {
                        printf("   accepted orphan tx %s\n", inv.hash.ToString().substr(0, 6).c_str());
                        AddToWalletIfMine(tx, NULL);  // TODO
                        RelayMessage(inv, vMsg);
                        mapAlreadyAskedFor.erase(inv);
                        vWorkQueue.push_back(inv.hash);
                    }
                }

            }

            for(uint256 hash: vWorkQueue)
                EraseOrphanTx(hash);
        }
        else if (fMissingInputs)
        {
            printf("storing orphan tx %s\n", inv.hash.ToString().substr(0, 6).c_str());
            AddOrphanTx(vMsg);
        }
    }

    else if (strCommand == "review")
    {
    }

    else if (strCommand == "block")
    {
    }

    else if (strCommand == "getaddr")
    {
    }

    else if (strCommand == "checkorder")
    {
    }

    else if (strCommand == "submitorder")
    {
    }

    else if (strCommand == "reply")
    {
    }

    else
    {
        // Ignore unknown commands for extensibility
        printf("ProcessMessage(%s) : Ignored unknown message\n", strCommand.c_str());
    }

    if (!vRecv.empty())
        printf("ProcessMessage(%s) : %d extra bytes\n", strCommand.c_str(), vRecv.size());

    return true;

}



//bool SendMessages(CNode* pto)
//{
//    CheckForShutdown(2);
//    CRITICAL_BLOCK(cs_main)
//    {
//        // Don't send anything until we get their version message
//        if (pto->nVersion == 0)
//            return true;
//
//
//        //
//        // Message: addr
//        //
//        vector<CAddress> vAddrToSend;
//        vAddrToSend.reserve(pto->vAddrToSend.size());
//        for(const CAddress & addr: pto->vAddrToSend)
//            if (!pto->setAddrKnown.count(addr))
//                vAddrToSend.push_back(addr);
//        pto->vAddrToSend.clear();
//        if (!vAddrToSend.empty())
//            pto->PushMessage("addr", vAddrToSend);
//
//
//        //
//        // Message: inventory
//        //
//        vector<CInv> vInventoryToSend;
//        CRITICAL_BLOCK(pto->cs_inventory)
//        {
//            vInventoryToSend.reserve(pto->vInventoryToSend.size());
//            foreach(const CInv & inv, pto->vInventoryToSend)
//            {
//                // returns true if wasn't already contained in the set
//                if (pto->setInventoryKnown.insert(inv).second)
//                    vInventoryToSend.push_back(inv);
//            }
//            pto->vInventoryToSend.clear();
//            pto->setInventoryKnown2.clear();
//        }
//        if (!vInventoryToSend.empty())
//            pto->PushMessage("inv", vInventoryToSend);
//
//
//        //
//        // Message: getdata
//        //
//        vector<CInv> vAskFor;
//        int64 nNow = GetTime() * 1000000;
//        CTxDB txdb("r");
//        while (!pto->mapAskFor.empty() && (*pto->mapAskFor.begin()).first <= nNow)
//        {
//            const CInv& inv = (*pto->mapAskFor.begin()).second;
//            printf("sending getdata: %s\n", inv.ToString().c_str());
//            if (!AlreadyHave(txdb, inv))
//                vAskFor.push_back(inv);
//            pto->mapAskFor.erase(pto->mapAskFor.begin());
//        }
//        if (!vAskFor.empty())
//            pto->PushMessage("getdata", vAskFor);
//
//    }
//    return true;
//}






//
// CTransaction
//
bool CTransaction::AcceptTransaction(CTxDB& txdb, bool fCheckInputs, bool* pfMissingInputs)
{
    if (pfMissingInputs)
    {
        *pfMissingInputs = false;
    }

    // Coinbase is only valid in a block, not as a loose transaction
    if (IsCoinBase())
        return error("AcceptTransaction() : coinbase as individual tx");

    if (!CheckTransaction())
        return error("AcceptTransaction() : CheckTransaction failed");

    // Do we already have it?
    uint256 hash = GetHash();
    CRITICAL_BLOCK(cs_mapTransactions)
        if (mapTransactions.count(hash))
            return false;
    if (fCheckInputs)
        if (txdb.ContainsTx(hash))
            return false;

    // Check for conflicts with in-memory transactions
    CTransaction* ptxOld = NULL;
    for (int i = 0; i < vin.size(); i++)
    {
        COutPoint outpoint = vin[i].prevout;
        if (mapNextTx.count(outpoint))
        {
            // Allow replacing with a newer version of the same transaction
            if (i != 0)
                return false;
            ptxOld = mapNextTx[outpoint].ptx;
            if (!IsNewerThan(*ptxOld))
                return false;
            for (int i = 0; i < vin.size(); i++)
            {
                COutPoint outpoint = vin[i].prevout;
                if (!mapNextTx.count(outpoint) || mapNextTx[outpoint].ptx != ptxOld)
                    return false;
            }
            break;
        }
    }

    // Check against previous transactions
    std::map<uint256, CTxIndex> mapUnused;
    int64 nFees = 0;
    if (fCheckInputs && !ConnectInputs(txdb, mapUnused, CDiskTxPos(1, 1, 1), 0, nFees, false, false, 0))
    {
        if (pfMissingInputs)
            *pfMissingInputs = true;
        return error("AcceptTransaction() : ConnectInputs failed %s", hash.ToString().substr(0, 6).c_str());
    }

    // Store transaction in memory
    CRITICAL_BLOCK(cs_mapTransactions)
    {
        if (ptxOld)
        {
            printf("mapTransaction.erase(%s) replacing with new version\n", ptxOld->GetHash().ToString().c_str());
            mapTransactions.erase(ptxOld->GetHash());
        }
        AddToMemoryPool();
    }


    ///// are we sure this is ok when loading transactions or restoring block txes
    // If updated, erase old tx from wallet
    if (ptxOld)
        EraseFromWallet(ptxOld->GetHash());

    printf("AcceptTransaction(): accepted %s\n", hash.ToString().substr(0, 6).c_str());
    return true;
}

bool CTransaction::AcceptTransaction(bool fCheckInputs, bool* pfMissingInputs)
{
    CTxDB txdb("r");
    return AcceptTransaction(txdb, fCheckInputs, pfMissingInputs);
    //return true;
}




bool CTransaction::ClientConnectInputs()
{
    std::runtime_error("Not Implemented");
    return false;
}


bool CTransaction::ConnectInputs(CTxDB& txdb, std::map<uint256, CTxIndex>& mapTestPool, CDiskTxPos posThisTx, int nHeight, int64& nFees, bool fBlock, bool fMiner, int64 nMinFee)
{
    // Take over previous transactions' spent pointers
    if (!IsCoinBase())
    {
        int64 nValueIn = 0;
        for (int i = 0; i < vin.size(); i++)
        {
            COutPoint prevout = vin[i].prevout;

            // Read txindex
            CTxIndex txindex;
            bool fFound = true;
            if (fMiner && mapTestPool.count(prevout.hash))
            {
                // Get txindex from current proposed changes
                txindex = mapTestPool[prevout.hash];
            }
            else
            {
                // Read txindex from txdb
                fFound = txdb.ReadTxIndex(prevout.hash, txindex);
            }
            if (!fFound && (fBlock || fMiner))
                return fMiner ? false : error("ConnectInputs() : %s prev tx %s index entry not found", GetHash().ToString().substr(0, 6).c_str(), prevout.hash.ToString().substr(0, 6).c_str());

            // Read txPrev
            CTransaction txPrev;
            if (!fFound || txindex.pos == CDiskTxPos(1, 1, 1))
            {
                // Get prev tx from single transactions in memory
                CRITICAL_BLOCK(cs_mapTransactions)
                {
                    if (!mapTransactions.count(prevout.hash))
                        return error("ConnectInputs() : %s mapTransactions prev not found %s", GetHash().ToString().substr(0, 6).c_str(), prevout.hash.ToString().substr(0, 6).c_str());
                    txPrev = mapTransactions[prevout.hash];
                }
                if (!fFound)
                    txindex.vSpent.resize(txPrev.vout.size());
            }
            else
            {
                // Get prev tx from disk
                if (!txPrev.ReadFromDisk(txindex.pos))
                    return error("ConnectInputs() : %s ReadFromDisk prev tx %s failed", GetHash().ToString().substr(0, 6).c_str(), prevout.hash.ToString().substr(0, 6).c_str());
            }

            if (prevout.n >= txPrev.vout.size() || prevout.n >= txindex.vSpent.size())
                return error("ConnectInputs() : %s prevout.n out of range %d %d %d", GetHash().ToString().substr(0, 6).c_str(), prevout.n, txPrev.vout.size(), txindex.vSpent.size());

            // If prev is coinbase, check that it's matured
            if (txPrev.IsCoinBase())
                for (CBlockIndex* pindex = pindexBest; pindex && nBestHeight - pindex->nHeight < COINBASE_MATURITY - 1; pindex = pindex->pprev)
                    if (pindex->nBlockPos == txindex.pos.nBlockPos && pindex->nFile == txindex.pos.nFile)
                        return error("ConnectInputs() : tried to spend coinbase at depth %d", nBestHeight - pindex->nHeight);

            // Verify signature
            //if (!VerifySignature(txPrev, *this, i))
            //    return error("ConnectInputs() : %s VerifySignature failed", GetHash().ToString().substr(0, 6).c_str());

            // Check for conflicts
            if (!txindex.vSpent[prevout.n].IsNull())
                return fMiner ? false : error("ConnectInputs() : %s prev tx already used at %s", GetHash().ToString().substr(0, 6).c_str(), txindex.vSpent[prevout.n].ToString().c_str());

            // Mark outpoints as spent
            txindex.vSpent[prevout.n] = posThisTx;

            // Write back
            if (fBlock)
                txdb.UpdateTxIndex(prevout.hash, txindex);
            else if (fMiner)
                mapTestPool[prevout.hash] = txindex;

            nValueIn += txPrev.vout[prevout.n].nValue;
        }

        // Tally transaction fees
        int64 nTxFee = nValueIn - GetValueOut();
        if (nTxFee < 0)
            return error("ConnectInputs() : %s nTxFee < 0", GetHash().ToString().substr(0, 6).c_str());
        if (nTxFee < nMinFee)
            return false;
        nFees += nTxFee;
    }

    if (fBlock)
    {
        // Add transaction to disk index
        if (!txdb.AddTxIndex(*this, posThisTx, nHeight))
            return error("ConnectInputs() : AddTxPos failed");
    }
    else if (fMiner)
    {
        // Add transaction to test pool
        mapTestPool[GetHash()] = CTxIndex(CDiskTxPos(1, 1, 1), vout.size());
    }

    return true;
}
