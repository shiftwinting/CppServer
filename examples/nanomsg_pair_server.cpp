/*!
    \file nanomsg_pair_server.cpp
    \brief Nanomsg pair server example
    \author Ivan Shynkarenka
    \date 02.02.2017
    \copyright MIT License
*/

#include "server/nanomsg/pair_server.h"

#include <iostream>
#include <memory>

class ExamplePairServer : public CppServer::Nanomsg::PairServer
{
public:
    using CppServer::Nanomsg::PairServer::PairServer;

protected:
    void onStarted() override
    {
        std::cout << "Nanomsg pair server started!" << std::endl;
    }

    void onStopped() override
    {
        std::cout << "Nanomsg pair server stopped!" << std::endl;
    }

    void onReceived(CppServer::Nanomsg::Message& message) override
    {
        std::cout << "Incoming: " << message << std::endl;

        // Send the reversed message back to the client
        std::string result(message.string());
        Send(std::string(result.rbegin(), result.rend()));
    }

    void onError(int error, const std::string& message) override
    {
        std::cout << "Nanomsg pair server caught an error with code " << error << "': " << message << std::endl;
    }
};

int main(int argc, char** argv)
{
    // Nanomsg pair server address
    std::string address = "tcp://127.0.0.1:6667";
    if (argc > 1)
        address = std::atoi(argv[1]);

    std::cout << "Nanomsg pair server address: " << address << std::endl;

    // Create a new Nanomsg pair server
    auto server = std::make_shared<ExamplePairServer>(address);

    // Start the server
    std::cout << "Server starting...";
    server->Start();
    std::cout << "Done!" << std::endl;

    std::cout << "Press Enter to stop the server or '!' to restart the server..." << std::endl;

    // Perform text input
    std::string line;
    while (getline(std::cin, line))
    {
        if (line.empty())
            break;

        // Restart the server
        if (line == "!")
        {
            std::cout << "Server restarting...";
            server->Restart();
            std::cout << "Done!" << std::endl;
            continue;
        }

        // Send the entered text to the client
        server->Send(line);
    }

    // Stop the server
    std::cout << "Server stopping...";
    server->Stop();
    std::cout << "Done!" << std::endl;

    return 0;
}
