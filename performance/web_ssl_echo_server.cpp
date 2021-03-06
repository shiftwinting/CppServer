//
// Created by Ivan Shynkarenka on 16.03.2017
//

#include "server/asio/service.h"
#include "server/asio/web_ssl_server.h"

#include <iostream>

#include "../../modules/cpp-optparse/OptionParser.h"

using namespace CppServer::Asio;

class EchoServer : public WebSSLServer
{
public:
    explicit EchoServer(std::shared_ptr<Service> service, int port)
        : WebSSLServer(service, port)
    {
        // Create a resource
        auto resource = std::make_shared<restbed::Resource>();
        resource->set_path("/storage");
        resource->set_method_handler("POST", RestStoragePost);

        // Publish the resource
        server()->publish(resource);

        // Prepare SSL settings
        ssl_settings()->set_http_disabled(true);
        ssl_settings()->set_default_workarounds_enabled(true);
        ssl_settings()->set_sslv2_enabled(false);
        ssl_settings()->set_single_diffie_hellman_use_enabled(true);
        ssl_settings()->set_passphrase("qwerty");
        ssl_settings()->set_certificate_chain(restbed::Uri("file://../tools/certificates/server.pem"));
        ssl_settings()->set_private_key(restbed::Uri("file://../tools/certificates/server.pem"));
        ssl_settings()->set_temporary_diffie_hellman(restbed::Uri("file://../tools/certificates/dh4096.pem"));
    }

private:
    static void RestStoragePost(const std::shared_ptr<restbed::Session>& session)
    {
        auto request = session->get_request();
        size_t request_content_length = request->get_header("Content-Length", 0);
        session->fetch(request_content_length, [request](const std::shared_ptr<restbed::Session> session, const restbed::Bytes & body)
        {
            std::string data = std::string((char*)body.data(), body.size());
            session->close(restbed::OK, data, { { "Content-Length", std::to_string(data.size()) } });
        });
    }
};

int main(int argc, char** argv)
{
    auto parser = optparse::OptionParser().version("1.0.0.0");

    parser.add_option("-h", "--help").help("Show help");
    parser.add_option("-p", "--port").action("store").type("int").set_default(9000).help("Server port. Default: %default");

    optparse::Values options = parser.parse_args(argc, argv);

    // Print help
    if (options.get("help"))
    {
        parser.print_help();
        parser.exit();
    }

    // Server port
    int port = options.get("port");

    std::cout << "Server port: " << port << std::endl;

    // Create a new Asio service
    auto service = std::make_shared<Service>();

    // Start the service
    std::cout << "Asio service starting...";
    service->Start();
    std::cout << "Done!" << std::endl;

    // Create a new echo server
    auto server = std::make_shared<EchoServer>(service, port);

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
    }

    // Stop the server
    std::cout << "Server stopping...";
    server->Stop();
    std::cout << "Done!" << std::endl;

    // Stop the service
    std::cout << "Asio service stopping...";
    service->Stop();
    std::cout << "Done!" << std::endl;

    return 0;
}
