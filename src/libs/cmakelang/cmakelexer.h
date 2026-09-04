// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakelang.h"

#include <QAnyStringView>
#include <QString>

namespace CMakeLang {

class CMAKELANG_EXPORT Token
{
public:
    enum Separation {
        SeparationOkay,
        SeparationWarning,
        SeparationError
    };

    int kind = 0;
    int position = 0;
    int length = 0;
    int line = 1;
    int column = 1;
    int separation = SeparationOkay;

    // The raw source span.  Valid for as long as the Engine that owns the
    // source text lives.
    QStringView spelling;

    // The token text after processing, set for quoted and bracket arguments
    // only.  Every other token spells itself.
    const QString *value = nullptr;

    bool is(int k) const { return k == kind; }
    bool isNot(int k) const { return k != kind; }

    int begin() const { return position; }
    int end() const { return position + length; }

    QString text() const { return value ? *value : spelling.toString(); }
};

class CMAKELANG_EXPORT Lexer
{
public:
    Lexer(Engine *engine, QStringView source);

    Engine *engine() const { return _engine; }

    bool scanComments() const { return _scanComments; }
    void setScanComments(bool scanComments) { _scanComments = scanComments; }

    int yylex(Token *tk);

    static const char *name(int kind);

private:
    int scanQuotedArgument(Token *tk);
    int scanBracketArgument(Token *tk, int openLength);

    int matchBracketOpen(int pos) const;
    int matchIdentifier(int pos) const;
    int matchUnquoted(int pos) const;
    int matchLegacy(int pos) const;
    bool canMatchLegacy(int pos, int unquotedLength) const;
    int matchMakeVar(int pos) const;
    int matchLegacyElement(int pos) const;
    int matchUnquotedChar(int pos) const;
    int matchSpace(int pos) const;

    QChar at(int pos) const { return pos < _size && pos >= 0 ? _source.at(pos) : QChar(u'\0'); }

    void setToken(Token *tk, int kind, int position) const;
    int finish(Token *tk, int kind);

    Engine *_engine = nullptr;
    QStringView _source;
    int _size = 0;
    int _pos = 0;
    int _line = 1;
    int _column = 1;
    int _bracket = 0;
    int _separation = Token::SeparationOkay;
    bool _scanComments = false;
    QString _buffer;
};

} // namespace CMakeLang
