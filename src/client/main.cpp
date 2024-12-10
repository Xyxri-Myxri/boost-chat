#include <boost/asio.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <deque>
#include <thread>
#include "message.hpp"

using namespace ftxui;
using boost::asio::ip::tcp;

class ChatClient {
public:
    ChatClient(boost::asio::io_context& io_context,
        const tcp::resolver::results_type& endpoints)
        : io_context_(io_context), socket_(io_context) {
        do_connect(endpoints);
    }
    
    void write(const ChatMessage& msg) {
        boost::asio::post(io_context_,
            [this, msg]() {
                bool write_in_progress = !write_msgs_.empty();
                write_msgs_.push_back(msg);
                if (!write_in_progress) {
                    do_write();
                }
            });
    }
    
    void close() {
        boost::asio::post(io_context_, [this]() { socket_.close(); });
    }
    
    std::function<void(const std::string&)> on_message;
    
private:
    void do_connect(const tcp::resolver::results_type& endpoints) {
        boost::asio::async_connect(socket_, endpoints,
            [this](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) {
                    do_read_header();
                }
            });
    }
    
    void do_read_header() {
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.data(), ChatMessage::header_length),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec && read_msg_.decode_header()) {
                    do_read_body();
                } else {
                    socket_.close();
                }
            });
    }
    
    void do_read_body() {
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    if (on_message) {
                        std::string msg(read_msg_.body(), read_msg_.body_length());
                        on_message(msg);
                    }
                    do_read_header();
                } else {
                    socket_.close();
                }
            });
    }
    
    void do_write() {
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front().data(),
                write_msgs_.front().length()),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty()) {
                        do_write();
                    }
                } else {
                    socket_.close();
                }
            });
    }
    
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    ChatMessage read_msg_;
    std::deque<ChatMessage> write_msgs_;
};

int main() {
    try {
        // Инициализация ASIO
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("localhost", "8080");
        
        // Создаем клиент
        ChatClient client(io_context, endpoints);
        std::thread t([&io_context]() { io_context.run(); });
        
        // Создаем UI
        std::string username;
        std::string input;
        std::vector<std::string> messages;
        
        // Компоненты FTXUI
        auto username_input = Input(&username, "Enter username");
        auto message_input = Input(&input, "Type message");
        
        auto send_button = Button("Send", [&] {
            if (!input.empty() && !username.empty()) {
                std::string full_message = username + ": " + input;
                ChatMessage msg;
                msg.body_length(full_message.length());
                std::memcpy(msg.body(), full_message.c_str(), msg.body_length());
                msg.encode_header();
                client.write(msg);
                input.clear();
            }
        });
        
        // Обработчик входящих сообщений
        client.on_message = [&messages](const std::string& msg) {
            messages.push_back(msg);
            if (messages.size() > 100) {
                messages.erase(messages.begin());
            }
        };
        
        auto container = Container::Vertical({
            username_input,
            message_input,
            send_button
        });
        
        auto renderer = Renderer(container, [&] {
            std::vector<Element> message_elements;
            for (const auto& msg : messages) {
                message_elements.push_back(text(msg));
            }
            
            return vbox({
                text("Chat Client") | bold | center,
                separator(),
                vbox(message_elements) | frame | flex,
                separator(),
                hbox({
                    text("Username: "),
                    username_input->Render() | flex,
                }),
                hbox({
                    text("Message: "),
                    message_input->Render() | flex,
                }),
                send_button->Render() | center,
            });
        });
        
        auto screen = ScreenInteractive::TerminalOutput();
        screen.Loop(renderer);
        
        // Очистка
        client.close();
        t.join();
        
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    return 0;
}