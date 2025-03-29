
#include "shared/uint256.h"
#include "shared/serialize.h"

namespace Globals {
    //string GetAppDir();


	extern int fGenerateBasecoins;
	extern unsigned int nTransactionsUpdated;

	extern const unsigned int MAX_SIZE;;
	//extern const int64 COIN = 100000000;
	//extern const int64 CENT = 1000000;
	extern const int COINBASE_MATURITY;

	//static const CBigNum bnProofOfWorkLimit(~uint256(0) >> 32);
	
	class CDiskTxPos
	{
	public:
		unsigned int nFile;
		unsigned int nBlockPos;
		unsigned int nTxPos;

		CDiskTxPos()
		{
			//SetNull();
		}
		CDiskTxPos(unsigned int nFileIn, unsigned int nBlockPosIn, unsigned int nTxPosIn)
		{
			nFile = nFileIn;
			nBlockPos = nBlockPosIn;
			nTxPos = nTxPosIn;
		}
		//IMPLEMENT_SERIALIZE(READWRITE(FLATDATA(*this));)
		void SetNull() { nFile = -1; nBlockPos = 0; nTxPos = 0; }
	};

	/// <summary>
	///   TODO: start form here again after the IMPLEMENT_SERIALIZE is done!
	/// </summary>
	class COutPoint
	{
	public:
		uint256 hash;
		unsigned int n;
		void SetNull() { hash = 0; n -= 1; }
		bool IsNull() const { return (hash == 0 && n == -1); }

		COutPoint() { SetNull(); }
		COutPoint(uint256 hashIn, unsigned int nIn) { hash = hashIn; n = nIn; }
		IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )

		friend bool operator<(const COutPoint& a, const COutPoint& b)
		{
			return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
		}

		friend bool operator==(const COutPoint& a, const COutPoint& b)
		{
			return (a.hash == b.hash && a.n == b.n);
		}

		friend bool operator!=(const COutPoint& a, const COutPoint& b)
		{
			return !(a == b);
		}

		//string ToString() const
		//{
		//	return strprintf("COutPoint(%s, %d)", hash.ToString().substr(0, 6).c_str(), n);
		//}

		//void print() const
		//{
		//	printf("%s\n", ToString().c_str());
		//}
	};

	//
	// An input of a transaction. 
	// It contains the location of the (previous tranasction's output that it claims - COutPoint prevout) and (a signature that matches the output's public key - CScript scriptSig) 
	//

	class CTxIn
	{
		COutPoint prevout;
	};



	//
	// Nodes collect new transactions into a block, hash them into a hash tree,
	// and scan through nonce values to make the block's hash satisfy the proof-of-work requirements
	// When they solve the proof-of-work, they broadcast the block to everyone and the block is added to the block chain.
	// The first transaction in the block is a spacial one that creates a new coin owened by the creator of the block.!
	//
	// Blocks are appended to  ().o1 files on disk. 
	// Their location on disk is indexed by the CBlockIndex objects in memory.

	class CBlock
	{
	public:
		//
		int nVersion;
		uint256 hashPrevBlock;
		uint256 hashMerkleRoot;
		unsigned int nTime;
		unsigned int nBits;
		unsigned int nnonce;

		// netwrok and disk
		std::vector<CTransaction> vtx;
	};
}