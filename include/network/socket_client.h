#pragma once

#include <string>

namespace flexql {

class SocketClient {
public:
    SocketClient();
    ~SocketClient();

    bool connect_to(const std::string &host, int port, std::string &error);
    bool send_message(const std::string &message, std::string &error);
    bool receive_message(std::string &message, std::string &error);
    void close();
    bool is_open() const;

private:
    int fd_;
};

}  // namespace flexql
