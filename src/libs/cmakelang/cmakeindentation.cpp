// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeindentation.h"

#include "cmakeengine.h"
#include "cmakelexer.h"
#include "cmakeparser.h"

using namespace Qt::Literals::StringLiterals;

using namespace CMakeLang;

namespace {

enum Construct {
    NoConstruct,
    IfConstruct,
    ForEachConstruct,
    WhileConstruct,
    FunctionConstruct,
    MacroConstruct,
    BlockConstruct
};

enum Role {
    PlainCommand,
    OpensConstruct,
    ContinuesConstruct,
    ClosesConstruct
};

class CommandKind
{
public:
    Construct construct = NoConstruct;
    Role role = PlainCommand;
};

CommandKind commandKind(QStringView name)
{
    struct Entry
    {
        QLatin1StringView spelling;
        Construct construct;
        Role role;
    };
    static const Entry entries[] = {
        {"if"_L1, IfConstruct, OpensConstruct},
        {"elseif"_L1, IfConstruct, ContinuesConstruct},
        {"else"_L1, IfConstruct, ContinuesConstruct},
        {"endif"_L1, IfConstruct, ClosesConstruct},
        {"foreach"_L1, ForEachConstruct, OpensConstruct},
        {"endforeach"_L1, ForEachConstruct, ClosesConstruct},
        {"while"_L1, WhileConstruct, OpensConstruct},
        {"endwhile"_L1, WhileConstruct, ClosesConstruct},
        {"function"_L1, FunctionConstruct, OpensConstruct},
        {"endfunction"_L1, FunctionConstruct, ClosesConstruct},
        {"macro"_L1, MacroConstruct, OpensConstruct},
        {"endmacro"_L1, MacroConstruct, ClosesConstruct},
        {"block"_L1, BlockConstruct, OpensConstruct},
        {"endblock"_L1, BlockConstruct, ClosesConstruct},
    };

    for (const Entry &entry : entries) {
        if (name.size() == entry.spelling.size()
            && name.compare(entry.spelling, Qt::CaseInsensitive) == 0) {
            return {entry.construct, entry.role};
        }
    }
    return {};
}

// One open parenthesis.
class Frame
{
public:
    int base = 0;    // Where a line that starts with ")" goes.
    int content = 0; // Where an argument goes.
    QString command;
    CommandKind kind;
    bool keywordList = false;
};

class Scanner
{
public:
    explicit Scanner(const KeywordPredicate &isKeyword)
        : _isKeyword(isKeyword)
    {}

    QList<int> levels(QStringView source);

private:
    int levelFor(const QList<Token> &tokens) const;
    void apply(const QList<Token> &tokens, int level);
    void closeCommand(const CommandKind &kind);
    bool namesKeyword(const Frame &frame, const Token &token) const;

    const KeywordPredicate &_isKeyword;
    QList<Construct> _constructs;
    QList<Frame> _parens;
    QString _command;
};

QList<int> Scanner::levels(QStringView source)
{
    int lineCount = 1;
    for (const QChar c : source) {
        if (c == u'\n')
            ++lineCount;
    }

    QList<QList<Token>> tokensPerLine(lineCount);
    QList<bool> continued(lineCount, false);

    Engine engine;
    Lexer lexer(&engine, source);
    lexer.setScanComments(true);

    Token token;
    while (lexer.yylex(&token) != Parser::EOF_SYMBOL) {
        if (token.is(Parser::T_SPACE) || token.is(Parser::T_NEWLINE))
            continue;
        if (token.line < 1 || token.line > lineCount)
            continue;

        tokensPerLine[token.line - 1].append(token);

        const int last = qMin(token.line + int(token.spelling.count(u'\n')), lineCount);
        for (int line = token.line + 1; line <= last; ++line)
            continued[line - 1] = true;
    }

    QList<int> result;
    result.reserve(lineCount);
    for (int line = 0; line < lineCount; ++line) {
        // A line that carries the tail of a multi-line value still ends the
        // value and may close the call, so it takes part in the walk. Only its
        // own indentation is none of our business.
        const QList<Token> &tokens = tokensPerLine.at(line);
        const int level = levelFor(tokens);
        result.append(continued.at(line) ? Indentation::Keep : level);
        apply(tokens, level);
    }
    return result;
}

int Scanner::levelFor(const QList<Token> &tokens) const
{
    const Token *first = tokens.isEmpty() ? nullptr : &tokens.constFirst();

    if (_parens.isEmpty()) {
        const int level = _constructs.size();
        if (!first || first->isNot(Parser::T_IDENTIFIER) || tokens.size() < 2
            || tokens.at(1).isNot(Parser::T_LEFT_PAREN)) {
            return level;
        }
        const CommandKind kind = commandKind(first->spelling);
        const bool leaves = kind.role == ContinuesConstruct || kind.role == ClosesConstruct;
        if (leaves && !_constructs.isEmpty() && _constructs.constLast() == kind.construct)
            return level - 1;
        return level;
    }

    const Frame &frame = _parens.constLast();
    if (first && first->is(Parser::T_RIGHT_PAREN))
        return frame.base;
    if (first && namesKeyword(frame, *first))
        return frame.content;
    return frame.keywordList ? frame.content + 1 : frame.content;
}

void Scanner::apply(const QList<Token> &tokens, int level)
{
    const int lineFrame = _parens.size() - 1;

    for (const Token &token : tokens) {
        switch (token.kind) {
        case Parser::T_IDENTIFIER:
            if (_parens.isEmpty())
                _command = token.spelling.toString();
            break;
        case Parser::T_LEFT_PAREN: {
            Frame frame;
            if (_parens.isEmpty()) {
                frame.base = level;
                frame.command = _command;
                frame.kind = commandKind(_command);
            } else {
                frame.base = _parens.constLast().content;
                frame.command = _parens.constLast().command;
            }
            frame.content = frame.base + 1;
            _parens.append(frame);
            _command.clear();
            break;
        }
        case Parser::T_RIGHT_PAREN: {
            if (_parens.isEmpty())
                break;
            const CommandKind kind = _parens.takeLast().kind;
            if (_parens.isEmpty())
                closeCommand(kind);
            break;
        }
        default:
            break;
        }
    }

    if (lineFrame < 0 || lineFrame >= _parens.size())
        return;

    QList<const Token *> arguments;
    for (const Token &token : tokens) {
        if (token.isNot(Parser::T_COMMENT) && token.isNot(Parser::T_BRACKET_COMMENT))
            arguments.append(&token);
    }
    if (arguments.isEmpty())
        return;

    Frame &frame = _parens[lineFrame];
    if (namesKeyword(frame, *arguments.constFirst()))
        frame.keywordList = arguments.size() == 1;
}

void Scanner::closeCommand(const CommandKind &kind)
{
    switch (kind.role) {
    case OpensConstruct:
        _constructs.append(kind.construct);
        break;
    case ClosesConstruct:
        if (!_constructs.isEmpty() && _constructs.constLast() == kind.construct)
            _constructs.removeLast();
        break;
    default:
        break;
    }
}

bool Scanner::namesKeyword(const Frame &frame, const Token &token) const
{
    if (!_isKeyword || frame.command.isEmpty())
        return false;
    if (token.isNot(Parser::T_IDENTIFIER) && token.isNot(Parser::T_UNQUOTED_ARGUMENT))
        return false;

    // A command may take an argument that is spelled the way one of its values
    // is documented, so only what looks like a keyword counts as one.
    for (const QChar c : token.spelling) {
        if (!c.isUpper() && !c.isDigit() && c != u'_')
            return false;
    }
    return _isKeyword(frame.command, token.spelling.toString());
}

} // namespace

Indentation::Indentation(QStringView source, const KeywordPredicate &isKeyword)
{
    Scanner scanner(isKeyword);
    _levels = scanner.levels(source);
}

int Indentation::levelAt(int line) const
{
    if (line < 1 || line > _levels.size())
        return 0;
    return _levels.at(line - 1);
}
