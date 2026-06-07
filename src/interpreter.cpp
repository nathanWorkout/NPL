#include "../include/interpreter.hpp"
#include "../include/lexer.hpp"
#include "../include/parser.hpp"

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

void Interpreter::run(ASTNode* node) {
    push_scope();
    ScopeGuard guard{scopes_};

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
    if (curses_mode) {
        printw("%s", text.c_str());
        if (newline) printw("\n");
        refresh();
    } else {
        std::cout << text;
        if (newline) std::cout << '\n';
    }
}

void Interpreter::exec_output(Output* node) {
    std::string text = eval(node->value.get()).to_display();
    print(text);
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
                    Value result = Value::null();
                    try { execute(f->body.get()); } catch(ReturnException& ret) { result = ret.value; }
                    return result;
                }
            } catch(...) {}

            auto nat = natives_.find(call->name);
            auto it  = funcs_.find(call->name);
            if(nat != natives_.end()) return nat->second(args_values);
            if(it != funcs_.end()) {
                FuncDef* func = it->second;
                push_scope(); ScopeGuard guard{scopes_};
                def_var("__fn_boundary__", Value::from_bool(true));
                for(size_t i = 0; i < func->params.size(); i++) def_var(func->params[i], args_values[i]);
                Value result = Value::null();
                try { execute(func->body.get()); } catch(ReturnException& ret) { result = ret.value; }
                return result;
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
                    Value result = Value::null();
                    try { execute(f->body.get()); } catch(ReturnException& ret) { result = ret.value; }
                    return result;
                }
            } catch(...) {}

            auto nat = natives_.find(func_name);
            auto it  = funcs_.find(func_name);
            if(nat != natives_.end()) return nat->second(args_values);
            if(it != funcs_.end()) {
                FuncDef* func = it->second;
                if(args_values.size() == func->params.size()) {
                    push_scope(); ScopeGuard guard{scopes_};
                    def_var("__fn_boundary__", Value::from_bool(true));
                    for(size_t i = 0; i < func->params.size(); i++) def_var(func->params[i], args_values[i]);
                    Value result = Value::null();
                    try { execute(func->body.get()); } catch(ReturnException& ret) { result = ret.value; }
                    return result;
                }
            }
        }
    }
    return left;
}
