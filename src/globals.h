
#include "shared/uint256.h"
#include "shared/serialize.h"
#include "shared/utils.h"
#include "script/script.h"

//namespace Globals {
    //string GetAppDir();


	extern int fGenerateBasecoins;
	extern unsigned int nTransactionsUpdated;

	extern const unsigned int MAX_SIZE;;
	extern const int64 COIN;
	extern const int64 CENT;
	extern const int COINBASE_MATURITY;


	extern int nBestHeight;


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

		string ToString() const
		{
			return strprintf("COutPoint(%s, %d)", hash.ToString().substr(0, 6).c_str(), n);
		}

		void print() const
		{
			printf("%s\n", ToString().c_str());
		}
	};

	//
	// An input of a transaction. 
	// It contains the location of the (previous tranasction's output that it claims - COutPoint prevout) and (a signature that matches the output's public key - CScript scriptSig) 
	//

	class CTxIn
	{
	public:
		COutPoint prevout;
		CScript scriptSig;
		unsigned int nSequence;


		IMPLEMENT_SERIALIZE(
			READWRITE(prevout);
			READWRITE(scriptSig);
			READWRITE(nSequence);)

		CTxIn()
		{
			nSequence = UINT_MAX;
		}
		
		explicit CTxIn(COutPoint preoutIn, CScript scriptSigIn = CScript(), unsigned int nSequenceIn = UINT_MAX)
		{
			prevout = preoutIn;
			scriptSig = scriptSigIn;
			nSequence = nSequenceIn;
		}

		CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn = CScript(), unsigned int nSequenceIn = UINT_MAX)
		{
			prevout = COutPoint(hashPrevTx, nOut);
			scriptSig = scriptSigIn;
			nSequence = nSequenceIn;
		}

		bool IsFinal() const
		{
			return (nSequence == UINT_MAX);
		}


		friend bool operator==(const CTxIn& a, const CTxIn& b)
		{
			return (a.prevout == b.prevout &&
				a.scriptSig == b.scriptSig &&
				a.nSequence == b.nSequence);
		}


		friend bool operator!=(const CTxIn& a, const CTxIn& b)
		{
			return !(a == b);
		}

		std::string ToString() const
		{
			std::string str;
			str += strprintf("CTxIn(");
			str += prevout.ToString();
			if (prevout.IsNull())
				str += strprintf(", coinbase %s", HexStr(scriptSig.begin(), scriptSig.end(), false).c_str());
			else
				str += strprintf(", scriptSig=%s", scriptSig.ToString().substr(0, 24).c_str());
			if (nSequence != UINT_MAX)
				str += strprintf(", nSequence=%u", nSequence);
			str += ")";
			return str;
		}
		void print() const
		{
			printf("%s\n", ToString().c_str());
		}
		
		///

		bool IsMine() const;
		int64 GetDebit() const;

	};

	//
	// An output of a transaction.
	// IT contains the public key that the next input must be able to sign with in order to claim it.
	//

	class CTxOut
	{
	public:
		int64 nValue;
		CScript scriptPubKey;
		
		IMPLEMENT_SERIALIZE
		(
			READWRITE(nValue);
			READWRITE(scriptPubKey);
			)


		CTxOut()
		{
			SetNull();
		}

		CTxOut(int64 nValueIn, CScript scriptPubKeyIn)
		{
			nValue = nValueIn;
			scriptPubKey = scriptPubKeyIn;
		}

		void SetNull()
		{
			nValue = -1;
			scriptPubKey.clear();
		}

		bool IsNull()
		{
			return (nValue == -1);
		}

		uint256 GetHash() const
		{
			return SerializeHash(*this);
		}


		bool IsMine() const
		{
			//return ::IsMine(scriptPubKey); //TODO: scriptfunctions.cpp
			return true;
		}

		int64 GetCredit() const
		{
			if (IsMine())
				return nValue;
			return 0;
		}

		friend bool operator==(const CTxOut& a, const CTxOut& b)
		{
			return (a.nValue == b.nValue &&
				a.scriptPubKey == b.scriptPubKey);
		}

		friend bool operator!=(const CTxOut& a, const CTxOut& b)
		{
			return !(a == b);
		}

		/// <summary>
		/// PRINTING
		/// </summary>
		/// <returns></returns>
		std::string ToString() const
		{
			if (scriptPubKey.size() < 6)
				return "CTxOut(error)";
			return strprintf("CTxOut(nValue=%I64d.%08I64d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, scriptPubKey.ToString().substr(0, 24).c_str());
		}

		void print() const
		{
			printf("%s\n", ToString().c_str());
		}
	};


	//
	// The basic transaction that is broadcasted on the network and contained in
	// blocks.  A transaction can contain multiple inputs and outputs.
	//

	class CTransaction
	{
	public:
		int nVersion;
		std::vector<CTxIn> vin;
		std::vector<CTxOut> vout;
		int nLockTime;

		IMPLEMENT_SERIALIZE
		(
			READWRITE(this->nVersion);
			nVersion = this->nVersion;
			READWRITE(vin);
			READWRITE(vout);
			READWRITE(nLockTime);
		)


		CTransaction()
		{
			SetNull();
		}

		void SetNull()
		{
			nVersion = 1;
			vin.clear();
			vout.clear();
			nLockTime = 0;
		}

		bool IsNull() const
		{
			return vin.empty() && vout.empty();
		}

		uint256 GetHash() const
		{
			return SerializeHash(*this);
		}

		bool IsFinal() const
		{
			if (nLockTime == 0 || nLockTime < nBestHeight)
			{
				return true;
			}
			for (const CTxIn& txin : vin)
			{
				if (!txin.IsFinal())
				{
					return false;
				}
			}
			return true;
		}

		bool IsNewerThan(const CTransaction& old) const
		{
			if (vin.size() != old.vin.size())
			{
				return false;
			}
			for (int i = 0; i < vin.size(); i++)
			{
				if (vin[i].prevout != old.vin[i].prevout)
				{
					return false;
				}
			}

			bool fNewer = false;
			unsigned int nLowest = UINT_MAX;
			for (int i = 0; i < vin.size(); i++)
			{
				if (vin[i].nSequence != old.vin[i].nSequence)
				{
					if (vin[i].nSequence <= nLowest)
					{
						fNewer = false;
						nLowest = vin[i].nSequence;
					}
					if (old.vin[i].nSequence < nLowest)
					{
						fNewer = true;
						nLowest = old.vin[i].nSequence;
					}
				}
			}
			return fNewer;
		}

		bool IsCoinBase() const
		{
			return (vin.size() == 1 && vin[0].prevout.IsNull());
		}

		bool CheckTransaction() const
		{
			// Basic checks that don't depend on any context
			if (vin.empty() || vout.empty())
			{
				return error("CTransaction::CheckTransaction() : vin or vout empty");
			}
			
			// Check for negative values
			for (const CTxOut& txout: vout)
			{
				if (txout.nValue < 0)
				{
					return error("CTransaction::CheckTransaction() : txout.nValue negative");
				}
			}

			if (IsCoinBase())
			{
				if (vin[0].scriptSig.size() < 2 || vin[0].scriptSig.size() > 100)
				{
					return error("CTransaction::CheckTransaction() : coinbase script size");
				}
			}
			else
			{
				for (const CTxIn& txin : vin)
				{
					if (txin.prevout.IsNull())
					{
						return error("CTransaction::CheckTransaction() : prevout is null");
					}
				}
			}
			return true;
		}

		bool IsMine() const
		{
			for (const CTxOut& txout : vout)
			{
				if (txout.IsMine())
				{
					return true;
				}
			}
			return false;
		}

		int64 GetDebit() const
		{
			int64 nDebit = 0;
			for (const CTxIn& txin : vin)
			{
				nDebit += txin.GetDebit();
			}
			return nDebit;
		}

		int64 GetCredit() const
		{
			int64 nCredit = 0;
			for (const CTxOut& txout : vout)
			{
				nCredit += txout.GetCredit();
			}
			return nCredit;
		}

		int64 GetValueOut() const
		{
			int64 nValueOut = 0;
			for (const CTxOut& txout : vout)
			{
				if (txout.nValue > 0)
				{
					throw std::runtime_error("CTransaction::GetValueOut() : negative value");
				}
				nValueOut += txout.nValue;
				nValueOut += txout.nValue;
			}
			return nValueOut;
		}

		// TODO: experiment with 10% fee
		// try to use Value out * 0.1 (txout.nValue) * 0.1
		// this after actual debugging
		int64 GetMinFee(bool fDiscount = false) const
		{
			// Base fee 
			// right now it is 1 cent per kilobyte
			unsigned int nBytes = ::GetSerializeSize(*this, SER_NETWORK);
			int64 nMinFee = (1 + (int64)nBytes / 1000) * CENT;

			// First 100 transaction in a block are free]
			if (fDiscount && nBytes < 10000)
			{
				nMinFee = 0;
			}

			// To limit dust spam, require a 0.01 fee if any output is less than 
			if (nMinFee < CENT)
			{
				for (const CTxOut& txout : vout)
				{
					if (txout.nValue < CENT)
					{
						nMinFee = CENT;
					}
				}
			}
			return nMinFee;
		}

		//bool ReadFromDisk(CDiskTxPos pos, FILE** pfileRet = NULL)
		//{
		//	CAutoFile filein = 1; // TODO ING: CAUTOFILE in serialize
		//}

		friend bool operator==(const CTransaction& a, const CTransaction& b)
		{
			return (a.nVersion == b.nVersion &&
				a.vin == b.vin &&
				a.vout == b.vout &&
				a.nLockTime == b.nLockTime);
		}

		friend bool operator!=(const CTransaction& a, const CTransaction& b)
		{
			return !(a == b);
		}


		std::string ToString() const
		{
			std::string str;
			str += strprintf("CTransaction(hash=%s, ver=%d, vin.size=%d, vout.size=%d, nLockTime=%d)\n",
				GetHash().ToString().substr(0, 6).c_str(),
				nVersion,
				vin.size(),
				vout.size(),
				nLockTime);
			for (int i = 0; i < vin.size(); i++)
				str += "    " + vin[i].ToString() + "\n";
			for (int i = 0; i < vout.size(); i++)
				str += "    " + vout[i].ToString() + "\n";
			return str;
		}

		void print() const
		{
			printf("%s", ToString().c_str());
		}
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
		unsigned int nNonce;

		// netwrok and disk
		std::vector<CTransaction> vtx;

		// memory only
		mutable std::vector<uint256> vMerkleTree;

		IMPLEMENT_SERIALIZE
		(
			READWRITE(this->nVersion);
			nVersion = this->nVersion;
			READWRITE(hashPrevBlock);
			READWRITE(hashMerkleRoot);
			READWRITE(nTime);
			READWRITE(nBits);
			READWRITE(nNonce);

			// ConnectBlock depends on vtx being last so it can calculate offset
			if (!(nType & (SER_GETHASH | SER_BLOCKHEADERONLY)))
				READWRITE(vtx);
			else if (fRead)
				const_cast<CBlock*>(this)->vtx.clear();
			)

		CBlock()
		{
			SetNull();
		}

		void SetNull()
		{
			nVersion = 1;
			hashPrevBlock = 0;
			hashMerkleRoot = 0;
			nTime = 0;
			nBits = 0;
			nNonce = 0;
			vtx.clear();
			vMerkleTree.clear();
		}

		bool IsNull() const
		{
			return (nBits == 0);
		}

		uint256 GetHash() const
		{
			return Hash(BEGIN(nVersion), END(nNonce));
		}

		uint256 BuildMerkleTree() const
		{
			vMerkleTree.clear();
			for (const CTransaction& tx : vtx)
			{
				vMerkleTree.push_back(tx.GetHash());
			}
			int j = 0;
			for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
			{
				for (int i = 0; i < nSize; i += 2)
				{
					int i2 = std::min(i + 1, nSize - 1);
					vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j + i]), END(vMerkleTree[j + i]),
						BEGIN(vMerkleTree[j + i2]), END(vMerkleTree[j + i2])));
				}
				j += nSize;
			}
			return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
		}

		std::vector<uint256> GetMerkleBranch(int nIndex) const
		{
			if (vMerkleTree.empty())
				BuildMerkleTree();
			std::vector<uint256> vMerkleBranch;
			int j = 0;
			for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
			{
				int i = std::min(nIndex ^ 1, nSize - 1);
				vMerkleBranch.push_back(vMerkleTree[j + i]);
				nIndex >>= 1;
				j += nSize;
			}
			return vMerkleBranch;
		}

		/// <summary>
		///  Printing
		/// </summary>
		void print() const
		{
			printf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%d)\n",
				GetHash().ToString().substr(0, 14).c_str(),
				nVersion,
				hashPrevBlock.ToString().substr(0, 14).c_str(),
				hashMerkleRoot.ToString().substr(0, 6).c_str(),
				nTime, nBits, nNonce,
				vtx.size());
			for (int i = 0; i < vtx.size(); i++)
			{
				printf("  ");
				vtx[i].print();
			}
			printf("  vMerkleTree: ");
			for (int i = 0; i < vMerkleTree.size(); i++)
				printf("%s ", vMerkleTree[i].ToString().substr(0, 6).c_str());
			printf("\n");
		}
		



	};
//}