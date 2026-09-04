
#line 51 "./cmakelang.g"

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeparsertable_p.h"
#include "cmakeast.h"
#include "cmakeengine.h"
#include "cmakelexer.h"

#include <vector>

namespace CMakeLang {

class CMAKELANG_EXPORT Parser: public CMakeParserTable
{
public:
    union Value {
        void *ptr;
        AST *ast;
        ElementAST *element;
        List<ElementAST *> *element_list;
        CommandAST *command;
        ArgumentAST *argument;
        List<ArgumentAST *> *argument_list;
        ElseIfClauseAST *elseif_clause;
        List<ElseIfClauseAST *> *elseif_clause_list;
        ElseClauseAST *else_clause;
    };

    Parser(Engine *engine, const QString &source);
    ~Parser();

    // Always returns a node.  Whether the source parsed cleanly is told by the
    // diagnostics of the engine.
    SourceFileAST *parse();

    const std::vector<Token> &tokens() const { return _tokens; }

private:
    void classifyTokens();
    void reportSyntaxError();
    void reduce(int ruleno);
    bool parseElements();
    bool recover();

    // 1-based
    int &location(int n) { return _locationStack[_tos + n - 1]; }
    Value &sym(int n) { return _symStack[_tos + n - 1]; }

    int consumeToken()
    {
        if (_index < int(_tokens.size()))
            return _index++;
        return int(_tokens.size()) - 1;
    }

    const Token &tokenAt(int index) const { return _tokens.at(index); }
    int tokenKind(int index) const { return _tokens.at(index).kind; }

    template <typename T, typename... Args>
    T *makeAstNode(Args &&...args)
    {
        return new (_engine->pool()) T(std::forward<Args>(args)...);
    }

    static void setSpan(AST *node, const Token &first, const Token &last)
    {
        setSpan(node, first, last.end());
    }

    static void setSpan(AST *node, const Token &first, int end)
    {
        node->position = first.position;
        node->length = end - first.position;
        node->line = first.line;
        node->column = first.column;
    }

    // A clause ends with the last element it owns, and with its own newline
    // where it owns none.
    static void setClauseSpan(AST *node, const Token &first, const Token &newline,
                              List<ElementAST *> *elements)
    {
        if (!elements) {
            setSpan(node, first, newline);
            return;
        }
        const AST *last = elements->value;
        setSpan(node, first, last->position + last->length);
    }

    SourceFileAST *makeSourceFile(List<ElementAST *> *elements)
    {
        SourceFileAST *node = makeAstNode<SourceFileAST>(elements);
        node->position = 0;
        node->length = _tokens.back().position;
        node->line = 1;
        node->column = 1;
        return node;
    }

    CommandAST *makeCommand(int nameLoc, int leftParenLoc, List<ArgumentAST *> *arguments,
                            int rightParenLoc)
    {
        const Token &name = tokenAt(nameLoc);
        const Token &rightParen = tokenAt(rightParenLoc);
        CommandAST *node = makeAstNode<CommandAST>(name, tokenAt(leftParenLoc), arguments,
                                                   rightParen);
        setSpan(node, name, rightParen);
        return node;
    }

    template <typename T>
    List<T> *appendTo(List<T> *list, const T &value)
    {
        if (!list)
            return makeAstNode<List<T>>(value);
        return makeAstNode<List<T>>(list, value);
    }

    Engine *_engine;
    int _tos = -1;
    int _index = 0;
    int yyloc = -1;
    int yytoken = -1;
    std::vector<int> _stateStack;
    std::vector<int> _locationStack;
    std::vector<Value> _symStack;
    std::vector<Token> _tokens;
};

} // namespace CMakeLang
