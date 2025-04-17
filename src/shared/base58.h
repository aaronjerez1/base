#pragma once
#include "standard.h"
#include "globals.h"

static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";


inline bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet)
{
    BN_CTX* pctx = BN_CTX_new();
    BIGNUM* bn58 = BN_new();
    BIGNUM* bn = BN_new();
    BIGNUM* bnChar = BN_new();

    BN_set_word(bn58, 58);
    BN_set_word(bn, 0);

    while (isspace(*psz))
        psz++;

    // Convert big endian string to bignum
    for (const char* p = psz; *p; p++)
    {
        const char* p1 = strchr(pszBase58, *p);
        if (p1 == NULL)
        {
            while (isspace(*p))
                p++;
            if (*p != '\0')
                return false;
            break;
        }
        BN_set_word(bnChar, p1 - pszBase58);
        if (!BN_mul(bn, bn, bn58, pctx))
            throw std::runtime_error("DecodeBase58 : BN_mul failed");
        BN_add(bn, bn, bnChar);
    }

    // Get bignum as little endian data
    std::vector<unsigned char> vchTmp(BN_num_bytes(bn));
    BN_bn2bin(bn, vchTmp.data());

    // Trim off sign byte if present
    if (vchTmp.size() >= 2 && vchTmp.back() == 0 && vchTmp[vchTmp.size() - 2] >= 0x80)
        vchTmp.pop_back();

    // Restore leading zeros
    int nLeadingZeros = 0;
    for (const char* p = psz; *p == pszBase58[0]; p++)
        nLeadingZeros++;
    vchRet.assign(nLeadingZeros + vchTmp.size(), 0);

    // Convert little endian data to big endian
    std::reverse_copy(vchTmp.begin(), vchTmp.end(), vchRet.end() - vchTmp.size());

    // Clean up
    BN_free(bn);
    BN_free(bn58);
    BN_free(bnChar);
    BN_CTX_free(pctx);

    return true;
}

inline bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet)
{
    if (!DecodeBase58(psz, vchRet))
        return false;
    if (vchRet.size() < 4)
    {
        vchRet.clear();
        return false;
    }
    uint256 hash = Hash(vchRet.begin(), vchRet.end() - 4);
    if (memcmp(&hash, &vchRet.end()[-4], 4) != 0)
    {
        vchRet.clear();
        return false;
    }
    vchRet.resize(vchRet.size() - 4);
    return true;
}

inline bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>&vchRet)
{
    return DecodeBase58Check(str.c_str(), vchRet);
}

