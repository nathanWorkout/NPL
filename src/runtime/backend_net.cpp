#include "backend_net.hpp"
#include "../../include/interpreter.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace runtime {

void register_net_functions(Interpreter& interp) {
    interp.register_native("tcp_listen", [](std::vector<Value> args) {
        int port = (int)args[0].num;
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) return Value::from_num(-1);

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(server_fd);
            return Value::from_num(-1);
        }
        if (listen(server_fd, 10) < 0) {
            close(server_fd);
            return Value::from_num(-1);
        }
        return Value::from_num(server_fd);
    });

    interp.register_native("tcp_accept", [](std::vector<Value> args) {
        int server_fd = (int)args[0].num;
        int client_fd = accept(server_fd, nullptr, nullptr);
        return Value::from_num(client_fd);
    });

    interp.register_native("tcp_read", [](std::vector<Value> args) {
        int fd = (int)args[0].num;
        char buffer[8192];
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n <= 0) return Value::from_str("");
        buffer[n] = '\0';
        return Value::from_str(std::string(buffer));
    });

    interp.register_native("tcp_write", [](std::vector<Value> args) {
        int fd = (int)args[0].num;
        std::string data = args[1].str;

        size_t sent = 0;
        while(sent < data.size()) {
            ssize_t n = write(fd, data.c_str() + sent, data.size() - sent);
            if(n <= 0) return Value::from_bool(false);
            sent += n;
        }
        return Value::from_bool(true);
    });

    interp.register_native("tcp_close", [](std::vector<Value> args) {
        int fd = (int)args[0].num;
        close(fd);
        return Value::null();
    });
}

}
