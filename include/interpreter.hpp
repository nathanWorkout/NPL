#pragma once
#include "ast.hpp"
#include <string>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <fstream>
#include <functional>
#include "parser.hpp"
#include "lexer.hpp"

struct Value
{
    enum class Type { Number, String, Bool, Array, Map, Null } type = Type::Null;
    double num  = 0.0;
    std::string str;
    bool flag = false;
    std::vector<Value> arr;
    std::unordered_map<std::string, Value> map;

    static Value from_num (double v) { Value r; r.type=Type::Number; r.num=v; return r; }
    static Value from_str (const std::string& v) { Value r; r.type=Type::String; r.str=v; return r; }
    static Value from_bool(bool v) { Value r; r.type=Type::Bool; r.flag=v; return r; }
    static Value from_arr (std::vector<Value> v) { Value r; r.type=Type::Array; r.arr=std::move(v); return r; }
    static Value from_map (std::unordered_map<std::string, Value> v) { Value r; r.type=Type::Map; r.map=std::move(v); return r; }
    static Value null() { return {}; }

    bool truthy() const {
        switch(type){
            case Type::Number: return num != 0.0;
            case Type::String: return !str.empty();
            case Type::Bool:   return flag;
            case Type::Array:  return !arr.empty();
            case Type::Map:    return !map.empty();
            case Type::Null:   return false;
        }
        return false;
    }

    std::string to_display() const {
        switch(type){
            case Type::Number:
                if(num == std::floor(num) && std::abs(num) < 1e15)
                    return std::to_string((long long)num);
                {
                    std::string s = std::to_string(num);
                    s.erase(s.find_last_not_of('0') + 1);
                    if(s.back() == '.') s.erase(s.size()-1);
                    return s;
                }
            case Type::String: return str;
            case Type::Bool:   return flag ? "true" : "false";
            case Type::Array: {
                std::string s = "[";
                for(size_t i = 0; i < arr.size(); i++) {
                    s += arr[i].to_display();
                    if(i+1 < arr.size()) s += ", ";
                }
                return s + "]";
            }
            case Type::Map: {
                std::string s = "{";
                bool first = true;
                for(auto& [k, v] : map) {
                    if(!first) s += ", ";
                    s += "\"" + k + "\": " + v.to_display();
                    first = false;
                }
                return s + "}";
            }
            case Type::Null: return "null";
        }
        return "";
    }
};

struct ReturnException {
    Value value;
};

struct ScopeGuard {
    std::vector<std::unordered_map<std::string, Value>>& scopes;
    ~ScopeGuard() { scopes.pop_back(); }
};

class Interpreter
{
public:
    void run(ASTNode* node) {
        push_scope();
        execute(node);
    }

    std::unordered_map<std::string, std::function<Value(std::vector<Value>)>> natives_;
    void register_native(const std::string& name, std::function<Value(std::vector<Value>)> fn)
    {
        natives_[name] = fn;
    }

private:
    std::vector<std::unordered_map<std::string, Value>> scopes_;
    std::unordered_map<std::string, FuncDef*> funcs_;
    std::vector<std::unique_ptr<ASTNode>> loaded_asts_;

    void push_scope() { scopes_.push_back({}); }
    void pop_scope()  { scopes_.pop_back(); }

    Value get_var(const std::string& name) {
        for(int i = scopes_.size() - 1; i >= 0; i--)
            if(scopes_[i].count(name)) return scopes_[i][name];
        throw std::runtime_error("Variable inconnue : " + name);
    }

    void set_var(const std::string& name, Value val) {
        for(int i = scopes_.size() - 1; i >= 0; i--)
            if(scopes_[i].count(name)) {
                scopes_[i][name] = val;
                return;
            }
        scopes_.back()[name] = val;
    }

    void def_var(const std::string& name, Value val) {
        scopes_.back()[name] = val;
    }

    Value& get_var_ref(const std::string& name) {
        for(int i = scopes_.size() - 1; i >= 0; i--)
            if(scopes_[i].count(name))
                return scopes_[i][name];
        throw std::runtime_error("Variable inconnue : " + name);
    }

    void execute(ASTNode* node)
    {
        if(!node) return;
        if(auto n = dynamic_cast<Block*>(node))            exec_block(n);
        else if(auto n = dynamic_cast<Assign*>(node))      exec_assign(n);
        else if(auto n = dynamic_cast<IndexAssign*>(node)) exec_index_assign(n);
        else if(auto n = dynamic_cast<Output*>(node))      exec_output(n);
        else if(auto n = dynamic_cast<IfStmt*>(node))      exec_if(n);
        else if(auto n = dynamic_cast<RepeatStmt*>(node))  exec_repeat(n);
        else if(auto n = dynamic_cast<WhileStmt*>(node))   exec_while(n);
        else if(auto n = dynamic_cast<ForStmt*>(node))     exec_for(n);
        else if(auto n = dynamic_cast<FuncDef*>(node))     exec_funcdef(n);
        else if(auto n = dynamic_cast<FuncCall*>(node))    exec_funccall(n);
        else if(auto n = dynamic_cast<ReturnStmt*>(node))  throw ReturnException{eval(n->value.get())};
        else if(auto n = dynamic_cast<InputStmt*>(node))   exec_input(n);
        else if(auto n = dynamic_cast<UseStmt*>(node))     exec_use(n);
    }

    Value eval(ASTNode* node)
    {
        if(!node) return Value::null();

        if(auto n = dynamic_cast<NumberLit*>(node)) return Value::from_num(n->value);

        if(auto n = dynamic_cast<StringLit*>(node)) return Value::from_str(n->value);

        if(auto n = dynamic_cast<BoolLit*>(node)) return Value::from_bool(n->value);

        if(auto n = dynamic_cast<Identifier*>(node)) return get_var(n->name);

        if(auto n = dynamic_cast<ArrayLit*>(node)) {
            std::vector<Value> elems;
            for(auto& e : n->elements) elems.push_back(eval(e.get()));
            return Value::from_arr(std::move(elems));
        }

        if(auto n = dynamic_cast<MapLit*>(node)) {
            std::unordered_map<std::string, Value> m;
            for(auto& [k, v] : n->entries) m[k] = eval(v.get());
            return Value::from_map(std::move(m));
        }

        if(auto n = dynamic_cast<IndexExpr*>(node)) {
            Value container = get_var(n->name);
            Value idx = eval(n->index.get());
            if(container.type == Value::Type::Array) {
                if(idx.type != Value::Type::Number) throw std::runtime_error("Index tableau doit être un nombre");
                size_t i = (size_t)idx.num;
                if(i >= container.arr.size()) throw std::runtime_error("Index hors limites");
                return container.arr[i];
            }
            if(container.type == Value::Type::Map) {
                if(idx.type != Value::Type::String) throw std::runtime_error("Clé map doit être une string");
                auto it = container.map.find(idx.str);
                if(it == container.map.end()) throw std::runtime_error("Clé introuvable : " + idx.str);
                return it->second;
            }
            throw std::runtime_error(n->name + " n'est pas un tableau ou une map");
        }

        if(auto n = dynamic_cast<BinOp*>(node))   return eval_binop(n);
        if(auto n = dynamic_cast<FuncCall*>(node)) return exec_funccall(n);

        return Value::null();
    }

    Value eval_binop(BinOp* n)
    {
        Value l = eval(n->lhs.get());
        Value r = eval(n->rhs.get());

        if(l.type == Value::Type::Number && r.type == Value::Type::Number) {
            double lv = l.num, rv = r.num;
            if(n->op == "+")  return Value::from_num(lv + rv);
            if(n->op == "-")  return Value::from_num(lv - rv);
            if(n->op == "*")  return Value::from_num(lv * rv);
            if(n->op == "/") {
                if(rv == 0.0) throw std::runtime_error("Division par zéro");
                return Value::from_num(lv / rv);
            }
            if(n->op == ">")  return Value::from_bool(lv >  rv);
            if(n->op == "<")  return Value::from_bool(lv <  rv);
            if(n->op == "==") return Value::from_bool(lv == rv);
            if(n->op == "!=") return Value::from_bool(lv != rv);
            if(n->op == ">=") return Value::from_bool(lv >= rv);
            if(n->op == "<=") return Value::from_bool(lv <= rv);
            if(n->op == "%")  return Value::from_num(std::fmod(lv, rv));
        }
        if(l.type == Value::Type::String && r.type == Value::Type::String) {
            if(n->op == "+")  return Value::from_str(l.str + r.str);
            if(n->op == "==") return Value::from_bool(l.str == r.str);
            if(n->op == "!=") return Value::from_bool(l.str != r.str);
        }
        if(l.type == Value::Type::Bool && r.type == Value::Type::Bool) {
            if(n->op == "==") return Value::from_bool(l.flag == r.flag);
            if(n->op == "!=") return Value::from_bool(l.flag != r.flag);
            if(n->op == "&&") return Value::from_bool(l.flag && r.flag);
            if(n->op == "||") return Value::from_bool(l.flag || r.flag);
        }
        if(l.type == Value::Type::String && r.type == Value::Type::Number) {
            if(n->op == "+") return Value::from_str(l.str + r.to_display());
        }
        if(l.type == Value::Type::Number && r.type == Value::Type::String) {
            if(n->op == "+") return Value::from_str(l.to_display() + r.str);
        }

        throw std::runtime_error("Opérateur invalide : " + n->op);
    }

    void exec_block(Block* node) {
        for(auto& s : node->statements) execute(s.get());
    }

    void exec_assign(Assign* node) {
        set_var(node->name, eval(node->value.get()));
    }

    void exec_index_assign(IndexAssign* node) {
        Value& container = get_var_ref(node->name);
        Value idx = eval(node->index.get());
        if(container.type == Value::Type::Array) {
            if(idx.type != Value::Type::Number) throw std::runtime_error("Index doit être un nombre");
            size_t i = (size_t)idx.num;
            if(i >= container.arr.size()) throw std::runtime_error("Index hors limites");
            container.arr[i] = eval(node->value.get());
            return;
        }
        if(container.type == Value::Type::Map) {
            if(idx.type != Value::Type::String) throw std::runtime_error("Clé map doit être une string");
            container.map[idx.str] = eval(node->value.get());
            return;
        }
        throw std::runtime_error(node->name + " n'est pas un tableau ou une map");
    }

    void exec_output(Output* node) {
        std::cout << eval(node->value.get()).to_display() << "\n";
    }

    void exec_if(IfStmt* node) {
        for(auto& [cond, body] : node->branches) {
            if(eval(cond.get()).truthy()) {
                execute(body.get());
                return;
            }
        }
        if(node->else_body) execute(node->else_body.get());
    }

    void exec_repeat(RepeatStmt* node) {
        Value cv = eval(node->count.get());
        if(cv.type != Value::Type::Number) throw std::runtime_error("repeat attend un nombre");
        long long count = (long long)cv.num;
        for(long long it = 0; it < count; it++) execute(node->body.get());
    }

    void exec_while(WhileStmt* node) {
        while(eval(node->condition.get()).truthy()) execute(node->body.get());
    }

    void exec_for(ForStmt* node) {
        if(node->init) execute(node->init.get());
        while(!node->condition || eval(node->condition.get()).truthy()) {
            execute(node->body.get());
            if(node->increment) execute(node->increment.get());
        }
    }

    void exec_funcdef(FuncDef* node) {
        funcs_[node->name] = node;
    }

    Value exec_funccall(FuncCall* node) {
        auto nat = natives_.find(node->name);
        if(nat != natives_.end()) {
            std::vector<Value> args;
            for(auto& a : node->args)
                args.push_back(eval(a.get()));
            return nat->second(args);
        }
        auto it = funcs_.find(node->name);
        if(it == funcs_.end()) throw std::runtime_error("Fonction inconnue : " + node->name);

        FuncDef* func = it->second;
        if(node->args.size() != func->params.size()) throw std::runtime_error("Nombre d'arguments incorrect pour : " + node->name);

        std::vector<Value> arg_vals;
        for(size_t i = 0; i < func->params.size(); i++) arg_vals.push_back(eval(node->args[i].get()));

        push_scope();
        ScopeGuard guard{scopes_};

        for(size_t i = 0; i < func->params.size(); i++) def_var(func->params[i], arg_vals[i]);

        Value result = Value::null();
        try {
            execute(func->body.get());
        } catch(ReturnException& ret) {
            result = ret.value;
        }

        return result;
    }

    void exec_input(InputStmt* node) {
        if(!node->prompt.empty()) std::cout << node->prompt;
        std::string input;
        std::getline(std::cin, input);
        try { set_var(node->name, Value::from_num(std::stod(input))); }
        catch(...) { set_var(node->name, Value::from_str(input)); }
    }

    void exec_use(UseStmt* node) {
        std::string path = "/usr/local/lib/npl/" + node->lib + ".npl";
        std::ifstream file(path);
        if(!file.is_open()) throw std::runtime_error("Lib introuvable : " + node->lib);
        std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        auto tokens = tokenize(src);
        Parser parser(tokens);
        auto ast = parser.parse();
        loaded_asts_.push_back(std::move(ast));
        execute(loaded_asts_.back().get());
    }
};