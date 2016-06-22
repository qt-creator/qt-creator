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

#include "clangassistproposalitem.h"

#include "clangcompletionchunkstotextconverter.h"

#include <cplusplus/Icons.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/Token.h>

#include <texteditor/completionsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>

using namespace CPlusPlus;
using namespace ClangBackEnd;

namespace ClangCodeModel {
namespace Internal {

bool ClangAssistProposalItem::prematurelyApplies(const QChar &typedCharacter) const
{
    bool applies = false;

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT)
        applies = QString::fromLatin1("(,").contains(typedCharacter);
    else if (m_completionOperator == T_STRING_LITERAL || m_completionOperator == T_ANGLE_STRING_LITERAL)
        applies = (typedCharacter == QLatin1Char('/')) && text().endsWith(QLatin1Char('/'));
    else if (codeCompletion().completionKind() == CodeCompletion::ObjCMessageCompletionKind)
        applies = QString::fromLatin1(";.,").contains(typedCharacter);
    else
        applies = QString::fromLatin1(";.,:(").contains(typedCharacter);

    if (applies)
        m_typedCharacter = typedCharacter;

    return applies;
}

bool ClangAssistProposalItem::implicitlyApplies() const
{
    return false;
}

void ClangAssistProposalItem::apply(TextEditor::TextDocumentManipulatorInterface &manipulator,
                                    int basePosition) const
{
    const CodeCompletion ccr = codeCompletion();

    QString textToBeInserted = text();
    QString extraCharacters;
    int extraLength = 0;
    int cursorOffset = 0;
    bool setAutoCompleteSkipPos = false;

    bool autoParenthesesEnabled = true;
    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        extraCharacters += QLatin1Char(')');
        if (m_typedCharacter == QLatin1Char('(')) // Eat the opening parenthesis
            m_typedCharacter = QChar();
    } else  if (ccr.completionKind() == CodeCompletion::KeywordCompletionKind) {
        CompletionChunksToTextConverter converter;
        converter.setupForKeywords();

        converter.parseChunks(ccr.chunks());

        textToBeInserted = converter.text();
        if (converter.hasPlaceholderPositions())
            cursorOffset = converter.placeholderPositions().at(0) - converter.text().size();
    } else if (ccr.completionKind() == CodeCompletion::NamespaceCompletionKind) {
        CompletionChunksToTextConverter converter;

        converter.parseChunks(ccr.chunks()); // Appends "::" after name space name

        textToBeInserted = converter.text();
    } else if (!ccr.text().isEmpty()) {
        const TextEditor::CompletionSettings &completionSettings =
                TextEditor::TextEditorSettings::instance()->completionSettings();
        const bool autoInsertBrackets = completionSettings.m_autoInsertBrackets;

        if (autoInsertBrackets &&
                (ccr.completionKind() == CodeCompletion::FunctionCompletionKind
                 || ccr.completionKind() == CodeCompletion::DestructorCompletionKind
                 || ccr.completionKind() == CodeCompletion::SignalCompletionKind
                 || ccr.completionKind() == CodeCompletion::SlotCompletionKind)) {
            // When the user typed the opening parenthesis, he'll likely also type the closing one,
            // in which case it would be annoying if we put the cursor after the already automatically
            // inserted closing parenthesis.
            const bool skipClosingParenthesis = m_typedCharacter != QLatin1Char('(');

            if (completionSettings.m_spaceAfterFunctionName)
                extraCharacters += QLatin1Char(' ');
            extraCharacters += QLatin1Char('(');
            if (m_typedCharacter == QLatin1Char('('))
                m_typedCharacter = QChar();

            // If the function doesn't return anything, automatically place the semicolon,
            // unless we're doing a scope completion (then it might be function definition).
            const QChar characterAtCursor = manipulator.characterAt(manipulator.currentPosition());
            bool endWithSemicolon = m_typedCharacter == QLatin1Char(';')/*
                                            || (function->returnType()->isVoidType() && m_completionOperator != T_COLON_COLON)*/; //###
            const QChar semicolon = m_typedCharacter.isNull() ? QLatin1Char(';') : m_typedCharacter;

            if (endWithSemicolon && characterAtCursor == semicolon) {
                endWithSemicolon = false;
                m_typedCharacter = QChar();
            }

            // If the function takes no arguments, automatically place the closing parenthesis
            if (!isOverloaded() && !ccr.hasParameters() && skipClosingParenthesis) {
                extraCharacters += QLatin1Char(')');
                if (endWithSemicolon) {
                    extraCharacters += semicolon;
                    m_typedCharacter = QChar();
                }
            } else if (autoParenthesesEnabled) {
                const QChar lookAhead = manipulator.characterAt(manipulator.currentPosition() + 1);
                if (MatchingText::shouldInsertMatchingText(lookAhead)) {
                    extraCharacters += QLatin1Char(')');
                    --cursorOffset;
                    setAutoCompleteSkipPos = true;
                    if (endWithSemicolon) {
                        extraCharacters += semicolon;
                        --cursorOffset;
                        m_typedCharacter = QChar();
                    }
                }
            }
        }

#if 0
        if (autoInsertBrackets && data().canConvert<CompleteFunctionDeclaration>()) {
            if (m_typedChar == QLatin1Char('('))
                m_typedChar = QChar();

            // everything from the closing parenthesis on are extra chars, to
            // make sure an auto-inserted ")" gets replaced by ") const" if necessary
            int closingParen = toInsert.lastIndexOf(QLatin1Char(')'));
            extraChars = toInsert.mid(closingParen);
            toInsert.truncate(closingParen);
        }
#endif
    }

    // Append an unhandled typed character, adjusting cursor offset when it had been adjusted before
    if (!m_typedCharacter.isNull()) {
        extraCharacters += m_typedCharacter;
        if (cursorOffset != 0)
            --cursorOffset;
    }

    // Avoid inserting characters that are already there
    const int endsPosition = manipulator.positionAt(TextEditor::EndOfLinePosition);
    const QString existingText = manipulator.textAt(manipulator.currentPosition(), endsPosition - manipulator.currentPosition());
    int existLength = 0;
    if (!existingText.isEmpty() && ccr.completionKind() != CodeCompletion::KeywordCompletionKind) {
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

    const int length = manipulator.currentPosition() - basePosition + existLength + extraLength;

    const bool isReplaced = manipulator.replace(basePosition, length, textToBeInserted);
    if (isReplaced) {
        if (cursorOffset)
            manipulator.setCursorPosition(manipulator.currentPosition() + cursorOffset);
        if (setAutoCompleteSkipPos)
            manipulator.setAutoCompleteSkipPosition(manipulator.currentPosition());

        if (ccr.completionKind() == CodeCompletion::KeywordCompletionKind)
            manipulator.autoIndent(basePosition, textToBeInserted.size());
    }
}

void ClangAssistProposalItem::setText(const QString &text)
{
    m_text = text;
}

QString ClangAssistProposalItem::text() const
{
    return m_text;
}

QIcon ClangAssistProposalItem::icon() const
{
    using CPlusPlus::Icons;
    static const char SNIPPET_ICON_PATH[] = ":/texteditor/images/snippet.png";
    static const QIcon snippetIcon = QIcon(QLatin1String(SNIPPET_ICON_PATH));

    switch (m_codeCompletion.completionKind()) {
        case CodeCompletion::ClassCompletionKind:
        case CodeCompletion::TemplateClassCompletionKind:
        case CodeCompletion::TypeAliasCompletionKind:
            return Icons::iconForType(Icons::ClassIconType);
        case CodeCompletion::EnumerationCompletionKind:
            return Icons::iconForType(Icons::EnumIconType);
        case CodeCompletion::EnumeratorCompletionKind:
            return Icons::iconForType(Icons::EnumeratorIconType);
        case CodeCompletion::ConstructorCompletionKind:
        case CodeCompletion::DestructorCompletionKind:
        case CodeCompletion::FunctionCompletionKind:
        case CodeCompletion::TemplateFunctionCompletionKind:
        case CodeCompletion::ObjCMessageCompletionKind:
            switch (m_codeCompletion.availability()) {
                case CodeCompletion::Available:
                case CodeCompletion::Deprecated:
                    return Icons::iconForType(Icons::FuncPublicIconType);
                default:
                    return Icons::iconForType(Icons::FuncPrivateIconType);
            }
        case CodeCompletion::SignalCompletionKind:
            return Icons::iconForType(Icons::SignalIconType);
        case CodeCompletion::SlotCompletionKind:
            switch (m_codeCompletion.availability()) {
                case CodeCompletion::Available:
                case CodeCompletion::Deprecated:
                    return Icons::iconForType(Icons::SlotPublicIconType);
                case CodeCompletion::NotAccessible:
                case CodeCompletion::NotAvailable:
                    return Icons::iconForType(Icons::SlotPrivateIconType);
            }
        case CodeCompletion::NamespaceCompletionKind:
            return Icons::iconForType(Icons::NamespaceIconType);
        case CodeCompletion::PreProcessorCompletionKind:
            return Icons::iconForType(Icons::MacroIconType);
        case CodeCompletion::VariableCompletionKind:
            switch (m_codeCompletion.availability()) {
                case CodeCompletion::Available:
                case CodeCompletion::Deprecated:
                    return Icons::iconForType(Icons::VarPublicIconType);
                default:
                    return Icons::iconForType(Icons::VarPrivateIconType);
            }
        case CodeCompletion::KeywordCompletionKind:
            return Icons::iconForType(Icons::KeywordIconType);
        case CodeCompletion::ClangSnippetKind:
            return snippetIcon;
        case CodeCompletion::Other:
            return Icons::iconForType(Icons::UnknownIconType);
    }

    return QIcon();
}

QString ClangAssistProposalItem::detail() const
{
    QString detail = CompletionChunksToTextConverter::convertToToolTipWithHtml(m_codeCompletion.chunks());

    if (!m_codeCompletion.briefComment().isEmpty())
        detail += QStringLiteral("\n\n") + m_codeCompletion.briefComment().toString();

    return detail;
}

bool ClangAssistProposalItem::isSnippet() const
{
    return false;
}

bool ClangAssistProposalItem::isValid() const
{
    return true;
}

quint64 ClangAssistProposalItem::hash() const
{
    return 0;
}

void ClangAssistProposalItem::keepCompletionOperator(unsigned compOp)
{
    m_completionOperator = compOp;
}

bool ClangAssistProposalItem::isOverloaded() const
{
    return !m_overloads.isEmpty();
}

void ClangAssistProposalItem::addOverload(const CodeCompletion &ccr)
{
    m_overloads.append(ccr);
}

void ClangAssistProposalItem::setCodeCompletion(const CodeCompletion &codeCompletion)
{
    m_codeCompletion = codeCompletion;
}

const ClangBackEnd::CodeCompletion &ClangAssistProposalItem::codeCompletion() const
{
    return m_codeCompletion;
}

} // namespace Internal
} // namespace ClangCodeModel

