#include "httpserver.h"
#include <iostream>
#include "../../gameclient.h"
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <string>
#include <cctype>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace urls = boost::urls;

// Функция для URL-декодирования
std::string url_decode(const std::string &str) {
    std::ostringstream decoded;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.length()) {
                int hex_val;
                std::istringstream hex_stream(str.substr(i + 1, 2));
                if (hex_stream >> std::hex >> hex_val) {
                    decoded << static_cast<char>(hex_val);
                    i += 2;
                } else {
                    decoded << str[i];
                }
            } else {
                decoded << str[i];
            }
        } else if (str[i] == '+') {
            decoded << ' ';
        } else {
            decoded << str[i];
        }
    }
    return decoded.str();
}

std::string get_path(const std::string& target) {
    size_t query_pos = target.find('?');
    std::string path = target.substr(0, query_pos);
    return url_decode(path);
}

std::string remove_leading_slash(const std::string& path) {
    if (!path.empty() && path[0] == '/') {
        return path.substr(1);
    }
    return path;
}

HttpServer::HttpServer(unsigned short preferred_port) 
    : ioc_() // Инициализация io_context
{
    boost::system::error_code ec;
    unsigned short port = preferred_port;
    bool port_found = false;

    // Пытаемся найти свободный порт
    for (int i = 0; i < 100; ++i) {
        try {
            // Попытка создать временный acceptor
            tcp::acceptor temp_acceptor(ioc_);
            temp_acceptor.open(tcp::v4(), ec);
            if (ec) throw boost::system::system_error(ec);
            
            temp_acceptor.bind(tcp::endpoint(tcp::v4(), port), ec);
            if (!ec) {
                port_found = true;
                break;
            }
        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::address_in_use) {
                // Порт занят, пробуем следующий
                port++;
                ec.clear();
                continue;
            }
            throw; // Другие ошибки
        }
    }

    if (!port_found) {
        throw std::runtime_error("No available ports found");
    }

    // Создаем acceptor с использованием new (C++11)
    acceptor_.reset(new tcp::acceptor(ioc_, tcp::endpoint(tcp::v4(), port)));
    
    // Установка опции повторного использования адреса
    acceptor_->set_option(tcp::acceptor::reuse_address(true));
    
    std::cout << "DDBotAPI initialized on port " << port << std::endl;
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start(CGameClient *client) {
    if (is_running_) return;
    
    this->client = client;
    is_running_ = true;
    server_thread_.reset(new std::thread([this] {  // Замена make_unique
        accept_connections();
    }));
}

void HttpServer::stop() {
    if (!is_running_) return;
    
    ioc_.stop();
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
    is_running_ = false;

    std::cout << "DDBot API stopped. " << std::endl;
}

void HttpServer::accept_connections() {
    try {
        auto on_accept = [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                beast::flat_buffer buffer;
                http::request<http::string_body> req;
                http::read(socket, buffer, req);
                handle_request(std::move(req), std::move(socket));
            }
            if (is_running_) {
                accept_connections();
            }
        };
        
        acceptor_->async_accept(on_accept);
        ioc_.run();
    } catch (const beast::system_error& e) {
        client->Console()->Print(1, "ddbotapi/http", "Request processing failed.");
    }
}

void HttpServer::handle_request(
    http::request<http::string_body> req, 
    tcp::socket socket
) {

    // Получаем endpoint клиента
    boost::asio::ip::tcp::endpoint remote_ep = socket.remote_endpoint();
    std::string remote_ip = remote_ep.address().to_string();
    unsigned short remote_port = remote_ep.port();
    

    std::stringstream address;
    address << remote_ip << ":" << remote_port;
    // client->Console()->Print(1, "ddbotapi", 
    //     ("Processing new request to API from " + address.str() + ". " +
    //     "Method: " + std::string(req.method_string()) + ", " +
    //     "Target: " + std::string(req.target())).c_str());

    // Формируем ответ
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "DDBot API");
    res.set(http::field::content_type, "text/plain");

    beast::string_view target_view = req.target();
    std::string target_str(target_view.data(), target_view.size());
    std::string path = remove_leading_slash(get_path(target_str));

    std::stringstream resp;

    if (path == "_/output") {
        std::stringstream* out = client->Console()->GetOutputStream();
        res.body() = out->str();
    } else if (path == "_/players") {} else {
        resp << "Executed command '" << path << "'.";
        try {
            client->Console()->ExecuteLine(path.c_str());
            res.body() = resp.str().c_str();
        } catch (const std::exception& e) { res.body() = "Oops, command execution failed."; }
    }

    // Если нужно добавить IP в JSON-ответ
    // json response;
    // response["client_ip"] = remote_ip;
    // response["client_port"] = remote_port;
    // res.body() = response.dump();

    res.prepare_payload();
    http::write(socket, res);
}