#include <iostream>


#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include "miner.h"
#include "../shared/key.h"
#include "../shared/bignum.h"
#include "../globals.h"

//#include "../shared/uint256.h"
//#include "../shared/bignum.h"
//#include "../shared/sha.h"


//using Globals::fGenerateBasecoins;
//using Globals::nTransactionsUpdated;

//////////////////////////////////////////////////////////////////////////////
//
// BasecoinMiner
//

int FormatHashBlocks(void* pbuffer, unsigned int len)
{
    unsigned char* pdata = (unsigned char*)pbuffer;
    unsigned int blocks = 1 + ((len + 8) / 64);
    unsigned char* pend = pdata + 64 * blocks;
    //memset(pdata + len, 0, 64 * blocks - len);
    pdata[len] = 0x80;
    unsigned int bits = len * 8;
    pend[-1] = (bits >> 0) & 0xff;
    pend[-2] = (bits >> 8) & 0xff;
    pend[-3] = (bits >> 16) & 0xff;
    pend[-4] = (bits >> 24) & 0xff;
    return blocks;
}



void CoinMiner() {

    printf("Hello Coin Miner 4.");
    struct sched_param param;
    param.sched_priority = sched_get_priority_min(SCHED_OTHER);
    pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
    nice(19);  // Further reduce priority

    CKey key;
    key.MakeNewKey();
    CBigNum bnExtraNonce = 0;

    while (fGenerateBasecoins)
    {
        //    //Sleep(50);  //TODO: net.h/cpp 
        //    //CheckForShutdown(3);
        //    //while (vNodes.empty())
        //    //{
        //    //    Sleep(1000);
        //    //    CheckForShutdown(3);
        //    //}

        unsigned int nTransactionsUpdatedLast = nTransactionsUpdated;
        CBlockIndex* pindexPrev = pindexBest;
        unsigned int nBits = GetNextWorkRequired(pindexPrev);
        
        //
        // Create coinbase tx
        //
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig << nBits << ++bnExtraNonce;
        txNew.vout.resize(1);
        txNew.vout[0].scriptPubKey << key.GetPubKey() << OP_CHECKSIG;


        //
        // Create new block
        //
        std::unique_ptr
        
    
    }
    

}