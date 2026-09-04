-- Copyright (C) 2026 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

%decl cmakeparser.h
%impl cmakeparser.cpp
%parser CMakeParserTable
%token_prefix T_

-- Tokens produced by the lexer.
%token IDENTIFIER "identifier"
%token LEFT_PAREN "("
%token RIGHT_PAREN ")"
%token UNQUOTED_ARGUMENT "unquoted argument"
%token QUOTED_ARGUMENT "quoted argument"
%token BRACKET_ARGUMENT "bracket argument"
%token NEWLINE "newline"

-- Tokens the lexer produces but that normally never reach the parser:
-- whitespace and comments are dropped while the token stream is prepared.  A
-- bracket comment that is not followed by a newline is kept and makes the
-- parser fail, the way CMake requires a newline after one.
%token SPACE "space"
%token COMMENT "comment"
%token BRACKET_COMMENT "bracket comment"

-- Tokens for malformed input.  They reach the parser and make it fail.
%token ERROR "bad character"
%token UNTERMINATED_BRACKET "unterminated bracket"
%token UNTERMINATED_STRING "unterminated string"

-- Control flow.  CMake has no reserved words: these are ordinary identifiers
-- that Parser::classifyTokens() promotes when they name a command that opens
-- or closes a block, and demotes again when the block does not balance.
%token IF "if"
%token ELSEIF "elseif"
%token ELSE "else"
%token ENDIF "endif"
%token FOREACH "foreach"
%token ENDFOREACH "endforeach"
%token WHILE "while"
%token ENDWHILE "endwhile"
%token FUNCTION "function"
%token ENDFUNCTION "endfunction"
%token MACRO "macro"
%token ENDMACRO "endmacro"
%token BLOCK "block"
%token ENDBLOCK "endblock"

%start CMakeFile

/:
// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "$header"
#include "cmakeast.h"
#include "cmakeengine.h"
#include "cmakelexer.h"

#include <vector>

namespace CMakeLang {

class CMAKELANG_EXPORT Parser: public $table
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
:/

/.
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
./

/.
void Parser::reduce(int ruleno)
{
switch (ruleno) {
./

-- The element list stays the symbol at the bottom of the stack, where parse()
-- picks it up whether the file was accepted or not.
CMakeFile: ElementList ;
/.
    case $rule_number: {
    } break;
./

ElementList: ;
/.
    case $rule_number: {
        sym(1).element_list = nullptr;
    } break;
./

ElementList: ElementList NEWLINE ;
/.
    case $rule_number: {
    } break;
./

ElementList: ElementList Element NEWLINE ;
/.
    case $rule_number: {
        sym(1).element_list = appendTo(sym(1).element_list, sym(2).element);
    } break;
./

Element: Command ;
/.
    case $rule_number: {
        sym(1).element = sym(1).command;
    } break;
./

Element: IfStatement ;
Element: ForEachStatement ;
Element: WhileStatement ;
Element: FunctionDefinition ;
Element: MacroDefinition ;
Element: BlockStatement ;

Command: IDENTIFIER LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

IfCommand: IF LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

ElseIfCommand: ELSEIF LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

ElseCommand: ELSE LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

EndIfCommand: ENDIF LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

IfStatement: IfCommand NEWLINE ElementList ElseIfClauseList ElseClauseOpt EndIfCommand ;
/.
    case $rule_number: {
        IfAST *node = makeAstNode<IfAST>(sym(1).command, sym(3).element_list,
                                         sym(4).elseif_clause_list, sym(5).else_clause,
                                         sym(6).command);
        setSpan(node, tokenAt(location(1)), sym(6).command->rightParen);
        sym(1).element = node;
    } break;
./

ElseIfClauseList: ;
/.
    case $rule_number: {
        sym(1).elseif_clause_list = nullptr;
    } break;
./

ElseIfClauseList: ElseIfClauseList ElseIfClause ;
/.
    case $rule_number: {
        sym(1).elseif_clause_list = appendTo(sym(1).elseif_clause_list, sym(2).elseif_clause);
    } break;
./

ElseIfClause: ElseIfCommand NEWLINE ElementList ;
/.
    case $rule_number: {
        List<ElementAST *> *elements = sym(3).element_list;
        ElseIfClauseAST *node = makeAstNode<ElseIfClauseAST>(sym(1).command, elements);
        setClauseSpan(node, tokenAt(location(1)), tokenAt(location(2)), elements);
        sym(1).elseif_clause = node;
    } break;
./

ElseClauseOpt: ;
/.
    case $rule_number: {
        sym(1).else_clause = nullptr;
    } break;
./

ElseClauseOpt: ElseCommand NEWLINE ElementList ;
/.
    case $rule_number: {
        List<ElementAST *> *elements = sym(3).element_list;
        ElseClauseAST *node = makeAstNode<ElseClauseAST>(sym(1).command, elements);
        setClauseSpan(node, tokenAt(location(1)), tokenAt(location(2)), elements);
        sym(1).else_clause = node;
    } break;
./

ForEachCommand: FOREACH LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

EndForEachCommand: ENDFOREACH LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

ForEachStatement: ForEachCommand NEWLINE ElementList EndForEachCommand ;
/.
    case $rule_number: {
        ForEachAST *node = makeAstNode<ForEachAST>(sym(1).command, sym(3).element_list,
                                                   sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;
./

WhileCommand: WHILE LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

EndWhileCommand: ENDWHILE LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

WhileStatement: WhileCommand NEWLINE ElementList EndWhileCommand ;
/.
    case $rule_number: {
        WhileAST *node = makeAstNode<WhileAST>(sym(1).command, sym(3).element_list,
                                               sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;
./

FunctionCommand: FUNCTION LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

EndFunctionCommand: ENDFUNCTION LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

FunctionDefinition: FunctionCommand NEWLINE ElementList EndFunctionCommand ;
/.
    case $rule_number: {
        FunctionAST *node = makeAstNode<FunctionAST>(sym(1).command, sym(3).element_list,
                                                     sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;
./

MacroCommand: MACRO LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

EndMacroCommand: ENDMACRO LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

MacroDefinition: MacroCommand NEWLINE ElementList EndMacroCommand ;
/.
    case $rule_number: {
        MacroAST *node = makeAstNode<MacroAST>(sym(1).command, sym(3).element_list,
                                               sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;
./

BlockCommand: BLOCK LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

EndBlockCommand: ENDBLOCK LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        sym(1).command = makeCommand(location(1), location(2), sym(3).argument_list, location(4));
    } break;
./

BlockStatement: BlockCommand NEWLINE ElementList EndBlockCommand ;
/.
    case $rule_number: {
        BlockAST *node = makeAstNode<BlockAST>(sym(1).command, sym(3).element_list,
                                               sym(4).command);
        setSpan(node, tokenAt(location(1)), sym(4).command->rightParen);
        sym(1).element = node;
    } break;
./

ArgumentList: ;
/.
    case $rule_number: {
        sym(1).argument_list = nullptr;
    } break;
./

ArgumentList: ArgumentList Argument ;
/.
    case $rule_number: {
        sym(1).argument_list = appendTo(sym(1).argument_list, sym(2).argument);
    } break;
./

Argument: UNQUOTED_ARGUMENT ;
/.
    case $rule_number: {
        UnquotedArgumentAST *node = makeAstNode<UnquotedArgumentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).argument = node;
    } break;
./

Argument: IDENTIFIER ;
/.
    case $rule_number: {
        UnquotedArgumentAST *node = makeAstNode<UnquotedArgumentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).argument = node;
    } break;
./

Argument: QUOTED_ARGUMENT ;
/.
    case $rule_number: {
        QuotedArgumentAST *node = makeAstNode<QuotedArgumentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).argument = node;
    } break;
./

Argument: BRACKET_ARGUMENT ;
/.
    case $rule_number: {
        BracketArgumentAST *node = makeAstNode<BracketArgumentAST>(tokenAt(location(1)));
        setSpan(node, node->token, node->token);
        sym(1).argument = node;
    } break;
./

Argument: LEFT_PAREN ArgumentList RIGHT_PAREN ;
/.
    case $rule_number: {
        ParenGroupArgumentAST *node = makeAstNode<ParenGroupArgumentAST>(
            tokenAt(location(1)), sym(2).argument_list, tokenAt(location(3)));
        setSpan(node, node->leftParen, node->rightParen);
        sym(1).argument = node;
    } break;
./

/.
    } // switch
}
./
