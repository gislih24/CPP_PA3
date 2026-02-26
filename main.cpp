#include "AST.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

// All helper functions are kept in an anonymous namespace
namespace {

// usage of these functions will be defined by build/eval modes.
void write_pre(const Node* n, std::ostream& out);
int64_t eval_pre(std::istream& in);

/**
 * @brief Read an entire input stream into a std::string.
 *
 * This is used for expression input file reading in build mode.
 * The iterators consume the stream until EOF.
 *
 * @param in Input stream currently positioned at the first character to read.
 * Consumes until EOF.
 * @return Full stream content as a single string.
 */
std::string read_all(std::istream& in) {
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

/**
 * @brief Build mode:
 *   1. Read an expression from file
 *   2. Parse expression into AST using existing parser
 *   3. Write AST in compact preorder format to output file
 *
 * CLI contract:
 *   <program> build <ast_output_file> <expression_input_file>
 *
 * @param argc Argument count from main context.
 * Expected value:
 * - 4 => argv = [program, "build", ast_output_file, expression_input_file]
 * @param argv Argument vector from main context.
 * - argv[0]: executable name
 * - argv[1]: mode string (in this case: "build")
 * - argv[2]: AST output file path
 * - argv[3]: expression input file path
 * @return Exit code (0 on success, non-zero on error).
 */
int run_build_mode(int argc, char* argv[]) {
    // Require explicit input/output files.
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " build <ast_output_file> <expression_input_file>\n";
        return 1;
    }

    // Read expression text from input file.
    std::ifstream expression_file(argv[3]);
    if (!expression_file) {
        std::cerr << "Error: could not open expression input file\n";
        return 1;
    }
    const std::string expression = read_all(expression_file);

    // Open target file that will hold preorder AST.
    std::ofstream ast_output(argv[2]);
    if (!ast_output) {
        std::cerr << "Error: could not open AST output file\n";
        return 1;
    }

    // Parse expression into in-memory AST, then serialize in preorder.
    AST ast;
    ast.parse(expression);
    write_pre(ast.root(), ast_output);
    // Trailing newline keeps output file terminal-friendly.
    ast_output << '\n';
    return 0;
}

/**
 * @brief Eval mode:
 *   1) Read preorder AST stream from file
 *   2) Evaluate preorder recursively directly from stream
 *   3) Print result
 *
 * CLI contract:
 *   <program> eval <ast_input_file>
 *
 * @param argc Argument count from main context. Must be exactly 3.
 * @param argv Argument vector from main context.
 * - argv[0]: executable name
 * - argv[1]: mode string (must be "eval")
 * - argv[2]: AST input file path
 * @return Exit code (0 on success, non-zero on error).
 */
int run_eval_mode(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " eval <ast_input_file>\n";
        return 1;
    }

    // Open preorder AST input file.
    std::ifstream ast_input(argv[2]);
    if (!ast_input) {
        std::cerr << "Error: could not open AST input file\n";
        return 1;
    }

    // Evaluate preorder expression tree and print final numeric result.
    std::cout << eval_pre(ast_input) << '\n';
    return 0;
}

/**
 * @brief Serialize AST node in preorder.
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
 * - Number node => writes integer token
 * - Operator node => writes operator token and recurses into left/right
 * @param out Output stream receiving preorder tokens.
 */
void write_pre(const Node* n, std::ostream& out) {
    // Leaf case: output the integer value.
    if (n->type == NodeType::Number) { out << n->value << ' '; return; }

    // Internal node: emit operator first (preorder), then children.
    char op = (n->type == NodeType::Add ? '+' : n->type == NodeType::Sub ? '-' : '*');
    out << op << ' ';
    write_pre(n->left.get(), out);
    write_pre(n->right.get(), out);
}

/**
 * @brief Evaluate a preorder token stream recursively.
 *
 * Reading rules:
 * - If token is operator (+, -, *), recursively read/evaluate two operands.
 * - Otherwise token is parsed as a signed integer literal.
 *
 * This allows part 2 to evaluate directly from file content without
 * reconstructing a full Node tree in memory.
 *
 * @param in Input stream containing preorder tokens. The function consumes
 * exactly the tokens for one subtree and leaves the stream positioned
 * immediately after that subtree.
 * @return Computed 64-bit integer value of the parsed subtree.
 */
int64_t eval_pre(std::istream& in) {
    std::string tok;
    if (!(in >> tok)) throw std::runtime_error("bad preorder");

    // Operator token: recursively evaluate left and right subexpressions.
    if (tok == "+" || tok == "-" || tok == "*") {
        int64_t l = eval_pre(in), r = eval_pre(in);
        if (tok == "+") return l + r;
        if (tok == "-") return l - r;
        return l * r;
    }

    // Number token.
    return std::stoll(tok);
}

} // namespace

/**
 * @brief Program entry point with two modes:
 * - build: expression -> preorder AST file
 * - eval:  preorder AST file -> numeric result
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 * - argv[0]: executable name
 * - argv[1]: mode string ("build" or "eval")
 * - remaining entries: mode-specific parameters documented above.
 * @return Process exit code (0 on success, non-zero on error).
 */
int main(int argc, char* argv[]) {
    try {
        // Require at least mode argument.
        if (argc < 2) {
            // error indicating correct execution syntax
            std::cerr << "Usage:\n"
                      << "  " << argv[0]
                      << " build <ast_output_file> <expression_input_file>\n"
                      << "  " << argv[0] << " eval <ast_input_file>\n";
            return 1;
        }

        // Dispatch by mode token.
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
        // Centralized error handling for parsing/IO/format exceptions.
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
