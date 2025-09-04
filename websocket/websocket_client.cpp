#include "websocket_client.h"
#include <iostream>

namespace websocket {

WebSocketClient::WebSocketClient(boost::asio::io_context& ioc, ThreadPool& pool)
    : ws_(ioc),
      resolver_(ioc),
      thread_pool_(pool) {}

void WebSocketClient::connect(const std::string& host, const std::string& port, const std::string& path) {
    resolver_.async_resolve(host, port,
        [this, path](beast::error_code ec, tcp::resolver::results_type results) {
            if(ec) return error_handler_(ec);
            on_resolve(ec, results, path);
        });
}

void WebSocketClient::on_resolve(beast::error_code ec, tcp::resolver::results_type results, const std::string& path) {
    if(ec) return error_handler_(ec);
    
    beast::get_lowest_layer(ws_).async_connect(results,
        [this, path](beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
            if(ec) return error_handler_(ec);
            on_connect(ec, ep, path);
        });
}

void WebSocketClient::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep, const std::string& path) {
    if(ec) return error_handler_(ec);
    
    ws_.async_handshake(ep.address().to_string() + ":" + std::to_string(ep.port()), path,
        [this](beast::error_code ec) {
            if(ec) return error_handler_(ec);
            on_handshake(ec);
        });
}

void WebSocketClient::send(const std::string& message) {
    thread_pool_.enqueue([this, message]() {
        ws_.async_write(boost::asio::buffer(message),
            [](beast::error_code ec, std::size_t) {
                if(ec) std::cerr << "Write error: " << ec.message() << std::endl;
            });
    });
}

void WebSocketClient::read_message() {
    ws_.async_read(buffer_,
        [this](beast::error_code ec, std::size_t bytes_transferred) {
            on_read(ec, bytes_transferred);
        });
}

void WebSocketClient::on_read(beast::error_code ec, std::size_t) {
    if(ec == websocket::error::closed) return;
    if(ec) return error_handler_(ec);
    
    if(message_handler_) {
        auto data = beast::buffers_to_string(buffer_.data());
        thread_pool_.enqueue([this, data]() {
            message_handler_(data);
        });
    }
    buffer_.consume(buffer_.size());
    read_message();
}

void WebSocketClient::set_message_handler(std::function<void(std::string)> handler) {
    message_handler_ = handler;
    read_message();
}

void WebSocketClient::set_error_handler(std::function<void(beast::error_code)> handler) {
    error_handler_ = handler;
}

void WebSocketClient::close() {
    ws_.async_close(websocket::close_code::normal,
        [](beast::error_code ec) {
            if(ec) std::cerr << "Close error: " << ec.message() << std::endl;
        });
}

} // namespace websocket