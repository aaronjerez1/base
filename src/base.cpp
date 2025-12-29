// base.cpp : Defines the entry point for the application.
//
//#include <openssl/bn.h>

#include "base.h"
#include "miner/miner.h"
#include "database/addressdb/addressdb.h"
#include "database/walletdb/walletdb.h"

#include "globals.h"

using namespace std;

///
/// Generate genesis block tool
/// 
/// Notes:
/// A: Genesis wallet
/// B: Genesis Block
/// C: Hard code genesis block to in-database blocks and gensis block. 
/// 
vector<unsigned char> genesiswallet()
{
	vector<unsigned char> pubkey = GenerateNewKey();
	return pubkey;
}

void genesisblock(vector<unsigned char> pubkey)
{
	// Genesis block
	char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
	CTransaction txNew;
	txNew.vin.resize(1);
	txNew.vout.resize(1);
	txNew.vin[0].scriptSig = CScript() << 486604799 << CBigNum(4) << std::vector<unsigned char>((unsigned char*)pszTimestamp, (unsigned char*)pszTimestamp + strlen(pszTimestamp));
	txNew.vout[0].nValue = 50 * COIN;
	txNew.vout[0].scriptPubKey = CScript(pubkey);
	
	CBlock block;

	block.vtx.push_back(txNew);

	block.hashPrevBlock = 0;
	block.hashMerkleRoot = block.BuildMerkleTree();
	block.nVersion = 1;

	block.nTime = 1231006505; // here you need to change the time.
	// here you need to mine it to get thos enumbers
	block.nBits = 0x1d00ffff; //diffulty set
	block.nNonce = 2083236893; // nonce found

	//// debug print, delete this later
	printf("%s\n", block.GetHash().ToString().c_str());
	printf("%s\n", block.hashMerkleRoot.ToString().c_str());
	printf("%s\n", hashGenesisBlock.ToString().c_str());
	txNew.vout[0].scriptPubKey.print();
	block.print();
	assert(block.hashMerkleRoot == uint256("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

	assert(block.GetHash() == hashGenesisBlock);
}

bool LoadBlockIndex(bool fAllowNew = true)
{
	//
	// Load block index
	//
		CTxDB txdb("cr");
		if (!txdb.LoadBlockIndex())
			return false;
		txdb.Close();

	//
	// Init with genesis block
	//
	if (mapBlockIndex.empty()) // from globals.h
	{
		// Genesis block
		char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
		CTransaction txNew;
		txNew.vin.resize(1);
		txNew.vout.resize(1);
		txNew.vin[0].scriptSig = CScript() << 486604799 << CBigNum(4) << std::vector<unsigned char>((unsigned char*)pszTimestamp, (unsigned char*)pszTimestamp + strlen(pszTimestamp));
		txNew.vout[0].nValue = 50 * COIN;
		txNew.vout[0].scriptPubKey = CScript() << CBigNum("0x5F1DF16B2B704C8A578D0BBAF74D385CDE12C11EE50455F3C438EF4C3FBCF649B6DE611FEAE06279A60939E028A8D65C10B73071A6F16719274855FEB0FD8A6704") << OP_CHECKSIG;
		
		CBlock block;
		
		block.vtx.push_back(txNew);

		block.hashPrevBlock = 0;
		block.hashMerkleRoot = block.BuildMerkleTree();
		block.nVersion = 1;
		block.nTime = 1231006505;
		block.nBits = 0x1d00ffff;
		block.nNonce = 2083236893;

		//// debug print, delete this later
		printf("%s\n", block.GetHash().ToString().c_str());
		printf("%s\n", block.hashMerkleRoot.ToString().c_str());
		printf("%s\n", hashGenesisBlock.ToString().c_str());
		txNew.vout[0].scriptPubKey.print();
		block.print();
		//assert(block.hashMerkleRoot == uint256("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

		//assert(block.GetHash() == hashGenesisBlock);

		// Start new block file
		unsigned int nFile;
		unsigned int nBlockPos;
		if (!block.WriteToDisk(!fClient, nFile, nBlockPos))
			return error("LoadBlockIndex() : writing genesis block to disk failed");
		if (!block.AddToBlockIndex(nFile, nBlockPos))
			return error("LoadBlockIndex() : genesis block not accepted");
	}

	return true;
}



int main()
{
	// data dir?
	// addrProxy
	// 
	// 
	genesiswallet();
	// Load addresses, block index, and wallet.
	string strErrors;
	struct timespec tStart, tEnd;
	double elapsed;
	// Load addresses
	printf("Loading addresses...\n");
	clock_gettime(CLOCK_MONOTONIC, &tStart);
	if (!LoadAddresses())
		strErrors += "Error loading addr.base      \n";
	clock_gettime(CLOCK_MONOTONIC, &tEnd);
	elapsed = (tEnd.tv_sec - tStart.tv_sec) * 1e9 + (tEnd.tv_nsec - tStart.tv_nsec);
	printf(" addresses   %20.0f\n", elapsed);

	// Load block index
	printf("Loading Block Index...\n");
	clock_gettime(CLOCK_MONOTONIC, &tStart);
	if (!LoadBlockIndex())
		strErrors += "Error loading blkindex.base      \n";
	clock_gettime(CLOCK_MONOTONIC, &tEnd);
	elapsed = (tEnd.tv_sec - tStart.tv_sec) * 1e9 + (tEnd.tv_nsec - tStart.tv_nsec);
	printf(" block index   %20.0f\n", elapsed);

	// Load wallet
	printf("Loading wallet...\n");
	clock_gettime(CLOCK_MONOTONIC, &tStart);
	//if (!LoadWallet())
	//	strErrors += "Error loading wallet.base      \n";
	clock_gettime(CLOCK_MONOTONIC, &tEnd);
	elapsed = (tEnd.tv_sec - tStart.tv_sec) * 1e9 + (tEnd.tv_nsec - tStart.tv_nsec);
	printf(" wallet   %20.0f\n", elapsed);


	printf("Done loading\n");
	//// debug print
		printf("mapBlockIndex.size() = %d\n", mapBlockIndex.size());
		printf("nBestHeight = %d\n", nBestHeight);
		printf("mapKeys.size() = %d\n", mapKeys.size());
		printf("mapPubKeys.size() = %d\n", mapPubKeys.size());
		printf("mapWallet.size() = %d\n", mapWallet.size());
		printf("mapAddressBook.size() = %d\n", mapAddressBook.size());

	// Add wallet transactions that aren't already in a block to mapTransactions
	ReacceptWalletTransactions();

	if (!CheckDiskSpace())
	{
		return 0;
	}

	//if (!StartNode(strErrors))
	//{
	//	return 0;
	//}


	CoinMiner();


	return 0;
}
