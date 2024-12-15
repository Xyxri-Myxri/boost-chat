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
        // Подключение клиента к серверу через указанные конечные точки
        do_connect(endpoints);
    }
    
    void write(const ChatMessage& msg) {
        // Асинхронная запись сообщения в сокет
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
        // Закрытие соединения с сервером
        boost::asio::post(io_context_, [this]() { socket_.close(); });
    }
    
    // Обработчик входящих сообщений
    std::function<void(const std::string&)> on_message;
    
private:
    void do_connect(const tcp::resolver::results_type& endpoints) {
        // Установка соединения с сервером
        boost::asio::async_connect(socket_, endpoints,
            [this](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) {
                    // Если соединение успешно установлено, начинаем чтение заголовка
                    do_read_header();
                }
            });
    }
    
    void do_read_header() {
        // Чтение заголовка сообщения
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.data(), ChatMessage::header_length),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec && read_msg_.decode_header()) {
                    // После успешного декодирования заголовка читаем тело сообщения
                    do_read_body();
                } else {
                    socket_.close();
                }
            });
    }
    
    void do_read_body() {
        // Чтение тела сообщения
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Передача сообщения обработчику
                    if (on_message) {
                        std::string msg(read_msg_.body(), read_msg_.body_length());
                        on_message(msg);
                    }
                    do_read_header(); // Чтение следующего сообщения
                } else {
                    socket_.close();
                }
            });
    }
    
    void do_write() {
        // Отправка сообщения серверу
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front().data(),
                write_msgs_.front().length()),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front(); // Удаляем отправленное сообщение из очереди
                    if (!write_msgs_.empty()) {
                        do_write(); // Отправка следующего сообщения
                    }
                } else {
                    socket_.close();
                }
            });
    }
    
    boost::asio::io_context& io_context_; // Контекст выполнения для операций ввода-вывода
    tcp::socket socket_; // TCP-сокет для связи с сервером
    ChatMessage read_msg_; // Объект для хранения текущего полученного сообщения
    std::deque<ChatMessage> write_msgs_; // Очередь сообщений для отправки
};

int main() {
    try {
        // Инициализация контекста ASIO
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("localhost", "8080");
        
        // Создание клиента чата
        ChatClient client(io_context, endpoints);
        std::thread t([&io_context]() { io_context.run(); });
        
        // Инициализация переменных для работы с UI
        std::string username;
        std::string input;
        std::vector<std::string> messages;
        
        // Компоненты пользовательского интерфейса FTXUI
        auto username_input = Input(&username, "Введите имя пользователя");
        auto message_input = Input(&input, "Введите сообщение");
        
        auto send_button = Button("Отправить", [&] {
            // Отправка сообщения при нажатии кнопки
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
                // Удаляем старые сообщения, чтобы избежать переполнения
                messages.erase(messages.begin());
            }
        };
        
        // Контейнер для размещения компонентов интерфейса
        auto container = Container::Vertical({
            username_input,
            message_input,
            send_button
        });
        
        auto renderer = Renderer(container, [&] {
            // Рендеринг элементов пользовательского интерфейса
            std::vector<Element> message_elements;
            for (const auto& msg : messages) {
                message_elements.push_back(text(msg));
            }
            
            return vbox({
                text("Клиент чата") | bold | center,
                separator(),
                vbox(message_elements) | frame | flex,
                separator(),
                hbox({
                    text("Имя пользователя: "),
                    username_input->Render() | flex,
                }),
                hbox({
                    text("Сообщение: "),
                    message_input->Render() | flex,
                }),
                send_button->Render() | center,
            });
        });
        
        // Запуск интерфейса
        auto screen = ScreenInteractive::TerminalOutput();
        screen.Loop(renderer);
        
        // Завершение работы клиента
        client.close();
        t.join();
        
    } catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
    }
    
    return 0;
}
