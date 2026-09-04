
#line 187 "./cmakelang.g"

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeparser.h"

using namespace Qt::Literals::StringLiterals;

using namespace CMakeLang;

Parser::Parser(Engine *engine, const QString &source)
    : _engine(engine)
{
    _stateStack.resize(128);
    _locationStack.resize(128);
    _symStack.resize(128);

    // Measured over this repository's own CMake files, a token covers about
    // four characters of source.
    _tokens.reserve(size_t(source.size()) / 4 + 16);
    _tokens.push_back(Token()); // invalid token at index 0

    Lexer lexer(engine, engine->setSource(source));
    Token tk;
    Token bracketComment;
    bool haveBracketComment = false;
    int parenDepth = 0;
    bool lastWasNewline = true;

    do {
        lexer.yylex(&tk);

        switch (tk.kind) {
        case T_SPACE:
        case T_COMMENT:
            continue;
        case T_BRACKET_COMMENT:
            // CMake wants a newline after a bracket comment that stands where a
            // command may start.  Hold it back and hand it to the parser, which
            // has no rule for it, if a command follows instead.
            if (parenDepth == 0) {
                bracketComment = tk;
                haveBracketComment = true;
            }
            continue;
        case T_LEFT_PAREN:
            ++parenDepth;
            break;
        case T_RIGHT_PAREN:
            if (parenDepth > 0)
                --parenDepth;
            break;
        case T_NEWLINE:
            if (parenDepth > 0)
                continue;
            break;
        case EOF_SYMBOL:
            if (!lastWasNewline) {
                Token newline = tk;
                newline.kind = T_NEWLINE;
                newline.length = 0;
                _tokens.push_back(newline);
            }
            break;
        default:
            break;
        }

        if (haveBracketComment) {
            if (tk.is(T_IDENTIFIER))
                _tokens.push_back(bracketComment);
            haveBracketComment = false;
        }

        lastWasNewline = tk.kind == T_NEWLINE;
        _tokens.push_back(tk);
    } while (tk.isNot(EOF_SYMBOL));

    classifyTokens();
    _index = 1;
}

Parser::~Parser() = default;

static int blockKeywordKind(QStringView name)
{
    struct Keyword
    {
        QLatin1StringView spelling;
        int kind;
    };
    static const Keyword keywords[] = {
        {"if"_L1, CMakeParserTable::T_IF},
        {"else"_L1, CMakeParserTable::T_ELSE},
        {"block"_L1, CMakeParserTable::T_BLOCK},
        {"macro"_L1, CMakeParserTable::T_MACRO},
        {"elseif"_L1, CMakeParserTable::T_ELSEIF},
        {"endif"_L1, CMakeParserTable::T_ENDIF},
        {"while"_L1, CMakeParserTable::T_WHILE},
        {"foreach"_L1, CMakeParserTable::T_FOREACH},
        {"function"_L1, CMakeParserTable::T_FUNCTION},
        {"endblock"_L1, CMakeParserTable::T_ENDBLOCK},
        {"endmacro"_L1, CMakeParserTable::T_ENDMACRO},
        {"endwhile"_L1, CMakeParserTable::T_ENDWHILE},
        {"endforeach"_L1, CMakeParserTable::T_ENDFOREACH},
        {"endfunction"_L1, CMakeParserTable::T_ENDFUNCTION},
    };

    for (const Keyword &keyword : keywords) {
        if (name.size() == keyword.spelling.size()
            && name.compare(keyword.spelling, Qt::CaseInsensitive) == 0) {
            return keyword.kind;
        }
    }
    return CMakeParserTable::T_IDENTIFIER;
}

void Parser::classifyTokens()
{
    // Promote identifiers that name a block command.  A block command is only
    // ever recognized at file level and only when it is directly followed by
    // "(", so "if" stays an identifier wherever CMake would treat it as an
    // argument or as a variable name.
    int parenDepth = 0;
    std::vector<int> promoted;

    for (int i = 1; i < int(_tokens.size()); ++i) {
        Token &token = _tokens[i];
        if (token.kind == T_LEFT_PAREN) {
            ++parenDepth;
            continue;
        }
        if (token.kind == T_RIGHT_PAREN) {
            if (parenDepth > 0)
                --parenDepth;
            continue;
        }
        if (parenDepth != 0 || token.kind != T_IDENTIFIER)
            continue;
        if (i + 1 >= int(_tokens.size()) || _tokens[i + 1].kind != T_LEFT_PAREN)
            continue;

        const int kind = blockKeywordKind(token.spelling);
        if (kind != T_IDENTIFIER) {
            token.kind = kind;
            promoted.push_back(i);
        }
    }

    // Demote every keyword that does not take part in a balanced block, so
    // that a half-written file still parses as a sequence of commands.
    struct Frame
    {
        int openIndex;
        int openKind;
        bool seenElse;
        std::vector<int> clauses;
    };
    std::vector<Frame> stack;

    const auto demote = [this](int index) { _tokens[index].kind = T_IDENTIFIER; };

    const auto close = [&](int index, int openKind) {
        if (!stack.empty() && stack.back().openKind == openKind)
            stack.pop_back();
        else
            demote(index);
    };

    for (const int index : promoted) {
        switch (_tokens[index].kind) {
        case T_IF:
        case T_FOREACH:
        case T_WHILE:
        case T_FUNCTION:
        case T_MACRO:
        case T_BLOCK:
            stack.push_back({index, _tokens[index].kind, false, {}});
            break;
        case T_ELSEIF:
            if (stack.empty() || stack.back().openKind != T_IF || stack.back().seenElse)
                demote(index);
            else
                stack.back().clauses.push_back(index);
            break;
        case T_ELSE:
            if (stack.empty() || stack.back().openKind != T_IF || stack.back().seenElse) {
                demote(index);
            } else {
                stack.back().seenElse = true;
                stack.back().clauses.push_back(index);
            }
            break;
        case T_ENDIF:
            close(index, T_IF);
            break;
        case T_ENDFOREACH:
            close(index, T_FOREACH);
            break;
        case T_ENDWHILE:
            close(index, T_WHILE);
            break;
        case T_ENDFUNCTION:
            close(index, T_FUNCTION);
            break;
        case T_ENDMACRO:
            close(index, T_MACRO);
            break;
        case T_ENDBLOCK:
            close(index, T_BLOCK);
            break;
        default:
            break;
        }
    }

    for (const Frame &frame : stack) {
        demote(frame.openIndex);
        for (const int clause : frame.clauses)
            demote(clause);
    }
}

void Parser::reportSyntaxError()
{
    const Token &token = tokenAt(yyloc);
    const QString message = yytoken == EOF_SYMBOL
        ? QStringLiteral("Unexpected end of file.")
        : QStringLiteral("Parse error.  Unexpected %1 with text \"%2\".")
              .arg(QLatin1String(Lexer::name(yytoken)), token.text());
    _engine->error(token.line, token.column, token.position, token.length, message);
}

// Resume after a syntax error on the line that follows the token the parser
// choked on.  Newlines inside parentheses are not in the token stream, so every
// newline that is left starts an element.
bool Parser::recover()
{
    for (int index = qMax(yyloc, 1); tokenKind(index) != EOF_SYMBOL; ++index) {
        if (tokenKind(index) == T_NEWLINE) {
            _index = index + 1;
            return tokenKind(_index) != EOF_SYMBOL;
        }
    }
    return false;
}

SourceFileAST *Parser::parse()
{
    List<ElementAST *> *elements = nullptr;

    for (;;) {
        const bool accepted = parseElements();

        // The elements of the file are reduced onto the bottom of the symbol
        // stack, so whatever was complete when the parser stopped is there.
        List<ElementAST *> *parsed = _symStack[0].element_list;
        if (accepted && !elements)
            return makeSourceFile(parsed);

        for (List<ElementAST *> *it = parsed ? parsed->finish() : nullptr; it; it = it->next)
            elements = appendTo(elements, it->value);

        if (accepted || !recover())
            break;
    }

    return makeSourceFile(elements);
}

bool Parser::parseElements()
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

#line 504 "./cmakelang.g"

void Parser::reduce(int ruleno)
{
switch (ruleno) {

#line 513 "./cmakelang.g"

    case 0: {
    } break;

#line 519 "./cmakelang.g"

    case 1: {
        sym(1).element_list = nullptr;
    } break;

#line 526 "./cmakelang.g"

    case 2: {
    } break;

#line 532 "./cmakelang.g"

    case 3: {
        sym(1).element_list = appendTo(sym(1).element_list, sym(2).element);
    } break;

#line 539 "./cmakelang.g"

    case 4: {
        sym(1).element = sym(1).command;
    } break;

#line 553 "./cmakelang.g"

    case 11: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 560 "./cmakelang.g"

    case 12: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 567 "./cmakelang.g"

    case 13: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 574 "./cmakelang.g"

    case 14: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 581 "./cmakelang.g"

    case 15: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 588 "./cmakelang.g"

    case 16: {
        IfAST *node = makeAstNode<IfAST>(sym(1).command, sym(3).element_list,
                                         sym(4).elseif_clause_list, sym(5).else_clause,
                                         sym(6).command);
        setSpan(node, tokenAt(location(1)), sym(6).command->rightParen);
        sym(1).element = node;
    } break;

#line 599 "./cmakelang.g"

    case 17: {
        sym(1).elseif_clause_list = nullptr;
    } break;

#line 606 "./cmakelang.g"

    case 18: {
        sym(1).elseif_clause_list = appendTo(sym(1).elseif_clause_list, sym(2).elseif_clause);
    } break;

#line 613 "./cmakelang.g"

    case 19: {
        List<ElementAST *> *elements = sym(3).element_list;
        ElseIfClauseAST *node = makeAstNode<ElseIfClauseAST>(sym(1).command, elements);
        setClauseSpan(node, tokenAt(location(1)), tokenAt(location(2)), elements);
        sym(1).elseif_clause = node;
    } break;

#line 623 "./cmakelang.g"

    case 20: {
        sym(1).else_clause = nullptr;
    } break;

#line 630 "./cmakelang.g"

    case 21: {
        List<ElementAST *> *elements = sym(3).element_list;
        ElseClauseAST *node = makeAstNode<ElseClauseAST>(sym(1).command, elements);
        setClauseSpan(node, tokenAt(location(1)), tokenAt(location(2)), elements);
        sym(1).else_clause = node;
    } break;

#line 640 "./cmakelang.g"

    case 22: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 647 "./cmakelang.g"

    case 23: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 654 "./cmakelang.g"

    case 24: {
        ForEachAST *node = makeAstNode<ForEachAST>(sym(1).command, sym(3).element_list,
                                                   sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;

#line 664 "./cmakelang.g"

    case 25: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 671 "./cmakelang.g"

    case 26: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 678 "./cmakelang.g"

    case 27: {
        WhileAST *node = makeAstNode<WhileAST>(sym(1).command, sym(3).element_list,
                                               sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;

#line 688 "./cmakelang.g"

    case 28: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 695 "./cmakelang.g"

    case 29: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 702 "./cmakelang.g"

    case 30: {
        FunctionAST *node = makeAstNode<FunctionAST>(sym(1).command, sym(3).element_list,
                                                     sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;

#line 712 "./cmakelang.g"

    case 31: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 719 "./cmakelang.g"

    case 32: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 726 "./cmakelang.g"

    case 33: {
        MacroAST *node = makeAstNode<MacroAST>(sym(1).command, sym(3).element_list,
                                               sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;

#line 736 "./cmakelang.g"

    case 34: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 743 "./cmakelang.g"

    case 35: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;

#line 750 "./cmakelang.g"

    case 36: {
        BlockAST *node = makeAstNode<BlockAST>(sym(1).command, sym(3).element_list,
                                               sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;

#line 760 "./cmakelang.g"

    case 37: {
        sym(1).argument_list = nullptr;
    } break;

#line 767 "./cmakelang.g"

    case 38: {
        sym(1).argument_list = appendTo(sym(1).argument_list, sym(2).argument);
    } break;

#line 774 "./cmakelang.g"

    case 39: {
        UnquotedArgumentAST *node = makeAstNode<UnquotedArgumentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).argument = node;
    } break;

#line 783 "./cmakelang.g"

    case 40: {
        UnquotedArgumentAST *node = makeAstNode<UnquotedArgumentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).argument = node;
    } break;

#line 792 "./cmakelang.g"

    case 41: {
        QuotedArgumentAST *node = makeAstNode<QuotedArgumentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).argument = node;
    } break;

#line 801 "./cmakelang.g"

    case 42: {
        BracketArgumentAST *node = makeAstNode<BracketArgumentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).argument = node;
    } break;

#line 810 "./cmakelang.g"

    case 43: {
        ParenGroupArgumentAST *node = makeAstNode<ParenGroupArgumentAST>(
            tokenAt(location(1)), sym(2).argument_list, tokenAt(location(3)));
        setSpan(node, node->leftParen, node->rightParen);
        sym(1).argument = node;
    } break;

#line 819 "./cmakelang.g"

    } // switch
}
