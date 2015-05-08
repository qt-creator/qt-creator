/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#include "clangcompletioncontextanalyzer.h"

#include <texteditor/codeassist/assistinterface.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/SimpleLexer.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTextBlock>
#include <QTextCursor>

using namespace CPlusPlus;

namespace {

int activationSequenceChar(const QChar &ch, const QChar &ch2, const QChar &ch3,
                           unsigned *kind, bool wantFunctionCall)
{
    int referencePosition = 0;
    int completionKind = T_EOF_SYMBOL;
    switch (ch.toLatin1()) {
    case '.':
        if (ch2 != QLatin1Char('.')) {
            completionKind = T_DOT;
            referencePosition = 1;
        }
        break;
    case ',':
        completionKind = T_COMMA;
        referencePosition = 1;
        break;
    case '(':
        if (wantFunctionCall) {
            completionKind = T_LPAREN;
            referencePosition = 1;
        }
        break;
    case ':':
        if (ch3 != QLatin1Char(':') && ch2 == QLatin1Char(':')) {
            completionKind = T_COLON_COLON;
            referencePosition = 2;
        }
        break;
    case '>':
        if (ch2 == QLatin1Char('-')) {
            completionKind = T_ARROW;
            referencePosition = 2;
        }
        break;
    case '*':
        if (ch2 == QLatin1Char('.')) {
            completionKind = T_DOT_STAR;
            referencePosition = 2;
        } else if (ch3 == QLatin1Char('-') && ch2 == QLatin1Char('>')) {
            completionKind = T_ARROW_STAR;
            referencePosition = 3;
        }
        break;
    case '\\':
    case '@':
        if (ch2.isNull() || ch2.isSpace()) {
            completionKind = T_DOXY_COMMENT;
            referencePosition = 1;
        }
        break;
    case '<':
        completionKind = T_ANGLE_STRING_LITERAL;
        referencePosition = 1;
        break;
    case '"':
        completionKind = T_STRING_LITERAL;
        referencePosition = 1;
        break;
    case '/':
        completionKind = T_SLASH;
        referencePosition = 1;
        break;
    case '#':
        completionKind = T_POUND;
        referencePosition = 1;
        break;
    }

    if (kind)
        *kind = completionKind;

    return referencePosition;
}

bool isTokenForIncludePathCompletion(unsigned tokenKind)
{
    return tokenKind == T_STRING_LITERAL
        || tokenKind == T_ANGLE_STRING_LITERAL
        || tokenKind == T_SLASH;
}

bool isTokenForPassThrough(unsigned tokenKind)
{
    return tokenKind == T_EOF_SYMBOL
        || tokenKind == T_DOT
        || tokenKind == T_COLON_COLON
        || tokenKind == T_ARROW
        || tokenKind == T_DOT_STAR;
}

} // anonymous namespace

namespace ClangCodeModel {
namespace Internal {

ClangCompletionContextAnalyzer::ClangCompletionContextAnalyzer(
        const TextEditor::AssistInterface *assistInterface,
        CPlusPlus::LanguageFeatures languageFeatures)
    : m_interface(assistInterface)
    , m_languageFeatures(languageFeatures)
{
}

void ClangCompletionContextAnalyzer::analyze()
{
    QTC_ASSERT(m_interface, return);
    const int startOfName = findStartOfName();
    m_positionForProposal = startOfName;
    setActionAndClangPosition(PassThroughToLibClang, -1);

    const int endOfOperator = skipPrecedingWhitespace(startOfName);
    m_completionOperator = T_EOF_SYMBOL;
    m_positionEndOfExpression = startOfOperator(endOfOperator, &m_completionOperator,
                                                /*want function call =*/ true);

    if (isTokenForPassThrough(m_completionOperator)) {
        setActionAndClangPosition(PassThroughToLibClang, endOfOperator);
        return;
    } else if (m_completionOperator == T_DOXY_COMMENT) {
        setActionAndClangPosition(CompleteDoxygenKeyword, -1);
        return;
    } else if (m_completionOperator == T_POUND) {
        // TODO: Check if libclang can complete preprocessor directives
        setActionAndClangPosition(CompletePreprocessorDirective, -1);
        return;
    } else if (isTokenForIncludePathCompletion(m_completionOperator)) {
        setActionAndClangPosition(CompleteIncludePath, -1);
        return;
    }

    ExpressionUnderCursor expressionUnderCursor(m_languageFeatures);
    QTextCursor textCursor(m_interface->textDocument());

    if (m_completionOperator == T_COMMA) { // For function hints
        textCursor.setPosition(m_positionEndOfExpression);
        const int start = expressionUnderCursor.startOfFunctionCall(textCursor);
        QTC_ASSERT(start != -1, setActionAndClangPosition(PassThroughToLibClang, startOfName); return);
        m_positionEndOfExpression = start;
        m_positionForProposal = start + 1; // After '(' of function call
        m_completionOperator = T_LPAREN;
    }

    if (m_completionOperator == T_LPAREN) {
        textCursor.setPosition(m_positionEndOfExpression);
        const QString expression = expressionUnderCursor(textCursor);

        if (expression.endsWith(QLatin1String("SIGNAL"))) {
            setActionAndClangPosition(CompleteSignal, endOfOperator);
        } else if (expression.endsWith(QLatin1String("SLOT"))) {
            setActionAndClangPosition(CompleteSlot, endOfOperator);
        } else if (m_interface->position() != endOfOperator) {
            // No function completion if cursor is not after '(' or ','
            m_positionForProposal = startOfName;
            setActionAndClangPosition(PassThroughToLibClang, endOfOperator);
        } else {
            const FunctionInfo functionInfo = analyzeFunctionCall(endOfOperator);
            m_functionName = functionInfo.functionName;
            setActionAndClangPosition(PassThroughToLibClangAfterLeftParen,
                                      functionInfo.functionNamePosition);
        }

        return;
    }

    QTC_CHECK(!"Unexpected completion context");
    setActionAndClangPosition(PassThroughToLibClang, startOfName);
    return;
}

ClangCompletionContextAnalyzer::FunctionInfo ClangCompletionContextAnalyzer::analyzeFunctionCall(
        int endOfOperator) const
{
    int index = skipPrecedingWhitespace(endOfOperator);
    QTextCursor textCursor(m_interface->textDocument());
    textCursor.setPosition(index);

    ExpressionUnderCursor euc(m_languageFeatures);
    index = euc.startOfFunctionCall(textCursor);
    const int functionNameStart = findStartOfName(index);

    QTextCursor textCursor2(m_interface->textDocument());
    textCursor2.setPosition(functionNameStart);
    textCursor2.setPosition(index, QTextCursor::KeepAnchor);

    FunctionInfo info;
    info.functionNamePosition = functionNameStart;
    info.functionName = textCursor2.selectedText().trimmed();
    return info;
}

int ClangCompletionContextAnalyzer::findStartOfName(int position) const
{
    if (position == -1)
        position = m_interface->position();
    QChar chr;

    do {
        chr = m_interface->characterAt(--position);
        // TODO: Check also chr.isHighSurrogate() / ch.isLowSurrogate()?
        // See also CppTools::isValidFirstIdentifierChar
    } while (chr.isLetterOrNumber() || chr == QLatin1Char('_'));

    return position + 1;
}

int ClangCompletionContextAnalyzer::skipPrecedingWhitespace(int position) const
{
    QTC_ASSERT(position >= 0, return position);
    while (m_interface->characterAt(position - 1).isSpace())
        --position;
    return position;
}

int ClangCompletionContextAnalyzer::startOfOperator(int pos,
                                                    unsigned *kind,
                                                    bool wantFunctionCall) const
{
    const QChar ch  = pos > -1 ? m_interface->characterAt(pos - 1) : QChar();
    const QChar ch2 = pos >  0 ? m_interface->characterAt(pos - 2) : QChar();
    const QChar ch3 = pos >  1 ? m_interface->characterAt(pos - 3) : QChar();

    int start = pos - activationSequenceChar(ch, ch2, ch3, kind, wantFunctionCall);
    if (start != pos) {
        QTextCursor tc(m_interface->textDocument());
        tc.setPosition(pos);

        // Include completion: make sure the quote character is the first one on the line
        if (*kind == T_STRING_LITERAL) {
            QTextCursor s = tc;
            s.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            QString sel = s.selectedText();
            if (sel.indexOf(QLatin1Char('"')) < sel.length() - 1) {
                *kind = T_EOF_SYMBOL;
                start = pos;
            }
        } else if (*kind == T_COMMA) {
            ExpressionUnderCursor expressionUnderCursor(m_languageFeatures);
            if (expressionUnderCursor.startOfFunctionCall(tc) == -1) {
                *kind = T_EOF_SYMBOL;
                start = pos;
            }
        }

        SimpleLexer tokenize;
        tokenize.setLanguageFeatures(m_languageFeatures);
        tokenize.setSkipComments(false);
        const Tokens &tokens = tokenize(tc.block().text(), BackwardsScanner::previousBlockState(tc.block()));
        const int tokenIdx = SimpleLexer::tokenBefore(tokens, qMax(0, tc.positionInBlock() - 1)); // get the token at the left of the cursor
        const Token tk = (tokenIdx == -1) ? Token() : tokens.at(tokenIdx);

        if (*kind == T_DOXY_COMMENT && !(tk.is(T_DOXY_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))) {
            *kind = T_EOF_SYMBOL;
            start = pos;
        }
        // Don't complete in comments or strings, but still check for include completion
        else if (tk.is(T_COMMENT) || tk.is(T_CPP_COMMENT) ||
                 (tk.isLiteral() && (*kind != T_STRING_LITERAL
                                     && *kind != T_ANGLE_STRING_LITERAL
                                     && *kind != T_SLASH))) {
            *kind = T_EOF_SYMBOL;
            start = pos;
        }
        // Include completion: can be triggered by slash, but only in a string
        else if (*kind == T_SLASH && (tk.isNot(T_STRING_LITERAL) && tk.isNot(T_ANGLE_STRING_LITERAL))) {
            *kind = T_EOF_SYMBOL;
            start = pos;
        }
        else if (*kind == T_LPAREN) {
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
                    start = pos;
                }
            }
        }
        // Check for include preprocessor directive
        else if (*kind == T_STRING_LITERAL || *kind == T_ANGLE_STRING_LITERAL || *kind == T_SLASH) {
            bool include = false;
            if (tokens.size() >= 3) {
                if (tokens.at(0).is(T_POUND) && tokens.at(1).is(T_IDENTIFIER) && (tokens.at(2).is(T_STRING_LITERAL) ||
                                                                                  tokens.at(2).is(T_ANGLE_STRING_LITERAL))) {
                    const Token &directiveToken = tokens.at(1);
                    QString directive = tc.block().text().mid(directiveToken.bytesBegin(),
                                                              directiveToken.bytes());
                    if (directive == QLatin1String("include") ||
                            directive == QLatin1String("include_next") ||
                            directive == QLatin1String("import")) {
                        include = true;
                    }
                }
            }

            if (!include) {
                *kind = T_EOF_SYMBOL;
                start = pos;
            }
        }
    }

    return start;
}

void ClangCompletionContextAnalyzer::setActionAndClangPosition(CompletionAction action,
                                                               int position)
{
    QTC_CHECK(position >= -1);
    m_completionAction = action;
    m_positionForClang = position;
}

} // namespace Internal
} // namespace ClangCodeModel
