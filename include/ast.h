#pragma once
#include <memory>
#include <vector>
#include <variant>
#include <string>

// Forward declarations
struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// Base expression class
struct Expr {
    virtual ~Expr() = default;
};

// Expression types
struct LiteralExpr : Expr {
    std::variant<int, float, std::string> value;
    LiteralExpr(int v) : value(v) {}
    LiteralExpr(float v) : value(v) {}
    LiteralExpr(const std::string &v) : value(v) {}
};

struct VarExpr : Expr {
    std::string name;
    int line;
    int column;
    VarExpr(std::string n, int ln = 1, int col = 1) : name(std::move(n)), line(ln), column(col) {}
};

struct BinaryExpr : Expr {
    enum class Op { ADD, SUB, MUL, DIV, EQ, NEQ, LT, LE, GT, GE, AND, OR };
    Op op;
    ExprPtr left, right;
    BinaryExpr(Op o, ExprPtr l, ExprPtr r) : op(o), left(std::move(l)), right(std::move(r)) {}
};

struct UnaryExpr : Expr {
    enum class Op { PRE_INC, PRE_DEC };
    Op op;
    std::string varName;
    UnaryExpr(Op o, std::string n) : op(o), varName(std::move(n)) {}
};

struct ArrayExpr : Expr {
    std::vector<ExprPtr> elements;
    ArrayExpr(std::vector<ExprPtr> elems) : elements(std::move(elems)) {}
};

struct IndexExpr : Expr {
    ExprPtr array;
    ExprPtr index;
    IndexExpr(ExprPtr a, ExprPtr idx) : array(std::move(a)), index(std::move(idx)) {}
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<ExprPtr> args;
    CallExpr(std::string c, std::vector<ExprPtr> a)
        : callee(std::move(c)), args(std::move(a)) {}
};

// Base statement class
struct Stmt { 
    virtual ~Stmt() = default; 
};

// Statement types
struct VarDeclStmt : Stmt {
    enum Kind { INT, FLOAT, STRING, INT_ARRAY, FLOAT_ARRAY, STRING_ARRAY, VOID } type;
    std::string name;
    ExprPtr init;
    VarDeclStmt(Kind t, std::string n, ExprPtr i) : type(t), name(std::move(n)), init(std::move(i)) {}
};

struct InferDeclStmt : Stmt {
    std::string name;
    ExprPtr init;
    InferDeclStmt(std::string n, ExprPtr i) : name(std::move(n)), init(std::move(i)) {}
};

struct AssignStmt : Stmt {
    std::string name;
    ExprPtr expr;
    int line;
    int column;
    AssignStmt(std::string n, ExprPtr e, int ln = 1, int col = 1) 
        : name(std::move(n)), expr(std::move(e)), line(ln), column(col) {}
};

struct PrintStmt : Stmt {
    ExprPtr expr;
    int line;
    int column;
    PrintStmt(ExprPtr e, int ln = 1, int col = 1) : expr(std::move(e)), line(ln), column(col) {}
};

struct IfStmt : Stmt {
    ExprPtr cond;
    std::vector<StmtPtr> thenBranch, elseBranch;
    IfStmt(ExprPtr c, std::vector<StmtPtr> t, std::vector<StmtPtr> e)
        : cond(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    std::vector<StmtPtr> body;
    WhileStmt(ExprPtr c, std::vector<StmtPtr> b)
        : cond(std::move(c)), body(std::move(b)) {}
};

struct ForStmt : Stmt {
    StmtPtr init;
    ExprPtr cond;
    StmtPtr post;
    std::vector<StmtPtr> body;
    ForStmt(StmtPtr i, ExprPtr c, StmtPtr p, std::vector<StmtPtr> b)
        : init(std::move(i)), cond(std::move(c)), post(std::move(p)), body(std::move(b)) {}
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    ExprStmt(ExprPtr e) : expr(std::move(e)) {}
};

struct ReturnStmt : Stmt {
    ExprPtr value; // puede ser nullptr para "return;"
    ReturnStmt(ExprPtr v = nullptr) : value(std::move(v)) {}
};

struct FunctionStmt : Stmt {
    bool inference; // true → se usó "fun"
    VarDeclStmt::Kind retType; // ignorado si inference==true
    std::string name;
    std::vector<std::string> params; // nombres de parametros
    std::vector<bool> paramIsArray; // true si el parámetro es array (nombre[])
    std::vector<StmtPtr> body; // bloque de la función
    
    // Constructor principal
    FunctionStmt(bool inf, VarDeclStmt::Kind rt, std::string n, 
                 std::vector<std::string> p, std::vector<bool> pa, std::vector<StmtPtr> b)
        : inference(inf), retType(rt), name(std::move(n)),
          params(std::move(p)), paramIsArray(std::move(pa)), body(std::move(b)) {}
          
    // Constructor de compatibilidad (sin paramIsArray)
    FunctionStmt(bool inf, VarDeclStmt::Kind rt, std::string n, 
                 std::vector<std::string> p, std::vector<StmtPtr> b)
        : inference(inf), retType(rt), name(std::move(n)),
          params(std::move(p)), body(std::move(b)) {
        // Inicializar paramIsArray con false para todos los parámetros
        paramIsArray.resize(params.size(), false);
    }
};
