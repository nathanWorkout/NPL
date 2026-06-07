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
#include <memory>
#include <ncurses.h>

struct StyledText {
    std::string text;
    int color = 0;
    int style = 0;
};

struct Value {
    enum class Type { Number, String, Bool, Array, Map, Null, Function, STYLED_TEXT } type = Type::Null;
    double num  = 0.0;
    std::string str;
    bool flag = false;
    std::vector<Value> arr;
    std::unordered_map<std::string, Value> map;
    std::shared_ptr<StyledText> styled;

    struct {
        FuncDef* func;
    } fn;

    static Value from_styled(const std::string& text, int color, int style);
    static Value from_func(FuncDef* f);
    static Value from_num (double v);
    static Value from_str (const std::string& v);
    static Value from_bool(bool v);
    static Value from_arr (std::vector<Value> v);
    static Value from_map (std::unordered_map<std::string, Value> v);
    static Value null();

    bool truthy() const;
    std::string to_display() const;
};

struct ReturnException {
    Value value;
};

struct ScopeGuard {
    std::vector<std::unordered_map<std::string, Value>>& scopes;
    ~ScopeGuard();
};

class Interpreter {
public:
    void run(ASTNode* node);
    void set_curses_mode(bool enabled);
    bool is_curses_mode() const;
    void register_native(const std::string& name, std::function<Value(std::vector<Value>)> fn);

    std::unordered_map<std::string, std::function<Value(std::vector<Value>)>> natives_;

private:
    bool curses_mode = false;
    std::vector<std::unordered_map<std::string, Value>> scopes_;
    std::unordered_map<std::string, FuncDef*> funcs_;
    std::vector<std::unique_ptr<ASTNode>> loaded_asts_;
    std::unordered_set<std::string> loaded_libs_;

    enum class LoopSignal { None, Break, Continue };
    LoopSignal loop_signal_ = LoopSignal::None;

    void push_scope();
    void pop_scope();
    void set_var(const std::string& name, Value val);
    void def_var(const std::string& name, Value val);
    Value get_var(const std::string& name);
    Value& get_var_ref(const std::string& name);

    void execute(ASTNode* node);
    Value eval(ASTNode* node);
    Value eval_binop(BinOp* n);
    Value eval_pipe(PipeExpr* n);

    void exec_block(Block* node);
    void exec_assign(Assign* node);
    void exec_index_assign(IndexAssign* node);
    void exec_output(Output* node);
    void exec_if(IfStmt* node);
    void exec_repeat(RepeatStmt* node);
    void exec_while(WhileStmt* node);
    void exec_for(ForStmt* node);
    void exec_funcdef(FuncDef* node);
    Value exec_funccall(FuncCall* node);
    void exec_input(InputStmt* node);
    void exec_use(UseStmt* node);
    void exec_try(TryStmt* node);

    std::string read_input();
    std::string read_input_terminal();
    std::string read_input_tui();
    void print(const std::string& text, bool newline = true);
};
