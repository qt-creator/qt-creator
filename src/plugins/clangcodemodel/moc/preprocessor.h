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

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "parser.h"
#include <qlist.h>
#include <qset.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

struct Macro
{
    Macro() : isFunction(false), isVariadic(false) {}
    bool isFunction;
    bool isVariadic;
    Symbols arguments;
    Symbols symbols;
};

#ifdef USE_LEXEM_STORE
typedef QByteArray MacroName;
#else
typedef SubArray MacroName;
#endif
typedef QHash<MacroName, Macro> Macros;

class QFile;

class Preprocessor : public Parser
{
public:
    Preprocessor(){}
    static bool preprocessOnly;
    QList<QByteArray> frameworks;
    QSet<QByteArray> preprocessedIncludes;
    QHash<QByteArray, QByteArray> nonlocalIncludePathResolutionCache;
    Macros macros;
    QByteArray resolveInclude(const QByteArray &filename, const QByteArray &relativeTo);
    Symbols preprocessed(const QByteArray &filename, QByteArray input);

    void parseDefineArguments(Macro *m);

    void skipUntilEndif();
    bool skipBranch();

    void substituteUntilNewline(Symbols &substituted);
    static Symbols macroExpandIdentifier(Preprocessor *that, SymbolStack &symbols, int lineNum, QByteArray *macroName);
    static void macroExpand(Symbols *into, Preprocessor *that, const Symbols &toExpand, int &index, int lineNum, bool one,
                               const QSet<QByteArray> &excludeSymbols = QSet<QByteArray>());

    int evaluateCondition();

    enum TokenizeMode { TokenizeCpp, TokenizePreprocessor, PreparePreprocessorStatement, TokenizePreprocessorStatement, TokenizeInclude, PrepareDefine, TokenizeDefine };
    static Symbols tokenize(const QByteArray &input, int lineNum = 1, TokenizeMode mode = TokenizeCpp);

private:
    void until(Token);

    void preprocess(const QByteArray &filename, Symbols &preprocessed);
};

QT_END_NAMESPACE

#endif // PREPROCESSOR_H
