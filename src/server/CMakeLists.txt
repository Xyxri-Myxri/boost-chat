add_executable(chat_server main.cpp)

target_include_directories(chat_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/common
)

target_link_libraries(chat_server PRIVATE
    Boost::system
    Threads::Threads
)