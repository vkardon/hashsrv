// 
// sessionTest.cpp
//
#include <gtest/gtest.h>
#include <asio.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include "server.hpp"

class ServerTest : public ::testing::Test 
{
protected:
    // Helper to find a random available port from the OS
    unsigned short get_free_port()
    {
        asio::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
        return acceptor.local_endpoint().port();
    }
};

// Test Lifecycle: Start and Stop via Signal
TEST_F(ServerTest, LifecycleGracefulShutdown) 
{
    unsigned short port = get_free_port();
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
    unsigned short port = get_free_port();
    Server server(port);

    std::thread serverThread([&server]() 
    {
        server.Run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Attempt a client connection
    asio::io_context clientIoc;
    tcp::socket clientSocket(clientIoc);
    std::error_code ec;
    
    clientSocket.connect(tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port), ec);
    
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
    unsigned short port = get_free_port();
    Server server(port);

    std::thread serverThread([&server]()
    {
        server.Run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    const int numClients = 10;
    std::vector<std::unique_ptr<tcp::socket>> clients;
    asio::io_context clientIoc;

    // Connect multiple clients simultaneously
    for(int i = 0; i < numClients; ++i)
    {
        auto sock = std::make_unique<tcp::socket>(clientIoc);
        std::error_code ec;
        sock->connect(tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port), ec);
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
    unsigned short port = get_free_port();
    
    // Bind a socket to the port manually to "steal" it
    asio::io_context ioc;
    tcp::acceptor blocker(ioc, tcp::endpoint(tcp::v4(), port));

    // The Server constructor should throw system_error (EADDRINUSE)
    EXPECT_THROW({ Server server(port); }, asio::system_error);
}

TEST_F(ServerTest, HandlesThunderingHerd) 
{
    unsigned short port = get_free_port();
    Server server(port);
    std::thread serverThread([&server]()
    { 
        server.Run(); 
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connect significantly more clients than hardware threads
    const int burstSize = std::thread::hardware_concurrency() * 10;
    std::vector<std::unique_ptr<tcp::socket>> clients;
    asio::io_context clientIoc;

    for(int i = 0; i < burstSize; ++i)
    {
        auto sock = std::make_unique<tcp::socket>(clientIoc);
        std::error_code ec;
        sock->connect(tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port), ec);
        EXPECT_FALSE(ec);
        clients.push_back(std::move(sock));
    }

    // Cleanup
    raise(SIGINT);
    if(serverThread.joinable())
        serverThread.join();
}