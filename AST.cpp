#include "AST.h"
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <vector>

// AST node constructors and tokenizer implementation
// For number nodes, left and right are not used, so we can set them to nullptr
Node::Node(std::int64_t v) : type(NodeType::Number), value(v) {}

// For operator nodes, value is not used, so we can set it to 0
Node::Node(NodeType t, std::unique_ptr<Node> l, std::unique_ptr<Node> r)
    : type(t), value(0), left(std::move(l)), right(std::move(r)) {}

class AST {
  public:
    std::vector<Token> tokens;

    // Tokenizer
    void tokenize(const std::string& s) {
        for (std::size_t i = 0; i < s.size();) {
            if (std::isspace(static_cast<unsigned char>(s[i]))) {
                ++i;
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(s[i]))) {
                std::int64_t value = 0;
                while (i < s.size() &&
                       std::isdigit(static_cast<unsigned char>(s[i]))) {
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
    }
};

int main() {
    std::string input;
    std::getline(std::cin, input);
    AST my_ast;
    my_ast.tokenize(input);

    for (const auto& t : my_ast.tokens) {
        if (t.type == TokenType::Number) {
            std::cout << "NUMBER " << t.value << "\n";
        } else {
            std::cout << "TOKEN\n";
        }
    }
}