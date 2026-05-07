#ifndef __HASHER_HPP__
#define __HASHER_HPP__

#include <openssl/evp.h>

class Hasher
{
public:
    Hasher();
    ~Hasher();

    void Reset();
    void Update(const void* data, size_t len);
    void Update(const std::string& data) { Update(data.c_str(), data.length()); }
    std::string FinalizeHex();
    std::string HexToString(const void* hash, size_t len) const;

private:
    EVP_MD_CTX* mCtx { nullptr };
};

inline Hasher::Hasher()
{
    mCtx = EVP_MD_CTX_new();
    Reset();
}

inline Hasher::~Hasher()
{
    if(mCtx)
        EVP_MD_CTX_free(mCtx);
}

inline void Hasher::Reset()
{
    EVP_DigestInit_ex(mCtx, EVP_sha256(), nullptr);
}

inline void Hasher::Update(const void* data, size_t len)
{
    EVP_DigestUpdate(mCtx, data, len);
}

inline std::string Hasher::FinalizeHex()
{
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    EVP_DigestFinal_ex(mCtx, hash, &len);

    // Reset immediately so the object is ready for the next hash
    Reset();

    // Convert to a human-readable hexadecimal string
    return HexToString(hash, len);
}

inline std::string Hasher::HexToString(const void* hash, size_t len) const
{
    // Converts raw binary hash data into a hexadecimal string and appends a newline.
 
    // Lookup table for hex digits
    static const char* lut = "0123456789abcdef";

    // Tight loop using bit-shifting (fastest way to split a byte)
    char hexBuffer[EVP_MAX_MD_SIZE * 2 + 1];
    char* ptr = hexBuffer;
    const uint8_t* data = static_cast<const uint8_t*>(hash);

    for(size_t i = 0; i < len; ++i)
    {
        *ptr++ = lut[data[i] >> 4];         // High nibble
        *ptr++ = lut[data[i] & 0x0f];       // Low nibble
    }

    len = len * 2;
    hexBuffer[len++] = '\n';
    return std::string(hexBuffer, len);
}

#endif // __HASHER_HPP__
