
#line 37 "./rstlang.g"

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "rstparsertable_p.h"
#include "rstast.h"
#include "rstlexer.h"

#include <parsing/engine.h>

#include <vector>

namespace RstLang {

class RSTLANG_EXPORT Parser: public RstParserTable
{
public:
    union Value {
        void *ptr;
        AST *ast;
        BlockAST *block;
        List<BlockAST *> *block_list;
        LineAST *line;
        List<LineAST *> *line_list;
    };

    Parser(Engine *engine, const QString &source);
    ~Parser();

    // Always returns a node.  Whether the source parsed cleanly is told by the
    // diagnostics of the engine.
    DocumentAST *parse();

    const std::vector<Token> &tokens() const { return _tokens; }

private:
    void reportSyntaxError();
    void reduce(int ruleno);
    bool parseBlocks();
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

    LineAST *makeLine(const Token &token)
    {
        LineAST *node = makeAstNode<LineAST>(token);
        setSpan(node, token, token);
        return node;
    }

    static void setSpan(AST *node, const Token &first, const Token &last)
    {
        setSpan(node, first, last.end());
    }

    static void setSpan(AST *node, const Token &first, int end)
    {
        node->position = first.position;
        node->length = qMax(0, end - first.position);
        node->line = first.line;
        node->column = first.column;
    }

    // A construct ends with the last line of the body it owns, and with its
    // own line where it owns none.
    void setBodySpan(AST *node, int firstLoc, List<BlockAST *> *body)
    {
        const Token &first = tokenAt(firstLoc);
        if (!body) {
            setSpan(node, first, first);
            return;
        }
        const AST *last = body->value;
        setSpan(node, first, last->position + last->length);
    }

    // The lines of a list have been closed by the time the rule that holds
    // them reduces, so the last one is the one the list ends at.
    static int endOf(List<LineAST *> *lines)
    {
        const AST *last = lines->value;
        return last->position + last->length;
    }

    DocumentAST *makeDocument(List<BlockAST *> *blocks)
    {
        DocumentAST *node = makeAstNode<DocumentAST>(blocks);
        node->position = 0;
        node->length = _tokens.back().position;
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
    bool _reported = false;
    std::vector<int> _stateStack;
    std::vector<int> _locationStack;
    std::vector<Value> _symStack;
    std::vector<Token> _tokens;
};

} // namespace RstLang
