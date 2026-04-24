//
// main.cpp
//
#include <iostream>
#include <string>
#include "server.hpp"

int main(int argc, char* argv[])
{
    try
    {
        // Default to 8080, but override if an argument is provided
        unsigned short port = 8080;
        if(argc > 1) 
        {
            int portTmp = std::stoi(argv[1]);
            if(portTmp < 1 || portTmp > 65535)
                throw std::out_of_range("Port number " + std::to_string(portTmp) + " is out of valid range (1-65535)");
            port = static_cast<unsigned short>(portTmp);          
        }

        std::cout << "Starting server on port " << port << "..." << std::endl;
        Server server(port);
        server.Run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Runtime exception: " << e.what() << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cerr << "Unknown c++ exception" << std::endl;
        return 1;
    }

    return 0;
}