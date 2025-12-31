
#include "shared/uint256.h"
#include "shared/serialize.h"
#include "shared/utils.h"
#include "script/script.h"
#include "network/net.h"
#include "database/db.h"
#include <algorithm>
#include "shared/types.h"

class CBlockIndex;
class CTransaction;
class CWalletTx;
class CTxDB;
class CTxIndex;
class CDiskTxPos;
class CKey;

inline unsigned int GetSerializeSize(const CScript& v, int nType, int nVersion)
{
	return GetSerializeSize((const std::vector<unsigned char>&)v, nType, nVersion);
}

template<typename Stream>
void Serialize(Stream& os, const CScript& v, int nType, int nVersion)
{
	Serialize(os, (const std::vector<unsigned char>&)v, nType, nVersion);
}

template<typename Stream>
void Unserialize(Stream& is, CScript& v, int nType, int nVersion)
{
	Unserialize(is, (std::vector<unsigned char>&)v, nType, nVersion);
}

//string GetAppDir();

// Settings
extern int fGenerateBasecoins;
extern int64 nTransactionFee;
extern CAddress addrIncoming;


extern unsigned int nTransactionsUpdated;
extern bool fDbEnvInit;
extern std::map<uint256, CTransaction> mapTransactions;


extern const unsigned int MAX_SIZE;
extern const int64 COIN;
extern const int64 CENT;
extern const int COINBASE_MATURITY;

extern const CBigNum bnProofOfWorkLimit;
extern const uint256 hashGenesisBlock;
extern uint256 hashBestChain;

extern int nBestHeight;
extern CBlockIndex* pindexBest;

extern std::map<uint256, CBlockIndex*> mapBlockIndex;
extern CBlockIndex* pindexGenesisBlock;


extern CCriticalSection cs_main;
extern CCriticalSection cs_mapTransactions;
extern CCriticalSection cs_db;

extern CKey keyUser;
extern std::map<std::vector<unsigned char>, CPrivKey> mapKeys;
extern std::map<uint160, std::vector<unsigned char> > mapPubKeys;
extern CCriticalSection cs_mapKeys;

bool AddKey(const CKey& key);
std::vector<unsigned char> GenerateNewKey();



extern std::map<uint256, CWalletTx> mapWallet;
extern std::vector<std::pair<uint256, bool> > vWalletUpdated;
extern CCriticalSection cs_mapWallet;



extern std::map<std::string, int> mapFileUseCount;
extern std::map<string, string> mapAddressBook;




//////////////
///// METHODS
//////////////

FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode = "rb");
FILE* AppendBlockFile(unsigned int& nFileRet);
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast);
void ReacceptWalletTransactions();
bool CheckDiskSpace(int64 nAdditionalBytes = 0);

bool ProcessMessages(CNode* pfrom);
bool ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv);
	
class CDiskTxPos
{
public:
	unsigned int nFile;
	unsigned int nBlockPos;
	unsigned int nTxPos;
	IMPLEMENT_SERIALIZE(READWRITE(FLATDATA(*this));)

	CDiskTxPos()
	{
		SetNull();
	}
	CDiskTxPos(unsigned int nFileIn, unsigned int nBlockPosIn, unsigned int nTxPosIn)
	{
		nFile = nFileIn;
		nBlockPos = nBlockPosIn;
		nTxPos = nTxPosIn;
	}
	void SetNull() { nFile = -1; nBlockPos = 0; nTxPos = 0; }
	bool IsNull() const { return (nFile == -1); }


	friend bool operator==(const CDiskTxPos& a, const CDiskTxPos& b)
	{
		return (a.nFile == b.nFile &&
			a.nBlockPos == b.nBlockPos &&
			a.nTxPos == b.nTxPos);
	}

	friend bool operator!=(const CDiskTxPos& a, const CDiskTxPos& b)
	{
		return !(a == b);
	}

	string ToString() const
	{
		if (IsNull())
			return strprintf("null");
		else
			return strprintf("(nFile=%d, nBlockPos=%d, nTxPos=%d)", nFile, nBlockPos, nTxPos);
	}

	void print() const
	{
		printf("%s", ToString().c_str());
	}

};


class CInPoint
{
public:
	CTransaction* ptx;
	unsigned int n;

	CInPoint() { SetNull(); }
	CInPoint(CTransaction* ptxIn, unsigned int nIn) { ptx = ptxIn; n = nIn; }
	void SetNull() { ptx = NULL; n = -1; }
	bool IsNull() const { return (ptx == NULL && n == -1); }
};


/// <summary>
///   
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
	std::vector<CTxIn> vin; // here you should do a CToken Transaction and serializehash it, then debug debug and debug.
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

// Define Tasks
	// get a grokable data set of the trasactions
	//  a token setence
	// seems to not need mathematical signs
	// ab=(a+b), 11=2, 22=4
// Define the operation in this transaction
	std::string what = GetSequence(*this);

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

	std::string GetSequence(const CTransaction& tx) {
		std::string sequence = "TX:" + tx.GetHash().ToString() + "|";

		// Add inputs
		sequence += "INPUTS[";
		for (int i = 0; i < tx.vin.size(); i++) {
			if (i > 0) sequence += ";";
			sequence += tx.vin[i].prevout.hash.ToString() + ":" +
				std::to_string(tx.vin[i].prevout.n);
		}
		sequence += "]|";

		// Add outputs
		sequence += "OUTPUTS[";
		for (int i = 0; i < tx.vout.size(); i++) {
			if (i > 0) sequence += ";";
			sequence += std::to_string(tx.vout[i].nValue) + ":" +
				tx.vout[i].scriptPubKey.ToString();
		}
		sequence += "]";

		return sequence;
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

	bool ReadFromDisk(CDiskTxPos pos, FILE** pfileRet = NULL)
	{
		CAutoFile filein = OpenBlockFile(pos.nFile, 0, pfileRet ? "rb+" : "rb");
		if (!filein)
		{
			return error("CTransaction::ReadFromDisk() : OpenBlockFile failed");
		}

		// Read transaction
		if (fseek(filein, pos.nTxPos, SEEK_SET) != 0)
		{
			return error("CTransaction::ReadFromDisk() : fseek failed");
		}
		filein >> *this;

		// Return file pointer
		if (pfileRet)
		{
			if (fseek(filein, pos.nTxPos, SEEK_SET) != 0);
			{
				return error("CTransaction::ReadFromDisk() : second fseek failed");
			}
			*pfileRet = filein.release();
		}
		return true;
	}

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

	//bool DisconnectInputs(CTxDB& txdb);
	bool ConnectInputs(CTxDB& txdb, std::map<uint256, CTxIndex>& mapTestPool, CDiskTxPos posThisTx, int nHeight, int64& nFees, bool fBlock, bool fMiner, int64 nMinFee);
	bool ClientConnectInputs();

	bool AcceptTransaction(CTxDB& txdb, bool fCheckInputs = true, bool* pfMissingInputs = NULL);

	bool AcceptTransaction(bool fCheckInputs = true, bool* pfMissingInputs = NULL);

	protected:
		bool AddToMemoryPool();
	public:
		bool RemoveFromMemoryPool();
};


//
// A tranasaction with a merkle branch linking it to the block chain.
//

class CMerkleTx : public CTransaction
{
public:
	uint256 hashBlock;
	std::vector<uint256> vMerkleBranch;
	int nIndex;

	// memory only
	mutable bool fMerkleVerified;


	IMPLEMENT_SERIALIZE
	(
		nSerSize += SerReadWrite(s, *(CTransaction*)this, nType, nVersion, ser_action);
		nVersion = this->nVersion;
		READWRITE(hashBlock);
		READWRITE(vMerkleBranch);
		READWRITE(nIndex);
		)

	void Init()
	{
		hashBlock = 0;
		nIndex = -1;
		fMerkleVerified = false;
	}

	CMerkleTx()
	{
		Init();
	}

	CMerkleTx(const CTransaction txIn) : CTransaction(txIn)
	{
		Init();
	}

	int64 GetCredit() const
	{
		// Must wait until coinbase is safely deep enough in the chain before valuing it
		if (IsCoinBase() && GetBlocksToMaturity() > 0)
		{
			return 0;
		}
		return CTransaction::GetCredit();
	}

	//int SetMerkleBranch(const CBlock* pblock = NULL);
	int GetDepthInMainChain() const;
	bool IsInMainChain() const { return GetDepthInMainChain() > 0; }
	int GetBlocksToMaturity() const;

	bool AcceptTransaction(CTxDB& txdb, bool fCheckInputs = true);
	//bool AcceptTransaction() { CTxDB txdb("r"); return AcceptTransaction(txdb); }
};


//
// A transaction with a bunch of additional info that only the owner cares about.
// It inlcudes any unrecorded transaction needed to link it back
// to the block chain. TODO: INCOMPLETE DUE TO CIRCULAR DEPENDENCY
//
class CWalletTx : public CMerkleTx
{
public:
	std::vector<CMerkleTx> vtxPrev;
	std::map<std::string, std::string> mapValue;
	std::vector<std::pair<std::string, std::string>> vOrderForm;
	unsigned int fTimeReceivedIsTxTime;
	unsigned int nTimeReceived;
	char fFromMe;
	char fSpent;

	// memory only
	mutable unsigned int nTimeDisplayed;


	IMPLEMENT_SERIALIZE
	(
		nSerSize += SerReadWrite(s, *(CMerkleTx*)this, nType, nVersion, ser_action);
		nVersion = this->nVersion;
		READWRITE(vtxPrev);
		READWRITE(mapValue);
		READWRITE(vOrderForm);
		READWRITE(fTimeReceivedIsTxTime);
		READWRITE(nTimeReceived);
		READWRITE(fFromMe);
		READWRITE(fSpent);
	)

	void Init()
	{
		fTimeReceivedIsTxTime = false;
		nTimeReceived = 0;
		fFromMe = false;
		fSpent = false;
		nTimeDisplayed = 0;
	}

	CWalletTx()
	{
		Init();
	}

	CWalletTx(const CMerkleTx& txIn) : CMerkleTx(txIn)
	{
		Init();
	}

	CWalletTx(const CTransaction& txIn) : CMerkleTx(txIn)
	{
		Init();
	}

	///
	/// 
	/// 

	//bool WriteToDisk()
	//{
	//	return CWalletDB().WriteTx(GetHash(), *this);
	//} // TODO: walletdb?? but this file is used by the db...

	//int64 GetTxTime() const;
	//void AddSupportingTransactions(CTxDB& txdb);

	bool AcceptWalletTransaction(CTxDB& txdb, bool fCheckInputs = true);
	//bool AcceptWalletTransaction() { CTxDB txdb("r"); return AcceptWalletTransaction(txdb); }

	//void RelayWalletTransaction(CTxDB& txdb);
	//void RelayWalletTransaction() { CTxDB txdb("r"); RelayWalletTransaction(txdb); }

};

//
// A txdb record that cotains the disk location of a transaction and the
// locations of the transactions that spend its ouputs. 
// vSpent is really only used as a flag, but having the locaiton is very helpful for debugging.
//
class CTxIndex
{
public:
	CDiskTxPos pos;
	std::vector<CDiskTxPos> vSpent;

	IMPLEMENT_SERIALIZE
	(
		if (!(nType & SER_GETHASH))
			READWRITE(nVersion);
		READWRITE(pos);
		READWRITE(vSpent);
	)

	CTxIndex()
	{
		SetNull();
	}

	CTxIndex(const CDiskTxPos& posIn, unsigned int nOutputs)
	{
		pos = posIn;
		vSpent.resize(nOutputs);
	}

	void SetNull()
	{
		pos.SetNull();
		vSpent.clear();
	}

	bool IsNull()
	{
		return pos.IsNull();
	}


	friend bool operator==(const CTxIndex& a, const CTxIndex& b)
	{
		if (a.pos != b.pos || a.vSpent.size() != b.vSpent.size())
			return false;
		for (int i = 0; i < a.vSpent.size(); i++)
			if (a.vSpent[i] != b.vSpent[i])
				return false;
		return true;
	}

	friend bool operator!=(const CTxIndex& a, const CTxIndex& b)
	{
		return !(a == b);
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


	static uint256 CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex)
	{
		if (nIndex == -1)
			return 0;
		for(const uint256 & otherside : vMerkleBranch)
		{
			if (nIndex & 1)
				hash = Hash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
			else
				hash = Hash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
			nIndex >>= 1;
		}
		return hash;
	}

	bool WriteToDisk(bool fWriteTransactions, unsigned int nFileRet, unsigned int& nBlockPosRet)
	{
		// Open history file to append
		CAutoFile fileout = AppendBlockFile(nFileRet);
		if (!fileout)
		{
			return error("CBlock::WriteToDisk() : AppendBlockFile failed");
		}

		if (!fWriteTransactions)
		{
			fileout.nType |= SER_BLOCKHEADERONLY; // bitwise OR operator
		}
		
		// Write index header
		unsigned int nSize = fileout.GetSerializeSize(*this);
		fileout << FLATDATA(pchMessageStart) << nSize;

		// Write block
		nBlockPosRet = ftell(fileout);
		if (nBlockPosRet == -1)
		{
			return error("CBlock::WriteToDisk() : ftell failed");
		}
		fileout << *this;

		return true;
	}

	bool ReadFromDisk(unsigned int nFile, unsigned int nBlockPos, bool fReadTransactions)
	{
		SetNull();

		// Open history file to read
		CAutoFile filein = OpenBlockFile(nFile, nBlockPos, "rb");
		if (!filein)
		{
			return error("CBlock::ReadFromDisk() : OpenBlockFile failed");
		}
		if (!fReadTransactions)
		{
			filein.nType |= SER_BLOCKHEADERONLY;
		}
		// Read block
		filein >> *this;

		// Check the header TODO: implement the bignum methods and operators for this to work
		//if (CBigNum().SetCompact(nBits) > bnProofOfWorkLimit)
		//{
		//	return error("CBlock::ReadFromDisk() : nBits errors in block header");
		//}
		//if (GetHash() ? CBigNum().SetCompact(nBits).getuint256())
		//{
		//	return error("CBlock::ReadFromDisk() : GetHash() errors in block header");
		//}
		return true;

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
		
	// TODO:
	//int64 GetBlockValue(int64 nFees) const;
	//bool DisconnectBlock(CTxDB& txdb, CBlockIndex* pindex);
	//bool ConnectBlock(CTxDB& txdb, CBlockIndex* pindex);
	//bool ReadFromDisk(const CBlockIndex* blockindex, bool fReadTransactions);
	bool AddToBlockIndex(unsigned int nFile, unsigned int nBlockPos);
	//bool CheckBlock() const;
	//bool AcceptBlock();
};


// 
// The blockchain is a tree shaped structure starting with the genesis block at the root,
// with each block potentially having multiple candidates to be the next block.
// pprev and pnext link a path through the main/longest chain.
// A blockindex may have multiple pprev pointing back to it, but pnext will only point foward to the longest branch, 
// or will be null if the block is not part of the longest chain.
//

class CBlockIndex
{
public:
	const uint256* phashBlock;
	CBlockIndex* pprev;
	CBlockIndex* pnext;
	unsigned int nFile;
	unsigned int nBlockPos;
	int nHeight;

	// block header
	int nVersion;
	uint256 hashMerkleRoot;
	unsigned int nTime;
	unsigned int nBits;
	unsigned int nNonce;

	CBlockIndex()
	{
		phashBlock = NULL;
		pprev = NULL;
		pnext = NULL;
		nFile = 0;
		nBlockPos = 0;
		nHeight = 0;

		nVersion = 0;
		hashMerkleRoot = 0;
		nTime = 0;
		nBits = 0;
		nNonce = 0;
	}

	CBlockIndex(unsigned int nFileIn, unsigned int nBlockPosIn, CBlock& block)
	{
		phashBlock = NULL;
		pprev = NULL;
		pnext = NULL;
		nFile = nFileIn;
		nBlockPos = nBlockPosIn;
		nHeight = 0;

		nVersion = block.nVersion;
		hashMerkleRoot = block.hashMerkleRoot;
		nTime = block.nTime;
		nBits = block.nBits;
		nNonce = block.nNonce;
	}

	uint256 GetBlockHash() const
	{
		return *phashBlock;
	}

	bool IsInMainChain() const
	{
		return (pnext || this == pindexBest);
	}

	bool EraseBlockFromDisk()
	{
		// Open history file
		CAutoFile fileout = OpenBlockFile(nFile, nBlockPos, "rb+");
		if (!fileout)
			return false;

		// Overwrite with empty null block
		CBlock block;
		block.SetNull();
		fileout << block;

		return true;
	}

	enum { nMedianTimeSpan=11};

	int64 GetMedianTimePast() const
	{
		unsigned int pmedian[nMedianTimeSpan];
		unsigned int* pbegin = &pmedian[nMedianTimeSpan];
		unsigned int* pend = &pmedian[nMedianTimeSpan];

		const CBlockIndex* pindex = this;
		for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->pprev)
		{
			*(--pbegin) = pindex->nTime;
		}
		std::sort(pbegin, pend);
		return pbegin[(pend - pbegin) / 2];
	}

	int64 GetMedianTime() const
	{
		const CBlockIndex* pindex = this;
		for (int i = 0; i < nMedianTimeSpan / 2; i++)
		{
			if (!pindex->pnext)
			{
				return nTime;
			}
			pindex = pindex->pnext;
		}
		return pindex->GetMedianTimePast();
	}

	string ToString() const
	{
		return strprintf("CBlockIndex(nprev=%08x, pnext=%08x, nFile=%d, nBlockPos=%-6d nHeight=%d, merkle=%s, hashBlock=%s)",
			pprev, pnext, nFile, nBlockPos, nHeight,
			hashMerkleRoot.ToString().substr(0, 6).c_str(),
			GetBlockHash().ToString().substr(0, 14).c_str());
	}

	void print() const
	{
		printf("%s\n", ToString().c_str());
	}
};



//
// Used to marshal pointers into hashes for db storage
//

class CDiskBlockIndex : public CBlockIndex
{
public:
	uint256 hashPrev;
	uint256 hashNext;


	IMPLEMENT_SERIALIZE
	(
		if (!(nType & SER_GETHASH))
			READWRITE(nVersion);

		READWRITE(hashNext);
		READWRITE(nFile);
		READWRITE(nBlockPos);
		READWRITE(nHeight);

		// block header
		READWRITE(this->nVersion);
		READWRITE(hashPrev);
		READWRITE(hashMerkleRoot);
		//READWRITE(hashContextMerkleRoot);
		READWRITE(nTime);
		READWRITE(nBits);
		READWRITE(nNonce);
	)

	CDiskBlockIndex()
	{
		hashPrev = 0;
		hashNext = 0;
	}
	explicit CDiskBlockIndex(CBlockIndex* pindex) : CBlockIndex(*pindex)
	{
		hashPrev = (pprev ? pprev->GetBlockHash() : 0);
		hashNext = (pnext ? pnext->GetBlockHash() : 0);
	}

	uint256 GetBlockHash() const
	{
		CBlock block;
		block.nVersion = nVersion;
		block.hashPrevBlock = hashPrev;
		block.hashMerkleRoot = hashMerkleRoot;
		block.nTime = nTime;
		block.nBits = nBits;
		block.nNonce = nNonce;
		return block.GetHash();
	}

	string ToString() const
	{
		string str = "CDiskBlockIndex(";
		str += CBlockIndex::ToString();
		str += strprintf("\n                hashBlock=%s, hashPrev=%s, hashNext=%s)",
			GetBlockHash().ToString().c_str(),
			hashPrev.ToString().substr(0, 14).c_str(),
			hashNext.ToString().substr(0, 14).c_str());
		return str;
	}

	void print() const
	{
		printf("%s\n", ToString().c_str());
	}
};

////
// CTxDB
////


class CTxDB : public CDB {
public:
	CTxDB(const char* pszMode = "r+", bool fTxn = false) : CDB(!fClient ? "blkindex.base" : nullptr, fTxn) {}

	bool ReadTxIndex(uint256 hash, CTxIndex& txindex);
	bool UpdateTxIndex(uint256 hash, const CTxIndex& txindex);
	bool AddTxIndex(const CTransaction& tx, const CDiskTxPos& pos, int nHeight);
	bool EraseTxIndex(const CTransaction& tx);
	bool ContainsTx(uint256 hash);
	bool ReadOwnerTxes(uint160 hash160, int nHeight, vector<CTransaction>& vtx);
	bool ReadDiskTx(uint256 hash, CTransaction& tx, CTxIndex& txindex);
	bool ReadDiskTx(uint256 hash, CTransaction& tx);
	bool ReadDiskTx(COutPoint outpoint, CTransaction& tx, CTxIndex& txindex);
	bool ReadDiskTx(COutPoint outpoint, CTransaction& tx);
	bool WriteBlockIndex(const CDiskBlockIndex& blockindex);
	bool EraseBlockIndex(uint256 hash);
	bool ReadHashBestChain(uint256& hashBestChain);
	bool WriteHashBestChain(uint256 hashBestChain);
	/*   bool WriteModelHeader(const GPT2* model);
	   bool WriteModelParameters(const GPT2* model);*/
	bool LoadBlockIndex();
	//bool LoadGPT2(GPT2* model);
};