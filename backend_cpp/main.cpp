#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cctype>
#include <cmath>

using namespace std;


struct Token {
    string type;
    string text;
    int line;
    int pos;
};

vector<Token> tokenize(const string &code, vector<string> &errors) {
    vector<Token> tokens;
    int i = 0, n = (int)code.size();
    int line = 1;
    while (i < n) {
        char c = code[i];
        if (c == '\n') { ++line; ++i; continue; }
        if (isspace((unsigned char)c)) { ++i; continue; }
        if (c == '/' && i+1 < n && code[i+1] == '/') {
            while (i < n && code[i] != '\n') ++i;
            continue;
        }
        auto match_op = [&](const string &op)->bool{
            if (i + (int)op.size() <= n && code.substr(i, op.size()) == op) {
                tokens.push_back({op, op, line, i}); i += (int)op.size(); return true;
            }
            return false;
        };
        vector<string> ops = {"==","!=","<=",">=","&&","||","+","-","*","/","(",")","{","}",";",":",",","=","<",">","!"};
        bool did = false;
        for (auto &op: ops) { if (match_op(op)) { did = true; break; } }
        if (did) continue;
        if (isdigit((unsigned char)c)) {
            int j = i; bool has_dot = false;
            while (j < n && (isdigit((unsigned char)code[j]) || code[j]=='.')) { if (code[j]=='.') has_dot = true; ++j; }
            string num = code.substr(i, j-i);
            tokens.push_back({ has_dot? string("FLOATNUM") : string("NUMBER"), num, line, i });
            i = j; continue;
        }
        if (isalpha((unsigned char)c) || c == '_') {
            int j = i; while (j < n && (isalnum((unsigned char)code[j]) || code[j]=='_')) ++j;
            string id = code.substr(i, j-i);
            static unordered_map<string,string> reserved = {
                {"if","IF"},{"else","ELSE"},{"while","WHILE"},{"for","FOR"},
                {"return","RETURN"},{"func","FUNC"},{"var","VAR"},
                {"int","INT"},{"float","FLOAT"},{"bool","BOOL"},
                {"true","TRUE"},{"false","FALSE"},{"print","PRINT"}
            };
            auto it = reserved.find(id);
            if (it != reserved.end()) tokens.push_back({it->second, id, line, i});
            else tokens.push_back({"IDENTIFIER", id, line, i});
            i = j; continue;
        }
        string bad(1,c);
        errors.push_back("Illegal character '" + bad + "' at line " + to_string(line));
        ++i;
    }
    return tokens;
}

struct AST {
    string node_type;
    string value;
    vector<shared_ptr<AST>> children;
    AST(string t): node_type(t) {}
};

string escape_json(const string &s) {
    string out;
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

string ast_to_json(const shared_ptr<AST> &node, int indent=0) {
    if (!node) return "null";
    string pad(indent, ' ');
    ostringstream ss;
    ss << "{\n" << pad << "  \"type\": \"" << node->node_type << "\"";
    if (!node->value.empty()) ss << ",\n" << pad << "  \"value\": \"" << escape_json(node->value) << "\"";
    if (!node->children.empty()) {
        ss << ",\n" << pad << "  \"children\": [\n";
        for (size_t i=0;i<node->children.size();++i) {
            ss << pad << "    " << ast_to_json(node->children[i], indent+4);
            if (i+1<node->children.size()) ss << ",\n"; else ss << "\n";
        }
        ss << pad << "  ]\n" << pad << "}";
    } else {
        ss << "\n" << pad << "}";
    }
    return ss.str();
}

struct Parser {
    vector<Token> toks;
    int idx = 0;
    vector<string> errors;
    Parser(const vector<Token> &t): toks(t), idx(0) {}
    Token peek(int offset=0) { if (idx+offset < (int)toks.size()) return toks[idx+offset]; return {"","",-1,-1}; }
    bool match(const string &type) { if (idx < (int)toks.size() && toks[idx].type==type) { ++idx; return true; } return false; }
    bool expect(const string &type, const string &msg) { if (match(type)) return true; errors.push_back(msg + "; found '" + (idx<(int)toks.size()?toks[idx].text:"EOF") + "'"); return false; }

    shared_ptr<AST> parse_program() {
        auto prog = make_shared<AST>("Program");
        while (idx < (int)toks.size()) {
            auto s = parse_statement();
            if (s) prog->children.push_back(s);
            else break;
        }
        return prog;
    }

    shared_ptr<AST> parse_statement() {
        if (match("VAR")) {
            if (!expect("IDENTIFIER","Expected identifier after 'var'")) return nullptr;
            string name = toks[idx-1].text;
            if (!expect(":","Expected ':' after identifier in var declaration")) return nullptr;
            string type;
            if (match("INT")) type = "int";
            else if (match("FLOAT")) type = "float";
            else if (match("BOOL")) type = "bool";
            else { errors.push_back("Unknown type in var declaration"); return nullptr; }
            auto node = make_shared<AST>("VarDecl"); node->value = name; node->children.push_back(make_shared<AST>(type));
            if (match("=")) {
                auto expr = parse_expression(); if (!expr) return nullptr; node->children.push_back(expr);
            }
            if (!expect(";","Expected ';' after var declaration")) return nullptr;
            return node;
        }
        if (match("FUNC")) {
            if (!expect("IDENTIFIER","Expected function name after 'func'")) return nullptr;
            string fname = toks[idx-1].text;
            if (!expect("(","Expected '(' after function name")) return nullptr;
            auto params = make_shared<AST>("Params");
            if (!match(")")) {
                while (true) {
                    if (!expect("IDENTIFIER","Expected parameter name")) return nullptr;
                    string pname = toks[idx-1].text;
                    if (!expect(":","Expected ':' after parameter name")) return nullptr;
                    string ptype;
                    if (match("INT")) ptype = "int";
                    else if (match("FLOAT")) ptype = "float";
                    else if (match("BOOL")) ptype = "bool";
                    else { errors.push_back("Unknown parameter type"); return nullptr; }
                    auto pn = make_shared<AST>("Param"); pn->value = pname; pn->children.push_back(make_shared<AST>(ptype));
                    params->children.push_back(pn);
                    if (match(")")) break;
                    if (!expect(",","Expected ',' between parameters")) return nullptr;
                }
            }
            if (!expect(":","Expected ':' after parameter list")) return nullptr;
            string rettype;
            if (match("INT")) rettype = "int";
            else if (match("FLOAT")) rettype = "float";
            else if (match("BOOL")) rettype = "bool";
            else { errors.push_back("Unknown return type"); return nullptr; }
            if (!expect("{","Expected '{' to start function body")) return nullptr;
            auto body = make_shared<AST>("Block");
            while (!match("}")) {
                if (idx >= (int)toks.size()) { errors.push_back("Unterminated function body"); return nullptr; }
                auto s = parse_statement(); if (s) body->children.push_back(s); else return nullptr;
            }
            auto node = make_shared<AST>("FunctionDecl"); node->value = fname; node->children.push_back(params); node->children.push_back(make_shared<AST>(rettype)); node->children.push_back(body);
            return node;
        }
        if (match("IF")) {
            if (!expect("(","Expected '(' after 'if'")) return nullptr;
            auto cond = parse_expression(); if (!cond) return nullptr;
            if (!expect(")","Expected ')' after condition")) return nullptr;
            if (!expect("{","Expected '{' to start if block")) return nullptr;
            auto thenb = make_shared<AST>("Block");
            while (!match("}")) { if (idx>= (int)toks.size()) { errors.push_back("Unterminated if block"); return nullptr;} auto s = parse_statement(); if (s) thenb->children.push_back(s); else return nullptr; }
            shared_ptr<AST> elseb = nullptr;
            if (match("ELSE")) {
                if (!expect("{","Expected '{' to start else block")) return nullptr;
                elseb = make_shared<AST>("Block");
                while (!match("}")) { if (idx>= (int)toks.size()) { errors.push_back("Unterminated else block"); return nullptr;} auto s = parse_statement(); if (s) elseb->children.push_back(s); else return nullptr; }
            }
            auto node = make_shared<AST>("If"); node->children.push_back(cond); node->children.push_back(thenb); if (elseb) node->children.push_back(elseb); return node;
        }
        if (match("WHILE")) {
            if (!expect("(","Expected '(' after 'while'")) return nullptr;
            auto cond = parse_expression(); if (!cond) return nullptr;
            if (!expect(")","Expected ')' after condition")) return nullptr;
            if (!expect("{","Expected '{' to start while body")) return nullptr;
            auto body = make_shared<AST>("Block");
            while (!match("}")) { if (idx>= (int)toks.size()) { errors.push_back("Unterminated while block"); return nullptr;} auto s = parse_statement(); if (s) body->children.push_back(s); else return nullptr; }
            auto node = make_shared<AST>("While"); node->children.push_back(cond); node->children.push_back(body); return node;
        }
        if (match("FOR")) {
            if (!expect("(","Expected '(' after 'for'")) return nullptr;
            shared_ptr<AST> init=nullptr, cond=nullptr, post=nullptr;
            if (!match(";")) {
                if (peek().type=="VAR") init = parse_statement();
                else {
                    auto a = parse_expression(); init = a; if (!expect(";","Expected ';' after for init")) return nullptr;
                }
            }
            if (!match(";")) {
                cond = parse_expression(); if (!expect(";","Expected ';' after for condition")) return nullptr;
            }
            if (!match(")")) {
                post = parse_expression(); if (!expect(")","Expected ')' after for post")) return nullptr;
            }
            if (!expect("{","Expected '{' to start for body")) return nullptr;
            auto body = make_shared<AST>("Block");
            while (!match("}")) { if (idx>= (int)toks.size()) { errors.push_back("Unterminated for block"); return nullptr;} auto s = parse_statement(); if (s) body->children.push_back(s); else return nullptr; }
            auto node = make_shared<AST>("For"); if (init) node->children.push_back(init); if (cond) node->children.push_back(cond); if (post) node->children.push_back(post); node->children.push_back(body); return node;
        }
        if (match("RETURN")) {
            auto node = make_shared<AST>("Return");
            if (!match(";")) { auto e = parse_expression(); if (!e) return nullptr; node->children.push_back(e); if (!expect(";","Expected ';' after return")) return nullptr; }
            return node;
        }
        if (match("PRINT")) {
            if (match("(")) {
                auto e = parse_expression(); if (!e) return nullptr; if (!expect(")","Expected ')' after print argument")) return nullptr; if (!expect(";","Expected ';' after print")) return nullptr; auto node = make_shared<AST>("Print"); node->children.push_back(e); return node;
            } else {
                auto e = parse_expression(); if (!e) return nullptr; if (!expect(";","Expected ';' after print")) return nullptr; auto node = make_shared<AST>("Print"); node->children.push_back(e); return node;
            }
        }
        if (peek().type=="IDENTIFIER" && peek(1).type=="=") {
            string name = peek().text; match("IDENTIFIER"); match("="); auto e = parse_expression(); if (!expect(";","Expected ';' after assignment")) return nullptr; auto node = make_shared<AST>("Assign"); node->value = name; node->children.push_back(e); return node;
        }
        auto expr = parse_expression(); if (expr) { if (!expect(";","Expected ';' after expression")) return nullptr; return expr; }
        return nullptr;
    }

    shared_ptr<AST> parse_expression() { return parse_or(); }
    shared_ptr<AST> parse_or() {
        auto left = parse_and();
        while (match("||")) {
            auto right = parse_and(); auto node = make_shared<AST>("BinaryOp"); node->value = "||"; node->children.push_back(left); node->children.push_back(right); left = node;
        }
        return left;
    }
    shared_ptr<AST> parse_and() {
        auto left = parse_eq();
        while (match("&&")) {
            auto right = parse_eq(); auto node = make_shared<AST>("BinaryOp"); node->value = "&&"; node->children.push_back(left); node->children.push_back(right); left = node;
        }
        return left;
    }
    shared_ptr<AST> parse_eq() {
        auto left = parse_rel();
        while (true) {
            if (match("==")) { auto right = parse_rel(); auto node = make_shared<AST>("BinaryOp"); node->value = "=="; node->children.push_back(left); node->children.push_back(right); left = node; }
            else if (match("!=")) { auto right = parse_rel(); auto node = make_shared<AST>("BinaryOp"); node->value = "!="; node->children.push_back(left); node->children.push_back(right); left = node; }
            else break;
        }
        return left;
    }
    shared_ptr<AST> parse_rel() {
        auto left = parse_add();
        while (true) {
            if (match("<")) { auto right = parse_add(); auto node = make_shared<AST>("BinaryOp"); node->value = "<"; node->children.push_back(left); node->children.push_back(right); left = node; }
            else if (match(">")) { auto right = parse_add(); auto node = make_shared<AST>("BinaryOp"); node->value = ">"; node->children.push_back(left); node->children.push_back(right); left = node; }
            else if (match("<=")) { auto right = parse_add(); auto node = make_shared<AST>("BinaryOp"); node->value = "<="; node->children.push_back(left); node->children.push_back(right); left = node; }
            else if (match(">=")) { auto right = parse_add(); auto node = make_shared<AST>("BinaryOp"); node->value = ">="; node->children.push_back(left); node->children.push_back(right); left = node; }
            else break;
        }
        return left;
    }
    shared_ptr<AST> parse_add() {
        auto left = parse_mul();
        while (true) {
            if (match("+")) { auto right = parse_mul(); auto node = make_shared<AST>("BinaryOp"); node->value = "+"; node->children.push_back(left); node->children.push_back(right); left = node; }
            else if (match("-")) { auto right = parse_mul(); auto node = make_shared<AST>("BinaryOp"); node->value = "-"; node->children.push_back(left); node->children.push_back(right); left = node; }
            else break;
        }
        return left;
    }
    shared_ptr<AST> parse_mul() {
        auto left = parse_unary();
        while (true) {
            if (match("*")) { auto right = parse_unary(); auto node = make_shared<AST>("BinaryOp"); node->value = "*"; node->children.push_back(left); node->children.push_back(right); left = node; }
            else if (match("/")) { auto right = parse_unary(); auto node = make_shared<AST>("BinaryOp"); node->value = "/"; node->children.push_back(left); node->children.push_back(right); left = node; }
            else break;
        }
        return left;
    }
    shared_ptr<AST> parse_unary() {
        if (match("!")) { auto v = parse_unary(); auto node = make_shared<AST>("UnaryOp"); node->value = "!"; node->children.push_back(v); return node; }
        if (match("-")) { auto v = parse_unary(); auto node = make_shared<AST>("UnaryOp"); node->value = "-"; node->children.push_back(v); return node; }
        return parse_primary();
    }
    shared_ptr<AST> parse_primary() {
        if (match("NUMBER")) { auto node = make_shared<AST>("Literal"); node->value = toks[idx-1].text; return node; }
        if (match("FLOATNUM")) { auto node = make_shared<AST>("Literal"); node->value = toks[idx-1].text; return node; }
        if (match("TRUE")) { auto node = make_shared<AST>("Literal"); node->value = "true"; return node; }
        if (match("FALSE")) { auto node = make_shared<AST>("Literal"); node->value = "false"; return node; }
        if (match("IDENTIFIER")) {
            string name = toks[idx-1].text;
            if (match("(")) {
                auto call = make_shared<AST>("Call"); call->value = name;
                if (!match(")")) {
                    while (true) {
                        auto arg = parse_expression(); if (!arg) return nullptr; call->children.push_back(arg);
                        if (match(")")) break;
                        if (!expect(",","Expected ',' between call arguments")) return nullptr;
                    }
                }
                return call;
            }
            auto node = make_shared<AST>("Identifier"); node->value = name; return node;
        }
        if (match("(")) { auto e = parse_expression(); if (!expect(")","Expected ')'")) return nullptr; return e; }
        return nullptr;
    }
};

struct Value {
    enum Type { INT, FLOAT, BOOL, NONE } type = NONE;
    long long i = 0;
    double f = 0.0;
    bool b = false;
    string toString() const {
        ostringstream ss;
        if (type==INT) ss<<i;
        else if (type==FLOAT) {
            // format with removal of trailing zeros
            ss<<f;
        }
        else if (type==BOOL) ss<<(b?"true":"false");
        return ss.str();
    }
};

struct FunctionInfo {
    string name;
    vector<pair<string,string>> params;
    string return_type;
    shared_ptr<AST> body;
};

// Semantic analyzer: performs a static AST walk and emits errors/warnings
class SemanticAnalyzer {
public:
    shared_ptr<AST> ast;
    // Reference to the interpreter's symbol/function tables collected earlier
    unordered_map<string, Value::Type> globals;
    const unordered_map<string, FunctionInfo>* functions = nullptr;

    vector<string> errors;
    vector<string> warnings;

    SemanticAnalyzer(const shared_ptr<AST> &a, const unordered_map<string, Value::Type> &g, const unordered_map<string, FunctionInfo> &f)
        : ast(a), globals(g), functions(&f) {}

    static Value::Type literal_type(const string &s) {
        if (s=="true"||s=="false") return Value::BOOL;
        if (s.find('.')!=string::npos) return Value::FLOAT;
        return Value::INT;
    }
    static string type_to_string(Value::Type t) {
        if (t==Value::INT) return "int";
        if (t==Value::FLOAT) return "float";
        if (t==Value::BOOL) return "bool";
        return "none";
    }
    static bool compatible(Value::Type expected, Value::Type actual) {
        if (expected==Value::NONE || actual==Value::NONE) return false;
        if (expected==actual) return true;
        // allow implicit int -> float promotion
        if (expected==Value::FLOAT && actual==Value::INT) return true;
        return false;
    }

    Value::Type string_to_type(const string &s) const {
        if (s=="int") return Value::INT;
        if (s=="float") return Value::FLOAT;
        if (s=="bool") return Value::BOOL;
        return Value::NONE;
    }

    // Infer expression type given a local scope (params + local vars)
    Value::Type infer_expr_type(const shared_ptr<AST> &node, const unordered_map<string, Value::Type> &locals) {
        if (!node) return Value::NONE;
        if (node->node_type=="Literal") return literal_type(node->value);
        if (node->node_type=="Identifier") {
            auto it = locals.find(node->value); if (it!=locals.end()) return it->second;
            auto git = globals.find(node->value); if (git!=globals.end()) return git->second;
            errors.push_back("Undefined identifier '" + node->value + "'");
            return Value::NONE;
        }
        if (node->node_type=="Call") {
            string fname = node->value;
            if (fname=="print") return Value::NONE; // print returns none
            auto fit = functions->find(fname);
            if (fit==functions->end()) { errors.push_back("Call to undefined function '" + fname + "'"); return Value::NONE; }
            auto &fi = fit->second;
            if (node->children.size() != fi.params.size()) {
                errors.push_back("Argument count mismatch in call to '" + fname + "'");
            }
            for (size_t i=0;i<node->children.size() && i<fi.params.size();++i) {
                Value::Type at = infer_expr_type(node->children[i], locals);
                Value::Type pt = string_to_type(fi.params[i].second);
                if (at==Value::NONE) continue;
                if (!compatible(pt, at)) {
                    errors.push_back("Argument " + to_string(i+1) + " type mismatch in call to '" + fname + "': expected " + type_to_string(pt) + ", got " + type_to_string(at));
                }
            }
            return string_to_type(fi.return_type);
        }
        if (node->node_type=="BinaryOp") {
            string op = node->value;
            Value::Type L = infer_expr_type(node->children[0], locals);
            Value::Type R = infer_expr_type(node->children[1], locals);
            if (L==Value::NONE || R==Value::NONE) return Value::NONE;
            if (op=="+"||op=="-"||op=="*"||op=="/") {
                // arithmetic: require numeric
                if ((L==Value::BOOL) || (R==Value::BOOL)) { errors.push_back("Invalid operand type for arithmetic operator '"+op+"'"); return Value::NONE; }
                if (L==Value::FLOAT || R==Value::FLOAT) return Value::FLOAT; return Value::INT;
            }
            if (op=="<"||op==">"||op=="<="||op==">=") {
                if ((L==Value::BOOL) || (R==Value::BOOL)) { errors.push_back("Invalid operand type for relational operator '"+op+"'"); return Value::NONE; }
                return Value::BOOL;
            }
            if (op=="=="||op=="!=") {
                // allow comparisons between numeric types or booleans
                if ((L==Value::BOOL) != (R==Value::BOOL)) {
                    // comparing bool to numeric allowed by interpreter (coercion), but warn
                    warnings.push_back("Comparison between boolean and numeric in '" + op + "'");
                }
                return Value::BOOL;
            }
            if (op=="&&"||op=="||") {
                // logical: operands should be boolean (or at least coercible)
                if (L!=Value::BOOL && L!=Value::INT && L!=Value::FLOAT) { errors.push_back("Invalid operand for logical operator '"+op+"'"); return Value::NONE; }
                if (R!=Value::BOOL && R!=Value::INT && R!=Value::FLOAT) { errors.push_back("Invalid operand for logical operator '"+op+"'"); return Value::NONE; }
                return Value::BOOL;
            }
            return Value::NONE;
        }
        if (node->node_type=="UnaryOp") {
            string op = node->value;
            Value::Type V = infer_expr_type(node->children[0], locals);
            if (V==Value::NONE) return Value::NONE;
            if (op=="-") {
                if (V==Value::BOOL) { errors.push_back("Invalid operand type for unary '-' on boolean"); return Value::NONE; }
                return (V==Value::FLOAT?Value::FLOAT:Value::INT);
            }
            if (op=="!") return Value::BOOL;
        }
        if (node->node_type=="Assign") {
            // assignment is treated at statement level; here infer RHS
            return infer_expr_type(node->children[0], locals);
        }
        // fallback
        return Value::NONE;
    }

    void analyze_var_decl(const shared_ptr<AST> &node, unordered_map<string, Value::Type> &locals, const string &context_name) {
        if (!node) return;
        string name = node->value;
        string t = node->children[0]->node_type;
        Value::Type vt = string_to_type(t);
        if (vt==Value::NONE) { errors.push_back("Unknown type for variable '" + name + "'"); return; }
        if (locals.count(name)) { errors.push_back("Redeclaration of variable '" + name + "' in " + context_name); return; }
        locals[name] = vt;
        if (node->children.size()>=2) {
            Value::Type rhs = infer_expr_type(node->children[1], locals);
            if (rhs!=Value::NONE && !compatible(vt, rhs)) {
                errors.push_back("Type mismatch in initializer for '" + name + "': expected " + type_to_string(vt) + ", got " + type_to_string(rhs));
            }
        }
    }

    void analyze_statement(const shared_ptr<AST> &st, unordered_map<string, Value::Type> &locals, const string &current_ret_type) {
        if (!st) return;
        if (st->node_type=="VarDecl") { analyze_var_decl(st, locals, "function"); return; }
        if (st->node_type=="Assign") {
            string name = st->value;
            if (!locals.count(name) && !globals.count(name)) { errors.push_back("Assignment to undeclared variable '" + name + "'"); }
            Value::Type rhs = infer_expr_type(st->children[0], locals);
            Value::Type dest = locals.count(name)?locals[name]:(globals.count(name)?globals[name]:Value::NONE);
            if (rhs!=Value::NONE && dest!=Value::NONE && !compatible(dest, rhs)) {
                errors.push_back("Type mismatch in assignment to '" + name + "': expected " + type_to_string(dest) + ", got " + type_to_string(rhs));
            }
            return;
        }
        if (st->node_type=="Print") { if (!st->children.empty()) infer_expr_type(st->children[0], locals); return; }
        if (st->node_type=="If") {
            infer_expr_type(st->children[0], locals);
            // then block
            for (auto &s : st->children[1]->children) analyze_statement(s, locals, current_ret_type);
            if (st->children.size()>=3) for (auto &s : st->children[2]->children) analyze_statement(s, locals, current_ret_type);
            return;
        }
        if (st->node_type=="While") {
            infer_expr_type(st->children[0], locals);
            for (auto &s : st->children[1]->children) analyze_statement(s, locals, current_ret_type);
            return;
        }
        if (st->node_type=="For") {
            // children: init?, cond?, post?, body
            if (st->children.size()>=1 && st->children[0]) {
                if (st->children[0]->node_type=="VarDecl") analyze_var_decl(st->children[0], locals, "for-loop");
                else infer_expr_type(st->children[0], locals);
            }
            if (st->children.size()>=2 && st->children[1]) infer_expr_type(st->children[1], locals);
            if (st->children.size()>=3 && st->children[2]) infer_expr_type(st->children[2], locals);
            if (!st->children.empty()) for (auto &s : st->children.back()->children) analyze_statement(s, locals, current_ret_type);
            return;
        }
        if (st->node_type=="Return") {
            if (!st->children.empty()) {
                Value::Type rv = infer_expr_type(st->children[0], locals);
                Value::Type declared = string_to_type(current_ret_type);
                if (rv!=Value::NONE && declared!=Value::NONE && !compatible(declared, rv)) {
                    errors.push_back("Return type mismatch: function expects " + type_to_string(declared) + ", returned " + type_to_string(rv));
                }
            } else {
                // void/none return: if function declares non-none return type, error
                Value::Type declared = string_to_type(current_ret_type);
                if (declared!=Value::NONE) errors.push_back("Missing return value in function that declares return type '" + current_ret_type + "'");
            }
            return;
        }
        if (st->node_type=="Block") {
            for (auto &s : st->children) analyze_statement(s, locals, current_ret_type);
            return;
        }
        // expression statements
        infer_expr_type(st, locals);
    }

    void analyze_function(const FunctionInfo &fi) {
        // build local scope from params and vardecls in the immediate body
        unordered_map<string, Value::Type> locals;
        for (auto &p : fi.params) {
            Value::Type pt = string_to_type(p.second);
            if (pt==Value::NONE) { errors.push_back("Unknown parameter type for '" + p.first + "' in function '" + fi.name + "'"); }
            if (locals.count(p.first)) { errors.push_back("Duplicate parameter name '" + p.first + "' in function '" + fi.name + "'"); }
            locals[p.first] = pt;
        }
        // collect var declarations at function body top-level (simple approach)
        for (auto &st : fi.body->children) {
            if (st->node_type=="VarDecl") {
                string vname = st->value; string t = st->children[0]->node_type; Value::Type vt = string_to_type(t);
                if (locals.count(vname)) warnings.push_back("Shadowing/redeclaration of '" + vname + "' in function '" + fi.name + "'");
                locals[vname] = vt;
            }
        }
        // analyze statements now
        for (auto &st : fi.body->children) analyze_statement(st, locals, fi.return_type);
    }

    void run() {
        if (!ast) return;
        // top-level: check global var initializers
        for (auto &child : ast->children) {
            if (child->node_type=="VarDecl") {
                string name = child->value; string t = child->children[0]->node_type; Value::Type vt = string_to_type(t);
                if (child->children.size()>=2) {
                    unordered_map<string, Value::Type> globals_copy = globals; // allow reading other globals
                    Value::Type rhs = infer_expr_type(child->children[1], globals_copy);
                    if (rhs!=Value::NONE && !compatible(vt, rhs)) errors.push_back("Type mismatch in initializer for global '" + name + "': expected " + type_to_string(vt) + ", got " + type_to_string(rhs));
                }
            }
        }
        // functions
        for (auto &kv : *functions) analyze_function(kv.second);
    }
};

struct Interpreter {
    shared_ptr<AST> ast;
    vector<string> errors;
    vector<string> warnings;
    vector<Token> tokens;
    string output;

    unordered_map<string, Value::Type> globals;
    unordered_map<string, Value> global_values;
    unordered_map<string, FunctionInfo> functions;

    Interpreter(shared_ptr<AST> a, const vector<Token>& t): ast(a), tokens(t) {}

    Value::Type type_from_string(const string &s) {
        if (s=="int") return Value::INT;
        if (s=="float") return Value::FLOAT;
        if (s=="bool") return Value::BOOL;
        return Value::NONE;
    }

    void collect_decls() {
        if (!ast) return;
        for (auto &child : ast->children) {
            if (child->node_type=="VarDecl") {
                string name = child->value;
                string t = child->children[0]->node_type;
                Value::Type vt = type_from_string(t);
                if (vt==Value::NONE) { errors.push_back("Unknown type for variable " + name); continue; }
                if (globals.count(name)) warnings.push_back("Redeclaration of variable " + name);
                globals[name] = vt;
                Value v; v.type = vt; if (vt==Value::INT) v.i=0; if (vt==Value::FLOAT) v.f=0.0; if (vt==Value::BOOL) v.b=false; global_values[name]=v;
                if (child->children.size()>=2) {
                    // initializer
                    Value init = eval_expression(child->children[1]); global_values[name]=init;
                }
            } else if (child->node_type=="FunctionDecl") {
                FunctionInfo fi; fi.name = child->value;
                auto params = child->children[0];
                for (auto &p : params->children) {
                    string pname = p->value; string ptype = p->children[0]->node_type; fi.params.push_back({pname, ptype});
                }
                fi.return_type = child->children[1]->node_type;
                fi.body = child->children[2];
                if (functions.count(fi.name)) errors.push_back("Redeclared function " + fi.name);
                functions[fi.name] = fi;
            }
        }
    }

    struct Frame { unordered_map<string, Value> locals; };
    vector<Frame> callstack;
    bool has_return = false; Value return_value;

    Value eval_expression(const shared_ptr<AST> &node) {
        Value res; if (!node) { res.type = Value::NONE; return res; }
        if (node->node_type=="Literal") {
            string s = node->value;
            if (s=="true" || s=="false") { res.type = Value::BOOL; res.b = (s=="true"); return res; }
            if (s.find('.')!=string::npos) { res.type = Value::FLOAT; try { res.f = stod(s); } catch(...) { res.f=0.0; } return res; }
            res.type = Value::INT; try { res.i = stoll(s); } catch(...) { res.i=0; } return res;
        }
        if (node->node_type=="Identifier") {
            string name = node->value;
            for (int i=(int)callstack.size()-1;i>=0;--i) {
                auto &frm = callstack[i]; if (frm.locals.count(name)) return frm.locals[name];
            }
            if (global_values.count(name)) return global_values[name];
            errors.push_back("Undefined variable: " + name);
            return res;
        }
        if (node->node_type=="Assign") {
            string name = node->value; Value v = eval_expression(node->children[0]);
            if (!callstack.empty() && callstack.back().locals.count(name)) callstack.back().locals[name] = v;
            else if (global_values.count(name)) global_values[name] = v;
            else { global_values[name] = v; warnings.push_back("Implicit global creation of " + name); }
            return v;
        }
        if (node->node_type=="Call") {
            string fname = node->value;
            if (fname=="print") {
                if (node->children.size()>=1) {
                    Value v = eval_expression(node->children[0]); output += v.toString(); output += "\n"; return v;
                } else { output += "\n"; Value v; v.type=Value::NONE; return v; }
            }
            if (!functions.count(fname)) { errors.push_back("Call to undefined function " + fname); return res; }
            auto &fi = functions[fname];
            if (node->children.size() != fi.params.size()) { errors.push_back("Argument count mismatch in call to " + fname); }
            vector<Value> args; for (auto &ch : node->children) args.push_back(eval_expression(ch));
            Frame f; for (size_t i=0;i<fi.params.size() && i<args.size();++i) f.locals[fi.params[i].first] = args[i];
            callstack.push_back(f);
            execute_block(fi.body);
            Value ret = return_value;
            has_return = false; return_value = Value();
            callstack.pop_back();
            return ret;
        }
        if (node->node_type=="BinaryOp") {
            auto L = eval_expression(node->children[0]); auto R = eval_expression(node->children[1]); string op = node->value;
            Value out;
            if (op=="+") {
                if (L.type==Value::FLOAT || R.type==Value::FLOAT) { out.type=Value::FLOAT; out.f = (L.type==Value::FLOAT?L.f:L.i) + (R.type==Value::FLOAT?R.f:R.i); }
                else { out.type=Value::INT; out.i = L.i + R.i; }
            } else if (op=="-") {
                if (L.type==Value::FLOAT || R.type==Value::FLOAT) { out.type=Value::FLOAT; out.f = (L.type==Value::FLOAT?L.f:L.i) - (R.type==Value::FLOAT?R.f:R.i); }
                else { out.type=Value::INT; out.i = L.i - R.i; }
            } else if (op=="*") {
                if (L.type==Value::FLOAT || R.type==Value::FLOAT) { out.type=Value::FLOAT; out.f = (L.type==Value::FLOAT?L.f:L.i) * (R.type==Value::FLOAT?R.f:R.i); }
                else { out.type=Value::INT; out.i = L.i * R.i; }
            } else if (op=="/") {
                if (R.type==Value::INT && R.i==0) { errors.push_back("Division by zero"); return out; }
                if (R.type==Value::FLOAT && R.f==0.0) { errors.push_back("Division by zero"); return out; }
                out.type = Value::FLOAT;
                double lv = (L.type==Value::FLOAT?L.f:L.i);
                double rv = (R.type==Value::FLOAT?R.f:R.i);
                out.f = lv / rv;
            } else if (op=="<" || op==">" || op=="<=" || op==">=") {
                double lv = (L.type==Value::FLOAT?L.f:L.i);
                double rv = (R.type==Value::FLOAT?R.f:R.i);
                out.type = Value::BOOL;
                if (op=="<") out.b = lv < rv;
                else if (op==">") out.b = lv > rv;
                else if (op=="<=") out.b = lv <= rv;
                else out.b = lv >= rv;
            } else if (op=="==" || op!="") {
                out.type = Value::BOOL;
                if (L.type==Value::BOOL || R.type==Value::BOOL) {
                    bool lb = (L.type==Value::BOOL?L.b:(L.type==Value::FLOAT?L.f!=0.0:L.i!=0));
                    bool rb = (R.type==Value::BOOL?R.b:(R.type==Value::FLOAT?R.f!=0.0:R.i!=0));
                    out.b = (op=="==") ? (lb==rb) : (lb!=rb);
                } else {
                    double lv = (L.type==Value::FLOAT?L.f:L.i);
                    double rv = (R.type==Value::FLOAT?R.f:R.i);
                    out.b = (op=="==") ? (fabs(lv-rv) < 1e-9) : !(fabs(lv-rv) < 1e-9);
                }
            } else if (op=="&&") {
                out.type = Value::BOOL;
                bool lb = (L.type==Value::BOOL?L.b:(L.type==Value::FLOAT?L.f!=0.0:L.i!=0));
                if (!lb) { out.b = false; return out; }
                bool rb = (R.type==Value::BOOL?R.b:(R.type==Value::FLOAT?R.f!=0.0:R.i!=0));
                out.b = rb;
            } else if (op=="||") {
                out.type = Value::BOOL;
                bool lb = (L.type==Value::BOOL?L.b:(L.type==Value::FLOAT?L.f!=0.0:L.i!=0));
                if (lb) { out.b = true; return out; }
                bool rb = (R.type==Value::BOOL?R.b:(R.type==Value::FLOAT?R.f!=0.0:R.i!=0));
                out.b = rb;
            }
            return out;
        }
        if (node->node_type=="UnaryOp") {
            string op = node->value; auto V = eval_expression(node->children[0]); Value out;
            if (op=="-") {
                if (V.type==Value::FLOAT) { out.type=Value::FLOAT; out.f = -V.f; }
                else { out.type=Value::INT; out.i = -V.i; }
            } else if (op=="!") {
                out.type = Value::BOOL;
                bool vb = (V.type==Value::BOOL?V.b:(V.type==Value::FLOAT?V.f!=0.0:V.i!=0));
                out.b = !vb;
            }
            return out;
        }
        if (node->node_type=="Call") {
            return eval_expression(node); // handled above
        }
        return res;
    }

    void execute_block(const shared_ptr<AST> &block) {
        if (!block) return;
        for (auto &st : block->children) {
            if (has_return) return;
            execute_statement(st);
            if (has_return) return;
        }
    }

    void execute_statement(const shared_ptr<AST> &node) {
        if (!node) return;
        if (node->node_type=="VarDecl") {
            string name = node->value; // type in child 0
            if (node->children.size()>=2) {
                Value v = eval_expression(node->children[1]);
                if (!callstack.empty()) callstack.back().locals[name] = v;
                else global_values[name] = v;
            } else {
                Value v; v.type = globals.count(name)?globals[name]:Value::NONE;
                if (!callstack.empty()) callstack.back().locals[name] = v; else global_values[name]=v;
            }
            return;
        }
        if (node->node_type=="Assign") { eval_expression(node); return; }
        if (node->node_type=="Print") { auto v = eval_expression(node->children[0]); output += v.toString(); output += "\n"; return; }
        if (node->node_type=="If") {
            Value c = eval_expression(node->children[0]); bool cond = (c.type==Value::BOOL?c.b:(c.type==Value::FLOAT?c.f!=0.0:c.i!=0));
            if (cond) execute_block(node->children[1]); else if (node->children.size()>=3) execute_block(node->children[2]);
            return;
        }
        if (node->node_type=="While") {
            while (true) {
                Value c = eval_expression(node->children[0]); if (has_return) return;
                bool cond = (c.type==Value::BOOL?c.b:(c.type==Value::FLOAT?c.f!=0.0:c.i!=0));
                if (!cond) break;
                execute_block(node->children[1]); if (has_return) return;
            }
            return;
        }
        if (node->node_type=="For") {
            int idx = 0;
            if (node->children.size()>=4) {
                if (node->children[0]) execute_statement(node->children[0]);
                while (true) {
                    if (node->children[1]) {
                        Value c = eval_expression(node->children[1]); bool cond = (c.type==Value::BOOL?c.b:(c.type==Value::FLOAT?c.f!=0.0:c.i!=0));
                        if (!cond) break;
                    }
                    execute_block(node->children.back()); if (has_return) return;
                    if (node->children[2]) eval_expression(node->children[2]);
                }
            }
            return;
        }
        if (node->node_type=="Return") {
            if (!node->children.empty()) return_value = eval_expression(node->children[0]);
            has_return = true; return;
        }
        if (node->node_type=="Block") { execute_block(node); return; }
        eval_expression(node);
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    std::ostringstream ss; ss << cin.rdbuf(); string src = ss.str();

    // Strip UTF-8 BOM if present (prevents illegal-character tokens for BOM bytes)
    if (src.size() >= 3 && (unsigned char)src[0] == 0xEF && (unsigned char)src[1] == 0xBB && (unsigned char)src[2] == 0xBF) {
        src = src.substr(3);
    }

    vector<string> lex_errors;
    auto tokens = tokenize(src, lex_errors);

    Parser p(tokens);
    auto ast = p.parse_program();

    Interpreter interp(ast, tokens);
    interp.errors.insert(interp.errors.end(), lex_errors.begin(), lex_errors.end());
    interp.errors.insert(interp.errors.end(), p.errors.begin(), p.errors.end());

    interp.collect_decls();

    SemanticAnalyzer analyzer(ast, interp.globals, interp.functions);
    analyzer.run();
    interp.errors.insert(interp.errors.end(), analyzer.errors.begin(), analyzer.errors.end());
    interp.warnings.insert(interp.warnings.end(), analyzer.warnings.begin(), analyzer.warnings.end());

    if (interp.errors.empty()) {
        for (auto &child : ast->children) {
            if (child->node_type=="FunctionDecl") continue;
            interp.execute_statement(child);
            if (!interp.errors.empty()) break;
        }
    }

    ostringstream out;
    out << "{\n";
    out << "  \"tokens\": [\n";
    for (size_t i=0;i<tokens.size();++i) {
        out << "    {\"type\": \"" << escape_json(tokens[i].type) << "\", \"text\": \"" << escape_json(tokens[i].text) << "\", \"line\": " << tokens[i].line << ", \"pos\": " << tokens[i].pos << "}";
        if (i+1<tokens.size()) out << ",\n"; else out << "\n";
    }
    out << "  ],\n";
    out << "  \"ast\": " << ast_to_json(ast,2) << ",\n";
    out << "  \"symbol_table\": {\n";
    size_t cnt=0; for (auto &kv : interp.globals) {
        out << "    \"" << escape_json(kv.first) << "\": \"" << (kv.second==Value::INT?"int":kv.second==Value::FLOAT?"float":"bool") << "\"";
        if (++cnt < interp.globals.size()) out << ",\n"; else out << "\n";
    }
    out << "  },\n";
    out << "  \"function_table\": {\n";
    cnt=0; for (auto &kv : interp.functions) {
        out << "    \"" << escape_json(kv.first) << "\": {\n";
        out << "      \"return_type\": \"" << escape_json(kv.second.return_type) << "\",\n";
        out << "      \"params\": [";
        for (size_t i=0;i<kv.second.params.size();++i) {
            out << "{\"name\": \"" << escape_json(kv.second.params[i].first) << "\", \"type\": \"" << escape_json(kv.second.params[i].second) << "\"}";
            if (i+1<kv.second.params.size()) out << ", ";
        }
        out << "]\n    }";
        if (++cnt < interp.functions.size()) out << ",\n"; else out << "\n";
    }
    out << "  },\n";
    out << "  \"errors\": [\n";
    for (size_t i=0;i<interp.errors.size();++i) {
        out << "    \"" << escape_json(interp.errors[i]) << "\"";
        if (i+1<interp.errors.size()) out << ",\n"; else out << "\n";
    }
    out << "  ],\n";
    out << "  \"warnings\": [\n";
    for (size_t i=0;i<interp.warnings.size();++i) {
        out << "    \"" << escape_json(interp.warnings[i]) << "\"";
        if (i+1<interp.warnings.size()) out << ",\n"; else out << "\n";
    }
    out << "  ],\n";
    out << "  \"output\": \"" << escape_json(interp.output) << "\"\n";
    out << "}\n";

    cout << out.str();
    return 0;
}

