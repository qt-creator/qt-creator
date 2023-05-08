// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlighter.h"

#include "tabsettings.h"
#include "textdocumentlayout.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/DefinitionDownloader>
#include <KSyntaxHighlighting/FoldingRegion>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

#include <QLoggingCategory>
#include <QMetaEnum>

using namespace Utils;

namespace TextEditor {

static Q_LOGGING_CATEGORY(highlighterLog, "qtc.editor.highlighter", QtWarningMsg)

TextStyle categoryForTextStyle(int style)
{
    switch (style) {
    case KSyntaxHighlighting::Theme::Normal: return C_TEXT;
    case KSyntaxHighlighting::Theme::Keyword: return C_KEYWORD;
    case KSyntaxHighlighting::Theme::Function: return C_FUNCTION;
    case KSyntaxHighlighting::Theme::Variable: return C_LOCAL;
    case KSyntaxHighlighting::Theme::ControlFlow: return C_KEYWORD;
    case KSyntaxHighlighting::Theme::Operator: return C_OPERATOR;
    case KSyntaxHighlighting::Theme::BuiltIn: return C_PRIMITIVE_TYPE;
    case KSyntaxHighlighting::Theme::Extension: return C_GLOBAL;
    case KSyntaxHighlighting::Theme::Preprocessor: return C_PREPROCESSOR;
    case KSyntaxHighlighting::Theme::Attribute: return C_LOCAL;
    case KSyntaxHighlighting::Theme::Char: return C_STRING;
    case KSyntaxHighlighting::Theme::SpecialChar: return C_STRING;
    case KSyntaxHighlighting::Theme::String: return C_STRING;
    case KSyntaxHighlighting::Theme::VerbatimString: return C_STRING;
    case KSyntaxHighlighting::Theme::SpecialString: return C_STRING;
    case KSyntaxHighlighting::Theme::Import: return C_PREPROCESSOR;
    case KSyntaxHighlighting::Theme::DataType: return C_TYPE;
    case KSyntaxHighlighting::Theme::DecVal: return C_NUMBER;
    case KSyntaxHighlighting::Theme::BaseN: return C_NUMBER;
    case KSyntaxHighlighting::Theme::Float: return C_NUMBER;
    case KSyntaxHighlighting::Theme::Constant: return C_KEYWORD;
    case KSyntaxHighlighting::Theme::Comment: return C_COMMENT;
    case KSyntaxHighlighting::Theme::Documentation: return C_DOXYGEN_COMMENT;
    case KSyntaxHighlighting::Theme::Annotation: return C_DOXYGEN_TAG;
    case KSyntaxHighlighting::Theme::CommentVar: return C_DOXYGEN_TAG;
    case KSyntaxHighlighting::Theme::RegionMarker: return C_PREPROCESSOR;
    case KSyntaxHighlighting::Theme::Information: return C_WARNING;
    case KSyntaxHighlighting::Theme::Warning: return C_WARNING;
    case KSyntaxHighlighting::Theme::Alert: return C_ERROR;
    case KSyntaxHighlighting::Theme::Error: return C_ERROR;
    case KSyntaxHighlighting::Theme::Others: return C_TEXT;
    }
    return C_TEXT;
}

Highlighter::Highlighter()
{
    setTextFormatCategories(QMetaEnum::fromType<KSyntaxHighlighting::Theme::TextStyle>().keyCount(),
                            &categoryForTextStyle);
}

KSyntaxHighlighting::Definition Highlighter::getDefinition()
{
    return definition();
}

static bool isOpeningParenthesis(QChar c)
{
    return c == QLatin1Char('{') || c == QLatin1Char('[') || c == QLatin1Char('(');
}

static bool isClosingParenthesis(QChar c)
{
    return c == QLatin1Char('}') || c == QLatin1Char(']') || c == QLatin1Char(')');
}

void Highlighter::highlightBlock(const QString &text)
{
    if (!definition().isValid()) {
        formatSpaces(text);
        return;
    }
    QTextBlock block = currentBlock();
    const QTextBlock previousBlock = block.previous();
    TextDocumentLayout::setBraceDepth(block, TextDocumentLayout::braceDepth(previousBlock));
    KSyntaxHighlighting::State previousLineState;
    if (TextBlockUserData *data = TextDocumentLayout::textUserData(previousBlock))
        previousLineState = data->syntaxState();
    KSyntaxHighlighting::State oldState;
    if (TextBlockUserData *data = TextDocumentLayout::textUserData(block)) {
        oldState = data->syntaxState();
        data->setFoldingStartIncluded(false);
        data->setFoldingEndIncluded(false);
    }
    KSyntaxHighlighting::State state = highlightLine(text, previousLineState);
    if (oldState != state) {
        TextBlockUserData *data = TextDocumentLayout::userData(block);
        data->setSyntaxState(state);
        // Toggles the LSB of current block's userState. It forces rehighlight of next block.
        setCurrentBlockState(currentBlockState() ^ 1);
    }

    Parentheses parentheses;
    int pos = 0;
    for (const QChar &c : text) {
        if (isOpeningParenthesis(c))
            parentheses.push_back(Parenthesis(Parenthesis::Opened, c, pos));
        else if (isClosingParenthesis(c))
            parentheses.push_back(Parenthesis(Parenthesis::Closed, c, pos));
        pos++;
    }
    TextDocumentLayout::setParentheses(currentBlock(), parentheses);

    const QTextBlock nextBlock = block.next();
    if (nextBlock.isValid()) {
        TextBlockUserData *data = TextDocumentLayout::userData(nextBlock);
        data->setFoldingIndent(TextDocumentLayout::braceDepth(block));
    }

    formatSpaces(text);
}

void Highlighter::applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format)
{
    const KSyntaxHighlighting::Theme defaultTheme;
    QTextCharFormat qformat = formatForCategory(format.textStyle());

    if (format.hasTextColor(defaultTheme)) {
        const QColor textColor = format.textColor(defaultTheme);
        if (format.hasBackgroundColor(defaultTheme)) {
            const QColor backgroundColor = format.hasBackgroundColor(defaultTheme);
            if (StyleHelper::isReadableOn(backgroundColor, textColor)) {
                qformat.setForeground(textColor);
                qformat.setBackground(backgroundColor);
            } else if (StyleHelper::isReadableOn(qformat.background().color(), textColor)) {
                qformat.setForeground(textColor);
            }
        } else if (StyleHelper::isReadableOn(qformat.background().color(), textColor)) {
            qformat.setForeground(textColor);
        }
    } else if (format.hasBackgroundColor(defaultTheme)) {
        const QColor backgroundColor = format.hasBackgroundColor(defaultTheme);
        if (StyleHelper::isReadableOn(backgroundColor, qformat.foreground().color()))
            qformat.setBackground(backgroundColor);
    }

    if (format.isBold(defaultTheme))
        qformat.setFontWeight(QFont::Bold);

    if (format.isItalic(defaultTheme))
        qformat.setFontItalic(true);

    if (format.isUnderline(defaultTheme))
        qformat.setFontUnderline(true);

    if (format.isStrikeThrough(defaultTheme))
        qformat.setFontStrikeOut(true);
    setFormat(offset, length, qformat);
}

void Highlighter::applyFolding(int offset,
                               int length,
                               KSyntaxHighlighting::FoldingRegion region)
{
    if (!region.isValid())
        return;
    QTextBlock block = currentBlock();
    const QString &text = block.text();
    TextBlockUserData *data = TextDocumentLayout::userData(currentBlock());
    const bool fromStart = TabSettings::firstNonSpace(text) == offset;
    const bool toEnd = (offset + length) == (text.length() - TabSettings::trailingWhitespaces(text));
    if (region.type() == KSyntaxHighlighting::FoldingRegion::Begin) {
        const int newBraceDepth = TextDocumentLayout::braceDepth(block) + 1;
        TextDocumentLayout::setBraceDepth(block, newBraceDepth);
        qCDebug(highlighterLog) << "Found folding start from '" << offset << "' to '" << length
                                << "' resulting in the bracedepth '" << newBraceDepth << "' in :";
        qCDebug(highlighterLog) << text;
        // if there is only a folding begin marker in the line move the current block into the fold
        if (fromStart && toEnd && length <= 1) {
            data->setFoldingIndent(TextDocumentLayout::braceDepth(block));
            data->setFoldingStartIncluded(true);
        }
    } else if (region.type() == KSyntaxHighlighting::FoldingRegion::End) {
        const int newBraceDepth = qMax(0, TextDocumentLayout::braceDepth(block) - 1);
        qCDebug(highlighterLog) << "Found folding end from '" << offset << "' to '" << length
                                << "' resulting in the bracedepth '" << newBraceDepth << "' in :";
        qCDebug(highlighterLog) << text;
        TextDocumentLayout::setBraceDepth(block, newBraceDepth);
        // if the folding end is at the end of the line move the current block into the fold
        if (toEnd)
            data->setFoldingEndIncluded(true);
        else
            data->setFoldingIndent(TextDocumentLayout::braceDepth(block));
    }
}

} // TextEditor
