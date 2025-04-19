#include "globals.h"
#include <sys/stat.h>
#include "shared/key.h"


// Settings
int fGenerateBasecoins = 1; // hehe
int64 nTransactionFee = 0;
CAddress addrIncoming;


unsigned int nTransactionsUpdated = 0;
bool fDbEnvInit = false;
std::map<uint256, CTransaction> mapTransactions;

const unsigned int MAX_SIZE = 0x02000000;
const int64 COIN = 100000000;
const int64 CENT = 1000000;
const int COINBASE_MATURITY = 100;

CBlockIndex* pindexBest = NULL;
CBlockIndex* pindexGenesisBlock = NULL;
uint256 hashBestChain = 0;

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
