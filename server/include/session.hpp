//
// session.hpp
//
#ifndef __SESSION_HPP__
#define __SESSION_HPP__

#include <asio.hpp>
#include <openssl/evp.h>

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(asio::ip::tcp::socket socket);
    ~Session();

    void Start() { ReadData(); }

private:
    void ReadData();
    void SendData(const std::string& response);

    void ProcessBuffer(const char* data, std::size_t bytes);
    void HexToString(std::string& dest, const void* hash, int len) const;

    asio::ip::tcp::socket mClientSocket;
    EVP_MD_CTX* mHashCtx{nullptr};
    std::vector<char> mTailBuffer;

    // Support test classes
    friend class SessionTestWrapper;
};

inline Session::Session(asio::ip::tcp::socket socket) : mClientSocket(std::move(socket)) 
{
    mHashCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mHashCtx, EVP_sha256(), nullptr); // SHA-256 default hash type
}

inline Session::~Session()
{
    EVP_MD_CTX_free(mHashCtx);
}

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
        ProcessBuffer(temp.data(), temp.size());
        return;
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
                ProcessBuffer(buffer->data(), bytesTransferred);
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

void Session::ProcessBuffer(const char* data, std::size_t bytes)
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
            EVP_DigestUpdate(mHashCtx, data, fragmentSize);

        // Finalize the hash
        unsigned char hash[EVP_MAX_MD_SIZE]{0};
        unsigned int len{0};
        EVP_DigestFinal_ex(mHashCtx, hash, &len);

        // Reset Context for next string
        EVP_DigestInit_ex(mHashCtx, EVP_sha256(), nullptr);

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

        // Send the result
        std::string hexResult;
        HexToString(hexResult, hash, len); 
        SendData(hexResult);
    }
    else
    {
        EVP_DigestUpdate(mHashCtx, data, bytes);
        ReadData(); 
    }
}

inline void Session::SendData(const std::string& response)
{
    std::shared_ptr<Session> self(shared_from_this());
    
    asio::async_write(mClientSocket, asio::buffer(response),
        [this, self](std::error_code ec, std::size_t /*length*/)
        {
            if(!ec)
            {
                ReadData();
            }
        });
}

inline void Session::HexToString(std::string& dest, const void* hash, int len) const
{
    // Converts raw binary hash data into a hexadecimal string and appends a newline.
    // This method manages its own memory by resizing the 'dest' string to accommodate
    // the new characters (2 hex digits per byte + 1 for '\n').

    // Lookup table for hex digits
    static const char* lut = "0123456789abcdef";

    // Reserve to accommodate hex characters plus the trailing newline
    dest.clear();
    dest.reserve((len * 2) + 1);

    // Tight loop using bit-shifting (fastest way to split a byte)
    const uint8_t* data = static_cast<const uint8_t*>(hash);
    for(int i = 0; i < len; ++i) 
    {
        dest.push_back(lut[data[i] >> 4]);   // High nibble
        dest.push_back(lut[data[i] & 0x0f]); // Low nibble
    }

    // Append newline terminator
    dest.push_back('\n');
}


#endif // __SESSION_HPP__

