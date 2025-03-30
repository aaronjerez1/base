#pragma once
#include <stdexcept>
#include <openssl/bn.h>
#include <algorithm>
#include <stdexcept>

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




	CBigNum(unsigned int n) { bn = BN_new(); setulong(n); }

	explicit CBigNum(const std::vector<unsigned char>& vch)
	{
		bn = BN_new();
		setvch(vch);
	}

	void setulong(unsigned long n)
	{
		if (!BN_set_word(bn, n))
			throw std::runtime_error("CBigNum conversion from unsigned long : BN_set_word failed");
	}

	unsigned long getulong() const
	{
		return BN_get_word(bn);
	}

	int getint() const
	{
		unsigned long n = BN_get_word(bn);
		if (!BN_is_negative(bn))
			return (n > INT_MAX ? INT_MAX : n);
		else
			return (n > INT_MAX ? INT_MIN : -(int)n);
	}




	void setvch(const std::vector<unsigned char>& vch)
	{
		std::vector<unsigned char> vch2(vch.size() + 4);
		unsigned int nSize = vch.size();
		vch2[0] = (nSize >> 24) & 0xff;
		vch2[1] = (nSize >> 16) & 0xff;
		vch2[2] = (nSize >> 8) & 0xff;
		vch2[3] = (nSize) & 0xff;
		std::reverse_copy(vch.begin(), vch.end(), vch2.begin() + 4);
		BN_mpi2bn(&vch2[0], vch2.size(), bn);
	}

	std::vector<unsigned char> getvch() const
	{
		unsigned int nSize = BN_bn2mpi(bn, NULL);
		if (nSize < 4)
			return std::vector<unsigned char>();
		std::vector<unsigned char> vch(nSize);
		BN_bn2mpi(bn, &vch[0]);
		vch.erase(vch.begin(), vch.begin() + 4);
		std::reverse(vch.begin(), vch.end());
		return vch;
	}

	CBigNum& operator=(const CBigNum& b)
	{
		if (!BN_copy(bn, b.bn))
			throw std::runtime_error("CBigNum::operator= : BN_copy failed");
		return *this;
	}

	bool operator!=(const CBigNum& b) { return (BN_cmp(bn, b.bn) != 0); }
};

