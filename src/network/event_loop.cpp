#include "network/event_loop.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <vector>

namespace flexql {

EventLoop::EventLoop() : fd_(-1) {}

EventLoop::~EventLoop() {
    close();
}

bool EventLoop::open(std::string &failure_reason) {
    close();
    fd_ = ::epoll_create1(EPOLL_CLOEXEC);
    if (fd_ < 0) {
        failure_reason = std::strerror(errno);
        return false;
    }
    return true;
}

void EventLoop::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool EventLoop::add(int file_descriptor, std::uint32_t event_mask, std::string &failure_reason) {
    epoll_event registration {};
    registration.events = event_mask;
    registration.data.fd = file_descriptor;
    if (::epoll_ctl(fd_, EPOLL_CTL_ADD, file_descriptor, &registration) < 0) {
        failure_reason = std::strerror(errno);
        return false;
    }
    return true;
}

bool EventLoop::modify(int file_descriptor, std::uint32_t event_mask, std::string &failure_reason) {
    epoll_event registration {};
    registration.events = event_mask;
    registration.data.fd = file_descriptor;
    if (::epoll_ctl(fd_, EPOLL_CTL_MOD, file_descriptor, &registration) < 0) {
        failure_reason = std::strerror(errno);
        return false;
    }
    return true;
}

bool EventLoop::remove(int file_descriptor, std::string &failure_reason) {
    if (::epoll_ctl(fd_, EPOLL_CTL_DEL, file_descriptor, nullptr) < 0 &&
        errno != ENOENT &&
        errno != EBADF) {
        failure_reason = std::strerror(errno);
        return false;
    }
    return true;
}

int EventLoop::wait(
    std::vector<EventLoopEvent> &events,
    int timeout_ms,
    std::string &failure_reason) {
    std::vector<epoll_event> raw_events(events.size());
    const int ready_count = ::epoll_wait(
        fd_,
        raw_events.data(),
        static_cast<int>(raw_events.size()),
        timeout_ms);
    if (ready_count < 0) {
        if (errno == EINTR) {
            return 0;
        }

        failure_reason = std::strerror(errno);
        return -1;
    }

    for (int ready_index = 0; ready_index < ready_count; ++ready_index) {
        events[static_cast<std::size_t>(ready_index)].fd =
            raw_events[static_cast<std::size_t>(ready_index)].data.fd;
        events[static_cast<std::size_t>(ready_index)].events =
            raw_events[static_cast<std::size_t>(ready_index)].events;
    }

    return ready_count;
}

}  // namespace flexql
