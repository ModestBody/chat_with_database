#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <string>
#include <functional>

using boost::asio::ip::tcp;

class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(read_msg_), '\n',
            [this, self](std::error_code ec, std::size_t length) {
                if (!ec) {
                    std::cout.write(read_msg_.data(), length);
                    read_msg_.erase(0, length);
                    do_write();
                }
            });
    }

    void do_write() {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(read_msg_),
            [this, self](std::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read();
                }
            });
    }

    tcp::socket socket_;
    std::string read_msg_;
};

class ChatClient {
public:
    ChatClient(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints)
        : io_context_(io_context), socket_(io_context) {
        do_connect(endpoints);
    }

    void write(const std::string& msg) {
        boost::asio::post(io_context_, [this, msg]() {
            do_write(msg);
            });
    }

    void close() {
        boost::asio::post(io_context_, [this]() { socket_.close(); });
    }

private:
    void do_connect(const tcp::resolver::results_type& endpoints) {
        boost::asio::async_connect(socket_, endpoints, [this](std::error_code ec, tcp::endpoint) {
            if (!ec) {
                do_read();
            }
            });
    }

    void do_read() {
        boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(read_msg_), '\n',
            [this](std::error_code ec, std::size_t length) {
                if (!ec) {
                    std::cout.write(read_msg_.data(), length);
                    read_msg_.erase(0, length);
                    do_read();
                }
            });
    }

    void do_write(const std::string& msg) {
        boost::asio::async_write(socket_, boost::asio::buffer(msg + "\n"),
            [this](std::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    socket_.close();
                }
            });
    }

    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::string read_msg_;
};

class ChatServer {
public:
    ChatServer(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
        : acceptor_(io_context, endpoint) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<ChatSession>(std::move(socket))->start();
            }
            do_accept();
            });
    }

    tcp::acceptor acceptor_;
};
