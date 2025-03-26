#pragma once
//#include <openssl/types.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <serialize.h>
#include <vector>


// todo: secure_allocator here and in serialize.h
typedef std::vector<unsigned char, secure_allocator<unsigned char>> CPrivKey;


class CKey
{
	EVP_PKEY* pkey;
	CKey()
	{
		EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
		if (!ctx)
			throw std::runtime_error("CKey::CKey() : EVP_PKEY_CTX_new_id failed");
		if (EVP_PKEY_keygen_init(ctx) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			throw std::runtime_error("CKey::CKey() : EVP_PKEY_keygen_init failed");
		}
		if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			throw std::runtime_error("CKey::CKey() : EVP_PKEY_CTX_set_rsa_keygen_bits failed");
		}

		// Generate key
		pkey = NULL;
		if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			throw std::runtime_error("CKey::CKey() : EVP_PKEY_keygen failed");
		}
		// Free the temporary context
		EVP_PKEY_CTX_free(ctx);
	}

	~CKey()
	{
		// Clean up
		if (pkey) EVP_PKEY_free(pkey);
	}

	void MakeNewKey()
	{
		EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
		if (!ctx)
			throw std::runtime_error("CKey::CKey() : EVP_PKEY_CTX_new_id failed");
		if (EVP_PKEY_keygen_init(ctx) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			throw std::runtime_error("CKey::CKey() : EVP_PKEY_keygen_init failed");
		}
		if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			throw std::runtime_error("CKey::CKey() : EVP_PKEY_CTX_set_rsa_keygen_bits failed");
		}

		// Generate key
		pkey = NULL;
		if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			throw std::runtime_error("CKey::CKey() : EVP_PKEY_keygen failed");
		}
		// Free the temporary context
		EVP_PKEY_CTX_free(ctx);
	}

	bool SetPrivKey(const CPrivKey& vchPrivKey)
	{
		if (pkey) EVP_PKEY_free(pkey);
		pkey = NULL;

		const unsigned char* pbegin = &vchPrivKey[0];
		pkey = d2i_PrivateKey(EVP_PKEY_RSA, NULL, &pbegin, vchPrivKey.size());
		return (pkey != NULL);
	}

	CPrivKey GetPrivKey() const
	{
		if (!pkey)
			throw std::runtime_error("CKeyu::GetPrivKey() : No private key");
		unsigned int nSize = i2d_PrivateKey(pkey, NULL);

		if (!nSize)
			throw std::runtime_error("CKey::GetPrivKey() : i2d_PrivateKey failed");

		CPrivKey vchPrivKey(nSize, 0);
		unsigned char* pbegin = &vchPrivKey[0];
		if (i2d_PrivateKey(pkey, &pbegin) != nSize)
			throw std::runtime_error("CKey::GetPrivKey() : i2d_PrivateKey returned unexpected size");

		return vchPrivKey;
	}

	bool SetPubKey(const std::vector<unsigned char>& vchPubKey)
	{
		if (pkey) EVP_PKEY_free(pkey);
		pkey = NULL;

		const unsigned char* pbegin = &vchPubKey[0];
		pkey = d2i_PublicKey(EVP_PKEY_RSA, NULL, &pbegin, vchPubKey.size());
		return (pkey != NULL);
	}

	std::vector<unsigned char> GetPubKey() const
	{
		if (!pkey)
			throw std::runtime_error("CKey::GetPubKey() : No public key");

		unsigned int nSize = i2d_PublicKey(pkey, NULL);
		if (!nSize)
			throw std::runtime_error("CKey::GetPubKey() : i2d_PublicKey failed ");

		std::vector<unsigned char> vchPubKey(nSize, 0);
		unsigned char* pbegin = &vchPubKey[0];
		if (i2d_PublicKey(pkey, &pbegin) != nSize)
		{
			throw std::runtime_error("CKey::GetPubKey() : i2d_PUBKEY returned unexpected size");
		}
		return vchPubKey;
	}

	bool Sign(unsigned char* hash, std::vector<unsigned char>& vchSig) // TODO: change hash to uint256
	{
		vchSig.clear();

		EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, NULL);
		if (!ctx) {
			throw std::runtime_error("CKey::Sign() : EVP_PKEY_CTX_new failed");
		}

		if (EVP_PKEY_sign_init(ctx) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			throw std::runtime_error("CKey::Sign() : EVP_PKEY_sign_init failed");
		}

		/// Determine the output buffer length for the signature
		size_t siglen = 0;
		if (EVP_PKEY_sign(ctx, NULL, &siglen, (const unsigned char*)&hash, sizeof(hash)) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			throw std::runtime_error("CKey::Sign() : EVP_PKEY_sign for lenght verification failed");
		}

		vchSig.resize(siglen);

		if (EVP_PKEY_sign(ctx, &vchSig[0], &siglen, (const unsigned char*)&hash, sizeof(hash)) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			throw std::runtime_error("CKey::Sign() : EVP_PKEY_sign failed");
		}

		EVP_PKEY_CTX_free(ctx);
		return true;
	}

	bool Verify(int hash, const std::vector<unsigned char>& vchSig)
	{
		// Create a context for the verification operation
		EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, NULL);
		if (!ctx) {
			return false;
		}

		// Initialize the verification operation
		if (EVP_PKEY_verify_init(ctx) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			return false;
		}

		// Perform the verification
		int result = EVP_PKEY_verify(ctx, vchSig.data(), vchSig.size(),
			(const unsigned char*)&hash, sizeof(hash));

		// Free the context
		EVP_PKEY_CTX_free(ctx);

		// Return true if verification succeeded (result == 1)
		return (result == 1);
	}
};