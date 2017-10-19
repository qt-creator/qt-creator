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

#pragma once

#include <clangcodemodel/clangcompletionassistinterface.h>

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
    ActivationSequenceContextProcessor(const ClangCompletionAssistInterface *assistInterface);

    CPlusPlus::Kind completionKind() const;
    int startOfNamePosition() const;   // e.g. points to 'b' in "foo.bar<CURSOR>"
    int operatorStartPosition() const; // e.g. points to '.' for "foo.bar<CURSOR>"

    const QTextCursor &textCursor_forTestOnly() const;

    enum class NameCategory { Function, NonFunction };
    static int findStartOfName(const TextEditor::AssistInterface *assistInterface,
                               int startPosition,
                               NameCategory category = NameCategory::NonFunction);
    static int skipPrecedingWhitespace(const TextEditor::AssistInterface *assistInterface,
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
    void processLeftParen();
    void processPreprocessorInclude();
    void resetPositionsForEOFCompletionKind();

    bool isCompletionKindStringLiteralOrSlash() const;
    bool isProbablyPreprocessorIncludeDirective() const;

private:
    QVector<CPlusPlus::Token> m_tokens;
    QTextCursor m_textCursor;
    CPlusPlus::Token m_token;
    const ClangCompletionAssistInterface *m_assistInterface;
    int m_tokenIndex;
    const int m_positionInDocument;
    int m_startOfNamePosition;
    int m_operatorStartPosition;
    CPlusPlus::Kind m_completionKind;
};

} // namespace Internal
} // namespace ClangCodeModel
