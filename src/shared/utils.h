#pragma once
#include <string>
#include "standard.h"
#include "serialize.h"
#include "uint256.h"
#include <openssl/sha.h>
//#include <openssl/ripemd.h>

#define loop                for (;;)
#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))
#define ARRAYLEN(array)     (sizeof(array)/sizeof((array)[0]))

#define PAIRTYPE(t1, t2)    std::pair<t1, t2>


std::string strprintf(const char* format, ...);
bool error(const char* format, ...);
int64 GetTime();
int64 GetAdjustedTime();
uint64_t GetRand(uint64_t nMax);
void ParseString(const std::string & str, char c, std::vector<std::string>&v);

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



inline uint160 Hash160(const vector<unsigned char>& vch)
{
    uint256 hash1;
    SHA256(&vch[0], vch.size(), (unsigned char*)&hash1);
    uint160 hash2;
    //RIPEMD160((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}



// Modern replacement for CCriticalSection using std::mutex
class CCriticalSection
{
protected:
    std::mutex mutex;
public:
    std::string sourceFile;
    int sourceLine = 0;

    CCriticalSection() = default;
    ~CCriticalSection() = default;

    void Enter() { mutex.lock(); }
    void Leave() { mutex.unlock(); }
    bool TryEnter() { return mutex.try_lock(); }

    // Get underlying mutex for advanced usage
    std::mutex& get_mutex() { return mutex; }
};

class CCriticalBlock
{
protected:
    std::mutex* pMutex;
    CCriticalSection* pCritSection = nullptr;

public:
    CCriticalBlock(std::mutex& mutexIn) : pMutex(&mutexIn) {
        pMutex->lock();
    }

    CCriticalBlock(CCriticalSection& csIn) : pMutex(&csIn.get_mutex()), pCritSection(&csIn) {
        pMutex->lock();
    }

    ~CCriticalBlock() {
        pMutex->unlock();
        if (pCritSection) {
            pCritSection->sourceFile = "";
            pCritSection->sourceLine = 0;
        }
    }

    // Disallow copying and moving
    CCriticalBlock(const CCriticalBlock&) = delete;
    CCriticalBlock& operator=(const CCriticalBlock&) = delete;
    CCriticalBlock(CCriticalBlock&&) = delete;
    CCriticalBlock& operator=(CCriticalBlock&&) = delete;
};

// Try-lock version of CCriticalBlock
class CTryCriticalBlock
{
protected:
    std::mutex* pMutex = nullptr;
    CCriticalSection* pCritSection = nullptr;
    bool entered = false;

public:
    CTryCriticalBlock(std::mutex& mutexIn) {
        entered = mutexIn.try_lock();
        if (entered) pMutex = &mutexIn;
    }

    CTryCriticalBlock(CCriticalSection& csIn) {
        entered = csIn.get_mutex().try_lock();
        if (entered) {
            pMutex = &csIn.get_mutex();
            pCritSection = &csIn;
        }
    }

    ~CTryCriticalBlock() {
        if (entered) {
            pMutex->unlock();
            if (pCritSection) {
                pCritSection->sourceFile = "";
                pCritSection->sourceLine = 0;
            }
        }
    }

    bool Entered() const { return entered; }

    // Disallow copying and moving
    CTryCriticalBlock(const CTryCriticalBlock&) = delete;
    CTryCriticalBlock& operator=(const CTryCriticalBlock&) = delete;
    CTryCriticalBlock(CTryCriticalBlock&&) = delete;
    CTryCriticalBlock& operator=(CTryCriticalBlock&&) = delete;
};

// Modern version of CRITICAL_BLOCK macro with source location tracking
#define CRITICAL_BLOCK(cs)                                                    \
    for (bool fcriticalblockonce = true; fcriticalblockonce;                   \
         assert(("break caught by CRITICAL_BLOCK!", !fcriticalblockonce)),     \
         fcriticalblockonce = false)                                           \
    for (CCriticalBlock criticalblock(cs);                                     \
         fcriticalblockonce &&                                                 \
         (cs.sourceFile = __FILE__, cs.sourceLine = __LINE__, true);           \
         fcriticalblockonce = false, cs.sourceFile = "", cs.sourceLine = 0)

// Modern version of TRY_CRITICAL_BLOCK macro with source location tracking
#define TRY_CRITICAL_BLOCK(cs)                                                \
    for (bool fcriticalblockonce = true; fcriticalblockonce;                   \
         assert(("break caught by TRY_CRITICAL_BLOCK!", !fcriticalblockonce)), \
         fcriticalblockonce = false)                                           \
    for (CTryCriticalBlock criticalblock(cs);                                  \
         fcriticalblockonce && criticalblock.Entered() &&                      \
         (cs.sourceFile = __FILE__, cs.sourceLine = __LINE__, true);           \
         fcriticalblockonce = false, cs.sourceFile = "", cs.sourceLine = 0)


// Simple string trim function
std::string TrimString(const std::string& str);
void RandAddSeed(bool fPerfmon);

void PrintException(std::exception* pex, const char* pszThread);



// Randomize the stack to help protect against buffer overrun exploits
#define IMPLEMENT_RANDOMIZE_STACK(ThreadFn)                         \
    {                                                               \
        static char nLoops;                                         \
        if (nLoops <= 0)                                            \
            nLoops = GetRand(50) + 1;                               \
        if (nLoops-- > 1)                                           \
        {                                                           \
            ThreadFn;                                               \
            return;                                                 \
        }                                                           \
    }


#define CATCH_PRINT_EXCEPTION(pszFn)     \
    catch (std::exception& e) {          \
        PrintException(&e, (pszFn));     \
    } catch (...) {                      \
        PrintException(NULL, (pszFn));   \
    }
