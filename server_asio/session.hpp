//
// session.hpp
//
#ifndef __SESSION_HPP__
#define __SESSION_HPP__

#include <asio.hpp>
#include <openssl/evp.h>

using asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket);
    ~Session();

    void Start() { ReadData(); }

private:
    void ReadData();
    void SendData(const std::string& response);
    void HexToString(std::string& dest, const void* hash, int len) const;

    tcp::socket mClientSocket;
    asio::streambuf mStreamBuffer;
    EVP_MD_CTX* mHashCtx{nullptr};
};

inline Session::Session(tcp::socket socket) : mClientSocket(std::move(socket)) 
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
    
    // async_read_until searches for the delimiter '\n' (ASCII 10)
    asio::async_read_until(mClientSocket, mStreamBuffer, '\n',
        [this, self](std::error_code ec, std::size_t bytesTransferred)
        {
            if(!ec)
            {
                std::istream is(&mStreamBuffer);
                std::string line;
                
                // If the client sends 5 lines at once, this handles them in order
                if(std::getline(is, line)) 
                {
                    if(line.empty()) 
                    {
                        ReadData(); // Skip empty line and look for the next one
                        return;
                    }

                    // Adjust for potential \r
                    if(line.back() == '\r') 
                        line.pop_back();

                    // Reset Context for fresh hash
                    EVP_DigestInit_ex(mHashCtx, EVP_sha256(), nullptr);
                    
                    // Hash the data
                    EVP_DigestUpdate(mHashCtx, line.data(), line.length());

                    unsigned char hash[EVP_MAX_MD_SIZE]{0};
                    unsigned int len{0};
                    EVP_DigestFinal_ex(mHashCtx, hash, &len);

                    // Convert and Send
                    std::string hexResult;
                    HexToString(hexResult, hash, len); 
                    SendData(hexResult);
                }
                else 
                {
                    // Fallback: If we were woken up but couldn't get a line,
                    // keep the session alive by reading more.
                    ReadData();
                }
            }
            else 
            {
                // TODO: Handle/Log disconnect or error - session will die here
            }
        });
}

inline void Session::SendData(const std::string& response)
{
    std::shared_ptr<Session> self(shared_from_this());
    
    asio::async_write(mClientSocket, asio::buffer(response),
        [this, self](std::error_code ec, std::size_t /*length*/)
        {
            if(!ec)
            {
                // Using SHA-256 as the default hash type
                EVP_DigestInit_ex(mHashCtx, EVP_sha256(), nullptr); // SHA-256 default hash type
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