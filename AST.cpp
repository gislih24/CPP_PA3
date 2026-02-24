#include "AST.h"
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <vector>

// AST node constructors and tokenizer implementation
// For number nodes, left and right are not used, so we can set them to nullptr
Node::Node(std::int64_t v) : type(NodeType::Number), value(v) {}

// For operator nodes, value is not used, so we can set it to 0
// Node::Node(NodeType t, std::unique_ptr<Node> l, std::unique_ptr<Node> r)
//     : type(t), value(0), left(std::move(l)), right(std::move(r)) {}
Node::Node(NodeType t, std::unique_ptr<Node> l, std::unique_ptr<Node> r)
    : type(t), value(0), left(std::move(l)), right(std::move(r)) {}

class AST {
  private:
    std::unique_ptr<Node> root_;

    static int precedence(TokenType t) {
        switch (t) {
        case TokenType::Mult:
            return 2;
        case TokenType::Plus:
            return 1;
        case TokenType::Minus:
            return 1;
        default:
            return -1;
        }
    }

    static NodeType to_node_type(TokenType t) {
        switch (t) {
        case TokenType::Plus:
            return NodeType::Add;
        case TokenType::Minus:
            return NodeType::Sub;
        case TokenType::Mult:
            return NodeType::Mult;
        default:
            throw std::runtime_error("unexpected operator token");
        }
    }

  public:
    std::vector<Token> tokens;

    void rotate_left(Node* old_parent) {
        if (old_parent->left == nullptr) {
            auto new_node = std::make_unique<Node>();
            new_node->value = old_parent->value;
            old_parent->value = old_parent->left->value;
        }
    }

    void add_node_to_tree(Node* node_to_add_to,
                          std::unique_ptr<Node> new_node) {
        if (new_node->type == NodeType::Number) {
            if (node_to_add_to->left == nullptr) {
                node_to_add_to->left = std::move(new_node);
            } else if (node_to_add_to->right == nullptr) {
                node_to_add_to->right = std::move(new_node);
            } else {
                add_node_to_tree(node_to_add_to->right.get(),
                                 std::move(new_node));
            }
        } else {
            if (node_to_add_to->right == nullptr) {
                node_to_add_to->right = std::move(new_node);
                if (node_to_add_to->type == NodeType::Number &&
                    new_node->type != NodeType::Number) {
                    rotate_left(node_to_add_to);
                }
            } else {
                add_node_to_tree(node_to_add_to->right.get(),
                                 std::move(new_node));
            }
        }
    }

    void add_tokens_to_tree() {
        for (Token curr_token : tokens) {
            if (curr_token.type == TokenType::Number) {
                auto new_node = std::make_unique<Node>(curr_token.value);
                add_node_to_tree(root_.get(), std::move(new_node));
            } else if (curr_token.type == TokenType::Plus) {
                auto new_node =
                    std::make_unique<Node>(NodeType::Add, nullptr, nullptr);
                add_node_to_tree(root_.get(), std::move(new_node));
            } else if (curr_token.type == TokenType::Minus) {
                auto new_node =
                    std::make_unique<Node>(NodeType::Sub, nullptr, nullptr);
                add_node_to_tree(root_.get(), std::move(new_node));
            } else if (curr_token.type == TokenType::Mult) {
                auto new_node =
                    std::make_unique<Node>(NodeType::Mult, nullptr, nullptr);
                add_node_to_tree(root_.get(), std::move(new_node));
            } else {
                throw std::runtime_error("unexpected token type");
            }
        }
    }

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
    // std::string input;
    // std::getline(std::cin, input);
    // AST my_ast;
    // my_ast.tokenize(input);

    // for (const auto& t : my_ast.tokens) {
    //     if (t.type == TokenType::Number) {
    //         std::cout << "NUMBER " << t.value << "\n";
    //     } else {
    //         std::cout << "TOKEN\n";
    //     }
    // }
    auto node_l1 =
        std::make_unique<Node>(NodeType::Add, std::make_unique<Node>(3),
                               std::make_unique<Node>(4)); // 3+4=7
    auto root_node =
        std::make_unique<Node>(NodeType::Add, std::make_unique<Node>(3),
                               std::move(node_l1)); // 3+7=10
    std::cout << "The value of root_node:" << root_node->get_value();
}