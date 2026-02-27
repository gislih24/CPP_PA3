#include "AST.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>

// All helper functions are kept in a separate anonymous namespace to avoid
// cluttering the AST class with implementation details.
// MARK: namespace
namespace {

// Usage of these functions will be defined by build/eval modes.
void write_pre(const Node* current_node, std::ostream& output_stream);
int64_t
eval_pre(std::istream& input_stream,
         const std::unordered_map<std::string, int64_t>& variable_values);
std::unordered_map<std::string, int64_t>
parse_variable_values_file(std::istream& input_stream);
bool is_variable_token(const std::string& token);
int64_t parse_int64_token(const std::string& token);

/**
 * @brief Read an entire input stream into a std::string.
 *
 * This is used for expression input file reading input_stream build mode. The
 * iterators consume the entire stream until EOF and construct a single string
 * with the content.
 *
 * @param input_stream Input stream currently positioned at the first character
 * to read. Consumes the entire stream content and leaves the stream positioned
 * at EOF.
 * @return The full content of the input stream as a single std::string.
 */
std::string read_all(std::istream& input_stream) {
    return {std::istreambuf_iterator<char>(input_stream),
            std::istreambuf_iterator<char>()};
}

/**
 * @brief Build mode:
 *   1. Read an expression from the input file.
 *   2. Parse the expression into an in-memory AST using the AST class.
 *   3. Write the AST in a compact preorder format to the output file.
 *
 * CLI contract:
 *     <program> build <ast_output_file> <expression_input_file>
 *
 * @param argc Argument count from main context. Expected value:
 * - 4 => argv = [program, "build", ast_output_file, expression_input_file]
 * @param argv Argument vector from main context.
 * - argv[0]: The executable name.
 * - argv[1]: The mode string (in this case: "build").
 * - argv[2]: The AST output file path
 * - argv[3]: The expression input file path containing the infix expression to
 *   parse.
 * @return Exit code (0 on success, non-zero on error).
 */
int run_build_mode(int argc, char* argv[]) {
    // Require explicit input/output files.
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " build <ast_output_file> <expression_input_file>\n";
        return 1;
    }

    // Read the expression text from the input file.
    std::ifstream expression_file(argv[3]);
    // The expression string to hold the full content of the input file.
    std::string expression;

    if (expression_file) {
        expression = read_all(expression_file);
    } else {
        // If it's missing, read from stdin:
        std::cerr << "Warning: could not open expression input file '"
                  << argv[3] << "', reading from stdin...\n";
        expression = read_all(std::cin);
    }

    // Open the target file that will hold the preorder AST.
    std::ofstream ast_output(argv[2]);
    if (!ast_output) {
        std::cerr << "Error: could not open AST output file\n";
        return 1;
    }

    // Parse expression into the in-memory AST, then serialize it in preorder.
    AST ast;
    ast.parse(expression);
    write_pre(ast.root(), ast_output);
    // Trailing newline for cleaner output files, for terminals.
    ast_output << '\n';
    return 0;
}

/**
 * @brief Eval mode:
 *   1. Read a preorder AST stream from the input file.
 *   2. Evaluate the preorder recursively directly from the stream.
 *   3. Print the final numeric result to stdout.
 *
 * CLI contract:
 *     <program> eval <ast_input_file> [variable_values_file]
 *
 * @param argc Argument count from main context. Must be 3 or 4.
 * @param argv Argument vector from main context.
 * - argv[0]: The executable name.
 * - argv[1]: The mode string (in this case: "eval").
 * - argv[2]: The AST input file path containing the preorder token stream to
 *   evaluate.
 * - argv[3]: Optional variable values file path. One assignment per line in
 *   the format "x=7".
 * @return Exit code (0 on success, non-zero on error).
 */
int run_eval_mode(int argc, char* argv[]) {
    // Support:
    //   <program> eval <ast_input_file>
    //   <program> eval <ast_input_file> <variable_values_file>
    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " eval <ast_input_file> [variable_values_file]\n";
        return 1;
    }

    // Open the input file containing the preorder AST token stream.
    std::ifstream ast_input(argv[2]);
    if (!ast_input) {
        std::cerr << "Error: could not open AST input file\n";
        return 1;
    }

    // The map of variable names to their integer values, if provided.
    std::unordered_map<std::string, int64_t> variable_values;
    // If a variable values file is provided, parse it into the variable_values
    // map.
    if (argc == 4) {
        std::ifstream variable_values_input(argv[3]);
        if (!variable_values_input) {
            std::cerr << "Error: could not open variable values file\n";
            return 1;
        }
        variable_values = parse_variable_values_file(variable_values_input);
    }

    // Evaluate the preorder stream directly and print the final result.
    try {
        const int64_t result = eval_pre(ast_input, variable_values);

        // Check for trailing garbage tokens after the full tree is read.
        if (std::string trailing; ast_input >> trailing) {
            throw ASTException("trailing garbage in preorder");
        }

        std::cout << result << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

/**
 * @brief Serialize an AST to a stream in preorder format.
 *
 * Format (space-separated tokens):
 * - Number node  -> <integer>
 * - Operator node-> <operator_symbol> <left-subtree> <right-subtree>
 *   where <operator_symbol> is one of +, -, *
 *
 * Example for 1 + 1:
 *   + 1 1
 *
 * @param current_node Current AST node pointer in recursive context.
 * - Number node => writes the integer token's value.
 * - Operator node => writes the operator token and recurses into left/right
 * @param output_stream Output stream receiving the preorder token stream.
 */
void write_pre(const Node* current_node, std::ostream& output_stream) {
    // Leaf case: output the integer value.
    if (current_node->type == NodeType::Number) {
        output_stream << current_node->value << ' ';
        return;
    }
    if (current_node->type == NodeType::Variable) {
        output_stream << current_node->variable_name << ' ';
        return;
    }

    // Internal node: emit the operator token first (preorder), then recurse
    // into its child nodes.
    char operator_symbol;
    if (current_node->type == NodeType::Add) {
        operator_symbol = '+';
    } else if (current_node->type == NodeType::Sub) {
        operator_symbol = '-';
    } else if (current_node->type == NodeType::Mult) {
        operator_symbol = '*';
    } else {
        // IF it's not one of these, then we have a malformed AST.
        throw ASTException("malformed AST");
    }
    output_stream << operator_symbol << ' ';
    write_pre(current_node->left.get(), output_stream);
    write_pre(current_node->right.get(), output_stream);
}

/**
 * @brief Evaluate a preorder token stream recursively.
 *
 * Reading rules:
 * - If the token is an operator (+, -, *), recursively read/evaluate two
 *   operands.
 * - Otherwise the token is parsed as a signed integer literal.
 *
 * @param input_stream The input stream containing preorder tokens. The
 * function consumes exactly the tokens for one subtree and leaves the stream
 * positioned immediately after that subtree.
 * @return Computed 64-bit integer value of the parsed subtree.
 */
int64_t eval_pre(std::istream& input_stream) {
    std::string parsed_token;
    // Read the next token. If we fail to read, the input is malformed.
    if (!(input_stream >> parsed_token)) {
        throw ASTException("bad preorder");
    }

    // Operator token: recursively evaluate left and right subexpressions.
    if (parsed_token == "+" || parsed_token == "-" || parsed_token == "*") {
        int64_t l = eval_pre(input_stream);
        int64_t r = eval_pre(input_stream);
        if (parsed_token == "+") {
            return l + r;
        }
        if (parsed_token == "-") {
            return l - r;
        }
        return l * r;
    }

    // Number token.
/**
 * @brief Check if a token is a valid variable token, which consists of one or
 * more lower-case letters.
 * @param token The token string to check.
 * @return True if the token is a valid variable token, false otherwise.
 */
bool is_variable_token(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    // Return whether all characters in the token can be parsed as lowercase
    // ASCII.
    return std::ranges::all_of(token, [](char character) {
        const auto curr_char = static_cast<unsigned char>(character);
        return std::islower(curr_char);
    });
}

/**
 * @brief Parse a token as a 64-bit signed integer. Throws an exception if the
 * token is not a valid integer or if it has trailing garbage after the
 * integer.
 * @param token The token string to parse as an integer.
 * @return The parsed integer value.
 */
int64_t parse_int64_token(const std::string& token) {
    try {
        std::size_t parsed_characters = 0;
        // Parse the token as a base-10 integer.
        const int64_t parsed_value = std::stoll(token, &parsed_characters);
        // If the token is not a valid integer, or if it has trailing garbage
        // after the integer, throw an error.
        if (parsed_characters != token.size()) {
            throw ASTException("bad integer token: " + token);
        }
        return parsed_value;
    } catch (const std::exception&) {
        throw ASTException("bad integer token: " + token);
    }
}

/**
 * @brief Parse a variable values file into a map of variable names to their
 * integer values.
 *
 * The variable values file should have one assignment per line in the format
 * "x=7", where the left-hand side is a variable name (lower-case letters only)
 * and the right-hand side is an integer value.
 * @param input_stream The input stream to read the variable assignments from.
 * Should be positioned at the beginning of the first line of the file. The
 * function reads until EOF.
 * @return An unordered_map mapping variable names to their integer values as
 * parsed from the file.
 */
std::unordered_map<std::string, int64_t>
parse_variable_values_file(std::istream& input_stream) {
    auto trim = [](const std::string& text) {
        std::size_t start = 0;
        while (start < text.size()) {
            if (const auto curr_char = static_cast<unsigned char>(text[start]);
                !std::isspace(curr_char)) {
                break;
            }
            ++start;
        }

        std::size_t end = text.size();
        while (end > start) {
            if (const auto curr_char =
                    static_cast<unsigned char>(text[end - 1]);
                !std::isspace(curr_char)) {
                break;
            }
            --end;
        }
        return text.substr(start, end - start);
    };

    std::unordered_map<std::string, int64_t> variable_values;
    std::size_t line_number = 0;
    std::string line;

    while (std::getline(input_stream, line)) {
        ++line_number;
        const std::string trimmed_line = trim(line);
        if (trimmed_line.empty()) {
            continue;
        }

        const std::size_t equal_sign = trimmed_line.find('=');
        if ((equal_sign == std::string::npos) ||
            (trimmed_line.find('=', equal_sign + 1) != std::string::npos)) {
            throw ASTException("invalid variable assignment on line " +
                               std::to_string(line_number));
        }

        const std::string variable_name =
            trim(trimmed_line.substr(0, equal_sign));
        const std::string variable_value_text =
            trim(trimmed_line.substr(equal_sign + 1));

        if (!is_variable_token(variable_name)) {
            throw ASTException("invalid variable name on line " +
                               std::to_string(line_number));
        }

        variable_values[variable_name] = parse_int64_token(variable_value_text);
    }

    return variable_values;
}

} // namespace

// MARK: main()
/**
 * @brief Program entry point with two modes:
 * - build: builds an AST from an infix expression input file and writes it in
 *   preorder to an output file.
 * - eval: evaluates a preorder AST from an input file and prints the result to
 *   stdout.
 *
 * @param argc The number of command-line arguments.
 * @param argv The command-line argument vector.
 * - argv[0]: The executable name.
 * - argv[1]: The mode string (can be "build" or "eval").
 * - The remaining entries: mode-specific parameters documented above.
 * @return Process exit code (0 on success, non-zero on error).
 */
int main(int argc, char* argv[]) {
    try {
        // Require at least 2 arguments (program name + mode).
        if (argc < 2) {
            // Error indicating the correct usage of the program for both
            // modes.
            std::cerr << "Usage:\n"
                      << "  " << argv[0]
                      << " build <ast_output_file> <expression_input_file>\n"
                      << "  " << argv[0] << " eval <ast_input_file>\n";
            return 1;
        }

        // Dispatch to the appropriate mode handler based on the mode argument.
        const std::string mode = argv[1];
        if (mode == "build") {
            return run_build_mode(argc, argv);
        }
        if (mode == "eval") {
            return run_eval_mode(argc, argv);
        }

        // Unknown mode.
        std::cerr << "Error: unknown mode\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
