#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace flexql {

struct EventLoopEvent {
    int fd = -1;
    std::uint32_t events = 0;
};

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    bool open(std::string &error);
    void close();

    bool add(int fd, std::uint32_t events, std::string &error);
    bool modify(int fd, std::uint32_t events, std::string &error);
    bool remove(int fd, std::string &error);
    int wait(std::vector<EventLoopEvent> &events, int timeout_ms, std::string &error);

private:
    int fd_;
};

}  // namespace flexql
