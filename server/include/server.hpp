//
// server.hpp
//
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include <asio.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include "session.hpp"

using asio::ip::tcp;

class Server
{
public:
    Server(unsigned short port);
    ~Server() = default;

    void Run();

private:
    void DoAccept();
    void WaitForSignals();

    // Note: mIoContext must be initialized before mAcceptor (Order of members matters)
    asio::io_context mIoContext;
    asio::signal_set mSignals;
    tcp::acceptor mAcceptor;
    std::vector<std::thread> mThreads;
    unsigned short mPort{0};
};

inline Server::Server(unsigned short port) 
    : mSignals(mIoContext),
      mAcceptor(mIoContext), 
      mPort(port)
{
    // Manual setup to allow high-concurrency options (max backlog)
    mAcceptor.open(tcp::v4());
    mAcceptor.set_option(tcp::acceptor::reuse_address(true));
    mAcceptor.bind(tcp::endpoint(tcp::v4(), port));
    mAcceptor.listen(asio::socket_base::max_listen_connections); // set the backlog to maximum

    mSignals.add(SIGINT);
    mSignals.add(SIGTERM);
    mSignals.add(SIGHUP);
    mSignals.add(SIGQUIT);
}

inline void Server::Run()
{
    WaitForSignals();   // Start listening for signals
    DoAccept();         // Setup the first listener

    unsigned int threadCount = std::thread::hardware_concurrency();
    std::cout << "Server starting on port " << mPort << " with " << threadCount << " threads..." << std::endl;

    // Create the worker threads
    for(unsigned int i = 0; i < threadCount; ++i)
    {
        mThreads.emplace_back([this] { mIoContext.run(); });
    }

    // Wait for all threads to finish (which happens when mIoContext.stop() is called)
    for(auto& t : mThreads)
    {
        if(t.joinable())
            t.join();
    }
}

inline void Server::DoAccept()
{
    mAcceptor.async_accept(
        [this](std::error_code ec, tcp::socket socket)
        {
            if(!ec)
            {
                std::make_shared<Session>(std::move(socket))->Start();
            }
            DoAccept();
        });
}

inline void Server::WaitForSignals()
{
    mSignals.async_wait(
        [this](std::error_code ec, int signalNumber)
        {
            if (!ec)
            {
                std::cout << "\nShutdown signal received (" << signalNumber << "). Closing server..." << std::endl;
                mIoContext.stop(); // This tells all worker threads to exit run()
            }
        });
}

#endif // __SERVER_HPP__