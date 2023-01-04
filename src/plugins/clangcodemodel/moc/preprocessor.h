// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
