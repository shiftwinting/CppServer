//
// Created by Ivan Shynkarenka on 27.12.2016
//

#include "catch.hpp"

#include "server/asio/udp_client.h"
#include "server/asio/udp_server.h"
#include "threads/thread.h"

#include <atomic>
#include <chrono>
#include <vector>

using namespace CppCommon;
using namespace CppServer::Asio;

class MulticastUDPService : public Service
{
public:
    std::atomic<bool> thread_initialize;
    std::atomic<bool> thread_cleanup;
    std::atomic<bool> started;
    std::atomic<bool> stopped;
    std::atomic<bool> idle;
    std::atomic<bool> error;

    explicit MulticastUDPService()
        : thread_initialize(false),
          thread_cleanup(false),
          started(false),
          stopped(false),
          idle(false),
          error(false)
    {
    }

protected:
    void onThreadInitialize() override { thread_initialize = true; }
    void onThreadCleanup() override { thread_cleanup = true; }
    void onStarted() override { started = true; }
    void onStopped() override { stopped = true; }
    void onIdle() override { idle = true; }
    void onError(int error, const std::string& category, const std::string& message) override { error = true; }
};

class MulticastUDPClient : public UDPClient
{
public:
    std::atomic<bool> connected;
    std::atomic<bool> disconnected;
    std::atomic<bool> error;

    explicit MulticastUDPClient(std::shared_ptr<MulticastUDPService> service, const std::string& address, int port, bool reuse_address)
        : UDPClient(service, address, port, reuse_address),
          connected(false),
          disconnected(false),
          error(false)
    {
    }

protected:
    void onConnected() override { connected = true; }
    void onDisconnected() override { disconnected = true; }
    void onError(int error, const std::string& category, const std::string& message) override { error = true; }
};

class MulticastUDPServer : public UDPServer
{
public:
    std::atomic<bool> started;
    std::atomic<bool> stopped;
    std::atomic<bool> error;

    explicit MulticastUDPServer(std::shared_ptr<MulticastUDPService> service, InternetProtocol protocol, int port)
        : UDPServer(service, protocol, port),
          started(false),
          stopped(false),
          error(false)
    {
    }

protected:
    void onStarted() override { started = true; }
    void onStopped() override { stopped = true; }
    void onError(int error, const std::string& category, const std::string& message) override { error = true; }
};

TEST_CASE("UDP server multicast", "[CppServer][Asio]")
{
    const std::string listen_address = "0.0.0.0";
    const std::string multicast_address = "239.255.0.1";
    const int multicast_port = 2223;

    // Create and start Asio service
    auto service = std::make_shared<MulticastUDPService>();
    REQUIRE(service->Start(true));
    while (!service->IsStarted())
        Thread::Yield();

    // Create and start multicast server
    auto server = std::make_shared<MulticastUDPServer>(service, InternetProtocol::IPv4, 0);
    REQUIRE(server->Start(multicast_address, multicast_port));
    while (!server->IsStarted())
        Thread::Yield();

    // Create and connect multicast client
    auto client1 = std::make_shared<MulticastUDPClient>(service, listen_address, multicast_port, true);
    REQUIRE(client1->Connect());
    while (!client1->IsConnected())
        Thread::Yield();

    // Join multicast group
    client1->JoinMulticastGroup(multicast_address);
    Thread::Sleep(100);

    // Multicast some data to all clients
    server->Multicast("test");

    // Wait for all data processed...
    while (client1->bytes_received() != 4)
        Thread::Yield();

    // Create and connect multicast client
    auto client2 = std::make_shared<MulticastUDPClient>(service, listen_address, multicast_port, true);
    REQUIRE(client2->Connect());
    while (!client2->IsConnected())
        Thread::Yield();

    // Join multicast group
    client2->JoinMulticastGroup(multicast_address);
    Thread::Sleep(100);

    // Multicast some data to all clients
    server->Multicast("test");

    // Wait for all data processed...
    while ((client1->bytes_received() != 8) || (client2->bytes_received() != 4))
        Thread::Yield();

    // Create and connect multicast client
    auto client3 = std::make_shared<MulticastUDPClient>(service, listen_address, multicast_port, true);
    REQUIRE(client3->Connect());
    while (!client3->IsConnected())
        Thread::Yield();

    // Join multicast group
    client3->JoinMulticastGroup(multicast_address);
    Thread::Sleep(100);

    // Multicast some data to all clients
    server->Multicast("test");

    // Wait for all data processed...
    while ((client1->bytes_received() != 12) || (client2->bytes_received() != 8) || (client3->bytes_received() != 4))
        Thread::Yield();

    // Leave multicast group
    client1->LeaveMulticastGroup(multicast_address);
    Thread::Sleep(100);

    // Disconnect the multicast client
    REQUIRE(client1->Disconnect());
    while (client1->IsConnected())
        Thread::Yield();

    // Multicast some data to all clients
    server->Multicast("test");

    // Wait for all data processed...
    while ((client1->bytes_received() != 12) || (client2->bytes_received() != 12) || (client3->bytes_received() != 8))
        Thread::Yield();

    // Leave multicast group
    client2->LeaveMulticastGroup(multicast_address);
    Thread::Sleep(100);

    // Disconnect the multicast client
    REQUIRE(client2->Disconnect());
    while (client2->IsConnected())
        Thread::Yield();

    // Multicast some data to all clients
    server->Multicast("test");

    // Wait for all data processed...
    while ((client1->bytes_received() != 12) || (client2->bytes_received() != 12) || (client3->bytes_received() != 12))
        Thread::Yield();

    // Leave multicast group
    client3->LeaveMulticastGroup(multicast_address);
    Thread::Sleep(100);

    // Disconnect the multicast client
    REQUIRE(client3->Disconnect());
    while (client3->IsConnected())
        Thread::Yield();

    // Stop the multicast server
    REQUIRE(server->Stop());
    while (server->IsStarted())
        Thread::Yield();

    // Stop the Asio service
    REQUIRE(service->Stop());
    while (service->IsStarted())
        Thread::Yield();

    // Check the Asio service state
    REQUIRE(service->thread_initialize);
    REQUIRE(service->thread_cleanup);
    REQUIRE(service->started);
    REQUIRE(service->stopped);
    REQUIRE(service->idle);
    REQUIRE(!service->error);

    // Check the multicast server state
    REQUIRE(server->started);
    REQUIRE(server->stopped);
    REQUIRE(server->bytes_sent() == 20);
    REQUIRE(server->bytes_received() == 0);
    REQUIRE(!server->error);

    // Check the multicast client state
    REQUIRE(client1->bytes_sent() == 0);
    REQUIRE(client2->bytes_sent() == 0);
    REQUIRE(client3->bytes_sent() == 0);
    REQUIRE(client1->bytes_received() == 12);
    REQUIRE(client2->bytes_received() == 12);
    REQUIRE(client3->bytes_received() == 12);
    REQUIRE(!client1->error);
    REQUIRE(!client2->error);
    REQUIRE(!client3->error);
}

TEST_CASE("UDP server multicast random test", "[CppServer][Asio]")
{
    const std::string listen_address = "0.0.0.0";
    const std::string multicast_address = "239.255.0.1";
    const int multicast_port = 2225;

    // Create and start Asio service
    auto service = std::make_shared<MulticastUDPService>();
    REQUIRE(service->Start());
    while (!service->IsStarted())
        Thread::Yield();

    // Create and start multicast server
    auto server = std::make_shared<MulticastUDPServer>(service, InternetProtocol::IPv4, 0);
    REQUIRE(server->Start(multicast_address, multicast_port));
    while (!server->IsStarted())
        Thread::Yield();

    // Test duration in seconds
    const int duration = 10;

    // Clients collection
    std::vector<std::shared_ptr<MulticastUDPClient>> clients;

    // Start random test
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() < duration)
    {
        // Create a new client and connect
        if ((rand() % 100) == 0)
        {
            if (clients.size() < 100)
            {
                // Create and connect multicast client
                auto client = std::make_shared<MulticastUDPClient>(service, listen_address, multicast_port, true);
                clients.emplace_back(client);
                client->Connect();
                while (!client->IsConnected())
                    Thread::Yield();

                // Join multicast group
                client->JoinMulticastGroup(multicast_address);
                Thread::Sleep(100);
            }
        }
        // Connect/Disconnect the random client
        else if ((rand() % 100) == 0)
        {
            if (!clients.empty())
            {
                size_t index = rand() % clients.size();
                auto client = clients.at(index);
                if (client->IsConnected())
                {
                    // Leave multicast group
                    client->LeaveMulticastGroup(multicast_address);
                    Thread::Sleep(100);

                    client->Disconnect();
                    while (client->IsConnected())
                        Thread::Yield();
                }
                else
                {
                    client->Connect();
                    while (!client->IsConnected())
                        Thread::Yield();

                    // Join multicast group
                    client->JoinMulticastGroup(multicast_address);
                    Thread::Sleep(100);
                }
            }
        }
        // Multicast a message to all clients
        else if ((rand() % 10) == 0)
        {
            server->Multicast("test");
        }

        // Sleep for a while...
        Thread::Sleep(1);
    }

    // Stop the multicast server
    REQUIRE(server->Stop());
    while (server->IsStarted())
        Thread::Yield();

    // Stop the Asio service
    REQUIRE(service->Stop());
    while (service->IsStarted())
        Thread::Yield();

    // Check the multicast server state
    REQUIRE(server->started);
    REQUIRE(server->stopped);
    REQUIRE(server->bytes_sent() > 0);
    REQUIRE(server->bytes_received() == 0);
    REQUIRE(!server->error);
}
