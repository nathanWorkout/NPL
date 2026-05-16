#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <memory>
#include <vector>

struct ASTNode {
    virtual ~ASTNode() = default; // Qaund on supprime un noeud ça le supprime
};

struct NumberLit : ASTNode {
    double value;
};

struct StringLit : ASTNode {
    std::string value;
};

struct BoolLit : ASTNode {
    bool value;
};

struct Identifier : ASTNode {
    std::string name;
};

struct Assign : ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> value;
};

struct Output : ASTNode {
    std::unique_ptr<ASTNode> value;
};

struct BinOp : ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> lhs;
    std::unique_ptr<ASTNode> rhs;
};

struct IfStmt : ASTNode {
    std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>> branches;
    std::unique_ptr<ASTNode> else_body;
};

struct WhileStmt : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
};

struct ForStmt : ASTNode {
    std::unique_ptr<ASTNode> init;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> increment;
    std::unique_ptr<ASTNode> body;
};

struct RepeatStmt : ASTNode {
    std::unique_ptr<ASTNode> count;
    std::unique_ptr<ASTNode> body;
};

struct Block : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
};

struct FuncDef : ASTNode {
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<ASTNode> body;
};

struct FuncCall : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;
};

struct ArrayLit : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> elements;
};

struct IndexExpr : ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> index;
};

struct IndexAssign : ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> index;
    std::unique_ptr<ASTNode> value;
};

struct TryStmt : ASTNode {
    std::unique_ptr<ASTNode> body;
    std::string error_var;
    std::unique_ptr<ASTNode> catch_body;
};

struct ReturnStmt : ASTNode {
    std::unique_ptr<ASTNode> value;
};

struct InputStmt : ASTNode {
    std::string name;
    std::string prompt;
};

struct UseStmt : ASTNode {
    std::string lib;
};

struct MapLit : ASTNode {
    std::vector<std::pair<std::string, std::unique_ptr<ASTNode>>> entries;
};

struct ThrowStmt : ASTNode {
    std::unique_ptr<ASTNode> value;
};

struct NullLit : ASTNode {};

#endif