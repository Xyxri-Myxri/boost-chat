#include <boost/asio.hpp>
#include <deque>
#include <set>
#include <iostream>
#include "message.hpp"

using boost::asio::ip::tcp;

// Абстрактный класс участника чата
class ChatParticipant {
public:
    virtual ~ChatParticipant() {}
    // Виртуальная функция для отправки сообщения участнику
    virtual void deliver(const ChatMessage& msg) = 0;
};

typedef std::shared_ptr<ChatParticipant> ChatParticipantPtr;

// Класс, представляющий комнату чата
class ChatRoom {
public:
    void join(ChatParticipantPtr participant) {
        // Добавляем нового участника и отправляем ему последние сообщения
        participants_.insert(participant);
        for (const auto& msg : recent_msgs_)
            participant->deliver(msg);
    }
    
    void leave(ChatParticipantPtr participant) {
        // Удаляем участника из комнаты
        participants_.erase(participant);
    }
    
    void deliver(const ChatMessage& msg) {
        // Рассылаем сообщение всем участникам и добавляем его в историю
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs)
            recent_msgs_.pop_front();
            
        for (const auto& participant : participants_)
            participant->deliver(msg);
    }
    
private:
    std::set<ChatParticipantPtr> participants_; // Список участников
    enum { max_recent_msgs = 100 }; // Максимальное количество хранимых сообщений
    std::deque<ChatMessage> recent_msgs_; // Очередь последних сообщений
};

// Сессия чата, связанная с участником
class ChatSession : public ChatParticipant,
    public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket socket, ChatRoom& room)
        : socket_(std::move(socket)), room_(room) {}
        
    void start() {
        // Добавляем участника в комнату и начинаем чтение сообщений
        room_.join(shared_from_this());
        do_read_header();
    }
    
    void deliver(const ChatMessage& msg) override {
        // Добавляем сообщение в очередь отправки
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress) {
            do_write();
        }
    }
    
private:
    void do_read_header() {
        // Асинхронное чтение заголовка сообщения
        auto self(shared_from_this());
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.data(), ChatMessage::header_length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec && read_msg_.decode_header()) {
                    do_read_body();
                } else {
                    room_.leave(shared_from_this());
                }
            });
    }
    
    void do_read_body() {
        // Асинхронное чтение тела сообщения
        auto self(shared_from_this());
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Доставляем сообщение в комнату и читаем следующее
                    room_.deliver(read_msg_);
                    do_read_header();
                } else {
                    room_.leave(shared_from_this());
                }
            });
    }
    
    void do_write() {
        // Асинхронная отправка сообщения
        auto self(shared_from_this());
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front().data(),
                write_msgs_.front().length()),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front(); // Удаляем отправленное сообщение из очереди
                    if (!write_msgs_.empty()) {
                        do_write(); // Отправка следующего сообщения
                    }
                } else {
                    room_.leave(shared_from_this());
                }
            });
    }
    
    tcp::socket socket_; // Сокет для связи с клиентом
    ChatRoom& room_; // Ссылка на комнату чата
    ChatMessage read_msg_; // Текущее сообщение для чтения
    std::deque<ChatMessage> write_msgs_; // Очередь сообщений для отправки
};

// Сервер чата
class ChatServer {
public:
    ChatServer(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
        : acceptor_(io_context, endpoint) {
        do_accept();
    }
    
private:
    void do_accept() {
        // Ожидание подключения новых клиентов
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // Создание новой сессии для подключенного клиента
                    std::make_shared<ChatSession>(std::move(socket), room_)->start();
                }
                do_accept(); // Ожидание следующего подключения
            });
    }
    
    tcp::acceptor acceptor_; // Приёмник подключений
    ChatRoom room_; // Комната чата
};

int main() {
    try {
        // Инициализация сервера
        boost::asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v4(), 8080);
        ChatServer server(io_context, endpoint);
        std::cout << "Server started on port: 8080" << std::endl;
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return 0;
}
