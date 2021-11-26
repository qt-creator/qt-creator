/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PARSER_H
#define PARSER_H

#include "symbols.h"

#include <stack>

QT_BEGIN_NAMESPACE

struct MocParseException {};

class Parser
{
public:
    Parser():index(0), displayWarnings(true), displayNotes(true) {}
    Symbols symbols;
    int index;
    bool displayWarnings;
    bool displayNotes;

    struct IncludePath
    {
        inline explicit IncludePath(const QByteArray &_path)
            : path(_path), isFrameworkPath(false) {}
        QByteArray path;
        bool isFrameworkPath;
    };
    QList<IncludePath> includes;

    std::stack<QByteArray, QByteArrayList> currentFilenames;

    inline bool hasNext() const { return (index < symbols.size()); }
    inline Token next() { if (index >= symbols.size()) return NOTOKEN; return symbols.at(index++).token; }
    inline Token peek() { if (index >= symbols.size()) return NOTOKEN; return symbols.at(index).token; }
    bool test(Token);
    void next(Token);
    void next(Token, const char *msg);
    inline void prev() {--index;}
    inline Token lookup(int k = 1);
    inline const Symbol &symbol_lookup(int k = 1) { return symbols.at(index-1+k);}
    inline Token token() { return symbols.at(index-1).token;}
    inline QByteArray lexem() { return symbols.at(index-1).lexem();}
    inline QByteArray unquotedLexem() { return symbols.at(index-1).unquotedLexem();}
    inline const Symbol &symbol() { return symbols.at(index-1);}

    Q_NORETURN void error(int rollback);
    Q_NORETURN void error(const char *msg = nullptr);
    void warning(const char * = nullptr);
    void note(const char * = nullptr);

};

inline bool Parser::test(Token token)
{
    if (index < symbols.size() && symbols.at(index).token == token) {
        ++index;
        return true;
    }
    return false;
}

inline Token Parser::lookup(int k)
{
    const int l = index - 1 + k;
    return l < symbols.size() ? symbols.at(l).token : NOTOKEN;
}

inline void Parser::next(Token token)
{
    if (!test(token))
        error();
}

inline void Parser::next(Token token, const char *msg)
{
    if (!test(token))
        error(msg);
}

QT_END_NAMESPACE

#endif
