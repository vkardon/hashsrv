#ifndef __SESSION_TEST_WRAPPER_HPP__
#define __SESSION_TEST_WRAPPER_HPP__

#include "session.hpp"

// Create a wrapper to access protected/private members for testing
class SessionTestWrapper : public Session 
{
public:
    using Session::Session;
    using Session::ProcessBuffer;
    using Session::HexToString;
    using Session::mTailBuffer;
};

// Helper to consolidate EVP hashing and Hex conversion
inline std::string CalculateExpected(const std::string& input)
{
    // Perform binary SHA-256
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, input.data(), input.size());

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);

    // Convert to Hex using the production HexToString logic
    asio::io_context ioc;
    asio::ip::tcp::socket dummySocket(ioc);
    SessionTestWrapper wrapper(std::move(dummySocket));
    
    std::string hexResult;
    wrapper.HexToString(hexResult, hash, len);
    
    return hexResult;
}

#endif  // __SESSION_TEST_WRAPPER_HPP__
