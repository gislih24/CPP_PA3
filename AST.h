#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Custom exception for AST
class ASTException : public std::runtime_error {
  public:
    using runtime_error::runtime_error;
};

enum class NodeType { Number, Variable, Add, Sub, Mult };

struct Node {
    NodeType type;
    int64_t value; // lets allow large integers cause why not
    std::string variable_name;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    int64_t get_value() {
        if (type == NodeType::Number) {
            return value;
        } else if (type == NodeType::Variable) {
            throw ASTException("cannot evaluate variable without bindings");
        } else if (type == NodeType::Add) {
            return left->get_value() + right->get_value();
        } else if (type == NodeType::Sub) {
            return left->get_value() - right->get_value();
        } else if (type == NodeType::Mult) {
            return left->get_value() * right->get_value();
        } else {
            return 0;
        }
    }

    explicit Node(int64_t v);
    explicit Node(std::string variable);
    Node(NodeType t, std::unique_ptr<Node> l, std::unique_ptr<Node> r);
};

enum class TokenType {
    Number,
    Variable,
    Plus,
    Minus,
    Mult,
    LParen,
    RParen,
    End
};

struct Token {
    TokenType type;
    int64_t value;
    std::string variable_name;
};

class AST {
  public:
    void clear();
    void tokenize(const std::string& input);
    void add_tokens_to_tree();
    void parse(const std::string& input);
    int64_t evaluate();

    Node* root();
    const Node* root() const;
    const std::vector<Token>& tokens() const;

  private:
    std::unique_ptr<Node> root_;
    std::vector<Token> tokens_;
};
