#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <thread>
#include "../../gameclient.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpServer {
public:
    explicit HttpServer(unsigned short preffered_port);
    ~HttpServer();
    
    void start(CGameClient *client);
    void stop();

private:
    void accept_connections();
    void handle_request(http::request<http::string_body> req, tcp::socket socket);

    boost::asio::io_context ioc_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    std::unique_ptr<std::thread> server_thread_;
    bool is_running_ = false;
    CGameClient *client;
};

#endif // HTTPSERVER_H