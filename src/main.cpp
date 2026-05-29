#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/syscall.h>
#include <ctime>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <ncurses.h>
#include <locale.h>
#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include "interpreter.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cerr << "usage: <file.npl>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if(!file.is_open())
    {
        std::cerr << "erreur: impossible d'ouvrir " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buf;
    buf << file.rdbuf();
    std::string src = buf.str();

    std::vector<Token> tokens = tokenize(src);

    Parser parser(tokens);
    auto ast = parser.parse();

    Interpreter interp;
    interp.register_native("string_length", [](std::vector<Value> args) {
        return Value::from_num(args[0].str.size());
    });

    interp.register_native("string_upper", [](std::vector<Value> args) {
        std::string s = args[0].str;
        for(auto& c : s) c = std::toupper((unsigned char)c);

        return Value::from_str(s);
    });

    interp.register_native("string_lower", [](std::vector<Value> args) {
        std::string s = args[0].str;
        for(auto& c : s) c = std::tolower((unsigned char)c);

        return Value::from_str(s);
    });

    interp.register_native("string_trim", [](std::vector<Value> args) {
        std::string s = args[0].str;
        size_t start = s.find_first_not_of(" \t\n\r");
        size_t end = s.find_last_not_of(" \t\n\r");
        return Value::from_str(start == std::string::npos ? "" : s.substr(start, end - start + 1));
    });

    interp.register_native("string_contains", [](std::vector<Value> args) {
        return Value::from_bool(args[0].str.find(args[1].str) != std::string::npos);
    });

    interp.register_native("string_starts_with", [](std::vector<Value> args) {
        return Value::from_bool(args[0].str.starts_with(args[1].str));
    });

    interp.register_native("string_ends_with", [](std::vector<Value> args) {
        return Value::from_bool(args[0].str.ends_with(args[1].str));
    });

    interp.register_native("string_replace", [](std::vector<Value> args) {
        std::string s = args[0].str;
        std::string from = args[1].str;
        std::string to = args[2].str;
        size_t pos = 0;
        while((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
        return Value::from_str(s);
    });

    interp.register_native("string_split", [](std::vector<Value> args) {
        std::string s = args[0].str;
        std::string delim = args[1].str;
        std::vector<Value> result;
        size_t pos = 0, found;
        while((found = s.find(delim, pos)) != std::string::npos) {
            result.push_back(Value::from_str(s.substr(pos, found - pos)));
            pos = found + delim.size();
        }
        result.push_back(Value::from_str(s.substr(pos)));
        return Value::from_arr(result);
    });

    interp.register_native("string_join", [](std::vector<Value> args) {
        std::string delim = args[1].str;
        std::string result;
        for(size_t i = 0; i < args[0].arr.size(); i++) {
            result += args[0].arr[i].str;
            if(i + 1 < args[0].arr.size()) result += delim;
        }
        return Value::from_str(result);
    });

    interp.register_native("array_length", [](std::vector<Value> args) {
        return Value::from_num(args[0].arr.size());
    });

    interp.register_native("array_push", [](std::vector<Value> args) {
        std::vector<Value> arr = args[0].arr;
        arr.push_back(args[1]);
        return Value::from_arr(arr);
    });

    interp.register_native("array_pop", [](std::vector<Value> args) {
        std::vector<Value> arr = args[0].arr;
        if(arr.empty()) throw std::runtime_error("array_pop: tableau vide");
        arr.pop_back();
        return Value::from_arr(arr);
    });

    interp.register_native("array_last", [](std::vector<Value> args) {
        if(args[0].arr.empty()) throw std::runtime_error("array_last: tableau vide");
        return args[0].arr.back();
    });

    interp.register_native("array_slice", [](std::vector<Value> args) {
        std::vector<Value> arr = args[0].arr;
        size_t from = (size_t)args[1].num;
        size_t to   = (size_t)args[2].num;
        if(to > arr.size()) to = arr.size();
        return Value::from_arr(std::vector<Value>(arr.begin() + from, arr.begin() + to));
    });

    interp.register_native("array_reverse", [](std::vector<Value> args) {
        std::vector<Value> arr = args[0].arr;
        std::reverse(arr.begin(), arr.end());
        return Value::from_arr(arr);
    });

    // Lib math, l'exponentielle est faisable en npl pur, mais en terme de performance le c++ est bien meilleur
    interp.register_native("math_exp", [](std::vector<Value> args) {
        if(args.empty()) return Value::from_num(0.0);
        return Value::from_num(std::exp(args[0].num));
    });

    // Impossible sans ça de faire le json strinify en npl pur
    interp.register_native("map_keys", [](std::vector<Value> args) {
        std::vector<Value> keys;
        for(auto& [k, v] : args[0].map)
            keys.push_back(Value::from_str(k));
        return Value::from_arr(keys);
    });

    interp.register_native("value_type", [](std::vector<Value> args) {
        switch(args[0].type) {
            case Value::Type::Number:   return Value::from_str("number");
            case Value::Type::String:   return Value::from_str("string");
            case Value::Type::Bool:     return Value::from_str("bool");
            case Value::Type::Array:    return Value::from_str("array");
            case Value::Type::Map:      return Value::from_str("map");
            case Value::Type::Function: return Value::from_str("function");
            case Value::Type::Null:     return Value::from_str("null");
        }
        return Value::from_str("null");
    });

    interp.register_native("string_char_at", [](std::vector<Value> args) -> Value {
        if(args[0].type != Value::Type::String) throw std::runtime_error("string_char_at: attend une string");
        if(args[1].type != Value::Type::Number) throw std::runtime_error("string_char_at: attend un index");
        size_t i = (size_t)args[1].num;
        if(i >= args[0].str.size()) throw std::runtime_error("string_char_at: index hors limites");
        return Value::from_str(std::string(1, args[0].str[i]));
    });

    interp.register_native("string_to_num", [](std::vector<Value> args) -> Value {
        return Value::from_num(std::stod(args[0].str));
    });

    interp.register_native("string_to_str", [](std::vector<Value> args) -> Value {
        return Value::from_str(args[0].to_display());
    });

    interp.register_native("to_string", [](std::vector<Value> args) {
        if(args.empty()) return Value::from_str("");
        return Value::from_str(args[0].to_display());
    });

    interp.register_native("time_map", [](std::vector<Value> args) {
        time_t t = time(nullptr);
        struct tm* tm = localtime(&t);
        std::unordered_map<std::string, Value> result;
        result["seconds"] = Value::from_num(tm->tm_sec);
        result["minutes"] = Value::from_num(tm->tm_min);
        result["hours"]   = Value::from_num(tm->tm_hour);
        result["day"]     = Value::from_num(tm->tm_mday);
        result["month"]   = Value::from_num(tm->tm_mon + 1);
        result["year"]    = Value::from_num(tm->tm_year + 1900);
        return Value::from_map(result);
    });

    interp.register_native("time_parts", [](std::vector<Value> args) {
        time_t t = (time_t)args[0].num;
        struct tm* tm = localtime(&t);
        std::unordered_map<std::string, Value> result;
        result["seconds"] = Value::from_num(tm->tm_sec);
        result["minutes"] = Value::from_num(tm->tm_min);
        result["hours"]   = Value::from_num(tm->tm_hour);
        result["day"]     = Value::from_num(tm->tm_mday);
        result["month"]   = Value::from_num(tm->tm_mon + 1);
        result["year"]    = Value::from_num(tm->tm_year + 1900);
        return Value::from_map(result);
    });

    interp.register_native("file_read", [](std::vector<Value> args) -> Value {
         // Ouvre un fichier en lecture, lit tout le contenu, le retourne en string
        if(args.empty() || args[0].str.empty()) {
            throw std::runtime_error("file_read: chemin vide");
        }

        std::string path = args[0].str;
        std::ifstream f(path);
        if(!f.is_open()) {
            throw std::runtime_error("file_read: impossible d'ouvrir " + path);
        }

        std::stringstream buffer;
        buffer << f.rdbuf();
        std::string content = buffer.str();
        f.close();

        return Value::from_str(content);
    });

    interp.register_native("file_write", [](std::vector<Value> args) -> Value {
        if(args.size() < 2 || args[0].str.empty()) {
            return Value::from_bool(false);
        }

        std::string path = args[0].str;
        std::string content = args[1].str;

        std::ofstream f(path);
        if(!f.is_open()) return Value::from_bool(false);

        f << content;
        f.close();

        return Value::from_bool(true);
    });

    interp.register_native("file_exists", [](std::vector<Value> args) {
        if(args.empty()) return Value::from_bool(false);
        struct stat buffer;
        return Value::from_bool(stat(args[0].str.c_str(), &buffer) == 0);
    });

    interp.register_native("file_delete", [](std::vector<Value> args) -> Value {
        if(args.empty() || args[0].str.empty()) {
            return Value::from_bool(false);
        }
        return Value::from_bool(std::remove(args[0].str.c_str()) == 0);
    });

    interp.register_native("file_append", [](std::vector<Value> args) -> Value {
        if(args.size() < 2 || args[0].str.empty()) {
            return Value::from_bool(false);
        }

        std::ofstream f(args[0].str, std::ios::app);
        if(!f.is_open()) return Value::from_bool(false);

        f << args[1].str;
        f.close();

        return Value::from_bool(true);
    });

    interp.register_native("dir_list", [](std::vector<Value> args) -> Value {
        // retourne un tableau avec tout les chemins du dossier
        if(args.empty() || args[0].str.empty()) {
            return Value::from_arr({});
        }

        std::vector<Value> result;
        try {
            if(fs::exists(args[0].str) && fs::is_directory(args[0].str)) {
                for(const auto& entry : fs::directory_iterator(args[0].str)) {
                    result.push_back(Value::from_str(entry.path().string()));
                }
            }
        } catch(const std::exception& e) {
            // Retourne tableau vide au lieu de crash
            return Value::from_arr({});
        }
        return Value::from_arr(result);
    });

    interp.register_native("dir_exists", [](std::vector<Value> args) -> Value {
        if(args.empty() || args[0].str.empty()) {
            return Value::from_bool(false);
        }
        try {
            return Value::from_bool(fs::is_directory(args[0].str));
        } catch(...) {
            return Value::from_bool(false);
        }
    });

    interp.register_native("dir_create", [](std::vector<Value> args) -> Value {
        if(args.empty() || args[0].str.empty()) {
            return Value::from_bool(false);
        }
        try {
            return Value::from_bool(fs::create_directory(args[0].str));
        } catch(...) {
            return Value::from_bool(false);
        }
    });

    //================================================================
    //                      TUI (ncurse)
    //================================================================
    static int current_color = 0;
    static int current_style = 0;

    interp.register_native("curses_init", [](std::vector<Value> args) {
        setlocale(LC_ALL, "");

        initscr();

        if(has_colors()) {
            start_color();
            use_default_colors();
        }

        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);

        init_pair(1, COLOR_RED, -1);
        init_pair(2, COLOR_GREEN, -1);
        init_pair(3, COLOR_YELLOW, -1);
        init_pair(4, COLOR_BLUE, -1);
        init_pair(5, COLOR_MAGENTA, -1);
        init_pair(6, COLOR_CYAN, -1);
        init_pair(7, COLOR_WHITE, -1);

        init_pair(10, COLOR_BLACK, COLOR_RED);
        init_pair(11, COLOR_BLACK, COLOR_GREEN);
        init_pair(12, COLOR_BLACK, COLOR_YELLOW);
        init_pair(13, COLOR_BLACK, COLOR_BLUE);
        init_pair(14, COLOR_BLACK, COLOR_MAGENTA);
        init_pair(15, COLOR_BLACK, COLOR_CYAN);
        init_pair(16, COLOR_BLACK, COLOR_WHITE);

        refresh();

        return Value::null();
    });

    interp.register_native("curses_end", [](std::vector<Value> args) {
        endwin();
        return Value::null();
    });

    interp.register_native("curses_clear", [](std::vector<Value> args) {
        clear();
        return Value::null();
    });

    interp.register_native("curses_refresh", [](std::vector<Value> args) {
        refresh();
        return Value::null();
    });

    interp.register_native("curses_move", [](std::vector<Value> args) {

        int y = (int)args[0].num;
        int x = (int)args[1].num;
        int r = wmove(stdscr, y, x);

        return Value::null();
    });

    interp.register_native("curses_print", [](std::vector<Value> args) {

        std::string s = args[0].str;

        int y, x;
        getyx(stdscr, y, x);

        for (char c : s) {
            if (c == '\n') {
                y++;
                x = 0;
                wmove(stdscr, y, x);
            }
            else {
                waddch(stdscr, c);
            }
        }

        wrefresh(stdscr);
        return Value::null();
    });

    interp.register_native("curses_getch", [](std::vector<Value> args) {
        return Value::from_num(getch());
    });

    interp.register_native("curses_set_color", [](std::vector<Value> args) {
        int color = (int)args[0].num;
        int style = args.size() > 1 ? (int)args[1].num : 0;

        current_color = color;
        current_style = style;

        // Applique les styles
        if(style & 1) attron(A_BOLD);
        if(style & 2) attron(A_UNDERLINE);
        if(style & 4) attron(A_REVERSE);
        if(style & 8) attron(A_BLINK);
        if(style & 16) attron(A_DIM);

        // Applique la couleur
        if(color > 0) {
            attron(COLOR_PAIR(color));
        }

        return Value::null();
    });

    interp.register_native("curses_reset", [](std::vector<Value> args) {
        // Enlève les styles
        if(current_style & 1) attroff(A_BOLD);
        if(current_style & 2) attroff(A_UNDERLINE);
        if(current_style & 4) attroff(A_REVERSE);
        if(current_style & 8) attroff(A_BLINK);
        if(current_style & 16) attroff(A_DIM);

        // Enlève la couleur
        if(current_color > 0) {
            attroff(COLOR_PAIR(current_color));
        }

        current_color = 0;
        current_style = 0;

        return Value::null();
    });

    interp.register_native("chr", [](std::vector<Value> args) -> Value {
        if(args.empty()) return Value::from_str("");
        char c = (char)(int)args[0].num;
        return Value::from_str(std::string(1, c));
    });

    //================================================================
    //                      BACKEND (SERVEURS)
    //================================================================

    // TCP
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

        while(sent < data.size())
        {
            ssize_t n = write(
                fd,
                data.c_str() + sent,
                data.size() - sent
            );

            if(n <= 0)
                return Value::from_bool(false);

            sent += n;
        }

        return Value::from_bool(true);
    });

    interp.register_native("tcp_close", [](std::vector<Value> args) {
        int fd = (int)args[0].num;
        close(fd);
        return Value::null();
    });

    //=================================================================
    //                      Router
    // ================================================================
    struct Route {
        std::string method;
        std::string path;
        Value handler;
    };

    auto routes = std::make_shared<std::vector<Route>>();

    interp.register_native("route_add", [routes](std::vector<Value> args) {
        Route r;
        r.method  = args[0].str;
        r.path    = args[1].str;
        r.handler = args[2];
        routes->push_back(r);
        return Value::null();
    });

    interp.register_native("route_match", [routes](std::vector<Value> args) {
        std::string method = args[0].str;
        std::string path   = args[1].str;
        for(auto& r : *routes) {
            if(r.method == method && r.path == path)
                return r.handler;
        }
        return Value::null();
    });

    interp.run(ast.get());

    // Codegen codegen("output.asm");
    // codegen.generate(ast.get());
    // codegen.flush();

    return 0;
}
