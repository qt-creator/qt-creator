/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef CPLUSPLUS_PP_ENGINE_H
#define CPLUSPLUS_PP_ENGINE_H

#include "PPToken.h"
#include "PreprocessorClient.h"

#include <Lexer.h>
#include <Token.h>
#include <QVector>
#include <QBitArray>
#include <QByteArray>
#include <QPair>

namespace CPlusPlus {

class Environment;

namespace Internal {
class PPToken;
struct TokenBuffer;
struct Value;
}

class CPLUSPLUS_EXPORT Preprocessor
{
    typedef Internal::PPToken PPToken;
    typedef Internal::Value Value;

public:
    Preprocessor(Client *client, Environment *env);

    QByteArray run(const QString &filename, const QString &source);
    QByteArray run(const QString &filename, const QByteArray &source, bool noLines = false, bool markGeneratedTokens = true);

    bool expandMacros() const;
    void setExpandMacros(bool expandMacros);

    bool keepComments() const;
    void setKeepComments(bool keepComments);

private:
    void preprocess(const QString &filename,
                    const QByteArray &source,
                    QByteArray *result, bool noLines, bool markGeneratedTokens, bool inCondition,
                    unsigned offsetRef = 0, unsigned lineRef = 1);

    enum { MAX_LEVEL = 512 };

    enum ExpansionStatus {
        NotExpanding,
        ReadyForExpansion,
        Expanding,
        JustFinishedExpansion
    };

    struct State {
        State();

        void pushTokenBuffer(const PPToken *start, const PPToken *end, const Macro *macro);
        void popTokenBuffer();

        QString m_currentFileName;

        QByteArray m_source;
        Lexer *m_lexer;
        QBitArray m_skipping;
        QBitArray m_trueTest;
        int m_ifLevel;
        Internal::TokenBuffer *m_tokenBuffer;
        unsigned m_tokenBufferDepth;
        bool m_inPreprocessorDirective;

        QByteArray *m_result;
        bool m_markExpandedTokens;

        bool m_noLines;
        bool m_inCondition;

        unsigned m_offsetRef;
        unsigned m_lineRef;

        ExpansionStatus m_expansionStatus;
        QByteArray m_expansionResult;
        QVector<QPair<unsigned, unsigned> > m_expandedTokensInfo;
    };

    void handleDefined(PPToken *tk);
    void pushToken(PPToken *tk);
    void lex(PPToken *tk);
    void skipPreprocesorDirective(PPToken *tk);
    bool handleIdentifier(PPToken *tk);
    bool handleFunctionLikeMacro(PPToken *tk,
                                 const Macro *macro,
                                 QVector<PPToken> &body,
                                 const QVector<QVector<PPToken> > &actuals,
                                 unsigned lineRef);

    bool skipping() const
    { return m_state.m_skipping[m_state.m_ifLevel]; }

    QVector<CPlusPlus::Token> tokenize(const QByteArray &text) const;

    bool collectActualArguments(PPToken *tk, QVector<QVector<PPToken> > *actuals);
    void scanActualArgument(PPToken *tk, QVector<PPToken> *tokens);

    void handlePreprocessorDirective(PPToken *tk);
    void handleIncludeDirective(PPToken *tk);
    void handleDefineDirective(PPToken *tk);
    QByteArray expand(PPToken *tk, PPToken *lastConditionToken = 0);
    const Internal::PPToken evalExpression(PPToken *tk, Value &result);
    void handleIfDirective(PPToken *tk);
    void handleElifDirective(PPToken *tk, const PPToken &poundToken);
    void handleElseDirective(PPToken *tk, const PPToken &poundToken);
    void handleEndIfDirective(PPToken *tk, const PPToken &poundToken);
    void handleIfDefDirective(bool checkUndefined, PPToken *tk);
    void handleUndefDirective(PPToken *tk);

    static bool isQtReservedWord(const ByteArrayRef &name);

    void trackExpansionCycles(PPToken *tk);

    template <class T>
    void writeOutput(const T &t);
    void writeOutput(const ByteArrayRef &ref);
    bool atStartOfOutputLine() const;
    void maybeStartOutputLine();
    void generateOutputLineMarker(unsigned lineno);
    void synchronizeOutputLines(const PPToken &tk, bool forceLine = false);
    void removeTrailingOutputLines();

    const QByteArray *currentOutputBuffer() const;
    QByteArray *currentOutputBuffer();

    void enforceSpacing(const PPToken &tk, bool forceSpacing = false);
    static std::size_t computeDistance(const PPToken &tk, bool forceTillLine = false);

    PPToken generateToken(enum Kind kind,
                          const char *content, int length,
                          unsigned lineno,
                          bool addQuotes,
                          bool addToControl = true);
    PPToken generateConcatenated(const PPToken &leftTk, const PPToken &rightTk);

    void startSkippingBlocks(const PPToken &tk) const;

private:
    Client *m_client;
    Environment *m_env;
    QByteArray m_scratchBuffer;

    bool m_expandMacros;
    bool m_keepComments;

    State m_state;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_PP_ENGINE_H
