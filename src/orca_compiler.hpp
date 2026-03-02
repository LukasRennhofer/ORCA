/*
    ORCA-Lang Virtual Machine / Compiler
    Open Ridiculous Compilation Architecture

    Copyright (C) 2026 Lukas Rennhofer

    This file is part of the ORCA project.

    ORCA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3
    as published by the Free Software Foundation.

    ORCA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    SPDX-License-Identifier: GPL-3.0
*/

#pragma once

#include "orca_common.hpp"

#include <cctype>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <map>
#include <vector>

namespace orca {

/*
========================
SOURCE LOCATION
========================
*/

struct SourceLocation {
    size_t line = 1;
    size_t column = 1;
};

/*
========================
TOKENS
========================
*/

enum class TokenType : uint8_t {

    // Single
    PLUS,
    MINUS,
    STAR,
    SLASH,
    COLON,
    BANG,
    EQUAL,
    SEMICOLON,
    MACRO,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,

    // Literals
    NUMBER,
    IDENTIFIER,

    // Keywords
    MUT,
    TRUE,
    FALSE,
    AND,
    OR,
    XOR,
    GOTO,
    IF,
    ELSE,
    END,

    DEBUG, // TODO: THIS IS JUST FOR TESTING!

    END_OF_FILE
};

struct Token {

    TokenType type;
    std::string lexeme;
    SourceLocation location;

    std::variant<std::monostate, int64_t> literal;

};

/*
========================
LEXER
========================
*/

class Lexer {

public:

    explicit Lexer(std::string_view src)
        : m_source(src) {}

    std::vector<Token> scan() {

        while (!isAtEnd()) {

            m_start = m_current;
            scanToken();
        }

        addToken(TokenType::END_OF_FILE);

        return m_tokens;
    }

private:

    std::string_view m_source;

    size_t m_start = 0;
    size_t m_current = 0;

    size_t m_line = 1;
    size_t m_column = 1;

    std::vector<Token> m_tokens;

    std::unordered_map<std::string, TokenType> m_keywords{
        {"mut", TokenType::MUT},
        {"true", TokenType::TRUE},
        {"false", TokenType::FALSE},
        {"and", TokenType::AND},
        {"or", TokenType::OR},
        {"xor", TokenType::XOR},
        {"goto", TokenType::GOTO},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"end", TokenType::END},
        {"DEBUG", TokenType::DEBUG}
    };

private:

    bool isAtEnd() const {
        return m_current >= m_source.size();
    }

    char advance() {

        char c = m_source[m_current++];

        if (c == '\n') {
            m_line++;
            m_column = 1;
        } else {
            m_column++;
        }

        return c;
    }

    char peek() const {

        if (isAtEnd())
            return '\0';

        return m_source[m_current];
    }

    char peekNext() const {

        if (m_current + 1 >= m_source.size())
            return '\0';

        return m_source[m_current + 1];
    }

    void addToken(TokenType type) {

        Token t;
        t.type = type;
        
        if (type == TokenType::END_OF_FILE) {
            t.lexeme = "";
        } else {
            t.lexeme = std::string(m_source.substr(m_start, m_current - m_start));
        }
        
        t.location = {m_line, m_column};

        m_tokens.push_back(t);
    }

    void addNumber(int64_t value) {

        Token t;

        t.type = TokenType::NUMBER;
        t.lexeme = std::string(m_source.substr(m_start, m_current - m_start));
        t.location = {m_line, m_column};
        t.literal = value;

        m_tokens.push_back(t);
    }

    void scanToken() {

        char c = advance();

        switch (c) {

            case '(':
                addToken(TokenType::LPAREN);
                return;

            case ')':
                addToken(TokenType::RPAREN);
                return;

            case '{':
                addToken(TokenType::LBRACE);
                return;

            case '}':
                addToken(TokenType::RBRACE);
                return;

            case '+':
                addToken(TokenType::PLUS);
                return;

            case '-':
                addToken(TokenType::MINUS);
                return;

            case '*':
                addToken(TokenType::STAR);
                return;

            case '/':
                // Check for comments
                if (peek() == '/') {
                    singleLineComment();
                    return;
                } else if (peek() == '*') {
                    multiLineComment();
                    return;
                }
                addToken(TokenType::SLASH);
                return;

            case ':':
                addToken(TokenType::COLON);
                return;

            case '=':
                addToken(TokenType::EQUAL);
                return;

            case '!':
                addToken(TokenType::BANG);
                return;

            case ';':
                addToken(TokenType::SEMICOLON);
                return;

            case '#':
                addToken(TokenType::MACRO);
                return;

            case ' ':
            case '\t':
            case '\r':
            case '\n':
                return;

            default:
                break;
        }

        if (std::isdigit(c)) {
            number();
            return;
        }

        if (std::isalpha(c)) {
            identifier();
            return;
        }

        throw std::runtime_error("Unexpected character");
    }

    void number() {

        while (std::isdigit(peek()))
            advance();

        auto text =
            m_source.substr(m_start, m_current - m_start);

        addNumber(std::stoll(std::string(text)));
    }

    void identifier() {

        while (std::isalnum(peek()))
            advance();

        auto text =
            std::string(m_source.substr(m_start,
                                        m_current - m_start));

        auto it = m_keywords.find(text);

        if (it != m_keywords.end())
            addToken(it->second);
        else
            addToken(TokenType::IDENTIFIER);
    }

    void singleLineComment() {
        advance();
        
        while (peek() != '\n' && !isAtEnd())
            advance();
    }

    void multiLineComment() {
        // Skip the star
        advance();
        
        while (!isAtEnd()) {
            if (peek() == '*' && peekNext() == '/') {
                advance();
                advance();
                return;
            }
            advance();
        }
        
        // If we reach here, we hit EOF without closing comment
        throw std::runtime_error("Unterminated multi-line comment");
    }
};



/*
========================
AST
========================
*/

namespace ast {

struct Node {
    virtual ~Node() = default;
};

struct Expression : Node {};

struct Literal : Expression {
    int64_t value;
};

struct Variable : Expression {
    std::string name;
};

struct Unary : Expression {

    Token op;
    std::unique_ptr<Expression> right;
};

struct Binary : Expression {

    std::unique_ptr<Expression> left;
    Token op;
    std::unique_ptr<Expression> right;
};


struct Statement : Node {};

struct VarDecl : Statement {

    bool mutableFlag;

    std::string name;
    std::string typeName;

    std::unique_ptr<Expression> initializer;
};

// TODO: This is just for testing!!
struct DebugStmt : Statement {
};

struct GotoStmt: Statement {
    uint64_t line;
};

struct Assignment : Statement {

    std::string name;

    std::unique_ptr<Expression> value;
};

struct ExprStmt : Statement {

    std::unique_ptr<Expression> expr;
};

struct BlockStmt : Statement {
    std::vector<std::unique_ptr<Statement>> statements;
};

struct IfStmt : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;
};

}


/*
========================
PARSER
========================
*/

class Parser {

public:

    explicit Parser(const std::vector<Token>& tokens)
        : m_tokens(tokens) {}

    std::vector<std::unique_ptr<ast::Statement>> parse() {

        std::vector<std::unique_ptr<ast::Statement>> stmts;

        while (!isAtEnd()) {
            stmts.push_back(statement());
        }

        return stmts;
    }

private:

    const std::vector<Token>& m_tokens;

    size_t m_current = 0;


/*
-------------------
STATEMENTS
-------------------
*/

    std::unique_ptr<ast::Statement> statement() {

        if (match(TokenType::LBRACE))
            return blockStatement();

        if (match(TokenType::IF))
            return ifStatement();

        if (match(TokenType::MUT))
            return varDecl(true);

        // TODO: Move away, just for testing now
        if (match(TokenType::DEBUG)) {
            consume(TokenType::SEMICOLON, "Expected ';' after DEBUG");
            return std::make_unique<ast::DebugStmt>();
        }

        if (match(TokenType::GOTO)) {
            consume(TokenType::NUMBER, "Expected line number after 'goto' keyword");

            auto g = std::make_unique<ast::GotoStmt>();
            g->line = std::get<int64_t>(previous().literal);

            consume(TokenType::SEMICOLON,
                    "Expected ';'");

            return g;
        }

        // Check for immutable declaration: name : type = value
        if (check(TokenType::IDENTIFIER) &&
            checkNext(TokenType::COLON))
            return varDecl(false);

        // Check for assignment: name = value
        if (check(TokenType::IDENTIFIER) &&
            checkNext(TokenType::EQUAL))
            return assignment();

        return exprStmt();
    }


    std::unique_ptr<ast::Statement> ifStatement() {

        auto i = std::make_unique<ast::IfStmt>();
        i->condition = expression();

        bool endDelimited = false;

        if (match(TokenType::LBRACE)) {
            i->thenBranch = blockStatement();
        } else {
            endDelimited = true;
            i->thenBranch = blockUntilElseOrEnd();
        }

        if (match(TokenType::ELSE)) {
            // Check for else if
            if (match(TokenType::IF)) {
                i->elseBranch = ifStatement();
            } else if (match(TokenType::LBRACE)) {
                i->elseBranch = blockStatement();
            } else {
                endDelimited = true;
                i->elseBranch = blockUntilEnd();
            }
        }

        if (endDelimited) {
            consume(TokenType::END,
                    "Expected 'end' after if statement");
        }

        return i;
    }


    std::unique_ptr<ast::Statement> blockStatement() {

        auto b = std::make_unique<ast::BlockStmt>();

        while (!check(TokenType::RBRACE) &&
               !isAtEnd()) {
            b->statements.push_back(statement());
        }

        consume(TokenType::RBRACE,
                "Expected '}' after block");

        return b;
    }


    std::unique_ptr<ast::Statement> blockUntilElseOrEnd() {

        auto b = std::make_unique<ast::BlockStmt>();

        while (!check(TokenType::ELSE) &&
               !check(TokenType::END) &&
               !isAtEnd()) {
            b->statements.push_back(statement());
        }

        return b;
    }


    std::unique_ptr<ast::Statement> blockUntilEnd() {

        auto b = std::make_unique<ast::BlockStmt>();

        while (!check(TokenType::END) &&
               !isAtEnd()) {
            b->statements.push_back(statement());
        }

        return b;
    }


    std::unique_ptr<ast::Statement> varDecl(bool isMutable) {

        auto name =
            consume(TokenType::IDENTIFIER,
                    "Expected variable name");

        consume(TokenType::COLON,
                "Expected ':'");

        auto type =
            consume(TokenType::IDENTIFIER,
                    "Expected type name");

        // TODO: Optional guaranteed-valid marker like int!
        if (match(TokenType::BANG)) {
            // consumed
        }

        consume(TokenType::EQUAL,
                "Expected '='");

        auto init = expression();

        consume(TokenType::SEMICOLON,
                "Expected ';'");

        auto v = std::make_unique<ast::VarDecl>();

        v->mutableFlag = isMutable;
        v->name = name.lexeme;
        v->typeName = type.lexeme;
        v->initializer = std::move(init);

        return v;
    }


    std::unique_ptr<ast::Statement> assignment() {

        auto name =
            consume(TokenType::IDENTIFIER,
                    "Expected name");

        consume(TokenType::EQUAL,
                "Expected '='");

        auto val = expression();

        consume(TokenType::SEMICOLON,
                "Expected ';'");

        auto a = std::make_unique<ast::Assignment>();

        a->name = name.lexeme;
        a->value = std::move(val);

        return a;
    }


    std::unique_ptr<ast::Statement> exprStmt() {

        auto e = expression();

        consume(TokenType::SEMICOLON,
                "Expected ';'");

        auto s = std::make_unique<ast::ExprStmt>();

        s->expr = std::move(e);

        return s;
    }


/*
-------------------
EXPRESSIONS
PRATT PARSER
-------------------
*/

    std::unique_ptr<ast::Expression> expression() {
        return logicalXor();
    }


    std::unique_ptr<ast::Expression> addition() {

        auto expr = multiplication();

        while (match(TokenType::PLUS) || match(TokenType::MINUS)) {

            Token op = previous();

            auto right = multiplication();

            auto b = std::make_unique<ast::Binary>();

            b->left = std::move(expr);
            b->op = op;
            b->right = std::move(right);

            expr = std::move(b);
        }

        return expr;
    }

    std::unique_ptr<ast::Expression> logicalXor() {
        auto expr = logicalOr();

        while (match(TokenType::XOR)) {

            Token op = previous();

            auto right = logicalOr();

            auto b = std::make_unique<ast::Binary>();

            b->left = std::move(expr);
            b->op = op;
            b->right = std::move(right);

            expr = std::move(b);
        }

        return expr;
    }

    std::unique_ptr<ast::Expression> logicalOr() {
        auto expr = logicalAnd();

        while (match(TokenType::OR)) {

            Token op = previous();

            auto right = logicalAnd();

            auto b = std::make_unique<ast::Binary>();

            b->left = std::move(expr);
            b->op = op;
            b->right = std::move(right);

            expr = std::move(b);
        }

        return expr;
    }

    std::unique_ptr<ast::Expression> logicalAnd() {

        auto expr = addition();

        while (match(TokenType::AND)) {

            Token op = previous();

            auto right = addition();

            auto b = std::make_unique<ast::Binary>();

            b->left = std::move(expr);
            b->op = op;
            b->right = std::move(right);

            expr = std::move(b);
        }

        return expr;
    }


    std::unique_ptr<ast::Expression> multiplication() {

        auto expr = unary();

        while (match(TokenType::STAR) || match(TokenType::SLASH)) {

            Token op = previous();

            auto right = unary();

            auto b = std::make_unique<ast::Binary>();

            b->left = std::move(expr);
            b->op = op;
            b->right = std::move(right);

            expr = std::move(b);
        }

        return expr;
    }


    std::unique_ptr<ast::Expression> unary() {

        if (match(TokenType::BANG)) {

            Token op = previous();

            auto r = unary();

            auto u = std::make_unique<ast::Unary>();

            u->op = op;
            u->right = std::move(r);

            return u;
        }

        return primary();
    }


    std::unique_ptr<ast::Expression> primary() {

        if (match(TokenType::NUMBER)) {

            auto l =
                std::make_unique<ast::Literal>();

            l->value =
                std::get<int64_t>(
                    previous().literal);

            return l;
        }

        if (match(TokenType::TRUE)) {

            auto l =
                std::make_unique<ast::Literal>();

            l->value = 1;  // true = 1

            return l;
        }

        if (match(TokenType::FALSE)) {

            auto l =
                std::make_unique<ast::Literal>();

            l->value = 0;  // false = 0

            return l;
        }

        if (match(TokenType::IDENTIFIER)) {

            auto v =
                std::make_unique<ast::Variable>();

            v->name =
                previous().lexeme;

            return v;
        }

        if (match(TokenType::LPAREN)) {

            auto e = expression();

            consume(TokenType::RPAREN,
                    "Expected ')'");

            return e;
        }

        throw std::runtime_error("Expected expression");
    }


/*
-------------------
HELPERS
-------------------
*/

    bool match(TokenType t) {

        if (!check(t))
            return false;

        advance();
        return true;
    }


    bool check(TokenType t) const {

        if (isAtEnd())
            return false;

        return peek().type == t;
    }


    bool checkNext(TokenType t) const {

        if (m_current + 1 >= m_tokens.size())
            return false;

        return m_tokens[m_current + 1].type == t;
    }


    Token consume(TokenType t,
                  const char* msg) {

        if (check(t))
            return advance();

        throw std::runtime_error(msg);
    }


    Token advance() {

        if (!isAtEnd())
            m_current++;

        return previous();
    }


    const Token& peek() const {
        return m_tokens[m_current];
    }


    Token previous() const {
        return m_tokens[m_current - 1];
    }


    bool isAtEnd() const {
        return peek().type ==
               TokenType::END_OF_FILE;
    }
};

namespace compile {

    class BinaryCompiler {
        public:
            std::vector<uint8_t> compileByteCode(const std::vector<std::unique_ptr<ast::Statement>>& statements);
        
        private:
            struct ScopeMarker {
                size_t symbolRollbackStart;
                uint16_t localStart;
            };

            struct SymbolRollback {
                std::string name;
                bool hadPrevious;
                uint16_t previousSlot;
            };

            struct VariableInfo {
                uint16_t slot;
                std::string typeName;
            };

            std::map<std::string, VariableInfo> m_symbolTable;

            uint16_t m_nextLocal = 0;

            std::vector<uint8_t> m_code;
            std::vector<ScopeMarker> m_scopeStack;
            std::vector<SymbolRollback> m_symbolRollbacks;

            void emitOp(Opcode op);
            void emitU16(uint16_t v);
            void emitInt(int32_t v);
            void emitLong(int64_t v);
            size_t emitJumpPlaceholder(Opcode op);
            void patchJumpTarget(size_t operandPos, uint16_t target);

            void beginScope();
            void endScope(bool emitCleanupOpcode);
            uint16_t declareLocal(const std::string& name, const std::string& typeName);
            BinaryCompiler::VariableInfo resolveLocal(const std::string& name);

            std::string getExpressionType(ast::Expression* expr);

            void compileStatement(ast::Statement* stmt);
            void compileExpression(ast::Expression* expr, const std::string& expectedType = "");
    };

    std::vector<uint8_t> BinaryCompiler::compileByteCode(const std::vector<std::unique_ptr<ast::Statement>>& statements) {
        m_code.clear();
        m_symbolTable.clear();
        m_nextLocal = 0;
        m_scopeStack.clear();
        m_symbolRollbacks.clear();

        // Emit magic number and version at start
        emitInt((int32_t)BYTECODE_MAGIC);
        emitU16(BYTECODE_VERSION);

        beginScope();

        for (auto& stmt : statements)
            compileStatement(stmt.get());

        endScope(false);

        return m_code;
    }

    /*
    ========================
    EMIT
    ========================
    */

    void BinaryCompiler::emitOp(Opcode op)
    {
        m_code.push_back((uint8_t)op);
    }


    void BinaryCompiler::emitU16(uint16_t v)
    {
        m_code.push_back(v & 0xFF);
        m_code.push_back((v >> 8) & 0xFF);
    }


    void BinaryCompiler::emitInt(int32_t v)
    {
        m_code.push_back(v & 0xFF);
        m_code.push_back((v >> 8) & 0xFF);
        m_code.push_back((v >> 16) & 0xFF);
        m_code.push_back((v >> 24) & 0xFF);
    }


    void BinaryCompiler::emitLong(int64_t v)
    {
        m_code.push_back(v & 0xFF);
        m_code.push_back((v >> 8) & 0xFF);
        m_code.push_back((v >> 16) & 0xFF);
        m_code.push_back((v >> 24) & 0xFF);
        m_code.push_back((v >> 32) & 0xFF);
        m_code.push_back((v >> 40) & 0xFF);
        m_code.push_back((v >> 48) & 0xFF);
        m_code.push_back((v >> 56) & 0xFF);
    }


    size_t BinaryCompiler::emitJumpPlaceholder(Opcode op)
    {
        emitOp(op);
        size_t operandPos = m_code.size();
        emitU16(0);
        return operandPos;
    }


    void BinaryCompiler::patchJumpTarget(size_t operandPos,
                                         uint16_t target)
    {
        if (operandPos + 1 >= m_code.size()) {
            throw std::runtime_error("Internal compiler error: invalid jump patch position");
        }

        m_code[operandPos] = target & 0xFF;
        m_code[operandPos + 1] = (target >> 8) & 0xFF;
    }


    void BinaryCompiler::beginScope()
    {
        m_scopeStack.push_back({m_symbolRollbacks.size(), m_nextLocal});
    }


    void BinaryCompiler::endScope(bool emitCleanupOpcode)
    {
        if (m_scopeStack.empty()) {
            throw std::runtime_error("Internal compiler error: scope stack underflow");
        }

        ScopeMarker marker = m_scopeStack.back();
        m_scopeStack.pop_back();

        if (emitCleanupOpcode) {
            emitOp(Opcode::TRUNCVARS);
            emitU16(marker.localStart);
        }

        m_nextLocal = marker.localStart;

        for (size_t i = m_symbolRollbacks.size(); i > marker.symbolRollbackStart; --i) {
            const auto& rollback = m_symbolRollbacks[i - 1];
            if (rollback.hadPrevious) {
                // Restore previous slot, keep existing type from table
                auto it = m_symbolTable.find(rollback.name);
                if (it != m_symbolTable.end()) {
                    it->second.slot = rollback.previousSlot;
                }
            } else {
                m_symbolTable.erase(rollback.name);
            }
        }

        m_symbolRollbacks.resize(marker.symbolRollbackStart);
    }


    uint16_t BinaryCompiler::declareLocal(const std::string& name, const std::string& typeName)
    {
        auto existing = m_symbolTable.find(name);

        SymbolRollback rollback;
        rollback.name = name;
        rollback.hadPrevious = existing != m_symbolTable.end();
        rollback.previousSlot = rollback.hadPrevious ? existing->second.slot : 0;

        m_symbolRollbacks.push_back(rollback);

        uint16_t slot = m_nextLocal++;
        m_symbolTable[name] = {slot, typeName};

        return slot;
    }


    BinaryCompiler::VariableInfo BinaryCompiler::resolveLocal(const std::string& name)
    {
        auto it = m_symbolTable.find(name);
        if (it == m_symbolTable.end()) {
            throw std::runtime_error("Undefined variable: " + name);
        }

        return it->second;
    }


    std::string BinaryCompiler::getExpressionType(ast::Expression* expr)
    {
        if (auto v = dynamic_cast<ast::Variable*>(expr)) {
            return resolveLocal(v->name).typeName;
        }
        
        if (auto l = dynamic_cast<ast::Literal*>(expr)) {
            // Large literals are automatically long
            if (l->value > INT32_MAX || l->value < INT32_MIN) {
                return "long";
            }
            return "int";
        }
        
        if (auto b = dynamic_cast<ast::Binary*>(expr)) {
            // Binary operation type is determined by operands
            // For now, if either operand is long, result is long
            std::string leftType = getExpressionType(b->left.get());
            std::string rightType = getExpressionType(b->right.get());
            
            if (leftType == "long" || rightType == "long") {
                return "long";
            }
            return "int";
        }
        
        return "int"; // Default to int
    }



    /*
    ========================
    STATEMENTS
    ========================
    */

    void BinaryCompiler::compileStatement(ast::Statement* stmt)
    {

        // TODO: Move away, just for testing now
        if(auto v = dynamic_cast<ast::DebugStmt*>(stmt)) {
            emitOp(Opcode::DEBUG);

            return;
        }

        /*
        Var Declaration
        mut a:int = 10+4;
        */

        if(auto v = dynamic_cast<ast::VarDecl*>(stmt))
        {
            compileExpression(v->initializer.get());

            uint16_t slot = declareLocal(v->name, v->typeName);

            // Emit type-specific store opcode
            if (v->typeName == "long") {
                emitOp(Opcode::LSTORE);
            } else {
                // Default to int for now (includes bool)
                emitOp(Opcode::ISTORE);
            }
            emitU16(slot);

            return;
        }


        /*
        Assignment
        a = 5;
        */

        if(auto a = dynamic_cast<ast::Assignment*>(stmt))
        {
            compileExpression(a->value.get());

            VariableInfo varInfo = resolveLocal(a->name);

            // Emit type-specific store opcode
            if (varInfo.typeName == "long") {
                emitOp(Opcode::LSTORE);
            } else {
                emitOp(Opcode::ISTORE);
            }
            emitU16(varInfo.slot);

            return;
        }


        /*
        Expression Statement
        10+3;
        */

        if(auto e = dynamic_cast<ast::ExprStmt*>(stmt))
        {
            compileExpression(e->expr.get());

            emitOp(Opcode::POP);

            return;
        }

        if (auto b = dynamic_cast<ast::BlockStmt*>(stmt))
        {
            beginScope();

            for (auto& child : b->statements) {
                compileStatement(child.get());
            }

            endScope(true);

            return;
        }

        if (auto i = dynamic_cast<ast::IfStmt*>(stmt))
        {
            compileExpression(i->condition.get());

            size_t jumpToElseOperandPos =
                emitJumpPlaceholder(Opcode::JUMP_IF_FALSE);

            compileStatement(i->thenBranch.get());

            if (i->elseBranch) {
                size_t jumpToEndOperandPos =
                    emitJumpPlaceholder(Opcode::JUMP);

                patchJumpTarget(jumpToElseOperandPos,
                                (uint16_t)m_code.size());

                compileStatement(i->elseBranch.get());

                patchJumpTarget(jumpToEndOperandPos,
                                (uint16_t)m_code.size());
            } else {
                patchJumpTarget(jumpToElseOperandPos,
                                (uint16_t)m_code.size());
            }

            return;
        }

        if (auto g = dynamic_cast<ast::GotoStmt*>(stmt))
        {
            emitOp(Opcode::JUMP);
            emitU16((uint16_t)g->line);

            return;
        }

        throw std::runtime_error("Unknown statement");
    }




    /*
    ========================
    EXPRESSIONS
    ========================
    */

    // TODO: For now, just integer Variables and arithmetics work!
    void BinaryCompiler::compileExpression(ast::Expression* expr, const std::string& expectedType)
    {


        /*
        Literal
        10
        */

        if(auto l = dynamic_cast<ast::Literal*>(expr))
        {
            // Check if value exceeds int32 range OR expected type is long
            if (l->value > INT32_MAX || l->value < INT32_MIN || expectedType == "long") {
                emitOp(Opcode::LCONST);
                emitLong(l->value);
            } else {
                emitOp(Opcode::ICONST);
                emitInt((int32_t)l->value);
            }
            return;
        }


        /*
        Variable
        a
        */

        if(auto v = dynamic_cast<ast::Variable*>(expr))
        {
            VariableInfo varInfo = resolveLocal(v->name);

            // Emit type-specific load opcode
            if (varInfo.typeName == "long") {
                emitOp(Opcode::LLOAD);
            } else {
                emitOp(Opcode::ILOAD);
            }
            emitU16(varInfo.slot);

            return;
        }


        /*
        Binary
        a + b
        */

        if(auto b = dynamic_cast<ast::Binary*>(expr))
        {
            // Determine operation type based on operands
            std::string exprType = getExpressionType(expr);
            bool isLong = (exprType == "long");

            // Compile operands with expected type for proper promotion
            compileExpression(b->left.get(), exprType);
            compileExpression(b->right.get(), exprType);

            switch(b->op.type)
            {
                case TokenType::PLUS:
                    emitOp(isLong ? Opcode::LADD : Opcode::IADD);
                    break;

                case TokenType::MINUS:
                    emitOp(isLong ? Opcode::LSUB : Opcode::ISUB);
                    break;

                case TokenType::STAR:
                    emitOp(isLong ? Opcode::LMUL : Opcode::IMUL);
                    break;

                case TokenType::SLASH:
                    emitOp(isLong ? Opcode::LDIV : Opcode::IDIV);
                    break;

                case TokenType::AND:
                    emitOp(Opcode::IAND);
                    break;
                
                case TokenType::OR:
                    emitOp(Opcode::IOR);
                    break;
                
                case TokenType::XOR:
                    emitOp(Opcode::IXOR);
                    break;

                default:
                    throw std::runtime_error("Unknown binary operator");
            }

            return;
        }


        throw std::runtime_error("Unknown expression");
    }

}

}