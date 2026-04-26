// 
// sessionTest.cpp
//
#include <gtest/gtest.h>
#include "sessionTestWrapper.hpp"
#include <asio.hpp>

// Verify hex conversion correctness
TEST(SessionTest, HexConversionIsCorrect) 
{
    asio::io_context ioc;
    asio::ip::tcp::socket socket(ioc);
    auto session = std::make_shared<SessionTestWrapper>(std::move(socket));

    // Test with a known value: 0xDE 0xAD 0xBE 0xEF
    unsigned char mockHash[] = {0xDE, 0xAD, 0xBE, 0xEF};

    std::string result;
    session->HexToString(result, mockHash, 4);

    // Expecting "deadbeef\n" (including your newline terminator)
    EXPECT_EQ(result, "deadbeef\n");
}

// Multi-Message Packets: Ensure that if Msg1\nMsg2 arrives in one read, both are processed
TEST(SessionTest, HandlesMultipleMessagesInOnePacket)
{
    asio::io_context ioc;
    asio::ip::tcp::socket socket(ioc);
    auto session = std::make_shared<SessionTestWrapper>(std::move(socket));

    // Scenario: A single TCP packet arrives containing one full message 
    // and the start of a second message.
    const char* input = "Part1\nPart2";
    session->ProcessBuffer(input, strlen(input));

    // The tail buffer should contain "Part2"
    // Because ProcessBuffer finds the first '\n', hashes "Part1", 
    // and saves everything after the '\n' into mTailBuffer.
    
    // 1. Verify the size
    EXPECT_EQ(session->mTailBuffer.size(), 5);

    // 2. Verify the content (Directly comparing vector to string contents)
    std::string tailContent(session->mTailBuffer.begin(), session->mTailBuffer.end());
    EXPECT_EQ(tailContent, "Part2");    

    // 3. Simulate the finishing newline arriving in the next packet (next read cycle)
    session->ProcessBuffer("\n", 1);
    
    // After processing the newline, the tail should now be empty
    EXPECT_TRUE(session->mTailBuffer.empty());
}

// Verify that messages arriving byte-by-byte are correctly reassembled
TEST(SessionTest, HandlesExtremeFragmentation) 
{
    asio::io_context ioc;
    asio::ip::tcp::socket socket(ioc);
    auto session = std::make_shared<SessionTestWrapper>(std::move(socket));

    // Simulate "Hi\n" arriving in 3 separate packets

    // 1. Send "H" - It gets hashed immediately, tail stays empty
    session->ProcessBuffer("H", 1);
    EXPECT_EQ(session->mTailBuffer.size(), 0); 

    // 2. Send "i" - It gets hashed immediately, tail stays empty
    session->ProcessBuffer("i", 1);
    EXPECT_EQ(session->mTailBuffer.size(), 0);

    // 3. Send "\n" - This triggers the finalization
    // Since we are in a test and can't easily "read" the async write,
    // we can check if the hash context was reset or if the tail is still empty.
    session->ProcessBuffer("\n", 1);
    EXPECT_EQ(session->mTailBuffer.size(), 0);
}

// Verify request 'tail' logic when '\n' is present
TEST(SessionTest, TailOnlyHoldsLeftovers) 
{
    asio::io_context ioc;
    asio::ip::tcp::socket socket(ioc);
    auto session = std::make_shared<SessionTestWrapper>(std::move(socket));

    // Send "Hi\nBye"
    // "Hi" is hashed, "\n" triggers finalization, "Bye" goes to tail.
    session->ProcessBuffer("Hi\nBye", 6);

    EXPECT_EQ(session->mTailBuffer.size(), 3);
    
    std::string tail(session->mTailBuffer.begin(), session->mTailBuffer.end());
    EXPECT_EQ(tail, "Bye");
}

// Verify request 'tail' logic when '\n' is not present
TEST(SessionTest, StreamingWithoutNewlinesDoesNotFillTail) 
{
    asio::io_context ioc;
    asio::ip::tcp::socket socket(ioc);
    auto session = std::make_shared<SessionTestWrapper>(std::move(socket));

    // Send a large-ish chunk without a newline
    std::string largeInput(1000, 'A');
    session->ProcessBuffer(largeInput.c_str(), largeInput.size());

    // Because we use incremental hashing (EVP_DigestUpdate), 
    // the 'A's should be consumed. mTailBuffer should NOT grow.
    EXPECT_EQ(session->mTailBuffer.size(), 0);
}

// Verify en empty request processing
TEST(SessionTest, HandlesEmptyMessages) 
{
    asio::io_context ioc;
    asio::ip::tcp::socket socket(ioc);
    auto session = std::make_shared<SessionTestWrapper>(std::move(socket));

    // Sending just a newline should result in the hash of an empty string
    // SHA256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    session->ProcessBuffer("\n", 1);
    
    EXPECT_EQ(session->mTailBuffer.size(), 0);
}

// Verify handling messaged terminated with Windows-style "\r\n"
TEST(SessionTest, HandlesWindowsLineEndings) 
{
    asio::io_context ioc;
    asio::ip::tcp::socket socket(ioc);
    auto session = std::make_shared<SessionTestWrapper>(std::move(socket));

    // "Test\r\n" should hash exactly the same as "Test\n"
    // We verify by ensuring the tail is cleared correctly and no \r remains
    session->ProcessBuffer("Test\r\n", 6);
    
    EXPECT_TRUE(session->mTailBuffer.empty());
    // If you add the result-capturing spy we discussed, 
    // you'd verify the hash matches "Test" (4 chars) not "Test\r" (5 chars).
}
