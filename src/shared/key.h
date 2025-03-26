//#pragma once
////#include <openssl/types.h>
//#include <openssl/evp.h>
//#include <openssl/rsa.h>
//#include <serialize.h>
//#include <vector>
//
//
//// todo: secure_allocator here and in serialize.h
//typedef std::vector<unsigned char, secure_allocator<unsigned char>> CPrivKey;
//
//
//class CKey
//{
//protected:
//	EVP_PKEY_CTX* ctx;
//	EVP_PKEY* pkey;
//	CKey()
//	{
//		ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
//		if (!ctx)
//			printf("error");
//		if (EVP_PKEY_keygen_init(ctx) <= 0)
//			printf("error");
//		if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0)
//			printf("error");
//
//		// GENERATE KEY
//		if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
//			printf("error");
//	}
//
//	~CKey()
//	{
//		// Clean up
//		if (pkey) EVP_PKEY_free(pkey);
//		if (ctx) EVP_PKEY_CTX_free(ctx);
//	}
//
//	void MakeNewKey()
//	{
//		if (!ctx)
//			printf("error");
//		if (EVP_PKEY_keygen_init(ctx) <= 0)
//			printf("error");
//		if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0)
//			printf("error");
//
//		// GENERATE KEY
//		if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
//			printf("error");
//	}
//
//	bool SetPrivKey(const CPrivKey& vchPrivKey)
//	{
//		if (pkey) EVP_PKEY_free(pkey);
//		pkey = NULL;
//
//		const unsigned char* pbegin = &vchPrivKey[0];
//		pkey = d2i_PrivateKey(EVP_PKEY_RSA, NULL, &pbegin, vchPrivKey.size());
//		return (pkey != NULL);
//	}
//
//	CPrivKey GetPrivKey() const
//	{
//		if (!pkey)
//			throw std::runtime_error("CKeyu::GetPrivKey() : No private key");
//		unsigned int nSize = i2d_PrivateKey(pkey, NULL);
//
//		if (!nSize)
//			throw std::runtime_error("CKey::GetPrivKey() : i2d_PrivateKey failed");
//
//		CPrivKey vchPrivKey(nSize, 0);
//		unsigned char* pbegin = &vchPrivKey[0];
//		if (i2d_PrivateKey(pkey, &pbegin) != nSize)
//			throw std::runtime_error("CKey::GetPrivKey() : i2d_PrivateKey returned unexpected size");
//
//		return vchPrivKey;
//	}
//
//	bool SetPubKey(const std::vector<unsigned char>& vchPubKey)
//	{
//		if (pkey) EVP_PKEY_free(pkey);
//		pkey = NULL;
//
//		const unsigned char* pbegin = &vchPubKey[0];
//		pkey = d2i_PublicKey(EVP_PKEY_RSA, NULL, &pbegin, vchPubKey.size());
//		return (pkey != NULL);
//	}
//
//	std::vector<unsigned char> GetPubKey() const
//	{
//		if (!pkey)
//			throw std::runtime_error("CKey::GetPubKey() : No public key");
//
//		unsigned int nSize = i2d_PublicKey(pkey, NULL);
//		if (!nSize)
//			throw std::runtime_error("CKey::GetPubKey() : i2d_PublicKey failed ");
//
//		std::vector<unsigned char> vchPubKey(nSize, 0);
//		unsigned char* pbegin = &vchPubKey[0];
//		if (i2d_PublicKey(pkey, &pbegin) != nSize)
//		{
//			throw std::runtime_error("CKey::GetPubKey() : i2d_PUBKEY returned unexpected size");
//		}
//		return vchPubKey;
//	}
//};