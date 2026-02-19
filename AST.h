#pragma once
#include <memory>

enum class NodeType {
    Number, Add, Sub, Mult
};

struct Node {
    NodeType type;
    std::int64_t value; //lets allow large integers cause why not
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    Node(std::int64_t v);
    Node(NodeType t,
        std::unique_ptr<Node> l,
        std::unique_ptr<Node> r);
};

enum class TokenType {
    Number,
    Plus,
    Minus,
    Mult,
    LParen,
    RParen,
    End
};

struct Token {
    TokenType type;
    long long value;
};