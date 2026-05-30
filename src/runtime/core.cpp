#include "core.hpp"
#include "../../include/interpreter.hpp"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace runtime {

static void register_string_functions(Interpreter& interp) {
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

    interp.register_native("chr", [](std::vector<Value> args) -> Value {
        if(args.empty()) return Value::from_str("");
        char c = (char)(int)args[0].num;
        return Value::from_str(std::string(1, c));
    });
}

static void register_array_functions(Interpreter& interp) {
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
}

static void register_math_functions(Interpreter& interp) {
    interp.register_native("math_exp", [](std::vector<Value> args) {
        if(args.empty()) return Value::from_num(0.0);
        return Value::from_num(std::exp(args[0].num));
    });
}

static void register_map_functions(Interpreter& interp) {
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
}

static void register_time_functions(Interpreter& interp) {
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
}

static void register_file_functions(Interpreter& interp) {
    interp.register_native("file_read", [](std::vector<Value> args) -> Value {
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
}

static void register_router_functions(Interpreter& interp) {
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
}

void register_core_functions(Interpreter& interp) {
    register_string_functions(interp);
    register_array_functions(interp);
    register_math_functions(interp);
    register_map_functions(interp);
    register_time_functions(interp);
    register_file_functions(interp);
    register_router_functions(interp);
}

}
