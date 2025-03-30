#pragma once
#include <string>
#include "standard.h"

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
