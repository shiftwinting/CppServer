/*!
    \file nanomsg_request_server.cpp
    \brief Nanomsg request server example
    \author Ivan Shynkarenka
    \date 02.02.2017
    \copyright MIT License
*/

#include "server/nanomsg/request_server.h"

#include <iostream>
#include <memory>

class ExampleRequestServer : public CppServer::Nanomsg::RequestServer
{
public:
    using CppServer::Nanomsg::RequestServer::RequestServer;

protected:
    void onStarted() override
    {
        std::cout << "Nanomsg request server started!" << std::endl;
    }

    void onStopped() override
    {
        std::cout << "Nanomsg request server stopped!" << std::endl;
    }

    void onReceived(CppServer::Nanomsg::Message& msg) override
    {
        std::string message((const char*)msg.buffer(), msg.size());
        std::cout << "Incoming: " << message << std::endl;

        // Send the reversed message back to the client
        Send(std::string(message.rbegin(), message.rend()));
    }

    void onError(int error, const std::string& message) override
    {
        std::cout << "Nanomsg request server caught an error with code " << error << "': " << message << std::endl;
    }
};

int main(int argc, char** argv)
{
    // Nanomsg request server address
    std::string address = "tcp://*:6668";
    if (argc > 1)
        address = std::atoi(argv[1]);

    std::cout << "Nanomsg request server address: " << address << std::endl;
    std::cout << "Press Enter to stop the server or '!' to restart the server..." << std::endl;

    // Create a new Nanomsg request server
    auto server = std::make_shared<ExampleRequestServer>(address);

    // Start the server
    server->Start();

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
    }

    // Stop the server
    server->Stop();

    return 0;
}