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

#include "clangactivationsequencecontextprocessor.h"

#include "clangactivationsequenceprocessor.h"

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/SimpleLexer.h>

#include <QRegExp>
#include <QTextDocument>

namespace ClangCodeModel {
namespace Internal {

ActivationSequenceContextProcessor::ActivationSequenceContextProcessor(const ClangCompletionAssistInterface *assistInterface)
    : m_textCursor(assistInterface->textDocument()),
      m_assistInterface(assistInterface),
      m_positionInDocument(assistInterface->position()),
      m_startOfNamePosition(m_positionInDocument),
      m_operatorStartPosition(m_positionInDocument)

{
    m_textCursor.setPosition(m_positionInDocument);

    process();
}

CPlusPlus::Kind ActivationSequenceContextProcessor::completionKind() const
{
    return m_completionKind;
}

const QTextCursor &ActivationSequenceContextProcessor::textCursor_forTestOnly() const
{
    return m_textCursor;
}

int ActivationSequenceContextProcessor::startOfNamePosition() const
{
    return m_startOfNamePosition;
}

int ActivationSequenceContextProcessor::operatorStartPosition() const
{
    return m_operatorStartPosition;
}

void ActivationSequenceContextProcessor::process()
{
    goBackToStartOfName();
    processActivationSequence();

    if (m_completionKind != CPlusPlus::T_EOF_SYMBOL) {
        processStringLiteral();
        processComma();
        generateTokens();
        processDoxygenComment();
        processComment();
        processInclude();
        processSlashOutsideOfAString();
        processLeftParen();
        processPreprocessorInclude();
    }

    resetPositionsForEOFCompletionKind();
}

void ActivationSequenceContextProcessor::processActivationSequence()
{
    const int nonSpacePosition = skipPrecedingWhitespace(m_assistInterface, m_startOfNamePosition);
    const auto activationSequence = m_assistInterface->textAt(nonSpacePosition - 3, 3);
    ActivationSequenceProcessor activationSequenceProcessor(activationSequence,
                                                            nonSpacePosition,
                                                            true);

    m_completionKind = activationSequenceProcessor.completionKind();
    m_operatorStartPosition = activationSequenceProcessor.operatorStartPosition();
}

void ActivationSequenceContextProcessor::processStringLiteral()
{
    if (m_completionKind == CPlusPlus::T_STRING_LITERAL) {
        QTextCursor selectionTextCursor = m_textCursor;
        selectionTextCursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        QString selection = selectionTextCursor.selectedText();
        if (selection.indexOf(QLatin1Char('"')) < selection.length() - 1)
            m_completionKind = CPlusPlus::T_EOF_SYMBOL;
    }
}

void ActivationSequenceContextProcessor::processComma()
{
    if (m_completionKind == CPlusPlus::T_COMMA) {
        CPlusPlus::ExpressionUnderCursor expressionUnderCursor(m_assistInterface->languageFeatures());
        if (expressionUnderCursor.startOfFunctionCall(m_textCursor) == -1)
            m_completionKind = CPlusPlus::T_EOF_SYMBOL;
    }
}

void ActivationSequenceContextProcessor::generateTokens()
{
    CPlusPlus::SimpleLexer tokenize;
    tokenize.setLanguageFeatures(m_assistInterface->languageFeatures());
    tokenize.setSkipComments(false);
    auto state = CPlusPlus::BackwardsScanner::previousBlockState(m_textCursor.block());
    m_tokens = tokenize(m_textCursor.block().text(), state);
    int leftOfCursorTokenIndex = std::max(0, m_textCursor.positionInBlock() - 1);
    m_tokenIndex= CPlusPlus::SimpleLexer::tokenBefore(m_tokens, leftOfCursorTokenIndex); // get the token at the left of the cursor
    if (m_tokenIndex > -1)
        m_token = m_tokens.at(m_tokenIndex);
}

void ActivationSequenceContextProcessor::processDoxygenComment()
{
    if (m_completionKind == CPlusPlus::T_DOXY_COMMENT
            && !(m_token.is(CPlusPlus::T_DOXY_COMMENT)
                 || m_token.is(CPlusPlus::T_CPP_DOXY_COMMENT)))
        m_completionKind = CPlusPlus::T_EOF_SYMBOL;
}

void ActivationSequenceContextProcessor::processComment()
{
    if (m_token.is(CPlusPlus::T_COMMENT) || m_token.is(CPlusPlus::T_CPP_COMMENT))
        m_completionKind = CPlusPlus::T_EOF_SYMBOL;
}

void ActivationSequenceContextProcessor::processInclude()
{
    if (m_token.isLiteral() && !isCompletionKindStringLiteralOrSlash())
        m_completionKind = CPlusPlus::T_EOF_SYMBOL;
}

void ActivationSequenceContextProcessor::processSlashOutsideOfAString()
{
    if (m_completionKind ==CPlusPlus::T_SLASH
            && (m_token.isNot(CPlusPlus::T_STRING_LITERAL)
                && m_token.isNot(CPlusPlus::T_ANGLE_STRING_LITERAL)))
        m_completionKind = CPlusPlus::T_EOF_SYMBOL;
}

void ActivationSequenceContextProcessor::processLeftParen()
{
    if (m_completionKind == CPlusPlus::T_LPAREN) {
        if (m_tokenIndex > 0) {
            // look at the token at the left of T_LPAREN
            const CPlusPlus::Token &previousToken = m_tokens.at(m_tokenIndex - 1);
            switch (previousToken.kind()) {
                case CPlusPlus::T_IDENTIFIER:
                case CPlusPlus::T_GREATER:
                case CPlusPlus::T_SIGNAL:
                case CPlusPlus::T_SLOT:
                    break; // good

                default:
                    // that's a bad token :)
                    m_completionKind = CPlusPlus::T_EOF_SYMBOL;
            }
        }
    }
}

bool ActivationSequenceContextProcessor::isCompletionKindStringLiteralOrSlash() const
{
    return m_completionKind == CPlusPlus::T_STRING_LITERAL
        || m_completionKind == CPlusPlus::T_ANGLE_STRING_LITERAL
        || m_completionKind == CPlusPlus::T_SLASH;
}

bool ActivationSequenceContextProcessor::isProbablyPreprocessorIncludeDirective() const
{
    return m_tokens.size() >= 3
            && m_tokens.at(0).is(CPlusPlus::T_POUND)
            && m_tokens.at(1).is(CPlusPlus::T_IDENTIFIER)
            && (m_tokens.at(2).is(CPlusPlus::T_STRING_LITERAL)
                ||  m_tokens.at(2).is(CPlusPlus::T_ANGLE_STRING_LITERAL));
}

void ActivationSequenceContextProcessor::processPreprocessorInclude()
{
    if (isCompletionKindStringLiteralOrSlash()) {
        if (isProbablyPreprocessorIncludeDirective()) {
            const CPlusPlus::Token &directiveToken = m_tokens.at(1);
            QString directive = m_textCursor.block().text().mid(directiveToken.bytesBegin(),
                                                                directiveToken.bytes());
            if (directive != QStringLiteral("include")
                    && directive != QStringLiteral("include_next")
                    && directive != QStringLiteral("import"))
                m_completionKind = CPlusPlus::T_EOF_SYMBOL;
        } else {
            m_completionKind = CPlusPlus::T_EOF_SYMBOL;
        }
    }
}

void ActivationSequenceContextProcessor::resetPositionsForEOFCompletionKind()
{
    if (m_completionKind == CPlusPlus::T_EOF_SYMBOL)
        m_operatorStartPosition = m_positionInDocument;
}

int ActivationSequenceContextProcessor::skipPrecedingWhitespace(
        const TextEditor::AssistInterface *assistInterface,
        int startPosition)
{
    int position = startPosition;
    while (assistInterface->characterAt(position - 1).isSpace())
        --position;
    return position;
}

static bool isValidIdentifierChar(const QChar &character)
{
    return character.isLetterOrNumber()
        || character == QLatin1Char('_')
        || character.isHighSurrogate()
        || character.isLowSurrogate();
}

int ActivationSequenceContextProcessor::findStartOfName(
        const TextEditor::AssistInterface *assistInterface,
        int startPosition)
{
    int position = startPosition;
    QChar character;
    do {
        character = assistInterface->characterAt(--position);
    } while (isValidIdentifierChar(character));

    return position + 1;
}

void ActivationSequenceContextProcessor::goBackToStartOfName()
{
    m_startOfNamePosition = findStartOfName(m_assistInterface, m_positionInDocument);

    if (m_startOfNamePosition != m_positionInDocument)
        m_textCursor.setPosition(m_startOfNamePosition);
}

} // namespace Internal
} // namespace ClangCodeModel

