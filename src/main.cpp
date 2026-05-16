#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
// #include "codegen.hpp"
#include "interpreter.hpp"

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
        for(auto& c : s) c = std::toupper(c);
        return Value::from_str(s);
    });

    interp.register_native("string_lower", [](std::vector<Value> args) {
        std::string s = args[0].str;
        for(auto& c : s) c = std::tolower(c);
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
            case Value::Type::Number: return Value::from_str("number");
            case Value::Type::String: return Value::from_str("string");
            case Value::Type::Bool:   return Value::from_str("bool");
            case Value::Type::Array:  return Value::from_str("array");
            case Value::Type::Map:    return Value::from_str("map");
            case Value::Type::Null:   return Value::from_str("null");
        }
        return Value::from_str("null");
    });


    interp.run(ast.get());

    // Codegen codegen("output.asm");
    // codegen.generate(ast.get());
    // codegen.flush();

    return 0;
}