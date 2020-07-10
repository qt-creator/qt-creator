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
#include "clangfixitoperation.h"
#include "clangutils.h"

#include <cplusplus/Icons.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/Token.h>

#include <texteditor/completionsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>

#include <QCoreApplication>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

#include <utils/algorithm.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace ClangBackEnd;
using namespace TextEditor;
using namespace Utils;

namespace ClangCodeModel {
namespace Internal {

bool ClangAssistProposalItem::prematurelyApplies(const QChar &typedCharacter) const
{
    bool applies = false;

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT)
        applies = QString::fromLatin1("(,").contains(typedCharacter);
    else if (m_completionOperator == T_STRING_LITERAL || m_completionOperator == T_ANGLE_STRING_LITERAL)
        applies = (typedCharacter == QLatin1Char('/')) && text().endsWith(QLatin1Char('/'));
    else if (firstCodeCompletion().completionKind == CodeCompletion::ObjCMessageCompletionKind)
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

static QString textUntilPreviousStatement(TextDocumentManipulatorInterface &manipulator,
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
static bool isAtUsingDeclaration(TextDocumentManipulatorInterface &manipulator,
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

    return contains(tokens, [](const Token &token) { return token.kind() == T_USING; });
}

static QString methodDefinitionParameters(const CodeCompletionChunks &chunks)
{
    QString result;

    auto typedTextChunkIt = std::find_if(chunks.begin(), chunks.end(),
                                         [](const CodeCompletionChunk &chunk) {
        return chunk.kind == CodeCompletionChunk::TypedText;
    });
    if (typedTextChunkIt == chunks.end())
        return result;

    std::for_each(++typedTextChunkIt, chunks.end(), [&result](const CodeCompletionChunk &chunk) {
        if (chunk.kind == CodeCompletionChunk::Placeholder && chunk.text.contains('=')) {
            Utf8String text = chunk.text.mid(0, chunk.text.indexOf('='));
            if (text.endsWith(' '))
                text.chop(1);
            result += text;
        } else {
            result += chunk.text;
        }
    });

    return result;
}

static bool skipParenForFunctionLikeSnippet(const std::vector<int> &placeholderPositions,
                                            const QString &text,
                                            int position)
{
    return placeholderPositions.size() == 1
           && position > 0
           && text[position - 1] == '('
           && text[position] == ')'
           && position + 1 == text.size();
}

static bool isFuncDeclAsSingleTypedText(const CodeCompletion &completion)
{
    // There is no libclang API to tell function call items from declaration items apart.
    // However, the chunks differ for these items (c-index-test -code-completion-at=...):
    //   An (override) declaration (available in derived class scope):
    //     CXXMethod:{TypedText void hello() override} (40)
    //   A function call:
    //     CXXMethod:{ResultType void}{TypedText hello}{LeftParen (}{RightParen )} (36)
    return completion.completionKind == CodeCompletion::FunctionDefinitionCompletionKind
           && completion.chunks.size() == 1
           && completion.chunks[0].kind == CodeCompletionChunk::TypedText;
}

void ClangAssistProposalItem::apply(TextDocumentManipulatorInterface &manipulator,
                                    int basePosition) const
{
    const CodeCompletion ccr = firstCodeCompletion();

    if (!ccr.requiredFixIts.empty()) {
        // Important: Calculate shift before changing the document.
        basePosition += fixItsShift(manipulator);

        ClangFixItOperation fixItOperation(Utf8String(), ccr.requiredFixIts);
        fixItOperation.perform();
    }

    QString textToBeInserted = m_text;
    QString extraCharacters;
    int extraLength = 0;
    int cursorOffset = 0;
    bool setAutoCompleteSkipPos = false;
    int currentPosition = manipulator.currentPosition();

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        extraCharacters += QLatin1Char(')');
        if (m_typedCharacter == QLatin1Char('(')) // Eat the opening parenthesis
            m_typedCharacter = QChar();
    } else if (ccr.completionKind == CodeCompletion::KeywordCompletionKind) {
        CompletionChunksToTextConverter converter;
        converter.setupForKeywords();
        converter.parseChunks(ccr.chunks);

        textToBeInserted = converter.text();

        if (converter.hasPlaceholderPositions()) {
            const std::vector<int> &placeholderPositions = converter.placeholderPositions();
            const int position = placeholderPositions[0];
            cursorOffset = position - converter.text().size();
            // If the snippet looks like a function call, e.g. "sizeof(<PLACEHOLDER>)",
            // ensure that we can "overtype" ')' after inserting it.
            setAutoCompleteSkipPos = skipParenForFunctionLikeSnippet(placeholderPositions,
                                                                     textToBeInserted,
                                                                     position);
        }
    } else if (ccr.completionKind == CodeCompletion::NamespaceCompletionKind) {
        CompletionChunksToTextConverter converter;
        converter.parseChunks(ccr.chunks); // Appends "::" after name space name
        textToBeInserted = converter.text();

        // Clang does not provide the "::" chunk consistently for namespaces, e.g.
        //
        //    namespace a { namespace b { namespace c {} } }
        //    <CURSOR> // complete "a" ==> "a::"
        //    a::<CURSOR> // complete "b" ==> "b", not "b::"
        //
        // Remove "::" to avoid any confusion for now.
        if (textToBeInserted.endsWith("::"))
            textToBeInserted.chop(2);
    } else if (!ccr.text.isEmpty()) {
        const CompletionSettings &completionSettings =
                TextEditorSettings::instance()->completionSettings();
        const bool autoInsertBrackets = completionSettings.m_autoInsertBrackets;

        if (autoInsertBrackets &&
                (ccr.completionKind == CodeCompletion::FunctionCompletionKind
                 || ccr.completionKind == CodeCompletion::FunctionDefinitionCompletionKind
                 || ccr.completionKind == CodeCompletion::DestructorCompletionKind
                 || ccr.completionKind == CodeCompletion::ConstructorCompletionKind
                 || ccr.completionKind == CodeCompletion::SignalCompletionKind
                 || ccr.completionKind == CodeCompletion::SlotCompletionKind)) {
            // When the user typed the opening parenthesis, he'll likely also type the closing one,
            // in which case it would be annoying if we put the cursor after the already automatically
            // inserted closing parenthesis.
            const bool skipClosingParenthesis = m_typedCharacter != QLatin1Char('(');
            QTextCursor cursor = manipulator.textCursorAt(basePosition);

            bool abandonParen = false;
            if (matchPreviousWord(manipulator, cursor, "&")) {
                moveToPreviousWord(manipulator, cursor);
                moveToPreviousChar(manipulator, cursor);
                const QChar prevChar = manipulator.characterAt(cursor.position());
                cursor.setPosition(basePosition);
                abandonParen = QString("(;,{}").contains(prevChar);
            }
            if (!abandonParen) {
                const bool isFullDecl = isFuncDeclAsSingleTypedText(ccr);
                if (isFullDecl)
                    extraCharacters += QLatin1Char(';');
                abandonParen = isAtUsingDeclaration(manipulator, basePosition) || isFullDecl;
            }

            if (!abandonParen && ccr.completionKind == CodeCompletion::FunctionDefinitionCompletionKind) {
                const CodeCompletionChunk resultType = ccr.chunks.first();
                if (resultType.kind == CodeCompletionChunk::ResultType) {
                    if (matchPreviousWord(manipulator, cursor, resultType.text.toString())) {
                        extraCharacters += methodDefinitionParameters(ccr.chunks);
                        // To skip the next block.
                        abandonParen = true;
                    }
                } else {
                    // Do nothing becasue it's not a function definition.

                    // It's a clang bug that the function might miss a ResultType chunk
                    // when the base class method is called from the overriding method
                    // of the derived class. For example:
                    // void Derived::foo() override { Base::<complete here> }
                }
            }
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
                if (!hasOverloadsWithParameters() && !ccr.hasParameters && skipClosingParenthesis) {
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

        if (ccr.completionKind == CodeCompletion::KeywordCompletionKind)
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

const QVector<ClangBackEnd::FixItContainer> &ClangAssistProposalItem::firstCompletionFixIts() const
{
    return firstCodeCompletion().requiredFixIts;
}

std::pair<int, int> fixItPositionsRange(const FixItContainer &fixIt, const QTextCursor &cursor)
{
    const QTextBlock startLine = cursor.document()->findBlockByNumber(fixIt.range.start.line - 1);
    const QTextBlock endLine = cursor.document()->findBlockByNumber(fixIt.range.end.line - 1);

    const int fixItStartPos = Text::positionInText(
                cursor.document(),
                fixIt.range.start.line,
                cppEditorColumn(startLine, fixIt.range.start.column));
    const int fixItEndPos = Text::positionInText(
                cursor.document(),
                fixIt.range.end.line,
                cppEditorColumn(endLine, fixIt.range.end.column));
    return std::make_pair(fixItStartPos, fixItEndPos);
}

static QString textReplacedByFixit(const FixItContainer &fixIt)
{
    TextEditorWidget *textEditorWidget = TextEditorWidget::currentTextEditorWidget();
    if (!textEditorWidget)
        return QString();
    const std::pair<int, int> fixItPosRange = fixItPositionsRange(fixIt,
                                                                  textEditorWidget->textCursor());
    return textEditorWidget->textAt(fixItPosRange.first,
                                    fixItPosRange.second - fixItPosRange.first);
}

QString ClangAssistProposalItem::fixItText() const
{
    const FixItContainer &fixIt = firstCompletionFixIts().first();
    return QCoreApplication::translate("ClangCodeModel::ClangAssistProposalItem",
                                       "Requires changing \"%1\" to \"%2\"")
            .arg(textReplacedByFixit(fixIt), fixIt.text.toString());
}

int ClangAssistProposalItem::fixItsShift(const TextDocumentManipulatorInterface &manipulator) const
{
    const QVector<ClangBackEnd::FixItContainer> &requiredFixIts = firstCompletionFixIts();
    if (requiredFixIts.empty())
        return 0;

    int shift = 0;
    const QTextCursor cursor = manipulator.textCursorAt(0);
    for (const FixItContainer &fixIt : requiredFixIts) {
        const std::pair<int, int> fixItPosRange = fixItPositionsRange(fixIt, cursor);
        shift += fixIt.text.toString().length() - (fixItPosRange.second - fixItPosRange.first);
    }
    return shift;
}

QIcon ClangAssistProposalItem::icon() const
{
    using namespace CPlusPlus::Icons;
    static const char SNIPPET_ICON_PATH[] = ":/texteditor/images/snippet.png";
    static const QIcon snippetIcon = QIcon(QLatin1String(SNIPPET_ICON_PATH));

    const ClangBackEnd::CodeCompletion &completion = firstCodeCompletion();
    switch (completion.completionKind) {
        case CodeCompletion::ClassCompletionKind:
        case CodeCompletion::TemplateClassCompletionKind:
        case CodeCompletion::TypeAliasCompletionKind:
            return CodeModelIcon::iconForType(CodeModelIcon::Class);
        case CodeCompletion::EnumerationCompletionKind:
            return CodeModelIcon::iconForType(CodeModelIcon::Enum);
        case CodeCompletion::EnumeratorCompletionKind:
            return CodeModelIcon::iconForType(CodeModelIcon::Enumerator);
        case CodeCompletion::ConstructorCompletionKind:
        case CodeCompletion::DestructorCompletionKind:
        case CodeCompletion::FunctionCompletionKind:
        case CodeCompletion::FunctionDefinitionCompletionKind:
        case CodeCompletion::TemplateFunctionCompletionKind:
        case CodeCompletion::ObjCMessageCompletionKind:
            switch (completion.availability) {
                case CodeCompletion::Available:
                case CodeCompletion::Deprecated:
                    return CodeModelIcon::iconForType(CodeModelIcon::FuncPublic);
                default:
                    return CodeModelIcon::iconForType(CodeModelIcon::FuncPrivate);
            }
        case CodeCompletion::SignalCompletionKind:
            return CodeModelIcon::iconForType(CodeModelIcon::Signal);
        case CodeCompletion::SlotCompletionKind:
            switch (completion.availability) {
                case CodeCompletion::Available:
                case CodeCompletion::Deprecated:
                    return CodeModelIcon::iconForType(CodeModelIcon::SlotPublic);
                case CodeCompletion::NotAccessible:
                case CodeCompletion::NotAvailable:
                    return CodeModelIcon::iconForType(CodeModelIcon::SlotPrivate);
            }
            break;
        case CodeCompletion::NamespaceCompletionKind:
            return CodeModelIcon::iconForType(CodeModelIcon::Namespace);
        case CodeCompletion::PreProcessorCompletionKind:
            return CodeModelIcon::iconForType(CodeModelIcon::Macro);
        case CodeCompletion::VariableCompletionKind:
            switch (completion.availability) {
                case CodeCompletion::Available:
                case CodeCompletion::Deprecated:
                    return CodeModelIcon::iconForType(CodeModelIcon::VarPublic);
                default:
                    return CodeModelIcon::iconForType(CodeModelIcon::VarPrivate);
            }
        case CodeCompletion::KeywordCompletionKind:
            return CodeModelIcon::iconForType(CodeModelIcon::Keyword);
        case CodeCompletion::ClangSnippetKind:
            return snippetIcon;
        case CodeCompletion::Other:
            return CodeModelIcon::iconForType(CodeModelIcon::Unknown);
        default:
            break;
    }

    return QIcon();
}

QString ClangAssistProposalItem::detail() const
{
    QString detail;
    for (const ClangBackEnd::CodeCompletion &codeCompletion : m_codeCompletions) {
        if (!detail.isEmpty())
            detail += "<br>";
        detail += CompletionChunksToTextConverter::convertToToolTipWithHtml(
                    codeCompletion.chunks, codeCompletion.completionKind);

        if (!codeCompletion.briefComment.isEmpty())
            detail += "<br>" + codeCompletion.briefComment.toString();
    }

    if (requiresFixIts())
        detail += "<br><br><b>" + fixItText() + "</b>";

    return detail;
}

bool ClangAssistProposalItem::isKeyword() const
{
    // KeywordCompletionKind includes real keywords but also "code patterns"/snippets.
    return m_codeCompletions[0].completionKind == CodeCompletion::KeywordCompletionKind;
}

Qt::TextFormat ClangAssistProposalItem::detailFormat() const
{
    return Qt::RichText;
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

bool ClangAssistProposalItem::requiresFixIts() const
{
    return !firstCompletionFixIts().empty();
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

void ClangAssistProposalItem::appendCodeCompletion(const CodeCompletion &codeCompletion)
{
    m_codeCompletions.push_back(codeCompletion);
}

const ClangBackEnd::CodeCompletion &ClangAssistProposalItem::firstCodeCompletion() const
{
    return m_codeCompletions.at(0);
}

void ClangAssistProposalItem::removeFirstCodeCompletion()
{
    QTC_ASSERT(!m_codeCompletions.empty(), return;);
    m_codeCompletions.erase(m_codeCompletions.begin());
}

} // namespace Internal
} // namespace ClangCodeModel
