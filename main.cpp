#include <boost/asio.hpp>
#include <iostream>

int main() {
    boost::asio::io_context io;
    std::cout << "Boost.Asio setup successful!" << std::endl;
    return 0;
}