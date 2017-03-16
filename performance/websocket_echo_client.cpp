//
// Created by Ivan Shynkarenka on 16.03.2017
//

#include "benchmark/reporter_console.h"
#include "server/asio/service.h"
#include "server/asio/websocket_client.h"
#include "system/cpu.h"
#include "threads/thread.h"
#include "time/timestamp.h"

#include <atomic>
#include <iostream>
#include <vector>

#include "../../modules/cpp-optparse/OptionParser.h"

using namespace CppServer::Asio;

uint64_t timestamp_start;
uint64_t timestamp_sent;
uint64_t timestamp_received;

std::atomic<size_t> total_errors(0);
std::atomic<size_t> total_sent_bytes(0);
std::atomic<size_t> total_sent_messages(0);
std::atomic<size_t> total_received_bytes(0);
std::atomic<size_t> total_received_messages(0);

class EchoClient : public WebSocketClient
{
public:
    using WebSocketClient::WebSocketClient;

protected:
    void onReceived(WebSocketMessage message) override
    {
        timestamp_received = CppCommon::Timestamp::nano();
        total_received_bytes += message->get_payload().size();
    }

    void onError(int error, const std::string& category, const std::string& message) override
    {
        std::cout << "Client caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
        total_errors++;
    }
};

int main(int argc, char** argv)
{
    auto parser = optparse::OptionParser().version("1.0.0.0");

    parser.add_option("-h", "--help").help("Show help");
    parser.add_option("-a", "--address").set_default("127.0.0.1").help("Server address. Default: %default");
    parser.add_option("-p", "--port").action("store").type("int").set_default(4444).help("Server port. Default: %default");
    parser.add_option("-t", "--threads").action("store").type("int").set_default(CppCommon::CPU::LogicalCores()).help("Count of working threads. Default: %default");
    parser.add_option("-c", "--clients").action("store").type("int").set_default(100).help("Count of working clients. Default: %default");
    parser.add_option("-m", "--messages").action("store").type("int").set_default(100000).help("Count of messages to send. Default: %default");
    parser.add_option("-s", "--size").action("store").type("int").set_default(32).help("Single message size. Default: %default");

    optparse::Values options = parser.parse_args(argc, argv);

    // Print help
    if (options.get("help"))
    {
        parser.print_help();
        parser.exit();
    }

    // Server address and port
    std::string address(options.get("address"));
    int port = options.get("port");
    int threads_count = options.get("threads");
    int clients_count = options.get("clients");
    int messages_count = options.get("messages");
    int message_size = options.get("size");

    // WebSocket server uri
    std::string uri = "ws://" + address + ":" + std::to_string(port);

    std::cout << "Server address: " << address << std::endl;
    std::cout << "Server port: " << port << std::endl;
    std::cout << "Server uri: " << uri << std::endl;
    std::cout << "Working threads: " << threads_count << std::endl;
    std::cout << "Working clients: " << clients_count << std::endl;
    std::cout << "Messages to send: " << messages_count << std::endl;
    std::cout << "Message size: " << message_size << std::endl;

    // Prepare a message to send
    std::vector<uint8_t> message(message_size, 0);

    // Create Asio services
    std::vector<std::shared_ptr<Service>> services;
    for (int i = 0; i < threads_count; ++i)
    {
        auto service = std::make_shared<Service>();
        services.emplace_back(service);
    }

    // Start Asio services
    std::cout << "Asio services starting...";
    for (auto& service : services)
        service->Start();
    std::cout << "Done!" << std::endl;

    // Create echo clients
    std::vector<std::shared_ptr<EchoClient>> clients;
    for (int i = 0; i < clients_count; ++i)
    {
        auto client = std::make_shared<EchoClient>(services[i % services.size()], uri);
        clients.emplace_back(client);
    }

    // Connect clients
    std::cout << "Clients connecting...";
    for (auto& client : clients)
    {
        client->Connect();
        while (!client->IsConnected())
            CppCommon::Thread::Yield();
    }
    std::cout << "Done!" << std::endl;

    timestamp_start = CppCommon::Timestamp::nano();

    // Send messages to the server
    for (int i = 0; i < messages_count; ++i)
    {
        clients[i % clients.size()]->Send(message.data(), message.size());
        total_sent_bytes += message.size();
    }

    timestamp_sent = CppCommon::Timestamp::nano();

    // Wait for received data
    size_t received = 0;
    do
    {
        CppCommon::Thread::Sleep(100);
        if (received < total_received_bytes)
            received = total_received_bytes;
        else
            break;
    } while (received < total_sent_bytes);

    // Disconnect clients
    std::cout << "Clients disconnecting...";
    for (auto& client : clients)
        client->Disconnect();
    std::cout << "Done!" << std::endl;

    // Stop Asio services
    std::cout << "Asio services stopping...";
    for (auto& service : services)
        service->Stop();
    std::cout << "Done!" << std::endl;

    std::cout << std::endl;

    total_sent_messages = total_sent_bytes / message_size;
    total_received_messages = total_received_bytes / message_size;

    std::cout << "Send time: " << CppBenchmark::ReporterConsole::GenerateTimePeriod(timestamp_sent - timestamp_start) << std::endl;
    std::cout << "Send bytes: " << total_sent_bytes << std::endl;
    std::cout << "Send messages: " << total_sent_messages << std::endl;
    std::cout << "Send bytes throughput: " << total_sent_bytes * 1000000000 / (timestamp_sent - timestamp_start) << " bytes per second" << std::endl;
    std::cout << "Send messages throughput: " << total_sent_messages * 1000000000 / (timestamp_sent - timestamp_start) << " messages per second" << std::endl;
    std::cout << "Receive time: " << CppBenchmark::ReporterConsole::GenerateTimePeriod(timestamp_received - timestamp_start) << std::endl;
    std::cout << "Receive bytes: " << total_received_bytes << std::endl;
    std::cout << "Receive messages: " << total_received_messages << std::endl;
    std::cout << "Receive bytes throughput: " << total_received_bytes * 1000000000 / (timestamp_received - timestamp_start) << " bytes per second" << std::endl;
    std::cout << "Receive messages throughput: " << total_received_messages * 1000000000 / (timestamp_received - timestamp_start) << " messages per second" << std::endl;
    std::cout << "Errors: " << total_errors << std::endl;

    return 0;
}