/*!
    \file websocket_ssl_session.h
    \brief WebSocket SSL session definition
    \author Ivan Shynkarenka
    \date 06.01.2016
    \copyright MIT License
*/

#ifndef CPPSERVER_ASIO_WEBSOCKET_SSL_SESSION_H
#define CPPSERVER_ASIO_WEBSOCKET_SSL_SESSION_H

#include "service.h"
#include "websocket.h"

#include "system/uuid.h"

namespace CppServer {
namespace Asio {

template <class TServer, class TSession>
class WebSocketSSLServer;

//! WebSocket SSL session
/*!
    WebSocket SSL session is used to read and write data from the connected WebSocket SSL client.

    Thread-safe.
*/
template <class TServer, class TSession>
class WebSocketSSLSession : public std::enable_shared_from_this<WebSocketSSLSession<TServer, TSession>>
{
    template <class TSomeServer, class TSomeSession>
    friend class WebSocketSSLServer;

public:
    //! Initialize the session with a given server
    /*!
        \param server - Connected server
    */
    explicit WebSocketSSLSession(std::shared_ptr<WebSocketSSLServer<TServer, TSession>> server);
    WebSocketSSLSession(const WebSocketSSLSession&) = delete;
    WebSocketSSLSession(WebSocketSSLSession&&) = default;
    virtual ~WebSocketSSLSession() = default;

    WebSocketSSLSession& operator=(const WebSocketSSLSession&) = delete;
    WebSocketSSLSession& operator=(WebSocketSSLSession&&) = default;

    //! Get the session Id
    const CppCommon::UUID& id() const noexcept { return _id; }

    //! Get the Asio service
    std::shared_ptr<Service>& service() noexcept { return _server->service(); }
    //! Get the session server
    std::shared_ptr<WebSocketSSLServer<TServer, TSession>>& server() noexcept { return _server; }
    //! Get the session connection
    websocketpp::connection_hdl& connection() noexcept { return _connection; }

    //! Get the number messages sent by this session
    uint64_t messages_sent() const noexcept { return _messages_sent; }
    //! Get the number messages received by this session
    uint64_t messages_received() const noexcept { return _messages_received; }
    //! Get the number of bytes sent by this session
    uint64_t bytes_sent() const noexcept { return _bytes_sent; }
    //! Get the number of bytes received by this session
    uint64_t bytes_received() const noexcept { return _bytes_received; }

    //! Is the session connected?
    bool IsConnected() const noexcept { return _connected; }

    //! Disconnect the session
    /*!
        \param code - Close code to send (default is normal)
        \param reason - Close reason to send (default is "")
        \return 'true' if the section was successfully disconnected, 'false' if the section is already disconnected
    */
    bool Disconnect(websocketpp::close::status::value code = websocketpp::close::status::normal, const std::string& reason = "") { return Disconnect(false, code, reason); }

    //! Send data into the session
    /*!
        \param buffer - Buffer to send
        \param size - Buffer size
        \param opcode - Data opcode (default is websocketpp::frame::opcode::binary)
        \return Count of sent bytes
    */
    size_t Send(const void* buffer, size_t size, websocketpp::frame::opcode::value opcode = websocketpp::frame::opcode::binary);
    //! Send a text string into the session
    /*!
        \param text - Text string to send
        \param opcode - Data opcode (default is websocketpp::frame::opcode::text)
        \return Count of sent bytes
    */
    size_t Send(const std::string& text, websocketpp::frame::opcode::value opcode = websocketpp::frame::opcode::text);
    //! Send a message into the session
    /*!
        \param message - Message to send
        \return Count of sent bytes
    */
    size_t Send(const WebSocketSSLMessage& message);

protected:
    //! Handle session connected notification
    virtual void onConnected() {}
    //! Handle session disconnected notification
    virtual void onDisconnected() {}

    //! Handle message received notification
    /*!
        \param message - Received message
    */
    virtual void onReceived(const WebSocketSSLMessage& message) {}

    //! Handle error notification
    /*!
        \param error - Error code
        \param category - Error category
        \param message - Error message
    */
    virtual void onError(int error, const std::string& category, const std::string& message) {}

private:
    // Session Id
    CppCommon::UUID _id;
    // Session server & connection
    std::shared_ptr<WebSocketSSLServer<TServer, TSession>> _server;
    websocketpp::connection_hdl _connection;
    std::atomic<bool> _connected;
    // Session statistic
    uint64_t _messages_sent;
    uint64_t _messages_received;
    uint64_t _bytes_sent;
    uint64_t _bytes_received;

    //! Connect the session
    /*!
        \param connection - WebSocket connection
    */
    void Connect(websocketpp::connection_hdl connection);
    //! Disconnect the session
    /*!
        \param dispatch - Dispatch flag
        \param code - Close code to send (default is normal)
        \param reason - Close reason to send (default is "")
        \return 'true' if the session was successfully disconnected, 'false' if the session is already disconnected
    */
    bool Disconnect(bool dispatch, websocketpp::close::status::value code = websocketpp::close::status::normal, const std::string& reason = "");

    //! Disconnected session handler
    void Disconnected();

    //! Send error notification
    void SendError(std::error_code ec);
};

} // namespace Asio
} // namespace CppServer

#endif // CPPSERVER_ASIO_WEBSOCKET_SSL_SESSION_H
