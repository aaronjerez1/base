
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
}