#include "AST.h"

#include <cctype>
#include <cstdint>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>

// MARK: namespace
namespace {

/**
 * @brief Returns the precedence of the given operator token.
 * @param t The operator token type to get the precedence of.
 * @return The precedence of the given operator token. Higher number means
 * higher precedence. Returns -1 if the token is not an operator.
 */
int get_precedence(TokenType t) {
    switch (t) {
    case TokenType::Div:
    case TokenType::Mult:
        return 2;
    case TokenType::Plus:
    case TokenType::Minus:
        return 1;
    default:
        return -1;
    }
}

/**
 * @brief Converts an operator token type to the corresponding node type.
 * @param t The operator token type to convert.
 * @return The corresponding node type for the given operator token type.
 */
NodeType token_type_to_node_type(TokenType t) {
    switch (t) {
    case TokenType::Plus:
        return NodeType::Add;
    case TokenType::Minus:
        return NodeType::Sub;
    case TokenType::Mult:
        return NodeType::Mult;
    case TokenType::Div:
        return NodeType::Div;
    default:
        throw ASTException("unexpected operator token");
    }
}

/**
 * @brief Returns whether the given token type is a binary operator.
 * @param t The token type to check.
 * @return true if the given token type is a binary operator, false otherwise.
 */
bool is_arithmetic_operator(TokenType t) {
    return t == TokenType::Plus || t == TokenType::Minus ||
           t == TokenType::Mult || t == TokenType::Div;
}

/**
 * @brief Parses a number from the input string starting at the given index,
 * and advances the index to the first character after the number.
 * @param input_string The input string to parse the number from.
 * @param index The index to start parsing from. Will be advanced to the first
 * character after the number. (size_t is an integer made just for storing the
 * size of things).
 * @return The parsed number.
 */
int64_t parse_number(const std::string& input_string, std::size_t& index) {
    int64_t parsed_number = 0;
    while (index < input_string.size()) {
        // If conversion fails, break.
        if (const auto input_digit_char =
                static_cast<unsigned char>(input_string[index]);
            !std::isdigit(input_digit_char)) {
            break;
        }
        // Doing "- '0'" to a character that's a digit gives you the actual
        // integer value. E.g. '3' - '0' = 3.
        int64_t digit_value = input_string[index] - '0';
        // Since we're parsing each digit from left to right →, we wanna move
        // the digits we've already parsed by one place when we add the newest
        // digit.
        parsed_number = parsed_number * 10 + digit_value;
        ++index;
    }
    return parsed_number;
}

/**
 * @brief Parses a lower-case ASCII variable name from the input string
 * starting at the given index and advances the index to the first character
 * after the variable.
 * @param input_string The input string to parse from.
 * @param index The index to start parsing from. Advanced past the variable.
 * @return The parsed variable name.
 */
std::string parse_variable_name(const std::string& input_string,
                                std::size_t& index) {
    const std::size_t start_index = index;
    while (index < input_string.size()) {
        // If the current input character isn't a lowercase ASCII character,
        // then break.
        if (const auto curr_char =
                static_cast<unsigned char>(input_string[index]);
            !std::islower(curr_char)) {
            break;
        }
        ++index;
    }
    return input_string.substr(start_index, index - start_index);
}

/**
 * @brief Pops the top operator from the operator stack, pops the top two
 * values from the value stack, applies the operator to the values, and pushes
 * the result back onto the value stack.
 * @param value_stack The stack of values to pop the operands from and push the
 * result onto.
 * @param operator_stack The stack of operators to pop the operator from.
 */
void apply_top_operator(std::stack<std::unique_ptr<Node>>& value_stack,
                        std::stack<TokenType>& operator_stack) {
    if (operator_stack.empty()) {
        throw ASTException("missing operator");
    }
    if (value_stack.size() < 2) {
        throw ASTException("missing operand");
    }

    // The operation we want to do is essentially:
    // <left_hand_side_value> <operator> <right_hand_side_value>
    // Get the current operator.
    const TokenType current_operator = operator_stack.top();
    operator_stack.pop();

    // Get the values we want to apply the operation to.
    auto right_hand_side_value = std::move(value_stack.top());
    value_stack.pop();
    auto left_hand_side_value = std::move(value_stack.top());
    value_stack.pop();

    // Create a new node, with:
    value_stack.push(std::make_unique<Node>(
        token_type_to_node_type(current_operator),
        std::move(left_hand_side_value), std::move(right_hand_side_value)));
}

/**
 * @brief Helper function that handles an operator token by popping and
 * applying operators from the operator stack until we find an operator with
 * lower precedence.
 * @param op_token_type The type of the operator token we're handling.
 */
void handle_operator(TokenType op_token_type,
                     std::stack<std::unique_ptr<Node>>& value_stack,
                     std::stack<TokenType>& operator_stack) {
    // While: the stack isn't empty,
    // and the top token isn't a '(',
    // and the top operator has a greater precedence than our operator,
    while ((!operator_stack.empty()) &&
           (operator_stack.top() != TokenType::LParen) &&
           (get_precedence(operator_stack.top()) >=
            get_precedence(op_token_type))) {
        // apply the top operator to the top 2 values of the value stack.
        apply_top_operator(value_stack, operator_stack);
    }
    // Finally, after applying all operators with higher precedence, we can
    // push our operator.
    operator_stack.push(op_token_type);
}

} // namespace

// ---------------------------- Node constructors ----------------------------
// Constructor for number nodes.
Node::Node(int64_t v)
    : type(NodeType::Number), value(v), variable_name(""), left(nullptr),
      right(nullptr) {}

// Constructor for variable nodes.
Node::Node(std::string variable)
    : type(NodeType::Variable), value(0), variable_name(std::move(variable)),
      left(nullptr), right(nullptr) {}

// Constructor for operator nodes.
Node::Node(NodeType t, std::unique_ptr<Node> l, std::unique_ptr<Node> r)
    : type(t), value(0), variable_name(""), left(std::move(l)),
      right(std::move(r)) {}

// MARK: AST
// ----------------------------------- AST -----------------------------------

/**
 * @brief Clears the AST by resetting the root and clearing the tokens_ vector.
 */
void AST::clear() {
    root_.reset();
    tokens_.clear();
}

/**
 * @brief Tokenizes the input string into a vector of tokens, which are stored
 * in the tokens_ field.
 * @param input_string The input string to tokenize.
 */
void AST::tokenize(const std::string& input_string) {
    tokens_.clear(); // Clear the tokens first.

    // size_t is used here since it's a type that's guaranteed to be able to
    // represent the size of any object in memory.
    std::size_t i = 0;
    bool expecting_operand = true;
    // Go through the characters of the string.
    while (i < input_string.size()) {
        // Convert the current character. unsigned char is used here for extra
        // safety.
        const auto curr_char = static_cast<unsigned char>(input_string[i]);

        // Ignore whitespace.
        if (std::isspace(curr_char)) {
            ++i;
            continue;
        }

        // Handle unary minus by rewriting it to either a negative number
        // token or to -1 * (...), depending on what follows.
        if (input_string[i] == '-' && expecting_operand) {
            std::size_t lookahead = i + 1;
            while (lookahead < input_string.size() &&
                   std::isspace(
                       static_cast<unsigned char>(input_string[lookahead]))) {
                ++lookahead;
            }

            // Case: -(digits...)  -> Number(-digits...)
            if (lookahead < input_string.size() &&
                std::isdigit(
                    static_cast<unsigned char>(input_string[lookahead]))) {
                i = lookahead;
                int64_t parsed_number = parse_number(input_string, i);
                tokens_.emplace_back(TokenType::Number, -parsed_number);
                expecting_operand = false;
                continue;
            }

            // Case: -(...) (or another unary expression)
            // Rewrite as: -1 * (...)
            tokens_.emplace_back(TokenType::Number, -1);
            tokens_.emplace_back(TokenType::Mult, 0);
            ++i;
            expecting_operand = true;
            continue;
        }

        // If it's a digit, we have a number, so we try to parse that, along
        // with the rest of the digits of this number.
        if (std::isdigit(curr_char)) {
            int64_t parsed_number = parse_number(input_string, i);
            // "Emplace" a number token (construct it in-place in the vector)
            // with the parsed number as the value, and an empty variable name
            // (since it's not a variable).
            tokens_.emplace_back(TokenType::Number, parsed_number, "");
            expecting_operand = false;
            continue;
        }

        // If it's a lower-case letter, parse a variable name [a-z]+.
        if (std::islower(curr_char)) {
            std::string parsed_variable = parse_variable_name(input_string, i);
            tokens_.emplace_back(TokenType::Variable, 0,
                                 std::move(parsed_variable));
            continue;
        }

        // If it's a token, try to parse it.
        TokenType type;
        switch (input_string[i]) {
        case '+':
            type = TokenType::Plus;
            break;
        case '-':
            type = TokenType::Minus;
            break;
        case '*':
            type = TokenType::Mult;
            break;
        case '/':
            type = TokenType::Div;
            break;
        case '(':
            type = TokenType::LParen;
            break;
        case ')':
            type = TokenType::RParen;
            break;
        default:
            throw ASTException("invalid character in expression");
        }

        // Push the operator token, with the value 0 (since it's an operator).
        tokens_.emplace_back(type, 0, "");
        ++i;

        if (type == TokenType::RParen) {
            expecting_operand = false;
        } else {
            expecting_operand = true;
        }
    }

    tokens_.emplace_back(TokenType::End, 0, ""); // Push the end token.
}

/**
 * @brief Converts the tokens we've tokenized into an AST, and stores the root
 * in the root_ field.
 *
 * This is done using the "shunting yard algorithm", which uses the
 * operator_stack and value_stack to maintain the current state of the
 * conversion.
 */
void AST::add_tokens_to_tree() {
    root_.reset();

    // Initialize our stacks.
    std::stack<std::unique_ptr<Node>> value_stack; // The stack of values.
    std::stack<TokenType> operator_stack;

    // Iterate through all the tokens.
    for (const Token& current_token : tokens_) {
        // If we have a number token, push it onto the value stack.
        if (current_token.type == TokenType::Number) {
            value_stack.push(std::make_unique<Node>(current_token.value));
            continue;
        }

        if (current_token.type == TokenType::Variable) {
            value_stack.push(
                std::make_unique<Node>(current_token.variable_name));
            continue;
        }

        if (current_token.type == TokenType::LParen) {
            operator_stack.push(current_token.type);
            continue;
        }

        if (current_token.type == TokenType::RParen) {
            // While we don't find a '(', we apply the top operator to the top
            // 2 values of the value stack.
            while (!operator_stack.empty() &&
                   operator_stack.top() != TokenType::LParen) {
                apply_top_operator(value_stack, operator_stack);
            }
            // If we run out of operators before finding a '(', then we have a
            // mismatched parentheses error.
            if (operator_stack.empty()) {
                throw ASTException("mismatched ')'");
            }
            // Finally, pop the '(' from the operator stack and discard it.
            operator_stack.pop();
            continue;
        }

        // Handle the case if we have an arithmetic operator.
        if (is_arithmetic_operator(current_token.type)) {
            handle_operator(current_token.type, value_stack, operator_stack);
            continue;
        }

        if (current_token.type == TokenType::End) {
            break;
        }

        // If we have a token that's not a number, operator, or parentheses,
        // then we have an unexpected token error.
        throw ASTException("unexpected token");
    }

    // While the operator stack isn't empty, apply the top operator to the top
    // 2 values of the value stack.
    while (!operator_stack.empty()) {
        if (operator_stack.top() == TokenType::LParen) {
            throw ASTException("mismatched '('");
        }
        apply_top_operator(value_stack, operator_stack);
    }

    // At this point, the operator stack should be empty, and the value stack
    // should have exactly 1 value (the root of the AST). Otherwise, we have an
    // error.
    if (value_stack.size() != 1) {
        throw ASTException("invalid expression");
    }

    // Set the root of the AST to the only value left on the value stack.
    root_ = std::move(value_stack.top());
    value_stack.pop();
}

/**
 * @brief Parses the input string into an AST by first tokenizing it and then
 * converting the tokens into a tree. The resulting AST is stored in the root_
 * field.
 * @param input_expression The input string to parse into an AST.
 */
void AST::parse(const std::string& input_expression) {
    clear();
    tokenize(input_expression);
    add_tokens_to_tree();
}

/**
 * @brief Evaluates the AST by calling get_value() on the root node, which
 * recursively evaluates the entire tree and returns the result.
 * @return The result of evaluating the AST.
 */
int64_t AST::evaluate() {
    if (!root_) {
        throw ASTException("tree is empty");
    }
    return root_->get_value();
}

// Getter for root_ (because might need to be accessed afterwards).
Node* AST::root() {
    return root_.get();
}

// Const getter for root_.
const Node* AST::root() const {
    return root_.get();
}

// Const getter for tokens_.
const std::vector<Token>& AST::tokens() const {
    return tokens_;
}
