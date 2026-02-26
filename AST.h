#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class NodeType { Number, Add, Sub, Mult };

struct Node {
    NodeType type;
    int64_t value; // lets allow large integers cause why not
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    int64_t get_value() {
        if (type == NodeType::Number) {
            return value;
        } else if (type == NodeType::Add) {
            value = left->get_value() + right->get_value();
        } else if (type == NodeType::Sub) {
            value = left->get_value() - right->get_value();
        } else if (type == NodeType::Mult) {
            value = left->get_value() * right->get_value();
        } else {
            return 0;
        }
        // I think everything after this line never runs
        type = NodeType::Number;
        // Since we're using unique_ptr, this will automatically delete the
        // left node, since nothing is pointing to it.
        left = nullptr;
        right = nullptr;
        return value;
    }

    Node(int64_t v);
    Node(NodeType t, std::unique_ptr<Node> l, std::unique_ptr<Node> r);
};

enum class TokenType { Number, Plus, Minus, Mult, LParen, RParen, End };

struct Token {
    TokenType type;
    int64_t value;
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
