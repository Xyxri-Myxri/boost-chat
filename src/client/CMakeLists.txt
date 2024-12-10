add_executable(chat_client main.cpp)

target_include_directories(chat_client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/common
)

target_link_libraries(chat_client PRIVATE
    Boost::system
    ftxui::screen
    ftxui::dom
    ftxui::component
    Threads::Threads
)