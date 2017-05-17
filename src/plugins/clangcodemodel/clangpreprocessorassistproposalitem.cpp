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

#include "clangpreprocessorassistproposalitem.h"

#include <texteditor/texteditor.h>

#include <cplusplus/Token.h>

namespace ClangCodeModel {

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
    int cursorOffset = 0;

    if (isInclude()) {
        if (!textToBeInserted.endsWith(QLatin1Char('/'))) {
            extraCharacters += QLatin1Char((m_completionOperator == CPlusPlus::T_ANGLE_STRING_LITERAL) ? '>' : '"');
        } else {
            if (m_typedCharacter == QLatin1Char('/')) // Eat the slash
                m_typedCharacter = QChar();
        }
    }

    if (!m_typedCharacter.isNull()) {
        extraCharacters += m_typedCharacter;
        if (cursorOffset != 0)
            --cursorOffset;
    }

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

    const bool isReplaced = manipulator.replace(basePosition, length, textToBeInserted);
    if (isReplaced && cursorOffset)
        manipulator.setCursorPosition(manipulator.currentPosition() + cursorOffset);
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

} // namespace ClangCodeModel
