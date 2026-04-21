//
// main.cpp
//
#include "server.hpp"

int main()
{
    try
    {
        Server server(8080);
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