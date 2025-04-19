#pragma once
#include <bits/stdc++.h>


typedef long long  int64;
typedef unsigned long long  uint64;
static const int VERSION = 101; // 101 for now


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


typedef std::vector<unsigned char, secure_allocator<unsigned char>> CPrivKey;

