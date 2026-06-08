#include <thread>
#include <mutex>
#include "../include/interpreter.hpp"
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "framework_web/htmlBuilder.hpp"
#include "framework_web/html_elements.hpp"
#include "framework_web/web_natives.hpp"


static std::mutex g_print_mutex;

Value Value::from_styled(const std::string& text, int color, int style) {
    Value v;
    v.type = Type::STYLED_TEXT;
    v.styled = std::make_shared<StyledText>();
    v.styled->text = text;
    v.styled->color = color;
    v.styled->style = style;
    return v;
}

Value Value::from_func(FuncDef* f) {
    Value v;
    v.type = Type::Function;
    v.fn.func = f;
    return v;
}

Value Value::from_num(double v) { Value r; r.type=Type::Number; r.num=v; return r; }
Value Value::from_str(const std::string& v) { Value r; r.type=Type::String; r.str=v; return r; }
Value Value::from_bool(bool v) { Value r; r.type=Type::Bool; r.flag=v; return r; }
Value Value::from_arr(std::vector<Value> v) { Value r; r.type=Type::Array; r.arr=std::move(v); return r; }
Value Value::from_map(std::unordered_map<std::string, Value> v) { Value r; r.type=Type::Map; r.map=std::move(v); return r; }
Value Value::null() { return {}; }

bool Value::truthy() const {
    switch(type){
        case Type::Number:      return num != 0.0;
        case Type::String:      return !str.empty();
        case Type::Bool:        return flag;
        case Type::Array:       return !arr.empty();
        case Type::Map:         return !map.empty();
        case Type::Function:    return true;
        case Type::STYLED_TEXT: return !styled->text.empty();
        case Type::Null:        return false;
    }
    return false;
}

std::string Value::to_display() const {
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
                if(v.type == Type::String)
                    s += "\"" + k + "\": \"" + v.str + "\"";
                else
                    s += "\"" + k + "\": " + v.to_display();
                first = false;
            }
            return s + "}";
        }
        case Type::Function: return "function";
        case Type::STYLED_TEXT: return styled->text;
        case Type::Null: return "null";
    }
    return "";
}

ScopeGuard::~ScopeGuard() {
    scopes.pop_back();
}

void Interpreter::register_core_natives() {
    register_native("output", [this](std::vector<Value> args) {
        if (!args.empty()) {
            std::string text = args[0].to_display();
            print(text);
        } else {
            print("");
        }
        return Value::null();
    });

    register_native("input", [this](std::vector<Value> args) {
        if (!args.empty()) {
            print(args[0].to_display(), false);
        }

        std::string input_str = read_input();

        try {
            return Value::from_num(std::stod(input_str));
        } catch (const std::invalid_argument&) {
            return Value::from_str(input_str);
        } catch (const std::out_of_range&) {
            return Value::from_str(input_str);
        }
    });

    register_native("thread", [this](std::vector<Value> args) {
        if (args.empty() || args[0].type != Value::Type::Function) {
            throw std::runtime_error("thread() attend une fonction en paramètre !");
        }

        FuncDef* func_npl = args[0].fn.func;
        if (!func_npl) return Value::null();

        auto shared_funcs = this->funcs_;
        bool parent_curses_mode = this->is_curses_mode();

        std::thread t([parent_curses_mode, func_npl, shared_funcs]() {
            try {
                Interpreter thread_interpreter;
                thread_interpreter.set_curses_mode(parent_curses_mode);
                thread_interpreter.push_scope();
                thread_interpreter.register_core_natives();

                for (auto& [name, func_ptr] : shared_funcs) {
                    thread_interpreter.exec_funcdef(func_ptr);
                }

                if (func_npl && func_npl->body) {
                    thread_interpreter.run(func_npl->body.get());
                }
            }
            catch (const ReturnException&) {}
            catch (const std::exception& e) {
                std::cerr << "[Erreur Thread] " << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "[Erreur Thread] Erreur inconnue" << std::endl;
            }
        });

        t.detach();
        return Value::null();
    });
}
void Interpreter::run(ASTNode* node) {
    register_core_natives();
    NPL::register_web_natives(this);

    push_scope();
    ScopeGuard guard{scopes_};

    def_var("_", Value::from_str(""));

    try {
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
    catch (const ReturnException&) {
        // C'est un comportement normal (fin de script ou fin de fonction)
        // donc on l'attrape et on ne fait rien pour éviter que ça remonte au main.cpp
    }
}

void Interpreter::set_curses_mode(bool enabled) { curses_mode = enabled; }
bool Interpreter::is_curses_mode() const { return curses_mode; }

void Interpreter::register_native(const std::string& name, std::function<Value(std::vector<Value>)> fn) {
    natives_[name] = fn;
}

void Interpreter::push_scope() { scopes_.push_back({}); }
void Interpreter::pop_scope()  { scopes_.pop_back(); }

void Interpreter::set_var(const std::string& name, Value val) {
    if(scopes_.empty()) push_scope();
    for(int i = (int)scopes_.size() - 1; i >= 0; i--) {
        if(scopes_[i].count(name)) {
            scopes_[i][name] = std::move(val);
            return;
        }
        if(i > 0 && scopes_[i].count("__fn_boundary__")) break;
    }
    if(scopes_[0].count(name)) {
        scopes_[0][name] = std::move(val);
        return;
    }
    scopes_.back()[name] = std::move(val);
}

void Interpreter::def_var(const std::string& name, Value val) {
    scopes_.back()[name] = std::move(val);
}

Value Interpreter::get_var(const std::string& name) {
    if(scopes_.empty()) throw std::runtime_error("Aucun scope actif");
    for(int i = (int)scopes_.size() - 1; i >= 0; i--) {
        if(scopes_[i].count(name)) return scopes_[i][name];
        if(i > 0 && scopes_[i].count("__fn_boundary__")) break;
    }
    if(scopes_[0].count(name)) return scopes_[0][name];
    throw std::runtime_error("Variable inconnue : " + name);
}

Value& Interpreter::get_var_ref(const std::string& name) {
    if(scopes_.empty()) throw std::runtime_error("Aucun scope actif");
    for(int i = (int)scopes_.size() - 1; i >= 0; i--) {
        auto it = scopes_[i].find(name);
        if(it != scopes_[i].end()) return it->second;
        if(i > 0 && scopes_[i].count("__fn_boundary__")) break; // Protection ajoutée ici !
    }
    if(scopes_[0].count(name)) return scopes_[0][name];
    throw std::runtime_error("Variable inconnue : " + name);
}

void Interpreter::execute(ASTNode* node) {
    if(!node || loop_signal_ != LoopSignal::None) return;

    if(auto n = dynamic_cast<Block*>(node))             exec_block(n);
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
    else if(auto n = dynamic_cast<BreakStmt*>(node))    { loop_signal_ = LoopSignal::Break; }
    else if(auto n = dynamic_cast<ContinueStmt*>(node)) { loop_signal_ = LoopSignal::Continue; }
}

Value Interpreter::eval(ASTNode* node) {
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
            size_t i = (idx.type == Value::Type::Number) ? (size_t)idx.num : (size_t)std::stod(idx.str);
            if(i >= container.arr.size()) throw std::runtime_error("Index tableau hors limites : " + std::to_string(i));
            return container.arr[i];
        }
        if(container.type == Value::Type::Map) {
            if(idx.type != Value::Type::String) idx = Value::from_str(idx.to_display());
            auto it = container.map.find(idx.str);
            if(it == container.map.end()) throw std::runtime_error("Clé introuvable dans la map : " + idx.str);
            return it->second;
        }
        throw std::runtime_error("L'objet '" + n->name + "' n'est ni un tableau ni une map");
    }

    if(auto n = dynamic_cast<BinOp*>(node))   return eval_binop(n);
    if(auto n = dynamic_cast<FuncCall*>(node)) return exec_funccall(n);
    if(auto n = dynamic_cast<NullLit*>(node)) return Value::null();
    if(auto n = dynamic_cast<ComponentExpr*>(node)) return eval_component(n);
    if(auto n = dynamic_cast<PipeExpr*>(node)) return eval_pipe(n);
    if(auto n = dynamic_cast<Placeholder*>(node)) return get_var("_");

    if(auto n = dynamic_cast<InputExpr*>(node)) {
        if(!n->prompt.empty()) print(n->prompt);
        std::string input = read_input();
        try { return Value::from_num(std::stod(input)); }
        catch(...) { return Value::from_str(input); }
    }

    if(auto n = dynamic_cast<FuncDef*>(node)) {
        if(n->name.empty()) return Value::from_func(n);
        exec_funcdef(n);
        return get_var(n->name);
    }
    return Value::null();
}

Value Interpreter::eval_binop(BinOp* n) {
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

        if (n->op == "+")  return Value::from_num(lv + rv);
        if (n->op == "-")  return Value::from_num(lv - rv);
        if (n->op == "*")  return Value::from_num(lv * rv);
        if (n->op == "/") {
            if (rv == 0) throw std::runtime_error("Division par zéro impossible !");
            return Value::from_num(lv / rv);
        }
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

    throw std::runtime_error("Type Mismatch : L'opérateur '" + n->op + "' est incompatible.");
}

void Interpreter::exec_block(Block* node) {
    for(auto& s : node->statements) execute(s.get());
}

void Interpreter::exec_assign(Assign* node) {
    if (node->name.empty()) return;
    Value val = eval(node->value.get());
    set_var(node->name, val);
}

void Interpreter::exec_index_assign(IndexAssign* node) {
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

std::string Interpreter::read_input() {
    if (curses_mode) return read_input_tui();
    return read_input_terminal();
}

std::string Interpreter::read_input_terminal() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

std::string Interpreter::read_input_tui() {
    echo(); curs_set(1); timeout(-1);
    std::string input; int ch;
    while ((ch = getch()) != '\n') {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (!input.empty()) input.pop_back();
        } else if (isprint(ch)) {
            input.push_back((char)ch);
        }
    }
    noecho(); curs_set(0);
    return input;
}

void Interpreter::print(const std::string& text, bool newline) {
    std::lock_guard<std::mutex> lock(g_print_mutex);
    if (curses_mode) {
        printw("%s", text.c_str());
        if (newline) printw("\n");
        refresh();
    } else {
        std::cout << text;
        if (newline) std::cout << '\n';
    }
}

bool is_html(const std::string& str) {
    if (str.size() >= 5) {
        return str.compare(str.size() - 5, 5, ".html") == 0;
    }
    return false;
}

void Interpreter::exec_output(Output* node) {
    std::string text = eval(node->value.get()).to_display();
    if(is_html(text)) {
        Value web_value = get_var("_");
        std::string code_components = web_value.to_display();

        HtmlBuilder builder;
        std::string page = builder.buildPage(code_components);
        builder.saveFile(text, page);

    } else print(text);
}

void Interpreter::exec_if(IfStmt* node) {
    for(auto& [cond, body] : node->branches) {
        if(eval(cond.get()).truthy()) {
            execute(body.get());
            return;
        }
    }
    if(node->else_body) execute(node->else_body.get());
}

void Interpreter::exec_repeat(RepeatStmt* node) {
    Value cv = eval(node->count.get());
    double count = 0.0;
    if(cv.type == Value::Type::Number) count = cv.num;
    else if(cv.type == Value::Type::String) {
        try { count = std::stod(cv.str); } catch(...) { throw std::runtime_error("repeat attend un nombre"); }
    } else throw std::runtime_error("repeat attend un nombre");

    long long n = (long long)count;
    for(long long i = 0; i < n; i++) {
        execute(node->body.get());
        if(loop_signal_ == LoopSignal::Break) { loop_signal_ = LoopSignal::None; break; }
        if(loop_signal_ == LoopSignal::Continue) { loop_signal_ = LoopSignal::None; continue; }
    }
}

void Interpreter::exec_while(WhileStmt* node) {
    while(true) {
        Value cond = eval(node->condition.get());
        if (!cond.truthy()) break;
        execute(node->body.get());
        if(loop_signal_ == LoopSignal::Break) { loop_signal_ = LoopSignal::None; break; }
        if(loop_signal_ == LoopSignal::Continue) { loop_signal_ = LoopSignal::None; continue; }
    }
}

void Interpreter::exec_for(ForStmt* node) {
    if(node->init) execute(node->init.get());
    while(!node->condition || eval(node->condition.get()).truthy()) {
        execute(node->body.get());
        if(loop_signal_ == LoopSignal::Break) { loop_signal_ = LoopSignal::None; break; }
        if(loop_signal_ == LoopSignal::Continue) { loop_signal_ = LoopSignal::None; }
        if(node->increment) execute(node->increment.get());
    }
}

void Interpreter::exec_funcdef(FuncDef* node) {
    funcs_[node->name] = node;
    def_var(node->name, Value::from_func(node));
}

Value Interpreter::exec_funccall(FuncCall* node) {
    std::vector<Value> arg_vals;
    for(auto& arg : node->args) arg_vals.push_back(eval(arg.get()));

    FuncDef* func_to_execute = nullptr;
    auto nat = natives_.find(node->name);
    if(nat != natives_.end()) return nat->second(arg_vals);

    auto it = funcs_.find(node->name);
    if(it != funcs_.end()) func_to_execute = it->second;
    else {
        bool var_exists = false;
        for(int i = (int)scopes_.size() - 1; i >= 0; i--) {
            if(scopes_[i].count(node->name)) {
                if(scopes_[i][node->name].type == Value::Type::Function) {
                    func_to_execute = scopes_[i][node->name].fn.func;
                    var_exists = true;
                }
                break;
            }
            if(i > 0 && scopes_[i].count("__fn_boundary__")) break;
        }
        if(!var_exists && scopes_[0].count(node->name)) {
            if(scopes_[0][node->name].type == Value::Type::Function) {
                func_to_execute = scopes_[0][node->name].fn.func;
            }
        }
    }

    if(!func_to_execute) throw std::runtime_error("Fonction inconnue : " + node->name);
    if(arg_vals.size() != func_to_execute->params.size()) throw std::runtime_error("Nombre d'arguments incorrect");

    push_scope();
    ScopeGuard guard{scopes_};
    def_var("__fn_boundary__", Value::from_bool(true));

    for(size_t i = 0; i < func_to_execute->params.size(); i++) {
        def_var(func_to_execute->params[i], arg_vals[i]);
    }

    Value result = Value::null();
    try { execute(func_to_execute->body.get()); }
    catch(ReturnException& ret) { result = ret.value; }
    return result;
}

void Interpreter::exec_input(InputStmt* node) {
    if(!node->prompt.empty()) print(node->prompt, false);
    std::string input = read_input();
    if (node->name.empty()) return;
    try { set_var(node->name, Value::from_num(std::stod(input))); }
    catch(...) { set_var(node->name, Value::from_str(input)); }
}

void Interpreter::exec_use(UseStmt* node) {
    if(loaded_libs_.count(node->lib)) return;
    loaded_libs_.insert(node->lib);

    std::string lib = node->lib;
    size_t pos;
    while((pos = lib.find("//")) != std::string::npos) lib.replace(pos, 2, "/");

    std::string path = "/usr/local/lib/npl/" + lib + ".npl";
    std::ifstream file(path);
    if(!file.is_open()) throw std::runtime_error("Lib introuvable : " + node->lib);
    std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto tokens = tokenize(src);
    Parser parser(tokens);
    auto ast = parser.parse();
    loaded_asts_.push_back(std::move(ast));

    if(auto block = dynamic_cast<Block*>(loaded_asts_.back().get())) {
        for(auto& s : block->statements) {
            if(auto fn = dynamic_cast<FuncDef*>(s.get())) exec_funcdef(fn);
            else if(!dynamic_cast<FuncCall*>(s.get())) execute(s.get());
        }
    } else {
        execute(loaded_asts_.back().get());
    }
}

void Interpreter::exec_try(TryStmt* node) {
    try { execute(node->body.get()); }
    catch(std::runtime_error& e) {
        push_scope();
        ScopeGuard guard{scopes_};
        def_var(node->error_var, Value::from_str(e.what()));
        execute(node->catch_body.get());
    }
}

Value Interpreter::eval_pipe(PipeExpr* n) {
    Value left = eval(n->lhs.get());
    set_var("_", left);

    if(auto out = dynamic_cast<Output*>(n->rhs.get())) {
        if(left.type == Value::Type::Array) {
            for(auto& item : left.arr) {
                set_var("_", item);
                if(out->value) print(eval(out->value.get()).to_display());
                else print(item.to_display());
            }
            return left;
        }
        set_var("_", left);
        if(out->value) print(eval(out->value.get()).to_display());
        else print(left.to_display());
        return left;
    }

    if(auto lambda = dynamic_cast<LambdaBlock*>(n->rhs.get())) {
        if(left.type == Value::Type::Array) {
            std::vector<Value> result;
            for(auto& item : left.arr) {
                push_scope(); ScopeGuard guard{scopes_};
                set_var("item", item); set_var("_", item);
                Value val = Value::null();
                try { execute(lambda->body.get()); }
                catch(ReturnException& ret) { val = ret.value; }
                result.push_back(val);
            }
            return Value::from_arr(result);
        }
        return left;
    }

    ASTNode* rhs = n->rhs.get();
    if(auto call = dynamic_cast<FuncCall*>(rhs)) {
        if(call->name == "repeat") {
            Value pattern;
            if(call->args.size() >= 1) pattern = eval(call->args[0].get());
            else {
                if(auto lit = dynamic_cast<StringLit*>(rhs)) pattern = Value::from_str(lit->value);
                else pattern = eval(rhs);
            }
            if(pattern.type != Value::Type::String) pattern = Value::from_str(pattern.to_display());
            if(left.type != Value::Type::Number) throw std::runtime_error("repeat attend un nombre à gauche");

            long long n = (long long)left.num;
            std::vector<Value> out;
            for(long long i = 0; i < n; i++) out.push_back(Value::from_str(pattern.str));
            return Value::from_arr(out);
        }

        bool has_placeholder = false;
        for(auto& arg : call->args) {
            if(dynamic_cast<Placeholder*>(arg.get())) { has_placeholder = true; break; }
        }

        Value pipe_result = Value::null();
        bool executed_call = false;

        if(has_placeholder) {
            std::vector<Value> args_values;
            for(auto& arg : call->args) {
                if(dynamic_cast<Placeholder*>(arg.get())) args_values.push_back(left);
                else args_values.push_back(eval(arg.get()));
            }

            try {
                Value var = get_var(call->name);
                if(var.type == Value::Type::Function) {
                    FuncDef* f = var.fn.func;
                    push_scope(); ScopeGuard guard{scopes_};
                    def_var("__fn_boundary__", Value::from_bool(true));
                    for(size_t i = 0; i < f->params.size(); i++) def_var(f->params[i], args_values[i]);
                    try { execute(f->body.get()); } catch(ReturnException& ret) { pipe_result = ret.value; }
                    executed_call = true;
                }
            } catch(...) {}

            if(!executed_call) {
                auto nat = natives_.find(call->name);
                auto it  = funcs_.find(call->name);
                if(nat != natives_.end()) { pipe_result = nat->second(args_values); executed_call = true; }
                else if(it != funcs_.end()) {
                    FuncDef* func = it->second;
                    push_scope(); ScopeGuard guard{scopes_};
                    def_var("__fn_boundary__", Value::from_bool(true));
                    for(size_t i = 0; i < func->params.size(); i++) def_var(func->params[i], args_values[i]);
                    try { execute(func->body.get()); } catch(ReturnException& ret) { pipe_result = ret.value; }
                    executed_call = true;
                }
            }
        } else {
            std::vector<Value> args_values;
            args_values.push_back(left);
            for(auto& arg : call->args) args_values.push_back(eval(arg.get()));

            if(call->name == "filter") {
                auto* lambda = dynamic_cast<LambdaBlock*>(call->args[0].get());
                if(!lambda) throw std::runtime_error("filter attend un bloc { }");
                std::vector<Value> result;
                for(auto& item : left.arr) {
                    push_scope(); ScopeGuard guard{scopes_};
                    def_var("item", item);
                    Value cond = Value::null();
                    try { execute(lambda->body.get()); } catch(ReturnException& ret) { cond = ret.value; }
                    if(cond.truthy()) result.push_back(item);
                }
                return Value::from_arr(result);
            }

            if(call->name == "each") {
                auto* lambda = dynamic_cast<LambdaBlock*>(call->args[0].get());
                if(!lambda) throw std::runtime_error("each attend un bloc { }");
                for(auto& item : left.arr) {
                    push_scope(); ScopeGuard guard{scopes_};
                    def_var("item", item);
                    try { execute(lambda->body.get()); } catch(ReturnException&) {}
                }
                return left;
            }

            std::string func_name = call->name;
            try {
                Value var = get_var(call->name);
                if(var.type == Value::Type::Function) {
                    FuncDef* f = var.fn.func;
                    if(args_values.size() != f->params.size()) throw std::runtime_error("Arguments invalides");
                    push_scope(); ScopeGuard guard{scopes_};
                    def_var("__fn_boundary__", Value::from_bool(true));
                    for(size_t i = 0; i < f->params.size(); i++) def_var(f->params[i], args_values[i]);
                    try { execute(f->body.get()); } catch(ReturnException& ret) { pipe_result = ret.value; }
                    executed_call = true;
                }
            } catch(...) {}

            if(!executed_call) {
                auto nat = natives_.find(func_name);
                auto it  = funcs_.find(func_name);
                if(nat != natives_.end()) { pipe_result = nat->second(args_values); executed_call = true; }
                else if(it != funcs_.end()) {
                    FuncDef* func = it->second;
                    if(args_values.size() == func->params.size()) {
                        push_scope(); ScopeGuard guard{scopes_};
                        def_var("__fn_boundary__", Value::from_bool(true));
                        for(size_t i = 0; i < func->params.size(); i++) def_var(func->params[i], args_values[i]);
                        try { execute(func->body.get()); } catch(ReturnException& ret) { pipe_result = ret.value; }
                        executed_call = true;
                    }
                }
            }
        }

        if(executed_call) {
            if (scopes_.size() > 1) {
                for (int i = (int)scopes_.size() - 2; i >= 0; i--) {
                    if (scopes_[i].count("_")) {
                        std::string old_html = scopes_[i]["_"].to_display();
                        std::string raw_child_html = left.to_display();

                        size_t pos = old_html.rfind(raw_child_html);
                        if (pos != std::string::npos && pos + raw_child_html.length() == old_html.length()) {
                            scopes_[i]["_"] = Value::from_str(old_html.substr(0, pos) + pipe_result.to_display());
                        } else {
                            scopes_[i]["_"] = pipe_result;
                        }
                        break;
                    }
                }
            }
            return pipe_result;
        }
    }

    return left;
}

Value Interpreter::eval_component(ComponentExpr* n) {

    // 1 : On récupère le nom du tag HTML (ex: h1 ou div...)
    std::string tag_name = "";
    if (n->name) {
        if (auto id = dynamic_cast<Identifier*>(n->name.get())) {
            tag_name = id->name;
        } else {
            Value evaluated_name = eval(n->name.get());
            if (evaluated_name.type != Value::Type::Null) {
                tag_name = evaluated_name.to_display();
            }
        }
    }

    // 2. On crée un scope local isolé pour le corps de ce composant
    push_scope();
    ScopeGuard guard{scopes_};

    // Le "_" est local au composant actuel (ex: la div parente)
    def_var("_", Value::from_str(""));

    std::string inner_html = "";

    // 3 : Évaluation du contenu (bloc d'instruction ou expression simple)
    if (auto block = dynamic_cast<Block*>(n->body.get())) {
        // C'est un bloc {}, on exécute chaque instruction à l'intérieur
        for (auto& stmt : block->statements) {
            // Si c'est un composant écrit seul dans le bloc (ex: h1 -> "salut")
            if (auto exprStmt = dynamic_cast<ExprStatement*>(stmt.get())) {
                // On l'évalue, le composant enfant va s'exécuter et, via l'étape 4,
                // il va venir injecter son code HTML directement dans notre variable "_" locale
                eval(exprStmt->expr.get());
            } else {
                // Pour toutes les autres instructions normales (Assign, Output, loops...)
                execute(stmt.get());
            }
        }
        inner_html = get_var("_").to_display();
    } else {
        inner_html = eval(n->body.get()).to_display();
    }

    std::string html_result = NPL::render_element(tag_name, inner_html);

    Value parent_underscore = Value::null();
    if (scopes_.size() > 1) {
        for (int i = (int)scopes_.size() - 2; i >= 0; i--) {
            if (scopes_[i].count("_")) {
                parent_underscore = scopes_[i]["_"];
                break;
            }
            if (i > 0 && scopes_[i].count("__fn_boundary__")) break;
        }
    }

    std::string accumulated = "";
    if (parent_underscore.type == Value::Type::String) {
        accumulated = parent_underscore.str;
    } else if (parent_underscore.type != Value::Type::Null) {
        accumulated = parent_underscore.to_display();
    }

    Value final_val = Value::from_str(accumulated + html_result);

    if (scopes_.size() > 1) {
        scopes_[scopes_.size() - 2]["_"] = final_val;
    } else {
        set_var("_", final_val);
    }

    return final_val;
}
