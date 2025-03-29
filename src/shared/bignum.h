#pragma once
#include <stdexcept>
#include <openssl/bn.h>

class CAutoBN_CTX
{
protected:
	BN_CTX* pctx;
	BN_CTX* operator=(BN_CTX* pnew) { return pctx = pnew; }
public:
	CAutoBN_CTX()
	{
		pctx = BN_CTX_new();
		if (pctx == NULL)
			throw std::runtime_error("CAutoBN_CTX::CAutoBN_CTX(): pctx NULL, BN_CTX_new() didn't work.");
	}

	~CAutoBN_CTX()
	{
		if (pctx != NULL)
			BN_CTX_free(pctx);
	}

	operator BN_CTX* () { return pctx; }
	BN_CTX& operator*() { return *pctx; }
	BN_CTX** operator&() { return &pctx; }
	bool operator!() { return (pctx == NULL); }
};

// here we are going to use composition by storing BIGNUM* pointer as a provate memeber instead of inheriting from BIGNUM

class CBigNum
{
private:
	BIGNUM* bn;
public:
	CBigNum()
	{
		bn = BN_new();
		if (!bn)
			throw std::runtime_error("CBigNum::CBigNum(): BN_new() failed.");
	}

	CBigNum(const CBigNum& b)
	{
		bn = BN_new();
		if (!bn || !BN_copy(bn, b.bn))
		{
			BN_free(bn);
			throw std::runtime_error("CBigNum::CBigNum(const CBigNum&) : BN_copy failed");
		}
	}

	CBigNum& operator=(const CBigNum& b)
	{
		if (!BN_copy(bn, b.bn))
			throw std::runtime_error("CBigNum::operator= : BN_copy failed");
		return *this;
	}


	CBigNum(unsigned int n) { bn = BN_new(); setulong(n); }

	void setulong(unsigned long n)
	{
		if (!BN_set_word(bn, n))
			throw std::runtime_error("CBigNum conversion from unsigned long : BN_set_word failed");
	}

};