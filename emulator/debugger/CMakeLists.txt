add_library(debugger OBJECT debugger.cpp)
target_sources(debugger
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gdbserver.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/socket/client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/socket/server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/socket/socket_internal.cpp
)

target_include_directories(debugger
    PRIVATE
        ${CMAKE_SOURCE_DIR}/emulator
)

target_link_libraries(debugger
    PRIVATE
        spdlog::spdlog_header_only
)