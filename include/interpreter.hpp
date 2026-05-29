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
#include <unordered_set>
#include "parser.hpp"
#include "lexer.hpp"

struct Value
{
    enum class Type { Number, String, Bool, Array, Map, Null, Function } type = Type::Null;
    double num  = 0.0;
    std::string str;
    bool flag = false;
    std::vector<Value> arr;
    std::unordered_map<std::string, Value> map;

    // permettre l'appel de focntion dans les arguments des fonctions
    struct {
        FuncDef* func;   // pour les fonctions utilisateur
    } fn;

    static Value from_func(FuncDef* f) {
        Value v;
        v.type = Type::Function;
        v.fn.func = f;
        return v;
    }

    // réserve une boîte pour le type et r...=v est la valeure qu'on met dedans
    static Value from_num (double v) { Value r; r.type=Type::Number; r.num=v; return r; }
    static Value from_str (const std::string& v) { Value r; r.type=Type::String; r.str=v; return r; }
    static Value from_bool(bool v) { Value r; r.type=Type::Bool; r.flag=v; return r; }
    static Value from_arr (std::vector<Value> v) { Value r; r.type=Type::Array; r.arr=std::move(v); return r; }
    static Value from_map (std::unordered_map<std::string, Value> v) { Value r; r.type=Type::Map; r.map=std::move(v); return r; }
    static Value null() { return {}; }

    // retourne vrai ou faux
    bool truthy() const {
        switch(type){
            case Type::Number: return num != 0.0;
            case Type::String: return !str.empty();
            case Type::Bool:   return flag;
            case Type::Array:  return !arr.empty();
            case Type::Map:    return !map.empty();
            case Type::Function: return true;
            case Type::Null:   return false;
        }
        return false;
    }


    std::string to_display() const {
        switch(type){
            case Type::Number:
                if(num == std::floor(num) && std::abs(num) < 1e15) // floor = valeure arondie, si vrai on le convertie en int (sasn virgule)
                    return std::to_string((long long)num);
                {
                    // si il y a des chiffres apès la virgule
                    // et si c'est 5.0000 ça convertit en txt, enlève les 0 et le point
                    std::string s = std::to_string(num);
                    s.erase(s.find_last_not_of('0') + 1);
                    if(s.back() == '.') s.erase(s.size()-1);
                    return s;
                }
            case Type::String: return str;
            case Type::Bool:   return flag ? "true" : "false";

            // si c'est des tableaux, on parcours tout les élements du tableuax
            // on met des virgules
            // si il y a un tableua dans un tableau, il relance la fonciton pour celui-ci, ce qui va le nettoyer
            case Type::Array: {
                std::string s = "[";
                for(size_t i = 0; i < arr.size(); i++) {
                    s += arr[i].to_display();
                    if(i+1 < arr.size()) s += ", ";
                }
                return s + "]";
            }

            // si ce sont des maps
            // on parcours la map, k = key, v = value
            // si ce n'est pas le 1er élement, on met une virgule après
            case Type::Map: {
                std::string s = "{";
                bool first = true;
                for(auto& [k, v] : map) {
                    if(!first) s += ", ";

                    // si la valeur retournée est une string
                    // on l'entoure de "" ("nom": "nathan")
                    if(v.type == Type::String)
                        s += "\"" + k + "\": \"" + v.str + "\"";
                    else
                        s += "\"" + k + "\": " + v.to_display();
                    first = false; // pour mettre les virgules
                }
                return s + "}";
            }
            case Type::Function: return "function";

            case Type::Null: return "null";
        }
        return "";
    }
};

// quand il y a un return,  -> exeption -> transporte la value
struct ReturnException {
    Value value;
};

struct BreakException {};
struct ContinueException {};

// scope : espace entre 2 {}
// pop_back supprime les variables locales créer dans le bloc
struct ScopeGuard {
    std::vector<std::unordered_map<std::string, Value>>& scopes;
    ~ScopeGuard() { scopes.pop_back(); }
};

// unordered_map : Un dictionnaire qui associe le nom d'une variable (ex: "x") à sa valeur réelle.

class Interpreter
{
public:
void run(ASTNode* node) {
        push_scope();
        if(auto block = dynamic_cast<Block*>(node)) {
            for(auto& s : block->statements) {
                if(!s) continue;
                if(auto fn = dynamic_cast<FuncDef*>(s.get()))
                    exec_funcdef(fn);
                else
                    execute(s.get());
            }
            return;
        }
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
    std::unordered_set<std::string> loaded_libs_;

    void push_scope() { scopes_.push_back({}); }
    void pop_scope()  { scopes_.pop_back(); }

    void set_var(const std::string& name, Value val) {
        for(int i = scopes_.size() - 1; i >= 0; i--) {
            if(scopes_[i].count(name)) {
                scopes_[i][name] = val;
                return;
            }
        }
        scopes_.back()[name] = val;
    }

    void def_var(const std::string& name, Value val) {
        scopes_.back()[name] = val;
    }

    Value get_var(const std::string& name) {
        for(int i = scopes_.size() - 1; i >= 0; i--)
            if(scopes_[i].count(name)) return scopes_[i][name];
        throw std::runtime_error("Variable inconnue : " + name);
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
        else if(auto n = dynamic_cast<TryStmt*>(node))     exec_try(n);
        else if(auto n = dynamic_cast<ThrowStmt*>(node)) {
            throw std::runtime_error(eval(n->value.get()).to_display());
        }
        else if(auto n = dynamic_cast<ExprStatement*>(node)) eval(n->expr.get());
        else if(auto n = dynamic_cast<BreakStmt*>(node)) throw BreakException();
        else if(auto n = dynamic_cast<ContinueStmt*>(node)) throw ContinueException();
    }

    Value eval(ASTNode* node)
    {
        if(!node) return Value::null();

        if(auto n = dynamic_cast<NumberLit*>(node)) return Value::from_num(n->value);

        if(auto n = dynamic_cast<StringLit*>(node)) return Value::from_str(n->value);

        if(auto n = dynamic_cast<BoolLit*>(node)) return Value::from_bool(n->value);

        if(auto n = dynamic_cast<Identifier*>(node)) {
            try {
                return get_var(n->name);
            } catch(std::runtime_error&) {
                // Si la variable n'existe pas, regarde si une fonction du même nom est deja définie
                auto it = funcs_.find(n->name);
                if(it != funcs_.end())
                    return Value::from_func(it->second);
                throw; // met l'erreur variable inconnue
            }
        }

        // pour créer un tableau :créer une Value de type array, boucle sur chaques élements contenu entre crochets
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
                size_t i = (idx.type == Value::Type::Number) ? (size_t)idx.num : (size_t)std::stod(idx.str);

                if(i >= container.arr.size()) throw std::runtime_error("Index tableau hors limites : " + std::to_string(i));
                return container.arr[i];
            }

            if(container.type == Value::Type::Map) {
                if(idx.type != Value::Type::String) {
                    idx = Value::from_str(idx.to_display());
                }
                auto it = container.map.find(idx.str);
                if(it == container.map.end()) throw std::runtime_error("Clé introuvable dans la map : " + idx.str);
                return it->second;
            }

            throw std::runtime_error("L'objet '" + n->name + "' n'est ni un tableau ni une map");
        }

        if(auto n = dynamic_cast<BinOp*>(node))   return eval_binop(n);
        if(auto n = dynamic_cast<FuncCall*>(node)) return exec_funccall(n);
        if(auto n = dynamic_cast<NullLit*>(node)) return Value::null();
        if(auto n = dynamic_cast<PipeExpr*>(node)) return eval_pipe(n);

        if(auto n = dynamic_cast<Placeholder*>(node)) {
            return get_var("_");
        }

        if(auto n = dynamic_cast<InputExpr*>(node)) {
            if(!n->prompt.empty()) std::cout << n->prompt;
            std::string input;
            std::getline(std::cin, input);
            try { return Value::from_num(std::stod(input)); }
            catch(...) { return Value::from_str(input); }
        }

        if(auto n = dynamic_cast<FuncDef*>(node)) {
            if(n->name.empty())
                return Value::from_func(n);
            exec_funcdef(n);
            return get_var(n->name);
        }

        return Value::null();
    }

    Value eval_binop(BinOp* n) {
        if (n->op == "&&") {
            Value left = eval(n->lhs.get());
            if (!left.truthy()) return Value::from_bool(false);
            Value right = eval(n->rhs.get());
            return Value::from_bool(right.truthy());
        }
        if (n->op == "||") {
            Value left = eval(n->lhs.get());
            if (left.truthy()) return Value::from_bool(true);
            Value right = eval(n->rhs.get());
            return Value::from_bool(right.truthy());
        }

        Value l = eval(n->lhs.get());
        Value r = eval(n->rhs.get());

        // si l'un des 2 côtés est du texte, concaténation ("Salut" + 5 -> "Salut 5")
        // sinon addition
        if (n->op == "+" && (l.type == Value::Type::String || r.type == Value::Type::String)) {
            return Value::from_str(l.to_display() + r.to_display());
        }

        auto to_double = [](const Value& v) {
            if (v.type == Value::Type::Number) return v.num;
            if (v.type == Value::Type::String) {
                try { return std::stod(v.str); } catch (...) { return 0.0; }
            }
            if (v.type == Value::Type::Bool) return v.flag ? 1.0 : 0.0;
            return 0.0;
        };

        if (l.type == Value::Type::Number || r.type == Value::Type::Number) {
            double lv = to_double(l);
            double rv = to_double(r);

            // pour les comparaison, on transforme en texte
            // puis on évalue
            if (n->op == "+")  return Value::from_num(lv + rv);
            if (n->op == "-")  return Value::from_num(lv - rv);
            if (n->op == "*")  return Value::from_num(lv * rv);
            if (n->op == "/")  return Value::from_num(rv == 0 ? 0 : lv / rv);
            if (n->op == "%")  return Value::from_num(std::fmod(lv, rv));
            if (n->op == "==") return Value::from_bool(lv == rv);
            if (n->op == "!=") return Value::from_bool(lv != rv);
            if (n->op == ">")  return Value::from_bool(lv >  rv);
            if (n->op == "<")  return Value::from_bool(lv <  rv);
            if (n->op == ">=") return Value::from_bool(lv >= rv);
            if (n->op == "<=") return Value::from_bool(lv <= rv);
        }

        if (l.type == Value::Type::String && r.type == Value::Type::String) {
            if (n->op == "+")  return Value::from_str(l.str + r.str);
            if (n->op == "==") return Value::from_bool(l.str == r.str);
            if (n->op == "!=") return Value::from_bool(l.str != r.str);
            if (n->op == ">")  return Value::from_bool(l.str >  r.str);
            if (n->op == "<")  return Value::from_bool(l.str <  r.str);
            if (n->op == ">=") return Value::from_bool(l.str >= r.str);
            if (n->op == "<=") return Value::from_bool(l.str <= r.str);
        }

        if (l.type == Value::Type::Bool && r.type == Value::Type::Bool) {
            if (n->op == "==") return Value::from_bool(l.flag == r.flag);
            if (n->op == "!=") return Value::from_bool(l.flag != r.flag);
        }

        if(n->op == "==") return Value::from_bool(false);
        if(n->op == "!=") return Value::from_bool(true);

        throw std::runtime_error("Type Mismatch : Impossible d'utiliser l'opérateur '" +
                                n->op + "' entre " + l.to_display() + " et " + r.to_display());
    }

    void exec_block(Block* node) {
        for(auto& s : node->statements) execute(s.get());
    }

    void exec_assign(Assign* node) {
        Value val = eval(node->value.get());
        set_var(node->name, val);
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

        double count;

        if(cv.type == Value::Type::Number) {
            count = cv.num;
        }
        else if(cv.type == Value::Type::String) {
            try {
                count = std::stod(cv.str);
            } catch(...) {
                throw std::runtime_error("repeat attend un nombre");
            }
        }
        else {
            throw std::runtime_error("repeat attend un nombre");
        }

        long long n = (long long)count;

        for(long long i = 0; i < n; i++) {
            try {
                execute(node->body.get());
            } catch(BreakException&) {
                break;
            } catch(ContinueException&) {
                continue;
            }
        }
    }

    void exec_while(WhileStmt* node) {
        while(true) {
            Value cond = eval(node->condition.get());
            if (!cond.truthy()) break;
            try {
                execute(node->body.get());
            } catch(BreakException&) {
                break;
            } catch(ContinueException&) {
                continue;
            }
        }
    }

    void exec_for(ForStmt* node) {
        if(node->init) execute(node->init.get());
        while(!node->condition || eval(node->condition.get()).truthy()) {
            try {
                execute(node->body.get());
            } catch(BreakException&) {
                break;
            } catch(ContinueException&) {}
            if(node->increment) execute(node->increment.get());
        }
    }

   void exec_funcdef(FuncDef* node) {
        funcs_[node->name] = node;
        def_var(node->name, Value::from_func(node));// créer une variable du nom de la fonction qui contient la fonction
    }

    Value exec_funccall(FuncCall* node) {
        // Vérifie si c'est une variable de type fonction
        try {
            Value var = get_var(node->name);
            if(var.type == Value::Type::Function) {
                std::vector<Value> args;
                for(auto& a : node->args) args.push_back(eval(a.get()));
                FuncDef* f = var.fn.func;
                if(args.size() != f->params.size())
                    throw std::runtime_error("Nombre d'arguments incorrect pour " + node->name);
                push_scope();
                ScopeGuard guard{scopes_};
                def_var("__fn_boundary__", Value::from_bool(true));
                for(size_t i = 0; i < f->params.size(); i++)
                    def_var(f->params[i], args[i]);
                Value result = Value::null();
                try {
                    execute(f->body.get());
                } catch(ReturnException& ret) {
                    result = ret.value;
                }
                return result;
            }
        } catch(ReturnException&) {
            throw;
        } catch(std::runtime_error&) {
            // variable pas trouvée ou pas une Function -> continuer vers les fonctions natives_
        }

        auto nat = natives_.find(node->name); // ffi c++
        if(nat != natives_.end()) {
            std::vector<Value> args;
            for(auto& a : node->args)
                args.push_back(eval(a.get()));
            return nat->second(args);
        }

        auto it = funcs_.find(node->name);
        if(it == funcs_.end()) throw std::runtime_error("Fonction inconnue : " + node->name);

        FuncDef* func = it->second;
        if(node->args.size() != func->params.size())
            throw std::runtime_error("Nombre d'arguments incorrect pour : " + node->name);

        std::vector<Value> arg_vals;
        for(size_t i = 0; i < func->params.size(); i++) {
            arg_vals.push_back(eval(node->args[i].get()));
        }

        push_scope(); // créer un nouvel étage de variables
        ScopeGuard guard{scopes_};
        def_var("__fn_boundary__", Value::from_bool(true)); // dire ici commence une fonction

        for(size_t i = 0; i < func->params.size(); i++) {
            def_var(func->params[i], arg_vals[i]);
        }

        Value result = Value::null();
        try {
            execute(func->body.get()); // lancer le code
        } catch(ReturnException& ret) {
            result = ret.value;        // attrape le résultat du return
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
        if(loaded_libs_.count(node->lib)) return; // évite les overflow dues au mutiples utilisation du fichier

        std::string lib = node->lib;
        size_t pos;
        while((pos = lib.find("//")) != std::string::npos) lib.replace(pos, 2, "/");

        std::string path = "/usr/local/lib/npl/" + lib + ".npl";
        std::ifstream file(path);
        if(!file.is_open()) throw std::runtime_error("Lib introuvable : " + node->lib);
        std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        auto tokens = tokenize(src); //lexer
        Parser parser(tokens); // parser
        auto ast = parser.parse(); // ast
        loaded_asts_.push_back(std::move(ast)); // mettre enn mémoire

        if(auto block = dynamic_cast<Block*>(loaded_asts_.back().get())) {
            for(auto& s : block->statements) {
                if(auto fn = dynamic_cast<FuncDef*>(s.get()))
                    exec_funcdef(fn);
                else if(!dynamic_cast<FuncCall*>(s.get())) // ne pas exécuter les FuncCall top-level
                    execute(s.get());
            }
        } else {
            execute(loaded_asts_.back().get());
        }
    }

    void exec_try(TryStmt* node) {
        try {
            execute(node->body.get());
        } catch(std::runtime_error& e) {
            push_scope();
            ScopeGuard guard{scopes_};
            def_var(node->error_var, Value::from_str(e.what()));
            execute(node->catch_body.get());
        }
    }

    Value eval_pipe(PipeExpr* n)
    {
        Value left = eval(n->lhs.get());
        set_var("_", left);

        // si le côté droit est un >>, on affiche
        if(auto out = dynamic_cast<Output*>(n->rhs.get())) {

            // si le côté gauche est un tableau
            if(left.type == Value::Type::Array) {

                for(auto& item : left.arr) {

                    // _ devient l'élément actuel
                    set_var("_", item);

                    // >> avec une expression
                    if(out->value)
                        std::cout << eval(out->value.get()).to_display() << "\n";

                    // >> seul
                    else
                        std::cout << item.to_display() << "\n";
                }

                return left;
            }

            // si ce n'est pas un tableau
            set_var("_", left);

            // >> avec une expression : on l'évalue avec _ = left
            if(out->value) {
                std::cout << eval(out->value.get()).to_display() << "\n";
            }

            // >> seul sans expression
            else {
                std::cout << left.to_display() << "\n";
            }

            return left;
        }

        // si le côté droit est un bloc lambda { }
        if(auto lambda = dynamic_cast<LambdaBlock*>(n->rhs.get())) {
            if(left.type == Value::Type::Array) {
                std::vector<Value> result;

                for(auto& item : left.arr) {
                    push_scope();
                    ScopeGuard guard{scopes_};

                    def_var("item", item);

                    Value val = Value::null();
                    try {
                        execute(lambda->body.get());
                    } catch(ReturnException& ret) {
                        val = ret.value;
                    }

                    result.push_back(val);
                }

                return Value::from_arr(result);
            }

            return left;
        }

        ASTNode* rhs = n->rhs.get();

        if(auto call = dynamic_cast<FuncCall*>(rhs)) {

            // cas spécial : repeat (pipeline natif)
            if(call->name == "repeat") {

                Value pattern;

                // repeat "A"
                if(call->args.size() >= 1) {
                    pattern = eval(call->args[0].get());
                }

                // repeat | "A"
                else {
                    if(auto lit = dynamic_cast<StringLit*>(rhs)) {
                        pattern = Value::from_str(lit->value);
                    } else {
                        pattern = eval(rhs);
                    }
                }

                if(pattern.type != Value::Type::String) pattern = Value::from_str(pattern.to_display());

                if(left.type != Value::Type::Number)
                    throw std::runtime_error("repeat attend un nombre à gauche");

                long long n = (long long)left.num;

                std::vector<Value> out;

                for(long long i = 0; i < n; i++) out.push_back(Value::from_str(pattern.str));

                return Value::from_arr(out);
            }

            // vérifie si _ est présent dans les args
            bool has_placeholder = false;
            for(auto& arg : call->args) {
                if(dynamic_cast<Placeholder*>(arg.get())) {
                    has_placeholder = true;
                    break;
                }
            }

            if(has_placeholder) {

                // remplace chaque _ par left, sans injection automatique
                std::vector<Value> args_values;

                for(auto& arg : call->args) {
                    if(dynamic_cast<Placeholder*>(arg.get()))
                        args_values.push_back(left);
                    else
                        args_values.push_back(eval(arg.get()));
                }

                // vérifie si c'est une variable de type fonction
                try {
                    Value var = get_var(call->name);
                    if(var.type == Value::Type::Function) {
                        FuncDef* f = var.fn.func;

                        push_scope();
                        ScopeGuard guard{scopes_};

                        def_var("__fn_boundary__", Value::from_bool(true));

                        for(size_t i = 0; i < f->params.size(); i++)
                            def_var(f->params[i], args_values[i]);

                        Value result = Value::null();

                        try {
                            execute(f->body.get());
                        } catch(ReturnException& ret) {
                            result = ret.value;
                        }

                        return result;
                    }
                } catch(...) {}

                // cherche native ou fonction utilisateur
                auto nat = natives_.find(call->name);
                auto it  = funcs_.find(call->name);

                if(nat != natives_.end())
                    return nat->second(args_values);

                if(it != funcs_.end()) {
                    FuncDef* func = it->second;

                    push_scope();
                    ScopeGuard guard{scopes_};

                    def_var("__fn_boundary__", Value::from_bool(true));

                    for(size_t i = 0; i < func->params.size(); i++)
                        def_var(func->params[i], args_values[i]);

                    Value result = Value::null();

                    try {
                        execute(func->body.get());
                    } catch(ReturnException& ret) {
                        result = ret.value;
                    }

                    return result;
                }

            } else {

                // comportement classique : injection implicite en 1er arg

                std::vector<Value> args_values;
                args_values.push_back(left);

                for(auto& arg : call->args)
                    args_values.push_back(eval(arg.get()));

                if(call->name == "filter") {
                    auto* lambda = dynamic_cast<LambdaBlock*>(call->args[0].get());
                    if(!lambda) throw std::runtime_error("filter attend un bloc { }");

                    std::vector<Value> result;

                    for(auto& item : left.arr) {
                        push_scope();
                        ScopeGuard guard{scopes_};

                        def_var("item", item);

                        Value cond = Value::null();
                        try {
                            execute(lambda->body.get());
                        } catch(ReturnException& ret) {
                            cond = ret.value;
                        }

                        if(cond.truthy())
                            result.push_back(item);
                    }

                    return Value::from_arr(result);
                }

                if(call->name == "each") {
                    auto* lambda = dynamic_cast<LambdaBlock*>(call->args[0].get());
                    if(!lambda) throw std::runtime_error("each attend un bloc { }");

                    std::vector<Value> result;

                    for(auto& item : left.arr) {
                        push_scope();
                        ScopeGuard guard{scopes_};

                        def_var("item", item);

                        Value val = Value::null();
                        try {
                            execute(lambda->body.get());
                        } catch(ReturnException& ret) {
                            val = ret.value;
                        }

                        result.push_back(val);
                    }

                    return Value::from_arr(result);
                }

                // récupère le nom de la fonction
                std::string func_name = call->name;

                // stocke les arguments
                // met la valeur de gauche en 1er arg

                try {
                    Value var = get_var(call->name);

                    if(var.type == Value::Type::Function) {
                        FuncDef* f = var.fn.func;

                        if(args_values.size() != f->params.size())
                            throw std::runtime_error("Nombre d'arguments incorrect pour " + call->name);

                        push_scope();
                        ScopeGuard guard{scopes_};

                        def_var("__fn_boundary__", Value::from_bool(true));

                        for(size_t i = 0; i < f->params.size(); i++)
                            def_var(f->params[i], args_values[i]);

                        Value result = Value::null();

                        try {
                            execute(f->body.get());
                        } catch(ReturnException& ret) {
                            result = ret.value;
                        }

                        return result;
                    }
                } catch(...) {}

                // cherche fonction native ou fonction utilisateur
                auto nat = natives_.find(func_name);
                auto it  = funcs_.find(func_name);

                if(nat != natives_.end())
                    return nat->second(args_values);

                if(it != funcs_.end()) {
                    FuncDef* func = it->second;

                    if(args_values.size() == func->params.size()) {

                        push_scope();
                        ScopeGuard guard{scopes_};

                        def_var("__fn_boundary__", Value::from_bool(true));

                        for(size_t i = 0; i < func->params.size(); i++) {
                            def_var(func->params[i], args_values[i]);
                        }

                        Value result = Value::null();

                        try {
                            execute(func->body.get());
                        } catch(ReturnException& ret) {
                            result = ret.value;
                        }

                        return result;
                    }
                }
            }
        }

        return left;
    }
};
