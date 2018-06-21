/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
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
****************************************************************************/

#include "cppcompletionassistprocessor.h"

#include <cppeditor/cppeditorconstants.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/Token.h>

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

using namespace CPlusPlus;

namespace CppTools {

CppCompletionAssistProcessor::CppCompletionAssistProcessor(int snippetItemOrder)
    : m_positionForProposal(-1)
    , m_preprocessorCompletions(
          QStringList({"define", "error", "include", "line", "pragma", "pragma once",
                       "pragma omp atomic", "pragma omp parallel", "pragma omp for",
                       "pragma omp ordered", "pragma omp parallel for", "pragma omp section",
                       "pragma omp sections", "pragma omp parallel sections", "pragma omp single",
                       "pragma omp master", "pragma omp critical", "pragma omp barrier",
                       "pragma omp flush", "pragma omp threadprivate", "undef", "if", "ifdef",
                       "ifndef", "elif", "else", "endif"}))
    , m_hintProposal(0)
    , m_snippetCollector(QLatin1String(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID),
                         QIcon(QLatin1String(":/texteditor/images/snippet.png")),
                         snippetItemOrder)
{
}

void CppCompletionAssistProcessor::addSnippets()
{
    m_completions.append(m_snippetCollector.collect());
}

static bool isDoxygenTagCompletionCharacter(const QChar &character)
{
    return character == QLatin1Char('\\')
        || character == QLatin1Char('@') ;
}

void CppCompletionAssistProcessor::startOfOperator(QTextDocument *textDocument,
        int positionInDocument,
        unsigned *kind,
        int &start,
        const CPlusPlus::LanguageFeatures &languageFeatures,
        bool adjustForQt5SignalSlotCompletion,
        DotAtIncludeCompletionHandler dotAtIncludeCompletionHandler)
{
    if (start != positionInDocument) {
        QTextCursor tc(textDocument);
        tc.setPosition(positionInDocument);

        // Include completion: make sure the quote character is the first one on the line
        if (*kind == T_STRING_LITERAL) {
            QTextCursor s = tc;
            s.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            QString sel = s.selectedText();
            if (sel.indexOf(QLatin1Char('"')) < sel.length() - 1) {
                *kind = T_EOF_SYMBOL;
                start = positionInDocument;
            }
        }

        if (*kind == T_COMMA) {
            ExpressionUnderCursor expressionUnderCursor(languageFeatures);
            if (expressionUnderCursor.startOfFunctionCall(tc) == -1) {
                *kind = T_EOF_SYMBOL;
                start = positionInDocument;
            }
        }

        SimpleLexer tokenize;
        tokenize.setLanguageFeatures(languageFeatures);
        tokenize.setSkipComments(false);
        const Tokens &tokens = tokenize(tc.block().text(), BackwardsScanner::previousBlockState(tc.block()));
        const int tokenIdx = SimpleLexer::tokenBefore(tokens, qMax(0, tc.positionInBlock() - 1)); // get the token at the left of the cursor
        const Token tk = (tokenIdx == -1) ? Token() : tokens.at(tokenIdx);
        const QChar characterBeforePositionInDocument
                = textDocument->characterAt(positionInDocument - 1);

        if (adjustForQt5SignalSlotCompletion && *kind == T_AMPER && tokenIdx > 0) {
            const Token &previousToken = tokens.at(tokenIdx - 1);
            if (previousToken.kind() == T_COMMA)
                start = positionInDocument - (tk.utf16charOffset - previousToken.utf16charOffset) - 1;
        } else if (*kind == T_DOXY_COMMENT && !(tk.is(T_DOXY_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))) {
            *kind = T_EOF_SYMBOL;
            start = positionInDocument;
        // Do not complete in comments, except in doxygen comments for doxygen commands.
        // Do not complete in strings, except it is for include completion.
        } else if (tk.is(T_COMMENT) || tk.is(T_CPP_COMMENT)
                 || ((tk.is(T_CPP_DOXY_COMMENT) || tk.is(T_DOXY_COMMENT))
                        && !isDoxygenTagCompletionCharacter(characterBeforePositionInDocument))
                 || (tk.isLiteral() && (*kind != T_STRING_LITERAL
                                     && *kind != T_ANGLE_STRING_LITERAL
                                     && *kind != T_SLASH
                                     && *kind != T_DOT))) {
            *kind = T_EOF_SYMBOL;
            start = positionInDocument;
        // Include completion: can be triggered by slash, but only in a string
        } else if (*kind == T_SLASH && (tk.isNot(T_STRING_LITERAL) && tk.isNot(T_ANGLE_STRING_LITERAL))) {
            *kind = T_EOF_SYMBOL;
            start = positionInDocument;
        } else if (*kind == T_LPAREN) {
            if (tokenIdx > 0) {
                const Token &previousToken = tokens.at(tokenIdx - 1); // look at the token at the left of T_LPAREN
                switch (previousToken.kind()) {
                case T_IDENTIFIER:
                case T_GREATER:
                case T_SIGNAL:
                case T_SLOT:
                    break; // good

                default:
                    // that's a bad token :)
                    *kind = T_EOF_SYMBOL;
                    start = positionInDocument;
                }
            }
        }
        // Check for include preprocessor directive
        else if (*kind == T_STRING_LITERAL || *kind == T_ANGLE_STRING_LITERAL || *kind == T_SLASH
                 || (*kind == T_DOT
                        && (tk.is(T_STRING_LITERAL) || tk.is(T_ANGLE_STRING_LITERAL)))) {
            bool include = false;
            if (tokens.size() >= 3) {
                if (tokens.at(0).is(T_POUND) && tokens.at(1).is(T_IDENTIFIER) && (tokens.at(2).is(T_STRING_LITERAL) ||
                                                                                  tokens.at(2).is(T_ANGLE_STRING_LITERAL))) {
                    const Token &directiveToken = tokens.at(1);
                    QString directive = tc.block().text().mid(directiveToken.utf16charsBegin(),
                                                              directiveToken.utf16chars());
                    if (directive == QLatin1String("include") ||
                            directive == QLatin1String("include_next") ||
                            directive == QLatin1String("import")) {
                        include = true;
                    }
                }
            }

            if (!include) {
                *kind = T_EOF_SYMBOL;
                start = positionInDocument;
            } else if (*kind == T_DOT && dotAtIncludeCompletionHandler){
                dotAtIncludeCompletionHandler(start, kind);
            }
        }
    }
}

} // namespace CppTools
