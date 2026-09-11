
#line 172 "./rstlang.g"

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

#line 319 "./rstlang.g"

void Parser::reduce(int ruleno)
{
switch (ruleno) {

#line 328 "./rstlang.g"

    case 0: {
    } break;

#line 334 "./rstlang.g"

    case 1: {
        sym(1).block_list = nullptr;
    } break;

#line 341 "./rstlang.g"

    case 2: {
        if (sym(2).block)
            sym(1).block_list = appendTo(sym(1).block_list, sym(2).block);
    } break;

#line 350 "./rstlang.g"

    case 3: {
        sym(1).block = nullptr;
    } break;

#line 374 "./rstlang.g"

    case 20: {
        const int end = endOf(sym(1).line_list);
        ParagraphAST *node = makeAstNode<ParagraphAST>(sym(1).line_list);
        setSpan(node, tokenAt(location(1)), end);
        sym(1).block = node;
    } break;

#line 384 "./rstlang.g"

    case 21: {
        sym(1).line_list = appendTo<LineAST *>(nullptr, makeLine(tokenAt(location(1))));
    } break;

#line 391 "./rstlang.g"

    case 22: {
        sym(1).line_list = appendTo(sym(1).line_list, makeLine(tokenAt(location(2))));
    } break;

#line 400 "./rstlang.g"

    case 23: {
        const int end = endOf(sym(1).line_list);
        LiteralBlockAST *node = makeAstNode<LiteralBlockAST>(sym(1).line_list);
        setSpan(node, tokenAt(location(1)), end);
        sym(1).block = node;
    } break;

#line 410 "./rstlang.g"

    case 24: {
        sym(1).line_list = appendTo<LineAST *>(nullptr, makeLine(tokenAt(location(1))));
    } break;

#line 417 "./rstlang.g"

    case 25: {
        sym(1).line_list = appendTo(sym(1).line_list, makeLine(tokenAt(location(2))));
    } break;

#line 424 "./rstlang.g"

    case 26: {
        const int end = endOf(sym(1).line_list);
        LineBlockAST *node = makeAstNode<LineBlockAST>(sym(1).line_list);
        setSpan(node, tokenAt(location(1)), end);
        sym(1).block = node;
    } break;

#line 434 "./rstlang.g"

    case 27: {
        sym(1).line_list = appendTo<LineAST *>(nullptr, makeLine(tokenAt(location(1))));
    } break;

#line 441 "./rstlang.g"

    case 28: {
        sym(1).line_list = appendTo(sym(1).line_list, makeLine(tokenAt(location(2))));
    } break;

#line 448 "./rstlang.g"

    case 29: {
        BlockQuoteAST *node = makeAstNode<BlockQuoteAST>(sym(2).block_list);
        setSpan(node, tokenAt(location(1)), tokenAt(location(3)));
        sym(1).block = node;
    } break;

#line 459 "./rstlang.g"

    case 30: {
        DefinitionItemAST *node = makeAstNode<DefinitionItemAST>(sym(1).line_list,
                                                                 sym(3).block_list);
        setSpan(node, tokenAt(location(1)), tokenAt(location(4)));
        sym(1).block = node;
    } break;

#line 469 "./rstlang.g"

    case 31: {
        const Token &title = tokenAt(location(1));
        SectionAST *node = makeAstNode<SectionAST>(title, sym(3).block_list);
        setSpan(node, title, tokenAt(location(4)));
        sym(1).block = node;
    } break;

#line 479 "./rstlang.g"

    case 32: {
        BulletItemAST *node = makeAstNode<BulletItemAST>(tokenAt(location(1)),
                                                         sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;

#line 489 "./rstlang.g"

    case 33: {
        EnumeratedItemAST *node = makeAstNode<EnumeratedItemAST>(tokenAt(location(1)),
                                                                 sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;

#line 499 "./rstlang.g"

    case 34: {
        FieldAST *node = makeAstNode<FieldAST>(tokenAt(location(1)), sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;

#line 508 "./rstlang.g"

    case 35: {
        DirectiveAST *node = makeAstNode<DirectiveAST>(tokenAt(location(1)),
                                                       sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;

#line 518 "./rstlang.g"

    case 36: {
        SubstitutionAST *node = makeAstNode<SubstitutionAST>(tokenAt(location(1)),
                                                             sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;

#line 530 "./rstlang.g"

    case 37: {
        FootnoteAST *node = makeAstNode<FootnoteAST>(tokenAt(location(1)),
                                                     sym(2).block_list);
        setBodySpan(node, location(1), sym(2).block_list);
        sym(1).block = node;
    } break;

#line 542 "./rstlang.g"

    case 38: {
        sym(1).block_list = nullptr;
    } break;

#line 549 "./rstlang.g"

    case 39: {
        sym(1).block_list = sym(2).block_list;
    } break;

#line 556 "./rstlang.g"

    case 40: {
        TargetAST *node = makeAstNode<TargetAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).block = node;
    } break;

#line 565 "./rstlang.g"

    case 41: {
        CommentAST *node = makeAstNode<CommentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).block = node;
    } break;

#line 574 "./rstlang.g"

    case 42: {
        TransitionAST *node = makeAstNode<TransitionAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).block = node;
    } break;

#line 585 "./rstlang.g"

    case 43: {
        TableAST *node = makeAstNode<TableAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).block = node;
    } break;

#line 593 "./rstlang.g"

    } // switch
}
