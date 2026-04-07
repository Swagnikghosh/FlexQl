#include <iostream>
#include <string>

namespace flexql {
int run_server(int port, const std::string &root, const std::string &mode);
}

int main(int argument_count, char **argument_values) {
    constexpr int kRequiredArgs = 2;
    constexpr int kOptionalArgs = 3;
    if (argument_count != kRequiredArgs && argument_count != kOptionalArgs) {
        std::cerr << "usage: " << argument_values[0] << " <port> [--io-uring|--epoll]\n";
        return 1;
    }

    std::string server_mode = "threaded";
    if (argument_count == kOptionalArgs) {
        server_mode = argument_values[2];
    }

    const int port_number = std::stoi(argument_values[1]);
    return flexql::run_server(port_number, ".", server_mode);
}
