#pragma once
#include <string>
#include "standard.h"
#include "serialize.h"
#include "uint256.h"
#include <openssl/sha.h>

#define loop                for (;;)
#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))


std::string strprintf(const char* format, ...);

template<typename T>
std::string HexStr(const T itbegin, const T itend, bool fSpaces = true)
{
    const unsigned char* pbegin = (const unsigned char*)&itbegin[0];
    const unsigned char* pend = pbegin + (itend - itbegin) * sizeof(itbegin[0]);
    std::string str;
    for (const unsigned char* p = pbegin; p != pend; p++)
        str += strprintf((fSpaces && p != pend - 1 ? "%02x " : "%02x"), *p);
    return str;
}

template<typename T>
std::string HexNumStr(const T itbegin, const T itend, bool f0x = true)
{
    const unsigned char* pbegin = (const unsigned char*)&itbegin[0];
    const unsigned char* pend = pbegin + (itend - itbegin) * sizeof(itbegin[0]);
    std::string str = (f0x ? "0x" : "");
    for (const unsigned char* p = pend - 1; p >= pbegin; p--)
        str += strprintf("%02X", *p);
    return str;
}

template<typename T>
void PrintHex(const T pbegin, const T pend, const char* pszFormat = "%s", bool fSpaces = true)
{
    printf(pszFormat, HexStr(pbegin, pend, fSpaces).c_str());
}


template<typename T1>
inline uint256 Hash(const T1 pbegin, const T1 pend)
{
    uint256 hash1;
    SHA256((unsigned char*)&pbegin[0], (pend - pbegin) * sizeof(pbegin[0]), (unsigned char*)&hash1);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

template<typename T1, typename T2>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
    const T2 p2begin, const T2 p2end)
{
    uint256 hash1;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (unsigned char*)&p1begin[0], (p1end - p1begin) * sizeof(p1begin[0]));
    SHA256_Update(&ctx, (unsigned char*)&p2begin[0], (p2end - p2begin) * sizeof(p2begin[0]));
    SHA256_Final((unsigned char*)&hash1, &ctx);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}




template<typename T>
uint256 SerializeHash(const T& obj, int nType = SER_GETHASH, int nVersion = VERSION)
{
    // Most of the time is spent allocating and deallocating CDataStream's
    // buffer.  If this ever needs to be optimized further, make a CStaticStream
    // class with its buffer on the stack.
    CDataStream ss(nType, nVersion);
    ss.reserve(10000);
    ss << obj;
    return Hash(ss.begin(), ss.end());
}


bool error(const char* format, ...);
