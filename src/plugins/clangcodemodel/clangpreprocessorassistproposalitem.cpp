// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangpreprocessorassistproposalitem.h"

#include <texteditor/texteditor.h>

#include <cplusplus/Token.h>

namespace ClangCodeModel {
namespace Internal {

bool ClangPreprocessorAssistProposalItem::prematurelyApplies(const QChar &typedCharacter) const
{
    bool applies = false;

    if (isInclude())
        applies = typedCharacter == QLatin1Char('/') && text().endsWith(QLatin1Char('/'));

    if (applies)
        m_typedCharacter = typedCharacter;

    return applies;
}

bool ClangPreprocessorAssistProposalItem::implicitlyApplies() const
{
    return true;
}

void ClangPreprocessorAssistProposalItem::apply(TextEditor::TextDocumentManipulatorInterface &manipulator,
                                                int basePosition) const
{
    // TODO move in an extra class under tests

    QString textToBeInserted = text();

    QString extraCharacters;
    int extraLength = 0;

    if (isInclude()) {
        if (!textToBeInserted.endsWith(QLatin1Char('/'))) {
            extraCharacters += QLatin1Char((m_completionOperator == CPlusPlus::T_ANGLE_STRING_LITERAL) ? '>' : '"');
        } else {
            if (m_typedCharacter == QLatin1Char('/')) // Eat the slash
                m_typedCharacter = QChar();
        }
    }

    if (!m_typedCharacter.isNull())
        extraCharacters += m_typedCharacter;

    // Avoid inserting characters that are already there
    const int endsPosition = manipulator.positionAt(TextEditor::EndOfLinePosition);
    const QString existingText = manipulator.textAt(manipulator.currentPosition(), endsPosition - manipulator.currentPosition());
    int existLength = 0;
    if (!existingText.isEmpty()) {
        // Calculate the exist length in front of the extra chars
        existLength = textToBeInserted.length() - (manipulator.currentPosition() - basePosition);
        while (!existingText.startsWith(textToBeInserted.right(existLength))) {
            if (--existLength == 0)
                break;
        }
    }
    for (int i = 0; i < extraCharacters.length(); ++i) {
        const QChar a = extraCharacters.at(i);
        const QChar b = manipulator.characterAt(manipulator.currentPosition() + i + existLength);
        if (a == b)
            ++extraLength;
        else
            break;
    }

    textToBeInserted += extraCharacters;

    // Insert the remainder of the name
    const int length = manipulator.currentPosition() - basePosition + existLength + extraLength;

    manipulator.replace(basePosition, length, textToBeInserted);
}

void ClangPreprocessorAssistProposalItem::setText(const QString &text)
{
    m_text = text;
}

QString ClangPreprocessorAssistProposalItem::text() const
{
    return m_text;
}

void ClangPreprocessorAssistProposalItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

QIcon ClangPreprocessorAssistProposalItem::icon() const
{
    return m_icon;
}

void ClangPreprocessorAssistProposalItem::setDetail(const QString &detail)
{
    m_detail = detail;
}

QString ClangPreprocessorAssistProposalItem::detail() const
{
    return QString();
}

Qt::TextFormat ClangPreprocessorAssistProposalItem::detailFormat() const
{
    return Qt::RichText;
}

bool ClangPreprocessorAssistProposalItem::isSnippet() const
{
    return false;
}

bool ClangPreprocessorAssistProposalItem::isValid() const
{
    return true;
}

quint64 ClangPreprocessorAssistProposalItem::hash() const
{
    return 0;
}

void ClangPreprocessorAssistProposalItem::setCompletionOperator(uint completionOperator)
{
    m_completionOperator = completionOperator;
}

bool ClangPreprocessorAssistProposalItem::isInclude() const
{
    return m_completionOperator == CPlusPlus::T_STRING_LITERAL
        || m_completionOperator == CPlusPlus::T_ANGLE_STRING_LITERAL;
}

} // namespace Internal
} // namespace ClangCodeModel
