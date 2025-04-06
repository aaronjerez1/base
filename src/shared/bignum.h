#pragma once
#include <stdexcept>
#include <openssl/bn.h>
#include <algorithm>
#include <stdexcept>

#include "uint256.h"

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

	explicit CBigNum(const std::string& str)
	{
		bn = BN_new();
		if(!bn)
			throw std::runtime_error("CBigNum::CBigNum(string) : BN_new failed");
		//SetHex(str); // TODO: thsi needs to build the method and this method needs 2-3 operators we haven't yet built
	}

	~CBigNum()
	{
		BN_free(bn);
	}


	explicit CBigNum(uint256 n) { bn = BN_new(); setuint256(n); }




	CBigNum(unsigned int n) { bn = BN_new(); setulong(n); }

	explicit CBigNum(const std::vector<unsigned char>& vch)
	{
		bn = BN_new();
		setvch(vch);
	}



	void setuint256(const uint256& n)
	{
		// Prepare a byte array (MPI representation) with extra space for the size header
		unsigned char pch[sizeof(n) + 6];
		unsigned char* p = pch + 4;  // Skip the first 4 bytes (reserved for size header)

		bool fLeadingZeroes = true;
		const unsigned char* pbegin = (const unsigned char*)&n;
		const unsigned char* psrc = pbegin + sizeof(n);

		// Loop through the bytes of n (in reverse order) to strip leading zeroes
		while (psrc != pbegin)
		{
			unsigned char c = *(--psrc);
			if (fLeadingZeroes)
			{
				if (c == 0)  // Skip leading zeroes
					continue;
				if (c & 0x80)  // If the highest bit is set, add an extra byte
					*p++ = 0;
				fLeadingZeroes = false;
			}
			*p++ = c;
		}

		// Compute the size of the number and store it in the first 4 bytes of pch
		unsigned int nSize = p - (pch + 4);
		pch[0] = (nSize >> 24) & 0xff;
		pch[1] = (nSize >> 16) & 0xff;
		pch[2] = (nSize >> 8) & 0xff;
		pch[3] = (nSize >> 0) & 0xff;

		// Convert the byte array (MPI format) to a BIGNUM and store it in 'bn'
		BN_mpi2bn(pch, p - pch, bn);
	}

	uint256 getuint256() const
	{
		// Get the size of the BIGNUM in MPI format
		unsigned int nSize = BN_bn2mpi(bn, NULL);
		if (nSize < 4)
			return 0;  // If the size is less than 4, return 0

		// Allocate a vector to hold the MPI representation
		std::vector<unsigned char> vch(nSize);

		// Convert the BIGNUM to MPI format and store it in the vector
		BN_bn2mpi(bn, &vch[0]);

		// Ensure the highest bit is cleared
		if (vch.size() > 4)
			vch[4] &= 0x7f;

		// Convert the vector into a uint256
		uint256 n = 0;
		for (int i = 0, j = vch.size() - 1; i < sizeof(n) && j >= 4; i++, j--)
			((unsigned char*)&n)[i] = vch[j];

		return n;
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


	CBigNum& SetCompact(unsigned int nCompact)
	{
		// Extract the size from the compact representation
		unsigned int nSize = nCompact >> 24;

		// Create a vector to store the binary representation
		std::vector<unsigned char> vch(4 + nSize);
		vch[0] = (nSize >> 24) & 0xff;
		vch[1] = (nSize >> 16) & 0xff;
		vch[2] = (nSize >> 8) & 0xff;
		vch[3] = (nSize) & 0xff;

		// Fill the rest of the vector with the nCompact data
		if (nSize >= 1) vch[4] = (nCompact >> 16) & 0xff;
		if (nSize >= 2) vch[5] = (nCompact >> 8) & 0xff;
		if (nSize >= 3) vch[6] = (nCompact >> 0) & 0xff;

		// Convert the byte vector to a BIGNUM (use bn instead of this)
		BN_mpi2bn(&vch[0], vch.size(), bn);  // 'bn' is your BIGNUM member variable, not 'this'

		return *this;
	}

	unsigned int GetCompact() const
	{
		unsigned int nSize = BN_bn2mpi(bn, NULL);
		std::vector<unsigned char> vch(nSize);
		nSize -= 4;
		BN_bn2mpi(bn, &vch[0]);
		unsigned int nCompact = nSize << 24;
		if (nSize >= 1) nCompact |= (vch[4] << 16);
		if (nSize >= 2) nCompact |= (vch[5] << 8);
		if (nSize >= 3) nCompact |= (vch[6] << 0);
		return nCompact;
	}

	CBigNum& operator=(const CBigNum& b)
	{
		if (!BN_copy(bn, b.bn))
			throw std::runtime_error("CBigNum::operator= : BN_copy failed");
		return *this;
	}


	CBigNum& operator*=(const CBigNum& b)
	{
		CAutoBN_CTX pctx;
		if (!BN_mul(bn, bn, b.bn, pctx))
			throw std::runtime_error("CBigNum::operator*= : BN_mul failed");
		return *this;
	}

	CBigNum& operator/(const CBigNum& b)
	{
		CAutoBN_CTX pctx;
		CBigNum r;
		if (!BN_div(r.bn, NULL, bn, b.bn, pctx))
			throw std::runtime_error("CBigNum::operator/ : BN_div failed");
		return r;
	}

	CBigNum& operator/=(const CBigNum& b)
	{
		*this = *this / b;  // Use the already defined division operator
		return *this;
	}

	CBigNum& operator++()
	{
		if (!BN_add(bn, bn, BN_value_one()))  // Increment the internal BIGNUM*
			throw std::runtime_error("CBigNum::operator++ : BN_add failed");
		return *this;
	}


	bool operator!=(const CBigNum& b) { return (BN_cmp(bn, b.bn) != 0); }
	bool operator>(const CBigNum& b) { return (BN_cmp(bn, b.bn) > 0); }
};

