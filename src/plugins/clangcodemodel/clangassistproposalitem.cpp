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
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/Token.h>

#include <texteditor/completionsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QTextCursor>

#include <utils/algorithm.h>

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
    return true;
}

static void moveToPrevChar(TextEditor::TextDocumentManipulatorInterface &manipulator,
                           QTextCursor &cursor)
{
    cursor.movePosition(QTextCursor::PreviousCharacter);
    while (manipulator.characterAt(cursor.position()).isSpace())
        cursor.movePosition(QTextCursor::PreviousCharacter);
}

static QString textUntilPreviousStatement(TextEditor::TextDocumentManipulatorInterface &manipulator,
                                          int startPosition)
{
    static const QString stopCharacters(";{}#");

    int endPosition = 0;
    for (int i = startPosition; i >= 0 ; --i) {
        if (stopCharacters.contains(manipulator.characterAt(i))) {
            endPosition = i + 1;
            break;
        }
    }

    return manipulator.textAt(endPosition, startPosition - endPosition);
}

// 7.3.3: using typename(opt) nested-name-specifier unqualified-id ;
static bool isAtUsingDeclaration(TextEditor::TextDocumentManipulatorInterface &manipulator,
                                 int basePosition)
{
    SimpleLexer lexer;
    lexer.setLanguageFeatures(LanguageFeatures::defaultFeatures());
    const QString textToLex = textUntilPreviousStatement(manipulator, basePosition);
    const Tokens tokens = lexer(textToLex);
    if (tokens.empty())
        return false;

    // The nested-name-specifier always ends with "::", so check for this first.
    const Token lastToken = tokens[tokens.size() - 1];
    if (lastToken.kind() != T_COLON_COLON)
        return false;

    return Utils::contains(tokens, [](const Token &token) {
        return token.kind() == T_USING;
    });
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
    int currentPosition = manipulator.currentPosition();

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        extraCharacters += QLatin1Char(')');
        if (m_typedCharacter == QLatin1Char('(')) // Eat the opening parenthesis
            m_typedCharacter = QChar();
    } else if (ccr.completionKind() == CodeCompletion::KeywordCompletionKind) {
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
            QTextCursor cursor = manipulator.textCursorAt(basePosition);
            cursor.movePosition(QTextCursor::PreviousWord);
            while (manipulator.characterAt(cursor.position()) == ':')
                cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);

            // Move to the last character in the previous word
            cursor.movePosition(QTextCursor::NextWord);
            moveToPrevChar(manipulator, cursor);
            bool abandonParen = false;
            if (manipulator.characterAt(cursor.position()) == '&') {
                moveToPrevChar(manipulator, cursor);
                const QChar prevChar = manipulator.characterAt(cursor.position());
                abandonParen = QString("(;,{}").contains(prevChar);
            }
            if (!abandonParen)
                abandonParen = isAtUsingDeclaration(manipulator, basePosition);
            if (!abandonParen) {
                if (completionSettings.m_spaceAfterFunctionName)
                    extraCharacters += QLatin1Char(' ');
                extraCharacters += QLatin1Char('(');
                if (m_typedCharacter == QLatin1Char('('))
                    m_typedCharacter = QChar();

                // If the function doesn't return anything, automatically place the semicolon,
                // unless we're doing a scope completion (then it might be function definition).
                const QChar characterAtCursor = manipulator.characterAt(currentPosition);
                bool endWithSemicolon = m_typedCharacter == QLatin1Char(';')/*
                                                || (function->returnType()->isVoidType() && m_completionOperator != T_COLON_COLON)*/; //###
                const QChar semicolon = m_typedCharacter.isNull() ? QLatin1Char(';') : m_typedCharacter;

                if (endWithSemicolon && characterAtCursor == semicolon) {
                    endWithSemicolon = false;
                    m_typedCharacter = QChar();
                }

                // If the function takes no arguments, automatically place the closing parenthesis
                if (!hasOverloadsWithParameters() && !ccr.hasParameters() && skipClosingParenthesis) {
                    extraCharacters += QLatin1Char(')');
                    if (endWithSemicolon) {
                        extraCharacters += semicolon;
                        m_typedCharacter = QChar();
                    }
                } else {
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
        }
    }

    // Append an unhandled typed character, adjusting cursor offset when it had been adjusted before
    if (!m_typedCharacter.isNull()) {
        extraCharacters += m_typedCharacter;
        if (cursorOffset != 0)
            --cursorOffset;
    }

    // Avoid inserting characters that are already there
    QTextCursor cursor = manipulator.textCursorAt(basePosition);
    cursor.movePosition(QTextCursor::EndOfWord);
    const QString textAfterCursor = manipulator.textAt(currentPosition,
                                                       cursor.position() - currentPosition);

    if (textToBeInserted != textAfterCursor
            && textToBeInserted.indexOf(textAfterCursor, currentPosition - basePosition) >= 0) {
        currentPosition = cursor.position();
    }

    for (int i = 0; i < extraCharacters.length(); ++i) {
        const QChar a = extraCharacters.at(i);
        const QChar b = manipulator.characterAt(currentPosition + i);
        if (a == b)
            ++extraLength;
        else
            break;
    }

    textToBeInserted += extraCharacters;

    const int length = currentPosition - basePosition + extraLength;

    const bool isReplaced = manipulator.replace(basePosition, length, textToBeInserted);
    manipulator.setCursorPosition(basePosition + textToBeInserted.length());
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
        default:
            break;
    }

    return QIcon();
}

QString ClangAssistProposalItem::detail() const
{
    QString detail = CompletionChunksToTextConverter::convertToToolTipWithHtml(
                m_codeCompletion.chunks(), m_codeCompletion.completionKind());

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

bool ClangAssistProposalItem::hasOverloadsWithParameters() const
{
    return m_hasOverloadsWithParameters;
}

void ClangAssistProposalItem::setHasOverloadsWithParameters(bool hasOverloadsWithParameters)
{
    m_hasOverloadsWithParameters = hasOverloadsWithParameters;
}

void ClangAssistProposalItem::keepCompletionOperator(unsigned compOp)
{
    m_completionOperator = compOp;
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
