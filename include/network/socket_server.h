#pragma once

#include <string>

namespace flexql {

class SocketServer {
public:
    SocketServer();
    ~SocketServer();

    bool start(int port, std::string &error);
    int accept_client(std::string &error);
    void close();
    int fd() const;

    static bool send_message(int client_fd, const std::string &message, std::string &error);
    static bool receive_message(int client_fd, std::string &message, std::string &error);

private:
    int fd_;
};

}  // namespace flexql
