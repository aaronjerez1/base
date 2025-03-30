#pragma once
#include <cstring>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <bits/stdc++.h>
#include "../script/script.h"
#include "types.h"

//
// Basic types
//
#define WRITEDATA(s, obj)   s.write((char*)&(obj), sizeof(obj))
#define READDATA(s, obj)    s.read((char*)&(obj), sizeof(obj))

inline unsigned int GetSerializeSize(char a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(signed char a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(unsigned char a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(signed short a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(unsigned short a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(signed int a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(unsigned int a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(signed long a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(unsigned long a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(int64 a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(uint64 a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(float a, int, int = 0) { return sizeof(a); }
inline unsigned int GetSerializeSize(double a, int, int = 0) { return sizeof(a); }


template<typename Stream> inline void Serialize(Stream& s, char a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, signed char a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, unsigned char a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, signed short a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, unsigned short a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, signed int a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, unsigned int a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, signed long a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, unsigned long a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int64 a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint64 a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, float a, int, int = 0) { WRITEDATA(s, a); }
template<typename Stream> inline void Serialize(Stream& s, double a, int, int = 0) { WRITEDATA(s, a); }




//
// Compact size
//  size <  253        -- 1 byte
//  size <= USHRT_MAX  -- 3 bytes  (253 + 2 bytes)
//  size <= UINT_MAX   -- 5 bytes  (254 + 4 bytes)
//  size >  UINT_MAX   -- 9 bytes  (255 + 8 bytes)
//
inline unsigned int GetSizeOfCompactSize(uint64 nSize)
{
    if (nSize < UCHAR_MAX - 2)     return sizeof(unsigned char);
    else if (nSize <= USHRT_MAX) return sizeof(unsigned char) + sizeof(unsigned short);
    else if (nSize <= UINT_MAX)  return sizeof(unsigned char) + sizeof(unsigned int);
    else                         return sizeof(unsigned char) + sizeof(uint64);
}

template<typename Stream>
void WriteCompactSize(Stream& os, uint64 nSize)
{
    if (nSize < UCHAR_MAX - 2)
    {
        unsigned char chSize = nSize;
        WRITEDATA(os, chSize);
    }
    else if (nSize <= USHRT_MAX)
    {
        unsigned char chSize = UCHAR_MAX - 2;
        unsigned short xSize = nSize;
        WRITEDATA(os, chSize);
        WRITEDATA(os, xSize);
    }
    else if (nSize <= UINT_MAX)
    {
        unsigned char chSize = UCHAR_MAX - 1;
        unsigned int xSize = nSize;
        WRITEDATA(os, chSize);
        WRITEDATA(os, xSize);
    }
    else
    {
        unsigned char chSize = UCHAR_MAX;
        WRITEDATA(os, chSize);
        WRITEDATA(os, nSize);
    }
    return;
}

template<typename Stream>
uint64 ReadCompactSize(Stream& is)
{
    unsigned char chSize;
    READDATA(is, chSize);
    if (chSize < UCHAR_MAX - 2)
    {
        return chSize;
    }
    else if (chSize == UCHAR_MAX - 2)
    {
        unsigned short nSize;
        READDATA(is, nSize);
        return nSize;
    }
    else if (chSize == UCHAR_MAX - 1)
    {
        unsigned int nSize;
        READDATA(is, nSize);
        return nSize;
    }
    else
    {
        uint64 nSize;
        READDATA(is, nSize);
        return nSize;
    }
}


//
//
// string
//
template<typename C>
unsigned int GetSerializeSize(const std::basic_string<C>& str, int, int)
{
    return GetSizeOfCompactSize(str.size()) + str.size() * sizeof(str[0]);
}

template<typename Stream, typename C>
void Serialize(Stream& os, const std::basic_string<C>& str, int, int)
{
    WriteCompactSize(os, str.size());
    if (!str.empty())
        os.write((char*)&str[0], str.size() * sizeof(str[0]));
}

template<typename Stream, typename C>
void Unserialize(Stream& is, std::basic_string<C>& str, int, int)
{
    unsigned int nSize = ReadCompactSize(is);
    str.resize(nSize);
    if (nSize != 0)
        is.read((char*)&str[0], nSize * sizeof(str[0]));
}

//
// vector
//
template<typename T, typename A>
unsigned int GetSerializeSize_impl(const std::vector<T, A>& v, int nType, int nVersion, const std::true_type&)
{
    return (GetSizeOfCompactSize(v.size()) + v.size() * sizeof(T));
}

template<typename T, typename A>
unsigned int GetSerialize_impl(const std::vector<T, A>& v, int nType, int nVersion, const std::false_type&)
{
    unsigned int nSize = GetSizeOfCompactSize(v.size());
    for (typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
    {
        nSize += GetSerializeSize((*vi), nType, nVersion);
    }
    return nSize;
}

template<typename T, typename A>
inline unsigned int GetSerializeSize(const std::vector<T, A>& v, int nType, int nVersion)
{
    return GetSerializeSize_impl(v, nType, nVersion, std::is_fundamental<T>());
}

template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nType, int nVersion, const std::false_type&)
{
    WriteCompactSize(os, v.size());
    if (!v.empty())
    {
        os.write((char*)&v[0], v.size() * sizeof(T));
    }
}

template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, int nType, int nVersion, const std::true_type&)
{
    WriteCompactSize(os, v.size());
    for (typename std::vector<T, A>::const_iterator vi = v.begin(); vi != v.end(); ++vi)
    {
        ::Serialize(os, (*vi), nType, nVersion);
    }
}

template<typename Stream, typename T, typename A>
inline void Serialize(Stream& os, const std::vector<T, A>& v, int nType, int nVersion)
{
    Serialize_impl(os, v, nType, nVersion, std::is_fundamental<T>());
}

template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nType, int nVersion, const std::true_type&)
{
    v.clear();
    unsigned int nSize = ReadCompactSize(is);
    unsigned int i = 0;
    while (i < nSize)
    {
        unsigned int blk = std::min<unsigned int>(nSize - i, 1 + 4999999 / sizeof(T));
        v.resize(i + blk);
        is.read((char*)&v[i], blk * sizeof(T));
        i += blk;
    }
}

template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, int nType, int nVersion, const std::false_type&)
{
    v.clear();
    unsigned int nSize = ReadCompact(is);
    unsigned int i = 0;
    unsigned int nMid = 0;
    while (nMid < nSize)
    {
        nMid += 5000000 / sizeof(T);
        if (nMid > nSize) 
        {
            nMid = nSize;
        }
        v.resize(nMid);
        for (; i < nMid; i++)
            Unserialize(is, v[i], nType, nVersion);
    }
}

template<typename Stream, typename T, typename A>
inline void Unserialize(Stream& is, std::vector<T, A>& v, int nType, int nVersion)
{
    Unserialize_impl(is, v, nType, nVersion, std::is_fundamental<T>());
}


//
// pair
//
template<typename K, typename T>
unsigned int GetSerializeSize(const std::pair<K, T>& item, int nType, int nVersion)
{
    return GetSerializeSize(item.first, nType, nVersion) + GetSerializeSize(item.second, nType, nVersion);
}

template<typename Stream, typename K, typename T>
void Serialize(Stream& os, const std::pair<K, T>& item, int nType, int nVersion)
{
    Serialize(os, item.first, nType, nVersion);
    Serialize(os, item.second, nType, nVersion);
}

template<typename Stream, typename K, typename T>
void Unserialize(Stream& is, std::pair<K, T>& item, int nType, int nVersion)
{
    Unserialize(is, item.first, nType, nVersion);
    Unserialize(is, item.second, nType, nVersion);
}


//
// map
//
template<typename K, typename T, typename Pred, typename A>
unsigned int GetSerializeSize(const std::map<K, T, Pred, A>& m, int nType, int nVersion)
{
    unsigned int nSize = GetSizeOfCompactSize(m.size());
    for (typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        nSize += GetSerializeSize((*mi), nType, nVersion);
    return nSize;
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream& os, const std::map<K, T, Pred, A>& m, int nType, int nVersion)
{
    WriteCompactSize(os, m.size());
    for (typename std::map<K, T, Pred, A>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        Serialize(os, (*mi), nType, nVersion);
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream& is, std::map<K, T, Pred, A>& m, int nType, int nVersion)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    typename std::map<K, T, Pred, A>::iterator mi = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        std::pair<K, T> item;
        Unserialize(is, item, nType, nVersion);
        mi = m.insert(mi, item);
    }
}


//
// set
//
template<typename K, typename Pred, typename A>
unsigned int GetSerializeSize(const std::set<K, Pred, A>& m, int nType, int nVersion)
{
    unsigned int nSize = GetSizeOfCompactSize(m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        nSize += GetSerializeSize((*it), nType, nVersion);
    return nSize;
}


template<typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream& os, const std::set<K, Pred, A>& m, int nType, int nVersion)
{
    WriteCompactSize(os, m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
    {
        Serialize(os, (*it), nType, nVersion);
    }
}

template<typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream& is, std::set<K, Pred, A>& m, int nType, int nVersion)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    typename std::set<K, Pred, A>::iterator it = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        K key;
        Unserialize(is, key, nType, nVersion);
        it = m.insert(it, key);
    }
}

//
// others derived from vector
//
inline unsigned int GetSerializeSize(const CScript& v, int nType, int nVersion)
{
    return GetSerializeSize((const std::vector<unsigned char>&)v, nType, nVersion);
}

template<typename Stream>
void Serialize(Stream& os, const CScript& v, int nType, int nVersion)
{
    Serialize(os, (const std::vector<unsigned char>&)v, nType, nVersion);
}

template<typename Stream>
void Unserialize(Stream& is, CScript& v, int nType, int nVersion)
{
    Unserialize(is, (std::vector<unsigned char>&)v, nType, nVersion);
}



//
// If none of the specialized versions above matched, default to calling member function.
// "int nType" is changed to "long nType" to keep from getting an ambiguous overload error.
// The compiler will only cast int to long if none of the other templates matched.
// Thanks to Boost serialization for this idea.
//
template<typename T>
inline unsigned int GetSerializeSize(const T& a, long nType, int nVersion = VERSION)
{
    return a.GetSerializeSize((int)nType, nVersion);
}

template<typename Stream, typename T>
inline void Serialize(Stream& os, const T& a, long nType, int nVersion = VERSION)
{
    a.Serialize(os, (int)nType, nVersion);
}

template<typename Stream, typename T>
inline void Unserialize(Stream& is, T& a, long nType, int nVersion = VERSION)
{
    a.Unserialize(is, (int)nType, nVersion);
}







//
// Allocator that clears its contents before deletion
//
template<typename T>
struct secure_allocator : public std::allocator<T> {
    using base = std::allocator<T>;
    using size_type = typename base::size_type;
    using difference_type = typename base::difference_type;
    using pointer = typename base::pointer;
    using const_pointer = typename base::const_pointer;
    using reference = typename base::reference;
    using const_reference = typename base::const_reference;
    using value_type = typename base::value_type;

    // Constructors
    secure_allocator() noexcept = default;

    template<typename U>
    secure_allocator(const secure_allocator<U>&) noexcept {}

    // Rebind facility for containers
    template<typename U>
    struct rebind {
        using other = secure_allocator<U>;
    };

    // The key method that zeros memory before deallocation
    void deallocate(T* p, std::size_t n) {
        if (p != nullptr) {
            // Securely clear memory before deallocation
            std::memset(static_cast<void*>(p), 0, sizeof(T) * n);
        }
        base::deallocate(p, n);
    }

    // C++17 and later require this comparison operator
    bool operator==(const secure_allocator&) const noexcept {
        return true;
    }

    bool operator!=(const secure_allocator&) const noexcept {
        return false;
    }
};




//
// Support for IMPLEMENT_SERIALIZE and READWRITE macro
//
class CSerActionGetSerializeSize {};
class CSerActionSerialize {};
class CSerActionUnserialize {};

template<typename Stream, typename T>
inline unsigned int SerReadWrite(Stream& s, const T& obj, int nType, int nVersion, CSerActionGetSerializeSize ser_action)
{
    return ::GetSerializeSize(obj, nType, nVersion);
}

//template<typename Stream, typename T>
//inline unsigned int SerReadWrite(Stream& s, const T& obj, int nType, int nVersion, CSerActionSerialize ser_action)
//{
//    ::Serialize(s, obj, nType, nVersion);
//    return 0;
//}
//
//template<typename Stream, typename T>
//inline unsigned int SerReadWrite(Stream& s, T& obj, int nType, int nVersion, CSerActionUnserialize ser_action)
//{
//    ::Unserialize(s, obj, nType, nVersion);
//    return 0;
//}

struct ser_streamplaceholder
{
    int nType;
    int nVersion;
};


#define IMPLEMENT_SERIALIZE(statements) \
    unsigned int GetSerializeSize(int nType=0, int nVersion=VERSION) const \
    {                                      \
        CSerActionGetSerializeSize ser_action; \
        const bool fGetSize = true;            \
        const bool fWrite = false;             \
        const bool fRead = false;              \
        unsigned int nSerSize = 0;             \
        ser_streamplaceholder s;               \
        s.nType = nType;                       \
        s.nVersion = nVersion;                 \
        {statements}                           \
        return nSerSize;                       \
    }                                          \
    template<typename Stream>                  \
    void Serialize(Stream& s, int nType=0, int nVersion=VERSION) const \
    {                                          \
        CSerActionSerialize ser_action;        \
        const bool fGetSize = false;            \
        const bool fWrite = true;              \
        const bool fRead = false;              \
        unsigned int nSerSize = 0;             \
        {statements}                           \
    }                                          \
    template<typename Stream>                  \
    void Unserialize(Stream& s, int nType=0, int nVersion=VERSION) \
    {                                          \
        CSerActionSerialize ser_action;        \
        const bool fGetSize = false;           \
        const bool fWrite   = true;            \
        const bool fRead    = false;           \
        unsigned int nSerSize = 0;             \
        {statements}                           \
    }                                          \

#define READWRITE(obj)      (nSerSize += ::SerReadWrite(s, (obj), nType, nVersion, ser_action))




class CFlatData
{
protected:
    char* pbegin;
    char* pend;
public:
    CFlatData(void* pbeginIn, void* pendIn) : pbegin((char*)pbeginIn), pend((char*)pendIn) {}
    char* begin() { return pbegin; }
    const char* begin() const { return pbegin; }
    char* end() { return pend; }
    const char* end() const { return pend; }

    unsigned int GetSerializeSize(int, int = 0) const
    {
        return pend - pbegin;
    }

    template<typename Stream>
    void Serialize(Stream& s, int, int = 0) const
    {
        s.write(pbegin, pend - pbegin);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int, int = 0)
    {
        s.read(pbegin, pend - pbegin);
    }
};

template<typename T>
inline T& REF(const T& val)
{
    return (T&)val;
}

//
// Wrapper for serializing arrays and POD
// 
#define FLATDATA(obj)   REF(CFlatData((char*)&(obj), (char*)&(obj) + sizeof(obj)))



