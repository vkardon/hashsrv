//
// session.hpp
//
#ifndef __SESSION_HPP__
#define __SESSION_HPP__

#include "hasher.hpp"
#include <asio.hpp>

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(asio::ip::tcp::socket socket) : mClientSocket(std::move(socket)) {}
    ~Session() {}

    void Start() { ReadData(); }

private:
    void ReadData();
    void SendData(std::string&& response);
    bool ProcessBuffer(const char* data, std::size_t bytes, std::string& hexOut);

    asio::ip::tcp::socket mClientSocket;
    std::vector<char> mTailBuffer;
    Hasher mHasher;

    // Support test classes
    friend class SessionTestWrapper;
};

inline void Session::ReadData()
{
    std::shared_ptr<Session> self(shared_from_this());

    // If we have leftover data from the last read, process it first
    if(!mTailBuffer.empty()) 
    {
        // Move tail to a local so we can clear the member.
        // This is to ensure that if ProcessBuffer finds another tail,
        // it will not collide with the one being currently processed.
        std::vector<char> temp;
        temp.swap(mTailBuffer);
        std::string hexOut;

        if(ProcessBuffer(temp.data(), temp.size(), hexOut))
        {
            SendData(std::move(hexOut)); // Send completed hash
            return; // Exit and wait for SendData to call ReadData again
        }

        // If ProcessBuffer returned false, the data is now "inside"
        // the mHasher state. The tail is now empty (because of swap).
        // We safely proceed with the async read.
    }

    // Use a fixed 4KB buffer (matching OS page size) to enforce strict memory 
    // limits per session. This prevents "Memory Exhaustion" attacks where 
    // a client sends a massive stream without a newline to crash the server.
    auto buffer = std::make_shared<std::array<char, 4096>>();

    mClientSocket.async_read_some(asio::buffer(*buffer),
        [this, self, buffer](std::error_code ec, std::size_t bytesTransferred)
        {
            if(!ec) 
            {
                std::string hexOut;
                if(ProcessBuffer(buffer->data(), bytesTransferred, hexOut))
                {
                    SendData(std::move(hexOut)); // Send completed hash
                }
                else
                {
                    ReadData(); // No newline found, data is in Hasher; keep reading
                }
            }
            else if(ec != asio::error::eof && ec != asio::error::operation_aborted)
            {
                // Handle/Log disconnect or error - session will die here
                std::cerr << "Read error: " << ec.message() << std::endl;
            }
            else
            {
                // Graceful disconnect or abort - session dies naturally
            }
        });
}

bool Session::ProcessBuffer(const char* data, std::size_t bytes, std::string& hexOut)
{
    // Find new line delimeter
    const char* delimiter = std::find(data, data + bytes, '\n');

    if(delimiter != data + bytes)
    {
        // Found the delimiter!
        std::size_t fragmentSize = std::distance(data, delimiter);

        // Handle \r if it's there
        if(fragmentSize > 0 && data[fragmentSize - 1] == '\r') 
            fragmentSize--;

        if(fragmentSize > 0)
            mHasher.Update(data, fragmentSize);

        // Finalize the hash 
        hexOut = mHasher.FinalizeHex();

        // Capture the leftovers:
        // If we read more than one message, save anything after the '\n' into
        // the tail buffer. When ReadData() is called again, it will process these
        // leftovers before reading from the network again.
        if(std::distance(delimiter + 1, data + bytes) > 0) 
        {
            mTailBuffer.assign(delimiter + 1, data + bytes);
        }
        else
        {
            mTailBuffer.clear(); // Clear the tail buffer if nothing left
        }

        // We have completed the hash
        return true;
    }
    else
    {
        // Update hasher with this data fragment and keep reading
        mHasher.Update(data, bytes);
        return false;
    }
}

inline void Session::SendData(std::string&& response)
{
    std::shared_ptr<Session> self(shared_from_this());

    // Note: We capture the string by value (moving it) into
    // the lambda so it lives until the async operation completes
    asio::async_write(mClientSocket, asio::buffer(response),
        [this, self, response = std::move(response)](std::error_code ec, std::size_t /*length*/)
        {
            if(!ec)
            {
                ReadData();
            }
        });
}

#endif // __SESSION_HPP__

