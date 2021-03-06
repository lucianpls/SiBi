#pragma once

#include <cinttypes>
#include <vector>
#include <algorithm>
// bitstream in low endian format, up to 64 bits at a time
struct Bitstream {
    std::vector<uint8_t>& v;
    size_t bitp; // Next bit for input, absolute number
    Bitstream(std::vector<uint8_t>& data) : v(data), bitp(0) {};

    void clear() {
        bitp = 0;
        v.clear();
    }

    // Do not call with val having bits above "bits" set, the results will be corrupt
    template<typename T>
    void push(T val, size_t bits) {
        static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
            "Only works for unsigned integral types");
        while (bits) {
            size_t used = std::min(8 - bitp, bits);
            if (bitp)
                v.back() |= static_cast<uint8_t>(val << bitp);
            else
                v.push_back(static_cast<uint8_t>(val));
            bits -= used;
            bitp = (bitp + used) & 7;
            val >>= used;
        }
    }

    template<typename T = uint64_t>
    bool pull(T& val, size_t bits = 1) {
        static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value, 
            "Only works for unsigned integral types");
        val = 0;
        size_t accsz = 0; // bits in accumulator
        while (bits && (bitp < v.size() * 8)) {
            size_t used = std::min(8 - bitp % 8, bits);
            val |= static_cast<T>((v[bitp / 8] >> (bitp % 8)) & mask[used]) << accsz;
            bits -= used;
            bitp += used;
            accsz += used;
        }
        return bitp <= v.size() * 8;
    }

    // Single bit fetch
    uint64_t get() {
        if (bitp >= 8 * v.size()) return 0; // Don't go past the end
        uint64_t val = static_cast<uint64_t>((v[bitp / 8] >> (bitp % 8)) & 1);
        bitp++;
        return val;
    }

    static const uint8_t mask[9];
};

class BMap {
public:
    BMap(int x, int y);
    ~BMap() {};
    bool bit(int x, int y) {
        return 0 != (v[unit(x, y)] & bitmask(x, y));
    }
    void set(int x, int y) {
        v[unit(x, y)] |= bitmask(x, y);
    }
    void clear(int x, int y) {
        v[unit(x, y)] &= ~(bitmask(x, y));
    }
    size_t dsize() { return v.size() * sizeof(uint64_t); }
    void getsize(int& x, int& y) { x = _x; y = _y; }
    size_t pack(Bitstream& stream);
    size_t unpack(Bitstream& streamm);
    bool compare(BMap& other) {
        if (v.size() != other.v.size())
            return false;
        for (int i = 0; i < v.size(); i++)
            if (v[i] != other.v[i])
                return false;
        return true;
    };
private:
    size_t unit(int x, int y) {
        return _lw * (y / 8) + x / 8;
    }
    uint64_t bitmask(int x, int y) {
        static const uint8_t _xy[64] = {
            0,  1,  4,  5, 16, 17, 20, 21,
            2,  3,  6,  7, 18, 19, 22, 23,
            8,  9, 12, 13, 24, 25, 28, 29,
           10, 11, 14, 15, 26, 27, 30, 31,
           32, 33, 36, 37, 48, 49, 52, 53,
           34, 35, 38, 39, 50, 51, 54, 55,
           40, 41, 44, 45, 56, 57, 60, 61,
           42, 43, 46, 47, 58, 59, 62, 63
        };
        return 1ull << _xy[((y & 7) * 8) + (x & 7)];
    }
    int _x, _y;
    size_t _lw; // Line width
    std::vector<uint64_t> v;
};

void RLE(std::vector<uint8_t>& v, std::vector<uint8_t>& result);
void unRLE(std::vector<uint8_t>& v, std::vector<uint8_t>& result);