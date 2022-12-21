// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/Token.h>

#include <QTextCursor>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace ClangCodeModel {
namespace Internal {

class ActivationSequenceContextProcessor
{
public:
    ActivationSequenceContextProcessor(QTextDocument *document, int position,
                                       CPlusPlus::LanguageFeatures languageFeatures);

    CPlusPlus::Kind completionKind() const;
    int startOfNamePosition() const;   // e.g. points to 'b' in "foo.bar<CURSOR>"
    int operatorStartPosition() const; // e.g. points to '.' for "foo.bar<CURSOR>"

    const QTextCursor &textCursor_forTestOnly() const;

    enum class NameCategory { Function, NonFunction };
    static int findStartOfName(const QTextDocument *document,
                               int startPosition,
                               NameCategory category = NameCategory::NonFunction);
    static int skipPrecedingWhitespace(const QTextDocument *document,
                                       int startPosition);

protected:
    void process();
    void goBackToStartOfName();
    void processActivationSequence();
    void processStringLiteral();
    void processComma();
    void generateTokens();
    void processDoxygenComment();
    void processComment();
    void processInclude();
    void processSlashOutsideOfAString();
    void processLeftParenOrBrace();
    void processPreprocessorInclude();
    void resetPositionsForEOFCompletionKind();

    bool isCompletionKindStringLiteralOrSlash() const;
    bool isProbablyPreprocessorIncludeDirective() const;

private:
    QVector<CPlusPlus::Token> m_tokens;
    QTextCursor m_textCursor;
    CPlusPlus::Token m_token;
    QTextDocument * const m_document;
    const CPlusPlus::LanguageFeatures m_languageFeatures;
    int m_tokenIndex;
    const int m_positionInDocument;
    int m_startOfNamePosition;
    int m_operatorStartPosition;
    CPlusPlus::Kind m_completionKind;
};

} // namespace Internal
} // namespace ClangCodeModel
