#include "AST.h"

#include <cctype>
#include <charconv>
#include <cstdint>
#include <limits>
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
 * @brief Parses a signed positive int64 number from the input string.
 */
int64_t parse_number(const std::string& input_string, std::size_t& index) {
    const char* input_start = input_string.data() + index;
    const char* input_end = input_string.data() + input_string.size();

    // Initialize to silence warnings. Will be overwritten by from_chars if
    // parsing is successful.
    int64_t parsed_number = 0;
    // Use from_chars to parse the number, and handle errors according to the
    // result.
    const auto [end_of_parsed_input, parse_error] =
        std::from_chars(input_start, input_end, parsed_number);

    // If we didn't parse any characters, then we have a missing digits error.
    if (end_of_parsed_input == input_start) {
        throw ASTException("missing digits in number");
    }
    if (parse_error == std::errc::result_out_of_range) {
        throw ASTException("integer literal overflow");
    }
    // If we had any other error, such as invalid characters in the number, we
    // have an invalid numeric literal error.
    if (parse_error != std::errc{}) {
        throw ASTException("invalid numeric literal");
    }

    // Advance the index by the number of characters we parsed.
    index += static_cast<std::size_t>(end_of_parsed_input - input_start);
    return parsed_number;
}

/**
 * @brief Parses a negative int64 number from the input string (digits only).
 * Accepts one extra magnitude unit for INT64_MIN.
 */
int64_t parse_negative_number(const std::string_view& input_string,
                              std::size_t& index) {
    // To handle negative numbers, we parse the magnitude as a positive number,
    // and then negate it. This allows us to correctly handle the case of
    // INT64_MIN, which has a larger magnitude than INT64_MAX.
    constexpr uint64_t max_negative_magnitude =
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1ULL;
    const char* input_start = input_string.data() + index;
    const char* input_end = input_string.data() + input_string.size();

    // Initialize to silence warnings. Will be overwritten by from_chars if
    // parsing is successful.
    uint64_t magnitude = 0;
    // Use from_chars to parse the number, and handle errors according to the
    // result.
    const auto [end_of_parsed_input, parse_error] =
        std::from_chars(input_start, input_end, magnitude);

    // If we didn't parse any characters, then we have a missing digits error.
    if (end_of_parsed_input == input_start) {
        throw ASTException("missing digits in number");
    }
    if (parse_error == std::errc::result_out_of_range) {
        throw ASTException("integer literal overflow");
    }
    // If we had any other error, such as invalid characters in the number, we
    // have an invalid numeric literal error.
    if (parse_error != std::errc{}) {
        throw ASTException("invalid numeric literal");
    }
    if (magnitude > max_negative_magnitude) {
        throw ASTException("integer literal overflow");
    }

    // Advance the index by the number of characters we parsed.
    index += static_cast<std::size_t>(end_of_parsed_input - input_start);

    // Negate the magnitude to get the final parsed number. If the magnitude is
    // equal to max_negative_magnitude, then we have the case of INT64_MIN,
    // which we can return directly as a special case to avoid overflow from
    // negating it.
    if (magnitude == max_negative_magnitude) {
        return std::numeric_limits<int64_t>::min();
    }
    return -static_cast<int64_t>(magnitude);
}

/**
 * @brief Checked arithmetic operations that throw an ASTException on overflow
 * or other error conditions (such as division by zero).
 *
 * @param left The left operand of the operation.
 * @param right The right operand of the operation.
 * @return The result of the arithmetic operation if it does not overflow or
 * have other error conditions.
 */
int64_t checked_add(int64_t left, int64_t right) {
    int64_t result = 0;
    if (__builtin_add_overflow(left, right, &result)) {
        throw ASTException("overflow in addition");
    }
    return result;
}

int64_t checked_sub(int64_t left, int64_t right) {
    int64_t result = 0;
    if (__builtin_sub_overflow(left, right, &result)) {
        throw ASTException("overflow in subtraction");
    }
    return result;
}

int64_t checked_mul(int64_t left, int64_t right) {
    int64_t result = 0;
    if (__builtin_mul_overflow(left, right, &result)) {
        throw ASTException("overflow in multiplication");
    }
    return result;
}

int64_t checked_div(int64_t left, int64_t right) {
    if (right == 0) {
        throw ASTException("division by zero");
    }
    if (left == std::numeric_limits<int64_t>::min() && right == -1) {
        throw ASTException("overflow in division");
    }
    return left / right;
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

/**
 * @brief Handles unary minus by rewriting it to either a negative number
 * token or to -1 * (...).
 * @param input_string The input string being tokenized.
 * @param i The current index in the input string.
 * @param tokens The vector of tokens to add to.
 */
void handle_unary_minus(const std::string& input_string, std::size_t& i,
                        std::vector<Token>& tokens) {
    // Look ahead to find the next non-whitespace character after the unary
    // minus, to determine if we have a case like: -(digits...) or -(...) (or
    // another unary minus).
    std::size_t lookahead = i + 1;
    // Skip whitespace after the unary minus to find the next non-whitespace
    // character.
    while (lookahead < input_string.size() &&
           std::isspace(static_cast<unsigned char>(input_string[lookahead]))) {
        ++lookahead;
    }

    // If we reach the end of the string after skipping whitespace, then we
    // have a unary minus with no operand error.
    if (lookahead >= input_string.size()) {
        throw ASTException("missing operand after unary minus");
    }

    // Case: -(digits...)  -> Number(-digits...)
    if (std::isdigit(static_cast<unsigned char>(input_string[lookahead]))) {
        i = lookahead;
        const int64_t parsed_number = parse_negative_number(input_string, i);
        tokens.emplace_back(TokenType::Number, parsed_number, "");
        return;
    }

    // Case: -(...) (or another unary expression)
    // Rewrite as: -1 * (...)
    if (!std::islower(static_cast<unsigned char>(input_string[lookahead])) &&
        input_string[lookahead] != '(' && input_string[lookahead] != '-') {
        throw ASTException("missing operand after unary minus");
    }
    tokens.emplace_back(TokenType::Number, -1, "");
    tokens.emplace_back(TokenType::Mult, 0, "");
    ++i;
}

/**
 * @brief Handles parsing an operand (number, variable, or left paren) when
 * an operand is expected.
 * @param input_string The input string being tokenized.
 * @param i The current index in the input string.
 * @param tokens The vector of tokens to add to.
 * @return true if an operand was successfully parsed, false otherwise.
 */
bool is_operand_valid(const std::string& input_string, std::size_t& i,
                      std::vector<Token>& tokens) {
    const auto curr_char = static_cast<unsigned char>(input_string[i]);

    // Check if we have a number, variable, or left paren, and handle
    // accordingly.
    if (std::isdigit(curr_char)) {
        const int64_t parsed_number = parse_number(input_string, i);
        tokens.emplace_back(TokenType::Number, parsed_number, "");
        return true;
    }

    if (std::islower(curr_char)) {
        std::string parsed_variable = parse_variable_name(input_string, i);
        tokens.emplace_back(TokenType::Variable, 0, std::move(parsed_variable));
        return true;
    }

    if (input_string[i] == '(') {
        tokens.emplace_back(TokenType::LParen, 0, "");
        ++i;
        return true;
    }

    return false;
}

/**
 * @brief Validates and handles operators and closing parenthesis when an
 * operand is not expected.
 * @param input_string The input string being tokenized.
 * @param i The current index in the input string.
 * @param tokens The vector of tokens to add to.
 * @return true if a valid operator or closing paren was found, false otherwise.
 */
bool handle_operator_or_close_paren(const std::string& input_string,
                                    std::size_t& i,
                                    std::vector<Token>& tokens) {
    if (input_string[i] == '+') {
        tokens.emplace_back(TokenType::Plus, 0, "");
        ++i;
        return true;
    }
    if (input_string[i] == '-') {
        tokens.emplace_back(TokenType::Minus, 0, "");
        ++i;
        return true;
    }
    if (input_string[i] == '*') {
        tokens.emplace_back(TokenType::Mult, 0, "");
        ++i;
        return true;
    }
    if (input_string[i] == '/') {
        tokens.emplace_back(TokenType::Div, 0, "");
        ++i;
        return true;
    }
    if (input_string[i] == ')') {
        tokens.emplace_back(TokenType::RParen, 0, "");
        ++i;
        return true;
    }

    return false;
}

/**
 * @brief Validates a character when an operand is expected but not found.
 * @param input_string The input string being tokenized.
 * @param i The current index in the input string.
 */
[[noreturn]] void validate_expected_operand(const std::string& input_string,
                                            std::size_t const& i) {
    if (input_string[i] == ')') {
        throw ASTException("missing operand before ')'");
    }

    if (input_string[i] == '+' || input_string[i] == '*' ||
        input_string[i] == '/') {
        throw ASTException("missing operand");
    }

    throw ASTException("invalid character in expression");
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

/**
 * @brief Recursively evaluates the value of the AST rooted at this node.
 * @return The result of evaluating the AST rooted at this node.
 */
int64_t Node::get_value() {
    if (type == NodeType::Number) {
        return value;
    }
    if (type == NodeType::Variable) {
        throw ASTException("cannot evaluate variable without bindings");
    }

    if (!left || !right) {
        throw ASTException("malformed AST");
    }

    if (type == NodeType::Add) {
        return checked_add(left->get_value(), right->get_value());
    }
    if (type == NodeType::Sub) {
        return checked_sub(left->get_value(), right->get_value());
    }
    if (type == NodeType::Mult) {
        return checked_mul(left->get_value(), right->get_value());
    }
    if (type == NodeType::Div) {
        return checked_div(left->get_value(), right->get_value());
    }

    throw ASTException("malformed AST");
}

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

    std::size_t i = 0;
    bool is_awaiting_operand = true;
    bool saw_non_whitespace = false;

    // Go through the characters of the string.
    while (i < input_string.size()) {
        const auto curr_char = static_cast<unsigned char>(input_string[i]);

        // Ignore whitespace.
        if (std::isspace(curr_char)) {
            ++i;
            continue;
        }

        saw_non_whitespace = true;

        // Handle unary minus.
        if (input_string[i] == '-' && is_awaiting_operand) {
            handle_unary_minus(input_string, i, tokens_);
            // Unary minus can emit:
            // 1) Number(-x)         -> next token must be an operator.
            // 2) Number(-1), Mult   -> next token must be an operand.
            is_awaiting_operand = (tokens_.back().type == TokenType::Mult);
            continue;
        }

        // Handle operands when expected.
        if (is_awaiting_operand) {
            if (is_operand_valid(input_string, i, tokens_)) {
                // If we just consumed "(", we are still awaiting an operand.
                is_awaiting_operand =
                    (tokens_.back().type == TokenType::LParen);
                continue;
            }

            validate_expected_operand(input_string, i);
        }

        // Handle operators and closing parenthesis.
        if (handle_operator_or_close_paren(input_string, i, tokens_)) {
            is_awaiting_operand = (tokens_.back().type != TokenType::RParen);
            continue;
        }

        // Check for missing operator between operands.
        if (std::isdigit(curr_char) || std::islower(curr_char) ||
            input_string[i] == '(') {
            throw ASTException("missing operator between operands");
        }

        throw ASTException("invalid character in expression");
    }

    if (!saw_non_whitespace) {
        throw ASTException("empty expression");
    }
    if (is_awaiting_operand) {
        throw ASTException("expression ends with operator");
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
