#include "globals.h"
#include <sys/stat.h>




int fGenerateBasecoins = 1; // hehe
unsigned int nTransactionsUpdated = 0;
const unsigned int MAX_SIZE = 0x02000000;
const int64 COIN = 100000000;
const int64 CENT = 1000000;
const int COINBASE_MATURITY = 100;
CBlockIndex* pindexBest = NULL;
uint256 myvalue(0);

const CBigNum bnProofOfWorkLimit(~uint256(0) >> 32);


int nBestHeight = -1;



CCriticalSection cs_main;
CCriticalSection cs_mapTransactions;

std::string strSetDataDir;


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