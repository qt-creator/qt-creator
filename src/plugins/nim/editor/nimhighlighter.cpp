// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimhighlighter.h"

#include "../tools/nimlexer.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>
#include <utils/qtcassert.h>

namespace Nim {

NimHighlighter::NimHighlighter()
{
    setDefaultTextFormatCategories();
}

void NimHighlighter::highlightBlock(const QString &text)
{
    setCurrentBlockState(highlightLine(text, previousBlockState()));
}

TextEditor::TextStyle NimHighlighter::styleForToken(const NimLexer::Token &token,
                                                    const QString &tokenValue)
{
    switch (token.type) {
    case NimLexer::TokenType::Keyword:
        return TextEditor::C_KEYWORD;
    case NimLexer::TokenType::Identifier:
        return styleForIdentifier(token, tokenValue);
    case NimLexer::TokenType::Comment:
        return TextEditor::C_COMMENT;
    case NimLexer::TokenType::Documentation:
        return TextEditor::C_DOXYGEN_COMMENT;
    case NimLexer::TokenType::StringLiteral:
        return TextEditor::C_STRING;
    case NimLexer::TokenType::MultiLineStringLiteral:
        return TextEditor::C_STRING;
    case NimLexer::TokenType::Operator:
        return TextEditor::C_OPERATOR;
    case NimLexer::TokenType::Number:
        return TextEditor::C_NUMBER;
    default:
        return TextEditor::C_TEXT;
    }
}

TextEditor::TextStyle NimHighlighter::styleForIdentifier(const NimLexer::Token &token,
                                                         const QString &tokenValue)
{
    Q_UNUSED(token)
    QTC_ASSERT(token.type == NimLexer::TokenType::Identifier, return TextEditor::C_TEXT);

    static QSet<QString> nimBuiltInValues {
        "true", "false"
    };

    static QSet<QString> nimBuiltInFunctions {
        "echo", "isMainModule"
    };

    static QSet<QString> nimBuiltInTypes {
        "bool", "cbool", "string",
        "cstring", "int", "cint",
        "uint", "cuint", "long",
        "clong", "double", "cdouble",
        "table", "RootObj"
    };

    if (nimBuiltInFunctions.contains(tokenValue))
        return TextEditor::C_TYPE;
    if (nimBuiltInValues.contains(tokenValue))
        return TextEditor::C_KEYWORD;
    if (nimBuiltInTypes.contains(tokenValue))
        return TextEditor::C_TYPE;
    return TextEditor::C_TEXT;
}

int NimHighlighter::highlightLine(const QString &text, int initialState)
{
    NimLexer lexer(text.constData(),
                   text.size(),
                   static_cast<NimLexer::State>(initialState));

    NimLexer::Token tk;
    while ((tk = lexer.next()).type != NimLexer::TokenType::EndOfText) {
        TextEditor::TextStyle style = styleForToken(tk, text.mid(tk.begin, tk.length));
        setFormat(tk.begin, tk.length, formatForCategory(style));
    }

    return lexer.state();
}

} // Nim
