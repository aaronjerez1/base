#include "utils.h"
#include <cstdarg>
#include <openssl/rand.h>

//
//
//
//
///**
// * Safer snprintf for Linux
// * - prints up to limit-1 characters
// * - output string is always null terminated even if limit reached
// * - return value is the number of characters actually printed
// */
//int my_snprintf(char* buffer, size_t limit, const char* format, ...)
//{
//    if (limit == 0)
//        return 0;
//
//    va_list arg_ptr;
//    va_start(arg_ptr, format);
//    int ret = vsnprintf(buffer, limit, format, arg_ptr);
//    va_end(arg_ptr);
//
//    // vsnprintf on Linux returns the number of characters that would have
//    // been written if the buffer was large enough (not including null-terminator)
//    if (ret < 0)
//    {
//        // Error occurred
//        buffer[0] = '\0';
//        return 0;
//    }
//    else if (ret >= static_cast<int>(limit))
//    {
//        // Output was truncated
//        buffer[limit - 1] = '\0';
//        return limit - 1;
//    }
//
//    return ret;
//}
//
//
//
/**
 * Format a string with printf-style formatting
 * Dynamically allocates memory if needed to accommodate the formatted string
 * Returns a std::string with the formatted content
 */
std::string strprintf(const char* format, ...)
{
    // Initial buffer on the stack
    char buffer[1024];
    std::string result;

    va_list args;
    va_start(args, format);

    // Try with the stack buffer first
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);

    if (ret >= 0)
    {
        if (ret < static_cast<int>(sizeof(buffer)))
        {
            // It fit in our buffer
            result = std::string(buffer, ret);
        }
        else
        {
            // We need a bigger buffer
            result.resize(ret);
            va_end(args);
            va_start(args, format);
            vsnprintf(&result[0], ret + 1, format, args);
        }
    }

    va_end(args);
    return result;
}

bool error(const char* format, ...)
{
    char buffer[50000];
    int limit = sizeof(buffer);
    va_list arg_ptr;
    va_start(arg_ptr, format);
    int ret = vsnprintf(buffer, limit, format, arg_ptr);
    va_end(arg_ptr);
    if (ret < 0 || ret >= limit)
    {
        ret = limit - 1;
        buffer[limit - 1] = 0;
    }
    printf("ERROR: %s\n", buffer);
    return false;
}

int64 GetTime()
{
    return time(NULL);
}

static int64 nTimeOffset = 0;

int64 GetAdjustedTime()
{
    return GetTime() + nTimeOffset;
}


uint64_t GetRand(uint64_t nMax)
{
    if (nMax == 0)
        return 0;
    // The range of the random source must be a multiple of the modulus
    // to give every possible output value an equal possibility
    uint64_t nRange = (std::numeric_limits<uint64_t>::max() / nMax) * nMax;
    uint64_t nRand = 0;
    do
        RAND_bytes((unsigned char*)&nRand, sizeof(nRand));
    while (nRand >= nRange);
    return (nRand % nMax);
}

// Simple string trim function
std::string TrimString(const std::string& str) {
    // Find first non-whitespace
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return ""; // String is all whitespace
    }

    // Find last non-whitespace
    size_t end = str.find_last_not_of(" \t\n\r");

    // Return the trimmed substring
    return str.substr(start, end - start + 1);
}

void ParseString(const std::string& str, char c, std::vector<std::string>& v)
{
    unsigned int i1 = 0;
    unsigned int i2;
    do
    {
        i2 = str.find(c, i1);
        v.push_back(str.substr(i1, i2 - i1));
        i1 = i2 + 1;
    } while (i2 != str.npos);
}


#include <sys/resource.h> // For getrusage
#include <sys/sysinfo.h>  // For sysinfo

void RandAddSeed(bool fPerfmon)
{
    // Seed with high-resolution clock
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    RAND_add(&ts, sizeof(ts), 1.5);
    memset(&ts, 0, sizeof(ts));

    static int64 nLastPerfmon;
    if (fPerfmon || GetTime() > nLastPerfmon + 5 * 60)
    {
        nLastPerfmon = GetTime();

        // Prepare buffer
        unsigned char pdata[250000];
        memset(pdata, 0, sizeof(pdata));
        unsigned long nSize = 0;

        // Add WSL-specific information

        // Get entropy from process information
        pid_t pid = getpid();
        RAND_add(&pid, sizeof(pid), 0.1);

        // Get current resource usage
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            nSize += sizeof(usage);
            memcpy(pdata + nSize - sizeof(usage), &usage, sizeof(usage));
        }

        // Add memory information
        FILE* f = fopen("/proc/meminfo", "rb");
        if (f) {
            nSize += fread(pdata + nSize, 1, 4096, f);
            fclose(f);
        }

        // Get system uptime if available
        struct sysinfo s_info;
        if (sysinfo(&s_info) == 0) {
            nSize += sizeof(s_info);
            memcpy(pdata + nSize - sizeof(s_info), &s_info, sizeof(s_info));
        }

        // Get random data from /dev/urandom (should be available in WSL)
        f = fopen("/dev/urandom", "rb");
        if (f) {
            unsigned char random_buf[1024];
            size_t random_read = fread(random_buf, 1, sizeof(random_buf), f);
            RAND_add(random_buf, random_read, random_read);
            fclose(f);
            memset(random_buf, 0, sizeof(random_buf));
        }

        if (nSize > 0) {
            uint256 hash;
            SHA256(pdata, nSize, (unsigned char*)&hash);
            RAND_add(&hash, sizeof(hash), std::min(nSize / 500.0, (double)sizeof(hash)));
            hash = 0;
            memset(pdata, 0, nSize);

            time_t nTime;
            time(&nTime);
            struct tm* ptmTime = gmtime(&nTime);
            char pszTime[200];
            strftime(pszTime, sizeof(pszTime), "%x %H:%M:%S", ptmTime);
            printf("%s  RandAddSeed() got %d bytes of system data\n", pszTime, nSize);
        }
    }
}

bool GetExecutablePath(char* buffer, size_t size)
{
    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
    if (len == -1)
        return false;

    buffer[len] = '\0';
    return true;
}

std::string GetModulePath()
{
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return path;
#else
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1)
        return {};
    path[len] = '\0';
    return path;
#endif
}

void StrToLower(char* s)
{
    std::transform(s, s + std::strlen(s), s,
        [](unsigned char c) { return std::tolower(c); });
}

void PrintException(std::exception* pex, const char* pszThread)
{
    char pszModule[260];
    pszModule[0] = '\0';
    GetExecutablePath(pszModule, sizeof(pszModule));
    StrToLower(pszModule);
    char pszMessage[1000];
    if (pex)
        snprintf(pszMessage, sizeof(pszMessage),
            "EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(), pex->what(), pszModule, pszThread);
    else
        snprintf(pszMessage, sizeof(pszMessage),
            "UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, pszThread);
    printf("\n\n************************\n%s", pszMessage);
    //if (wxTheApp)
    //    wxMessageBox(pszMessage, "Error", wxOK | wxICON_ERROR);
    throw;
    //DebugBreak();
}


//#include <stdint.h>
//#include <string.h>
//#include <time.h>
//#include <stdio.h>
//
//#include <openssl/rand.h>
//#include <openssl/sha.h>
//
//#if defined(__linux__)
//#include <sys/random.h>   // getrandom
//#include <errno.h>
//#endif
//
//static void RandAddSeedLinux(bool fPerfmon)
//{
//    // Stir at most every 5 minutes unless forced
//    static int64 nLast = 0;
//    if (!fPerfmon && GetTime() <= nLast + 5 * 60)
//        return;
//    nLast = GetTime();
//
//#if defined(__linux__)
//    // 1) Pull from kernel CSPRNG (best source)
//    unsigned char buf[64];
//    ssize_t n = getrandom(buf, sizeof(buf), 0);
//
//    if (n > 0)
//    {
//        // Hash it first so we don't feed raw kernel bytes around the codebase
//        unsigned char h[32];
//        SHA256(buf, (size_t)n, h);
//
//        // credit <= 0.0 unless you are absolutely certain (we aren't)
//        // OpenSSL already seeds itself from getrandom/urandom; this is just extra stirring.
//        RAND_add(h, sizeof(h), 0.0);
//
//        // wipe
//        OPENSSL_cleanse(buf, sizeof(buf));
//        OPENSSL_cleanse(h, sizeof(h));
//    }
//    else
//    {
//        // If getrandom fails (rare), we just skip silently.
//        // errno can be EINTR, EAGAIN on very early boot, etc.
//        // printf("RandAddSeed: getrandom failed errno=%d\n", errno);
//    }
//#else
//    (void)fPerfmon; // non-linux: no-op or provide alternative
//#endif
//
//    // Optional: log similar to your old function
//    time_t t = time(nullptr);
//    struct tm tm {};
//    gmtime_r(&t, &tm);
//    char ts[64];
//    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm);
//    printf("%s  RandAddSeed() stirred entropy\n", ts);
//}
//
//// Keep the old name/signature so the rest of the code compiles
//void RandAddSeed(bool fPerfmon)
//{
//    RandAddSeedLinux(fPerfmon);
//}
