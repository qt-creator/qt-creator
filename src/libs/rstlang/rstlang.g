-- Copyright (C) 2026 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

%decl rstparser.h
%impl rstparser.cpp
%parser RstParserTable
%token_prefix T_

-- One token per line of the document.
%token TEXT "text"
%token VERBATIM "verbatim text"
%token TITLE "section title"
%token TRANSITION "transition"
%token BULLET "bullet"
%token ENUMERATOR "enumerator"
%token FIELD_NAME "field name"
%token DIRECTIVE "directive"
%token SUBSTITUTION "substitution definition"
%token TARGET "hyperlink target"
%token COMMENT "comment"
%token LINE "line block line"
%token FOOTNOTE "footnote"
%token TABLE "table"

-- The structure the lexer reads off the indentation and off the underlines.
%token BLANK "blank line"
%token INDENT "indent"
%token DEDENT "dedent"
%token SECTION_START "section start"
%token SECTION_END "section end"

-- Tokens for malformed input.  They reach the parser and make it fail.
%token ERROR "bad markup"

%start Document

/:
// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "$header"
#include "rstast.h"
#include "rstlexer.h"

#include <parsing/engine.h>

#include <vector>

namespace RstLang {

class RSTLANG_EXPORT Parser: public $table
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
:/

/.
// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rstparser.h"

using namespace RstLang;

Parser::Parser(Engine *engine, const QString &source)
    : _engine(engine)
{
    _stateStack.resize(128);
    _locationStack.resize(128);
    _symStack.resize(128);

    // A token stands for a line, and a line of documentation is about forty
    // characters long.
    _tokens.reserve(size_t(source.size()) / 40 + 16);
    _tokens.push_back(Token()); // invalid token at index 0

    Lexer lexer(engine, engine->setSource(source));
    Token tk;
    do {
        lexer.yylex(&tk);
        _tokens.push_back(tk);
    } while (tk.isNot(EOF_SYMBOL));

    _index = 1;
}

Parser::~Parser() = default;

// Only the first error is reported.  What the parser runs into after it has
// started over is what the broken block left open, not another mistake of
// the document.
void Parser::reportSyntaxError()
{
    if (_reported)
        return;
    _reported = true;

    const Token &token = tokenAt(yyloc);
    const QString message = yytoken == EOF_SYMBOL
        ? QStringLiteral("Unexpected end of document.")
        : QStringLiteral("Parse error.  Unexpected %1.").arg(QLatin1String(Lexer::name(yytoken)));
    _engine->error(token.line, token.column, token.position, token.length, message);
}

// Resume at the start of the next block of the outermost level.  Whatever the
// broken block had opened is left behind, so that the parser starts over where
// nothing is open.
bool Parser::recover()
{
    int depth = 0;
    for (int index = qMax(yyloc, 1); tokenKind(index) != EOF_SYMBOL; ++index) {
        switch (tokenKind(index)) {
        case T_INDENT:
        case T_SECTION_START:
            ++depth;
            break;
        case T_DEDENT:
        case T_SECTION_END:
            --depth;
            break;
        case T_BLANK:
            if (depth <= 0) {
                _index = index + 1;
                return tokenKind(_index) != EOF_SYMBOL;
            }
            break;
        default:
            break;
        }
    }
    return false;
}

DocumentAST *Parser::parse()
{
    List<BlockAST *> *blocks = nullptr;

    for (;;) {
        const bool accepted = parseBlocks();

        // The blocks of the document are reduced onto the bottom of the symbol
        // stack, so whatever was complete when the parser stopped is there.
        List<BlockAST *> *parsed = _symStack[0].block_list;
        if (accepted && !blocks)
            return makeDocument(parsed);

        for (List<BlockAST *> *it = parsed ? parsed->finish() : nullptr; it; it = it->next)
            blocks = appendTo(blocks, it->value);

        if (accepted || !recover())
            break;
    }

    return makeDocument(blocks);
}

bool Parser::parseBlocks()
{
    int action = 0;
    yytoken = -1;
    yyloc = -1;
    _tos = -1;
    _symStack[0].ptr = nullptr;

    do {
        if (unsigned(++_tos) == _stateStack.size()) {
            _stateStack.resize(_tos * 2);
            _locationStack.resize(_tos * 2);
            _symStack.resize(_tos * 2);
        }

        _stateStack[_tos] = action;

        if (yytoken == -1 && -TERMINAL_COUNT != action_index[action]) {
            yyloc = consumeToken();
            yytoken = tokenKind(yyloc);
        }

        action = t_action(action, yytoken);
        if (action > 0) {
            if (action == ACCEPT_STATE) {
                --_tos;
                return true;
            }
            _symStack[_tos].ptr = nullptr;
            _locationStack[_tos] = yyloc;
            yytoken = -1;
        } else if (action < 0) {
            const int ruleno = -action - 1;
            const int N = rhs[ruleno];
            _tos -= N;
            reduce(ruleno);
            action = nt_action(_stateStack[_tos], lhs[ruleno] - TERMINAL_COUNT);
        } else {
            reportSyntaxError();
            return false;
        }
    } while (action);

    return false;
}
./

/.
void Parser::reduce(int ruleno)
{
switch (ruleno) {
./

-- The block list stays the symbol at the bottom of the stack, where parse()
-- picks it up whether the document was accepted or not.
Document: BlockList ;
/.
    case $rule_number: {
    } break;
./

BlockList: ;
/.
    case $rule_number: {
        sym(1).block_list = nullptr;
    } break;
./

BlockList: BlockList Block ;
/.
    case $rule_number: {
        if (sym(2).block)
            sym(1).block_list = appendTo(sym(1).block_list, sym(2).block);
    } break;
./

-- A blank line that no block took carries nothing.
Block: BLANK ;
/.
    case $rule_number: {
        sym(1).block = nullptr;
    } break;
./

Block: Paragraph ;
Block: LiteralBlock ;
Block: LineBlock ;
Block: BlockQuote ;
Block: Section ;
Block: DefinitionItem ;
Block: BulletItem ;
Block: EnumeratedItem ;
Block: Field ;
Block: Directive ;
Block: Substitution ;
Block: Target ;
Block: Comment ;
Block: Transition ;
Block: Footnote ;
Block: Table ;

Paragraph: TextLines BLANK ;
/.
    case $rule_number: {
        const int end = endOf(sym(1).line_list);
        ParagraphAST *node = makeAstNode<ParagraphAST>(sym(1).line_list);
        setSpan(node, tokenAt(location(1)), end);
        sym(1).block = node;
    } break;
./

TextLines: TEXT ;
/.
    case $rule_number: {
        sym(1).line_list = appendTo<LineAST *>(nullptr, makeLine(tokenAt(location(1))));
    } break;
./

TextLines: TextLines TEXT ;
/.
    case $rule_number: {
        sym(1).line_list = appendTo(sym(1).line_list, makeLine(tokenAt(location(2))));
    } break;
./

-- The lines of a literal block stand outside the indentation the lexer
-- tracks: the lexer hands them over the way they are written.
LiteralBlock: VerbatimLines BLANK ;
/.
    case $rule_number: {
        const int end = endOf(sym(1).line_list);
        LiteralBlockAST *node = makeAstNode<LiteralBlockAST>(sym(1).line_list);
        setSpan(node, tokenAt(location(1)), end);
        sym(1).block = node;
    } break;
./

VerbatimLines: VERBATIM ;
/.
    case $rule_number: {
        sym(1).line_list = appendTo<LineAST *>(nullptr, makeLine(tokenAt(location(1))));
    } break;
./

VerbatimLines: VerbatimLines VERBATIM ;
/.
    case $rule_number: {
        sym(1).line_list = appendTo(sym(1).line_list, makeLine(tokenAt(location(2))));
    } break;
./

LineBlock: BlockLines BLANK ;
/.
    case $rule_number: {
        const int end = endOf(sym(1).line_list);
        LineBlockAST *node = makeAstNode<LineBlockAST>(sym(1).line_list);
        setSpan(node, tokenAt(location(1)), end);
        sym(1).block = node;
    } break;
./

BlockLines: LINE ;
/.
    case $rule_number: {
        sym(1).line_list = appendTo<LineAST *>(nullptr, makeLine(tokenAt(location(1))));
    } break;
./

BlockLines: BlockLines LINE ;
/.
    case $rule_number: {
        sym(1).line_list = appendTo(sym(1).line_list, makeLine(tokenAt(location(2))));
    } break;
./

BlockQuote: INDENT BlockList DEDENT ;
/.
    case $rule_number: {
        BlockQuoteAST *node = makeAstNode<BlockQuoteAST>(sym(2).block_list);
        setSpan(node, tokenAt(location(1)), tokenAt(location(3)));
        sym(1).block = node;
    } break;
./

-- A block indented under a paragraph with no blank line in between defines
-- the term the paragraph spells out.
DefinitionItem: TextLines INDENT BlockList DEDENT ;
/.
    case $rule_number: {
        DefinitionItemAST *node = makeAstNode<DefinitionItemAST>(sym(1).line_list,
                                                                 sym(3).block_list);
        setSpan(node, tokenAt(location(1)), tokenAt(location(4)));
        sym(1).block = node;
    } break;
./

Section: TITLE SECTION_START BlockList SECTION_END ;
/.
    case $rule_number: {
        const Token &title = tokenAt(location(1));
        SectionAST *node = makeAstNode<SectionAST>(title, sym(3).block_list);
        setSpan(node, title, tokenAt(location(4)));
        sym(1).block = node;
    } break;
./

BulletItem: BULLET Body ;
/.
    case $rule_number: {
        BulletItemAST *node = makeAstNode<BulletItemAST>(tokenAt(location(1)),
                                                         sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;
./

EnumeratedItem: ENUMERATOR Body ;
/.
    case $rule_number: {
        EnumeratedItemAST *node = makeAstNode<EnumeratedItemAST>(tokenAt(location(1)),
                                                                 sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;
./

Field: FIELD_NAME Body ;
/.
    case $rule_number: {
        FieldAST *node = makeAstNode<FieldAST>(tokenAt(location(1)), sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;
./

Directive: DIRECTIVE Body ;
/.
    case $rule_number: {
        DirectiveAST *node = makeAstNode<DirectiveAST>(tokenAt(location(1)),
                                                       sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;
./

Substitution: SUBSTITUTION Body ;
/.
    case $rule_number: {
        SubstitutionAST *node = makeAstNode<SubstitutionAST>(tokenAt(location(1)),
                                                             sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;
./

-- A footnote and a citation are written and read the same way: a label, and
-- what it stands for.
Footnote: FOOTNOTE Body ;
/.
    case $rule_number: {
        FootnoteAST *node = makeAstNode<FootnoteAST>(tokenAt(location(1)),
                                                     sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;
./

-- What a construct owns: the block indented under its own line, or nothing
-- where the blank line that ends it comes first.
Body: BLANK ;
/.
    case $rule_number: {
        sym(1).block_list = nullptr;
    } break;
./

Body: INDENT BlockList DEDENT ;
/.
    case $rule_number: {
        sym(1).block_list = sym(2).block_list;
    } break;
./

Target: TARGET ;
/.
    case $rule_number: {
        TargetAST *node = makeAstNode<TargetAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).block = node;
    } break;
./

Comment: COMMENT ;
/.
    case $rule_number: {
        CommentAST *node = makeAstNode<CommentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).block = node;
    } break;
./

Transition: TRANSITION ;
/.
    case $rule_number: {
        TransitionAST *node = makeAstNode<TransitionAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).block = node;
    } break;
./

-- The lexer hands a whole table over as one token: what stands in its cells is
-- a layout the rules of the table set, not a nesting the grammar could read.
Table: TABLE ;
/.
    case $rule_number: {
        TableAST *node = makeAstNode<TableAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).block = node;
    } break;
./

/.
    } // switch
}
./
