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

// Helper to generate a payload, send it to server and verify the response hash
void ExchangeAndVerify(asio::ip::tcp::socket& clientSocket, int id = 0)
{
    // Generate unique payload and calculate the expected result
    std::string inputData = "Calculate_SHA256_Integrity_Check: Request_ID_" + 
                                std::to_string(id) + "_" + std::to_string(std::rand());
    std::string expected = CalculateExpected(inputData);

    // Append newline to trigger ProcessBuffer logic and write to the stream
    std::string request = inputData + "\n";
    std::error_code ec;
    asio::write(clientSocket, asio::buffer(request), ec);
    ASSERT_FALSE(ec) << "Write failed on id " << id;

    // Read the specific response for this request
    asio::streambuf responseBuf;
    asio::read_until(clientSocket, responseBuf, "\n", ec);
    ASSERT_FALSE(ec) << "Read failed for: " << inputData;

    // Verify
    std::string actual{asio::buffers_begin(responseBuf.data()), asio::buffers_end(responseBuf.data())};
    EXPECT_EQ(actual, expected) << "Server hash mismatch: for id=" << id << ", input='" << inputData << "'";
}

#endif  // __SESSION_TEST_WRAPPER_HPP__
