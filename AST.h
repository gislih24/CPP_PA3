#pragma once
#include <memory>

enum class NodeType { Number, Add, Sub, Mult };

struct Node {
    NodeType type;
    std::int64_t value; // lets allow large integers cause why not
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    std::int64_t get_value() {
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
        type = NodeType::Number;
        // Since we're using unique_ptr, this will automatically delete the
        // left node, since nothing is pointing to it.
        left = nullptr;
        right = nullptr;
        return value;
    }

    Node(std::int64_t v);
    Node(NodeType t, std::unique_ptr<Node> l, std::unique_ptr<Node> r);
};

enum class TokenType { Number, Plus, Minus, Mult, LParen, RParen, End };

struct Token {
    TokenType type;
    long long value;
};