#pragma once
#include "standard.h"

// Not to be confused with BEGIN AND END
#define UBEGIN(a)           ((unsigned char*)&(a))
#define UEND(a)             ((unsigned char*)&((&(a))[1]))

static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";


inline bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet);

inline bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet);

inline bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet);




// EncodeBase58
inline std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend);

inline std::string EncodeBase58(const std::vector<unsigned char>& vch);

inline std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn);

//inline std::string Hash160ToAddress(uint160 hash160);

std::string PubKeyToAddress(const std::vector<unsigned char>& vchPubKey);