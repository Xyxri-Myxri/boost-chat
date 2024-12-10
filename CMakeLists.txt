cmake_minimum_required(VERSION 3.15)
project(asio_example CXX)

find_package(Boost REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE Boost::boost)
target_link_libraries(example PRIVATE nlohmann_json::nlohmann_json)