// 
// sessionTest.cpp
//
#include <gtest/gtest.h>
#include <asio.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include "server.hpp"
#include "sessionTestWrapper.hpp"

class ServerTest : public ::testing::Test 
{
public:
    ServerTest() { std::srand(std::time(nullptr)); }

protected:
    // Helper to find a random available port from the OS
    unsigned short GetFreePort()
    {
        asio::io_context ioc;
        asio::ip::tcp::acceptor acceptor(ioc, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
        return acceptor.local_endpoint().port();
    }
};

// Test Lifecycle: Start and Stop via Signal
TEST_F(ServerTest, LifecycleGracefulShutdown) 
{
    unsigned short port = GetFreePort();
    Server server(port);

    std::thread serverThread([&server]() 
    {
        server.Run();
    });

    // Give the threads a moment to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Simulate the user pressing Ctrl+C
    std::cout << "[ Test ] Sending SIGINT to server..." << std::endl;
    raise(SIGINT);

    if(serverThread.joinable())
        serverThread.join();

    SUCCEED(); // If we joined, the io_context.stop() worked
}

// Test Connection: Can a client actually connect?
TEST_F(ServerTest, ClientConnectionSuccess) 
{
    unsigned short port = GetFreePort();
    Server server(port);

    std::thread serverThread([&server]() 
    {
        server.Run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Attempt a client connection
    asio::io_context clientIoc;
    asio::ip::tcp::socket clientSocket(clientIoc);
    std::error_code ec;
    
    clientSocket.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port), ec);
    
    EXPECT_FALSE(ec);
    EXPECT_TRUE(clientSocket.is_open());

    // Cleanup
    clientSocket.close();
    raise(SIGINT);

    if(serverThread.joinable()) 
        serverThread.join();
}

// 3. Test Concurrency: Multiple clients at once
TEST_F(ServerTest, ConcurrentClientConnections) 
{
    unsigned short port = GetFreePort();
    Server server(port);

    std::thread serverThread([&server]()
    {
        server.Run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    const int numClients = 10;
    std::vector<std::unique_ptr<asio::ip::tcp::socket>> clients;
    asio::io_context clientIoc;

    // Connect multiple clients simultaneously
    for(int i = 0; i < numClients; ++i)
    {
        auto sock = std::make_unique<asio::ip::tcp::socket>(clientIoc);
        std::error_code ec;
        sock->connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port), ec);
        EXPECT_FALSE(ec);
        clients.push_back(std::move(sock));
    }

    EXPECT_EQ(clients.size(), numClients);

    // Shutdown
    raise(SIGINT);

    if(serverThread.joinable()) 
        serverThread.join();
}

// Test Error Handling: Port already in use
TEST_F(ServerTest, PortInUseThrows) 
{
    unsigned short port = GetFreePort();
    
    // Bind a socket to the port manually to "steal" it
    asio::io_context ioc;
    asio::ip::tcp::acceptor blocker(ioc, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));

    // The Server constructor should throw system_error (EADDRINUSE)
    EXPECT_THROW({ Server server(port); }, asio::system_error);
}

// Open a single connection, make multiple responses and verify the results
TEST_F(ServerTest, VerifyIndividualHashResponse) 
{
    // Setup Server on a dynamic port
    unsigned short port = GetFreePort();
    Server server(port);
    std::thread serverThread([&server]() { server.Run(); });
    
    // Allow io_context to spin up
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Establish connection
    asio::io_context clientIoc;
    asio::ip::tcp::socket clientSocket(clientIoc);
    std::error_code ec;
    clientSocket.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port), ec);
    ASSERT_FALSE(ec) << "Failed to connect to local server for integrity test.";

    // Send data
    const int requestCount = 100;
    for(int i = 0; i < requestCount; ++i)
    {
        ExchangeAndVerify(clientSocket, i);
    }

    // Cleanup (bring the server down with SIGINT)
    clientSocket.close();
    raise(SIGINT);
    if(serverThread.joinable())
        serverThread.join();
}

// Open multiple connections (significantly more than hardware threads).
// Make multiple responses from every connection and verify the results.
TEST_F(ServerTest, HandlesThunderingHerd) 
{
    unsigned short port = GetFreePort();
    Server server(port);
    std::thread serverThread([&server]() { server.Run(); });

    // Allow io_context to spin up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connect significantly more clients than hardware threads
    const int clientCount = std::thread::hardware_concurrency() * 10;
    std::vector<std::unique_ptr<asio::ip::tcp::socket>> clients;
    asio::io_context clientIoc;

    // Connection Phase (connect all clients)
    for(int i = 0; i < clientCount; ++i)
    {
        auto sock = std::make_unique<asio::ip::tcp::socket>(clientIoc);
        std::error_code ec;
        sock->connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port), ec);
        ASSERT_FALSE(ec) << "Client " << i << " failed to connect";
        clients.push_back(std::move(sock));
    }

    // Each client communicates with the server in parallel
    std::vector<std::future<void>> futures;
    const int requestsPerClient = 100;

    for(int i = 0; i < clientCount; ++i)
    {
        // Launch each client's conversation in its own thread
        futures.push_back(std::async(std::launch::async, [&sock = *clients[i], i, requestsPerClient]() 
            {
                for(int n = 0; n < requestsPerClient; ++n) 
                {
                    ExchangeAndVerify(sock, n);
                }
            }));
    }

    // Wait for clients to finish
    for (auto& f : futures)
        f.get(); 

    // Cleanup (bring the server down with SIGINT)
    raise(SIGINT);
    if(serverThread.joinable())
        serverThread.join();
}

