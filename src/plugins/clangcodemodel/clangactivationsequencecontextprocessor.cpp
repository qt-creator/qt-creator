// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangactivationsequencecontextprocessor.h"

#include "clangactivationsequenceprocessor.h"

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/SimpleLexer.h>
#include <utils/textutils.h>

#include <QTextDocument>

namespace ClangCodeModel {
namespace Internal {

ActivationSequenceContextProcessor::ActivationSequenceContextProcessor(
        QTextDocument *document, int position, CPlusPlus::LanguageFeatures languageFeatures)
    : m_textCursor(document),
      m_document(document),
      m_languageFeatures(languageFeatures),
      m_positionInDocument(position),
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
        processLeftParenOrBrace();
        processPreprocessorInclude();
    }

    resetPositionsForEOFCompletionKind();
}

void ActivationSequenceContextProcessor::processActivationSequence()
{
    const int nonSpacePosition = skipPrecedingWhitespace(m_document, m_startOfNamePosition);
    const auto activationSequence = Utils::Text::textAt(QTextCursor(m_document),
                                                        nonSpacePosition - 3, 3);
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
        CPlusPlus::ExpressionUnderCursor expressionUnderCursor(m_languageFeatures);
        if (expressionUnderCursor.startOfFunctionCall(m_textCursor) == -1)
            m_completionKind = CPlusPlus::T_EOF_SYMBOL;
    }
}

void ActivationSequenceContextProcessor::generateTokens()
{
    CPlusPlus::SimpleLexer tokenize;
    tokenize.setLanguageFeatures(m_languageFeatures);
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

void ActivationSequenceContextProcessor::processLeftParenOrBrace()
{
    if (m_completionKind == CPlusPlus::T_LPAREN || m_completionKind == CPlusPlus::T_LBRACE) {
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

int ActivationSequenceContextProcessor::skipPrecedingWhitespace(const QTextDocument *document,
                                                                int startPosition)
{
    int position = startPosition;
    while (document->characterAt(position - 1).isSpace())
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
        const QTextDocument *document,
        int startPosition,
        NameCategory category)
{
    int position = startPosition;
    QChar character;

    if (category == NameCategory::Function
            && position > 2 && document->characterAt(position - 1) == '>'
            && document->characterAt(position - 2) != '-') {
        uint unbalancedLessGreater = 1;
        --position;
        while (unbalancedLessGreater > 0 && position > 2) {
            character = document->characterAt(--position);
            // Do not count -> usage inside temlate argument list
            if (character == '<')
                --unbalancedLessGreater;
            else if (character == '>' && document->characterAt(position-1) != '-')
                ++unbalancedLessGreater;
        }
        position = skipPrecedingWhitespace(document, position) - 1;
    }

    do {
        character = document->characterAt(--position);
    } while (isValidIdentifierChar(character));

    int prevPosition = skipPrecedingWhitespace(document, position);
    if (category == NameCategory::Function
            && document->characterAt(prevPosition) == ':'
            && document->characterAt(prevPosition - 1) == ':') {
        // Handle :: case - go recursive
        prevPosition = skipPrecedingWhitespace(document, prevPosition - 2);
        return findStartOfName(document, prevPosition + 1, category);
    }

    return position + 1;
}

void ActivationSequenceContextProcessor::goBackToStartOfName()
{
    CPlusPlus::SimpleLexer tokenize;
    tokenize.setLanguageFeatures(m_languageFeatures);
    tokenize.setSkipComments(false);
    const int state = CPlusPlus::BackwardsScanner::previousBlockState(m_textCursor.block());
    const CPlusPlus::Tokens tokens = tokenize(m_textCursor.block().text(), state);
    const int tokenPos = std::max(0, m_textCursor.positionInBlock() - 1);
    const int tokIndex = CPlusPlus::SimpleLexer::tokenAt(tokens, tokenPos);
    if (tokIndex > -1 && tokens.at(tokIndex).isStringLiteral()) {
        const int tokenStart = tokens.at(tokIndex).utf16charOffset;
        const int slashIndex = m_textCursor.block().text().lastIndexOf(
            '/',
            std::min(m_textCursor.positionInBlock(), int(m_textCursor.block().text().length() - 1)));
        m_startOfNamePosition = m_textCursor.block().position() + std::max(slashIndex, tokenStart)
                + 1;
    } else {
        m_startOfNamePosition = findStartOfName(m_document, m_positionInDocument);
    }

    if (m_startOfNamePosition != m_positionInDocument)
        m_textCursor.setPosition(m_startOfNamePosition);
}

} // namespace Internal
} // namespace ClangCodeModel

