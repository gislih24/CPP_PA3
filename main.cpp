#include "AST.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

// All helper functions are kept in a separate anonymous namespace to avoid
// cluttering the AST class with implementation details.
// MARK: namespace
namespace {

// Usage of these functions will be defined by build/eval modes.
void write_pre(const Node* n, std::ostream& out);
int64_t eval_pre(std::istream& in);

/**
 * @brief Read an entire input stream into a std::string.
 *
 * This is used for expression input file reading in build mode. The iterators
 * consume the entire stream until EOF and construct a single string with the
 * content.
 *
 * @param in Input stream currently positioned at the first character to read.
 * Consumes the entire stream content and leaves the stream positioned at EOF.
 * @return The full content of the input stream as a single std::string.
 */
std::string read_all(std::istream& in) {
    return {std::istreambuf_iterator<char>(in),
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
    if (!expression_file) {
        std::cerr << "Error: could not open expression input file\n";
        return 1;
    }
    const std::string expression = read_all(expression_file);

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
 *     <program> eval <ast_input_file>
 *
 * @param argc Argument count from main context. Must be exactly 3.
 * @param argv Argument vector from main context.
 * - argv[0]: The executable name.
 * - argv[1]: The mode string (in this case: "eval").
 * - argv[2]: The AST input file path containing the preorder token stream to
 *   evaluate.
 * @return Exit code (0 on success, non-zero on error).
 */
int run_eval_mode(int argc, char* argv[]) {
    // Require 3 arguments total (program name + mode + input file).
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " eval <ast_input_file>\n";
        return 1;
    }

    // Open the input file containing the preorder AST token stream.
    std::ifstream ast_input(argv[2]);
    if (!ast_input) {
        std::cerr << "Error: could not open AST input file\n";
        return 1;
    }

    // Evaluate the preorder stream directly and print the final result.
    try {
        std::cout << eval_pre(ast_input) << '\n';
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
 * - Operator node-> <op> <left-subtree> <right-subtree>
 *   where <op> is one of +, -, *
 *
 * Example for 1 + 1:
 *   + 1 1
 *
 * @param n Current AST node pointer in recursive context.
 * - Number node => writes the integer token's value.
 * - Operator node => writes the operator token and recurses into left/right
 * @param out Output stream receiving the preorder token stream.
 */
void write_pre(const Node* n, std::ostream& out) {
    // Leaf case: output the integer value.
    if (n->type == NodeType::Number) {
        out << n->value << ' ';
        return;
    }

    // Internal node: emit the operator token first (preorder), then recurse
    // into its child nodes.
    char op;
    if (n->type == NodeType::Add) {
        op = '+';
    } else if (n->type == NodeType::Sub) {
        op = '-';
    } else {
        op = '*';
    }
    out << op << ' ';
    write_pre(n->left.get(), out);
    write_pre(n->right.get(), out);
}

/**
 * @brief Evaluate a preorder token stream recursively.
 *
 * Reading rules:
 * - If the token is an operator (+, -, *), recursively read/evaluate two
 *   operands.
 * - Otherwise the token is parsed as a signed integer literal.
 *
 * @param in The input stream containing preorder tokens. The function consumes
 * exactly the tokens for one subtree and leaves the stream positioned
 * immediately after that subtree.
 * @return Computed 64-bit integer value of the parsed subtree.
 */
int64_t eval_pre(std::istream& in) {
    std::string tok;
    // Read the next token. If we fail to read, the input is malformed.
    if (!(in >> tok))
        throw std::runtime_error("bad preorder");

    // Operator token: recursively evaluate left and right subexpressions.
    if (tok == "+" || tok == "-" || tok == "*") {
        int64_t l = eval_pre(in);
        int64_t r = eval_pre(in);
        if (tok == "+")
            return l + r;
        if (tok == "-")
            return l - r;
        return l * r;
    }

    // Number token.
    return std::stoll(tok);
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
