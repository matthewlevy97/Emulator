#include "client.h"

#include <vector>

namespace emulator::debugger::socket {

DebuggerSocketClient::DebuggerSocketClient(SOCKET client) : client_(client)
{}

DebuggerSocketClient::~DebuggerSocketClient()
{
    if (client_ != INVALID_SOCKET) {
        closesocket(client_);
    }
}

bool DebuggerSocketClient::IsReadable() noexcept
{
    if (client_ == INVALID_SOCKET) {
        return false;
    }

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(client_, &fdset);

    if (select(client_ + 1, &fdset, nullptr, nullptr, &timeout_) < 0) {
        return false;
    }
    return FD_ISSET(client_, &fdset);
}

bool DebuggerSocketClient::IsWritable() noexcept
{
    if (client_ == INVALID_SOCKET) {
        return false;
    }

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(client_, &fdset);

    if (select(client_ + 1, nullptr, &fdset, nullptr, &timeout_) < 0) {
        return false;
    }
    return FD_ISSET(client_, &fdset);
}

int DebuggerSocketClient::ReadAll(std::uint8_t** data) noexcept
{
    if (client_ == INVALID_SOCKET) {
        return -1;
    }

    int bufLen = 0;
    std::size_t bufCap = 4096;
    std::uint8_t *buf = new std::uint8_t[bufCap];

    do {
        auto n = recv(client_, buf, bufCap, MSG_DONTWAIT);
        if (n < 0) {
            delete[] buf;
            return -1;
        }

        bufLen += n;

        // Could not fill buffer, so end of the stream
        if (bufLen < bufCap || n == 0) {
            *data = buf;
            return bufLen;
        }

        // Resize for more data
        bufCap += 4096;
        auto bufNew = new std::uint8_t[bufCap];
        std::memcpy(bufNew, buf, bufLen);
        buf = bufNew;
    } while (true);
}

int DebuggerSocketClient::Write(const std::uint8_t* data, std::size_t len) noexcept
{
    if (client_ == INVALID_SOCKET) {
        return -1;
    }

    std::size_t offset = 0;
    do {
        auto n = send(client_, data + offset, len - offset, 0);
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            break;
        }

        offset += n;
    } while(offset < len);

    return offset;
}

}; // namespace emulator::debugger::socket