#include "AST.h"
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <vector>



Node::Node(std::int64_t v)
    : type(NodeType::Number), value(v) {}

Node::Node(NodeType t, std::unique_ptr<Node> l, std::unique_ptr<Node> r)
    : type(t), value(0), left(std::move(l)), right(std::move(r)) {}
//tokenizer
std::vector<Token> tokenize(const std::string& s) {
    std::vector<Token> tokens;

    for (std::size_t i = 0; i < s.size();) {
        if (std::isspace(static_cast<unsigned char>(s[i]))) {
            ++i;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(s[i]))) {
            std::int64_t value = 0;
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
                value = value * 10 + (s[i] - '0');
                ++i;
            }
            tokens.push_back({TokenType::Number, value});
            continue;
        }
        switch (s[i]) {
            case '+':
                tokens.push_back({TokenType::Plus, 0});
                break;
            case '-':
                tokens.push_back({TokenType::Minus, 0});
                break;
            case '*':
                tokens.push_back({TokenType::Mult, 0});
                break;
            case '(':
                tokens.push_back({TokenType::LParen, 0});
                break;
            case ')':
                tokens.push_back({TokenType::RParen, 0});
                break;
            default:
                throw std::runtime_error("invalid character");
        }
        ++i;
    }
    tokens.push_back({TokenType::End, 0});
    return tokens;
}
    
int main() {
    std::string input;
    std::getline(std::cin, input);
    auto tokens = tokenize(input);

    for (const auto& t : tokens) {
        if (t.type == TokenType::Number)
            std::cout << "NUMBER " << t.value << "\n";
        else
            std::cout << "TOKEN\n";
    }
}