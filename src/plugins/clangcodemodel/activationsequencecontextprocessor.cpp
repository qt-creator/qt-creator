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

#include "activationsequencecontextprocessor.h"

#include "activationsequenceprocessor.h"

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
      m_positionAfterOperator(m_positionInDocument),
      m_positionBeforeOperator(m_positionAfterOperator)

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

int ActivationSequenceContextProcessor::positionAfterOperator() const
{
    return m_positionAfterOperator;
}

int ActivationSequenceContextProcessor::positionBeforeOperator() const
{
    return m_positionBeforeOperator;
}

void ActivationSequenceContextProcessor::process()
{
    skipeWhiteSpacesAndIdentifierBeforeCursor();
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
        resetPositionForEOFCompletionKind();
    }
}

void ActivationSequenceContextProcessor::processActivationSequence()
{
    const auto activationSequence = m_assistInterface->textAt(m_positionInDocument - 3, 3);
    ActivationSequenceProcessor activationSequenceProcessor(activationSequence,
                                                            m_positionInDocument,
                                                            true);

    m_completionKind = activationSequenceProcessor.completionKind();
    m_positionBeforeOperator = activationSequenceProcessor.position();

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

void ActivationSequenceContextProcessor::resetPositionForEOFCompletionKind()
{
    if (m_completionKind == CPlusPlus::T_EOF_SYMBOL)
        m_positionBeforeOperator = m_positionInDocument;
}

void ActivationSequenceContextProcessor::skipeWhiteSpacesAndIdentifierBeforeCursor()
{
    QTextDocument *document = m_assistInterface->textDocument();

    const QRegExp findNonWhiteSpaceRegularExpression(QStringLiteral("[^\\s\\w]"));

    auto nonWhiteSpaceTextCursor = document->find(findNonWhiteSpaceRegularExpression,
                                                  m_positionInDocument,
                                                  QTextDocument::FindBackward);

    if (!nonWhiteSpaceTextCursor.isNull()) {
        m_positionInDocument = nonWhiteSpaceTextCursor.position();
        m_positionAfterOperator = m_positionInDocument;
        m_textCursor.setPosition(m_positionInDocument);
    }
}

} // namespace Internal
} // namespace ClangCodeModel

