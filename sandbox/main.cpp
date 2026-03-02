#include "../src/orca_compiler.hpp"
#include "../src/orca_vm.hpp"

#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

const char* tokenTypeToString(orca::TokenType type) {
    switch (type) {
        case orca::TokenType::EQUAL: return "EQUAL";
        case orca::TokenType::COLON: return "COLON";
        case orca::TokenType::BANG: return "BANG";
        case orca::TokenType::SEMICOLON: return "SEMICOLON";
        case orca::TokenType::MACRO: return "MACRO";
        case orca::TokenType::LPAREN: return "LPAREN";
        case orca::TokenType::RPAREN: return "RPAREN";
        case orca::TokenType::LBRACE: return "LBRACE";
        case orca::TokenType::RBRACE: return "RBRACE";
        case orca::TokenType::DEBUG: return "DEBUG";
        case orca::TokenType::PLUS: return "PLUS";
        case orca::TokenType::MINUS: return "MINUS";
        case orca::TokenType::STAR: return "STAR";
        case orca::TokenType::SLASH: return "SLASH";
        case orca::TokenType::NUMBER: return "NUMBER";
        case orca::TokenType::IDENTIFIER: return "IDENTIFIER";
        case orca::TokenType::MUT: return "MUTABLE";
        case orca::TokenType::TRUE: return "TRUE";
        case orca::TokenType::FALSE: return "FALSE";
        case orca::TokenType::AND: return "AND";
        case orca::TokenType::OR: return "OR";
        case orca::TokenType::XOR: return "XOR";
        case orca::TokenType::GOTO: return "GOTO";
        case orca::TokenType::IF: return "IF";
        case orca::TokenType::ELSE: return "ELSE";
        case orca::TokenType::END: return "END";
        case orca::TokenType::END_OF_FILE: return "END_OF_FILE";
    }
    return "UNKNOWN_TOKEN";
}

void printIndent(int level) {
    for (int i = 0; i < level; ++i) {
        std::cout << "  ";
    }
}

void printExpression(const orca::ast::Expression* expr, int indentLevel = 0) {
    if (expr == nullptr) {
        printIndent(indentLevel);
        std::cout << "<null expr>" << std::endl;
        return;
    }

    if (const auto* lit = dynamic_cast<const orca::ast::Literal*>(expr)) {
        printIndent(indentLevel);
        std::cout << "Literal(value=" << lit->value << ")" << std::endl;
        return;
    }

    if (const auto* var = dynamic_cast<const orca::ast::Variable*>(expr)) {
        printIndent(indentLevel);
        std::cout << "Variable(name=" << var->name << ")" << std::endl;
        return;
    }

    if (const auto* unary = dynamic_cast<const orca::ast::Unary*>(expr)) {
        printIndent(indentLevel);
        std::cout << "Unary(op=" << unary->op.lexeme << ")" << std::endl;
        printExpression(unary->right.get(), indentLevel + 1);
        return;
    }

    if (const auto* binary = dynamic_cast<const orca::ast::Binary*>(expr)) {
        printIndent(indentLevel);
        std::cout << "Binary(op=" << binary->op.lexeme << ")" << std::endl;
        printIndent(indentLevel + 1);
        std::cout << "left:" << std::endl;
        printExpression(binary->left.get(), indentLevel + 2);
        printIndent(indentLevel + 1);
        std::cout << "right:" << std::endl;
        printExpression(binary->right.get(), indentLevel + 2);
        return;
    }

    printIndent(indentLevel);
    std::cout << "<unknown expr node>" << std::endl;
}

void printStatement(const orca::ast::Statement* stmt, int indentLevel = 0) {
    if (stmt == nullptr) {
        printIndent(indentLevel);
        std::cout << "<null stmt>" << std::endl;
        return;
    }

    if (const auto* varDecl = dynamic_cast<const orca::ast::VarDecl*>(stmt)) {
        printIndent(indentLevel);
        std::cout << "VarDecl(mutable=" << (varDecl->mutableFlag ? "true" : "false")
                  << ", name=" << varDecl->name
                  << ", type=" << varDecl->typeName << ")" << std::endl;
        printIndent(indentLevel + 1);
        std::cout << "initializer:" << std::endl;
        printExpression(varDecl->initializer.get(), indentLevel + 2);
        return;
    }

    if (const auto* assignment = dynamic_cast<const orca::ast::Assignment*>(stmt)) {
        printIndent(indentLevel);
        std::cout << "Assignment(name=" << assignment->name << ")" << std::endl;
        printIndent(indentLevel + 1);
        std::cout << "value:" << std::endl;
        printExpression(assignment->value.get(), indentLevel + 2);
        return;
    }

    if (const auto* exprStmt = dynamic_cast<const orca::ast::ExprStmt*>(stmt)) {
        printIndent(indentLevel);
        std::cout << "ExprStmt" << std::endl;
        printExpression(exprStmt->expr.get(), indentLevel + 1);
        return;
    }

    if (const auto* debugStmt = dynamic_cast<const orca::ast::DebugStmt*>(stmt)) {
        printIndent(indentLevel);
        std::cout << "DebugStmt" << std::endl;
        return;
    }

    if (const auto* blockStmt = dynamic_cast<const orca::ast::BlockStmt*>(stmt)) {
        printIndent(indentLevel);
        std::cout << "BlockStmt" << std::endl;
        for (const auto& child : blockStmt->statements) {
            printStatement(child.get(), indentLevel + 1);
        }
        return;
    }

    if (const auto* ifStmt = dynamic_cast<const orca::ast::IfStmt*>(stmt)) {
        printIndent(indentLevel);
        std::cout << "IfStmt" << std::endl;
        printIndent(indentLevel + 1);
        std::cout << "condition:" << std::endl;
        printExpression(ifStmt->condition.get(), indentLevel + 2);
        printIndent(indentLevel + 1);
        std::cout << "then:" << std::endl;
        printStatement(ifStmt->thenBranch.get(), indentLevel + 2);
        if (ifStmt->elseBranch) {
            printIndent(indentLevel + 1);
            std::cout << "else:" << std::endl;
            printStatement(ifStmt->elseBranch.get(), indentLevel + 2);
        }
        return;
    }

    printIndent(indentLevel);
    std::cout << "<unknown stmt node>" << std::endl;
}

const char* TEST_ORCA = "mut a: bool = true; DEBUG; mut b: bool = true; mut t: bool = a and b; DEBUG;";

static std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static void appendIConst(std::vector<uint8_t>& bytecode, int32_t value)
{
    bytecode.push_back(static_cast<uint8_t>(orca::Opcode::ICONST));

    uint8_t raw[sizeof(int32_t)];
    std::memcpy(raw, &value, sizeof(int32_t));

    for (size_t i = 0; i < sizeof(int32_t); ++i) {
        bytecode.push_back(raw[i]);
    }
}

static void appendU16(std::vector<uint8_t>& bytecode, uint16_t value)
{
    bytecode.push_back(value & 0xFF);
    bytecode.push_back((value >> 8) & 0xFF);
}

int main(int argc, char* argv[]) {
    std::string source;
    std::string inputFile;
    bool saveBytecode = false;

    if (argc > 1) {
        // Read from file
        inputFile = argv[1];
        try {
            source = readFile(inputFile);
            std::cout << "=== READING FROM FILE: " << inputFile << " ===" << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
            return 1;
        }

        // Check for --save-bytecode flag
        if (argc > 2 && std::string(argv[2]) == "--save-bytecode") {
            saveBytecode = true;
        }
    } else {
        // Use default test
        source = TEST_ORCA;
        std::cout << "=== USING DEFAULT TEST CODE ===" << std::endl;
    }

    std::cout << "Source code:" << std::endl;
    std::cout << source << std::endl << std::endl;

    // Lexing
    orca::Lexer lexer(source);
    auto tokens = lexer.scan();

    std::cout << "Tokens:" << std::endl;
    for (const auto& tk : tokens) {
        std::cout << "  " << tokenTypeToString(tk.type) << " (\"" << tk.lexeme << "\")" << std::endl;
    }

    // Parsing
    std::cout << "\nParsing..." << std::endl;
    try {
        orca::Parser parser(tokens);
        auto statements = parser.parse();

        std::cout << "Parsed " << statements.size() << " statement(s)" << std::endl;
        for (size_t i = 0; i < statements.size(); ++i) {
            std::cout << "\nStatement[" << i << "]:" << std::endl;
            printStatement(statements[i].get(), 1);
        }

        std::cout << "\nCompiling to bytecode..." << std::endl;
        orca::compile::BinaryCompiler compiler;
        auto compiled_bytecode = compiler.compileByteCode(statements);

        std::cout << "Generated " << compiled_bytecode.size() << " bytes of bytecode" << std::endl;
        std::cout << "Bytecode (hex): ";
        for (size_t i = 0; i < compiled_bytecode.size(); ++i) {
            if (i > 0) std::cout << " ";
            printf("%02x", compiled_bytecode[i]);
        }
        std::cout << std::endl;

        // Save bytecode if requested
        if (saveBytecode && !inputFile.empty()) {
            std::string outputFile = inputFile;
            // Replace .orca extension with .vorca
            size_t dotPos = outputFile.rfind(".orca");
            if (dotPos != std::string::npos) {
                outputFile = outputFile.substr(0, dotPos) + ".vorca";
            } else {
                outputFile = outputFile + ".vorca";
            }

            try {
                std::ofstream file(outputFile, std::ios::binary);
                if (!file.is_open()) {
                    throw std::runtime_error("Could not open file for writing: " + outputFile);
                }
                file.write(reinterpret_cast<const char*>(compiled_bytecode.data()), compiled_bytecode.size());
                file.close();
                std::cout << "\nBytecode saved to: " << outputFile << std::endl;
            } catch (const std::exception& ex) {
                std::cerr << "Error saving bytecode: " << ex.what() << std::endl;
            }
        }

        std::cout << "\nExecuting compiled bytecode..." << std::endl;
        orca::VM vm;
        vm.runBinary(compiled_bytecode);
    }
    catch (const std::exception& ex) {
        std::cout << "Compiler/VM error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}