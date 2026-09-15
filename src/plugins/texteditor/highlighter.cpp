// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlighter.h"

#include "spellchecksettings.h"
#include "tabsettings.h"
#include "textdocumentlayout.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/stylehelper.h>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/DefinitionDownloader>
#include <KSyntaxHighlighting/FoldingRegion>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

#include <QLoggingCategory>
#include <QMetaEnum>

#include <algorithm>

using namespace Utils;

namespace TextEditor {

static Q_LOGGING_CATEGORY(highlighterLog, "qtc.editor.highlighter", QtWarningMsg)

static TextStyle categoryForTextStyle(int style)
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
    case KSyntaxHighlighting::Theme::Attribute: return C_ATTRIBUTE;
    case KSyntaxHighlighting::Theme::Char: return C_STRING;
    case KSyntaxHighlighting::Theme::SpecialChar: return C_STRING;
    case KSyntaxHighlighting::Theme::String: return C_STRING;
    case KSyntaxHighlighting::Theme::VerbatimString: return C_STRING;
    case KSyntaxHighlighting::Theme::SpecialString: return C_STRING;
    case KSyntaxHighlighting::Theme::Import: return C_MACRO;
    case KSyntaxHighlighting::Theme::DataType: return C_TYPE;
    case KSyntaxHighlighting::Theme::DecVal: return C_NUMBER;
    case KSyntaxHighlighting::Theme::BaseN: return C_NUMBER;
    case KSyntaxHighlighting::Theme::Float: return C_NUMBER;
    case KSyntaxHighlighting::Theme::Constant: return C_ENUMERATION;
    case KSyntaxHighlighting::Theme::Comment: return C_COMMENT;
    case KSyntaxHighlighting::Theme::Documentation: return C_DOXYGEN_COMMENT;
    case KSyntaxHighlighting::Theme::Annotation: return C_DOXYGEN_TAG;
    case KSyntaxHighlighting::Theme::CommentVar: return C_DOXYGEN_TAG;
    case KSyntaxHighlighting::Theme::RegionMarker: return C_PREPROCESSOR;
    case KSyntaxHighlighting::Theme::Information: return C_INFO;
    case KSyntaxHighlighting::Theme::Warning: return C_WARNING;
    case KSyntaxHighlighting::Theme::Alert: return C_ERROR_CONTEXT;
    case KSyntaxHighlighting::Theme::Error: return C_ERROR;
    case KSyntaxHighlighting::Theme::Others: return C_TEXT;
    }
    return C_TEXT;
}

Highlighter::Highlighter()
{
    setTextFormatCategories(QMetaEnum::fromType<KSyntaxHighlighting::Theme::TextStyle>().keyCount(),
                            &categoryForTextStyle);
    followSpellCheckSettings(this);
}

Highlighter::~Highlighter() = default;

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
        // A file no syntax definition matches holds nothing but prose.
        addProseRange(0, text.size());
        formatSpaces(text);
        spellCheck(text);
        return;
    }
    QTextBlock block = currentBlock();
    const QTextBlock previousBlock = block.previous();
    TextBlockUserData::setBraceDepth(block, TextBlockUserData::braceDepth(previousBlock));
    KSyntaxHighlighting::State previousLineState = TextBlockUserData::syntaxState(previousBlock);
    KSyntaxHighlighting::State oldState = TextBlockUserData::syntaxState(block);
    setFoldingStartIncluded(block, false);
    setFoldingEndIncluded(block, false);
    KSyntaxHighlighting::State state = highlightLine(text, previousLineState);
    if (oldState != state) {
        TextBlockUserData::setSyntaxState(block, state);
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
    TextBlockUserData::setParentheses(currentBlock(), parentheses);

    const QTextBlock nextBlock = block.next();
    if (nextBlock.isValid())
        setFoldingIndent(nextBlock, TextBlockUserData::braceDepth(block));

    formatSpaces(text);
    spellCheck(text);
}

void Highlighter::setDefinition(const KSyntaxHighlighting::Definition &definition)
{
    m_proseMarkingFormats.reset();
    KSyntaxHighlighting::AbstractHighlighter::setDefinition(definition);
}

// The categories that hold prose whichever language a file is written in. The normal
// text of a file is not among them: it is the code of every language that styles
// nothing else, and a file that no syntax definition matches is prose on other grounds.
static bool holdsProseInAnyLanguage(TextStyle category)
{
    return category == C_COMMENT || category == C_DOXYGEN_COMMENT;
}

// A syntax definition says of every format whether it holds prose, and the ones that
// come with Qt Creator turn it off for what is code: the commands of a CMake file, the
// link target of a Markdown one. The flag stays on where nothing sets it, so a
// definition that turns it off for no format of code - a Ruby one turns it off for the
// escape sequences of a string and for nothing else - was written without spell
// checking in mind, and only the styles that hold prose in any language are read from
// it.
static bool namesProse(const QList<KSyntaxHighlighting::Format> &formats)
{
    return std::any_of(
        formats.cbegin(), formats.cend(), [](const KSyntaxHighlighting::Format &format) {
            const TextStyle category = categoryForTextStyle(format.textStyle());
            return !format.spellCheck() && category != C_STRING
                   && !holdsProseInAnyLanguage(category);
        });
}

// The formats of a file come from the definition of its language and from the ones that
// definition embeds, and each of them answers for its own: the block a Markdown file
// fences off as YAML is as much YAML as a file of it is, down to the definition behind
// it having been written without spell checking in mind.
bool Highlighter::definitionMarksProse(const KSyntaxHighlighting::Format &format)
{
    if (!m_proseMarkingFormats) {
        m_proseMarkingFormats.emplace();
        // includedDefinitions() leaves out the definition it is asked of.
        QList<KSyntaxHighlighting::Definition> definitions = definition().includedDefinitions();
        definitions.prepend(definition());
        for (const KSyntaxHighlighting::Definition &language : definitions) {
            const QList<KSyntaxHighlighting::Format> formats = language.formats();
            if (!namesProse(formats))
                continue;
            for (const KSyntaxHighlighting::Format &marking : formats)
                m_proseMarkingFormats->insert(marking.id());
        }
    }
    return m_proseMarkingFormats->contains(format.id());
}

static bool isProse(const KSyntaxHighlighting::Format &format, bool checkStrings, bool marksProse)
{
    if (!format.spellCheck())
        return false;
    const TextStyle category = categoryForTextStyle(format.textStyle());
    if (category == C_STRING)
        return checkStrings;
    return marksProse || holdsProseInAnyLanguage(category);
}

void Highlighter::applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format)
{
    if (!spellCheckLanguage().isEmpty()
        && isProse(format, spellCheckStrings(), definitionMarksProse(format))) {
        addProseRange(offset, length);
    }

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
    const bool fromStart = TabSettingsData::firstNonSpace(text) == offset;
    const bool toEnd = (offset + length) == (text.size() - TabSettingsData::trailingWhitespaces(text));
    if (region.type() == KSyntaxHighlighting::FoldingRegion::Begin) {
        const int newBraceDepth = TextBlockUserData::braceDepth(block) + 1;
        TextBlockUserData::setBraceDepth(block, newBraceDepth);
        qCDebug(highlighterLog) << "Found folding start from '" << offset << "' to '" << length
                                << "' resulting in the bracedepth '" << newBraceDepth << "' in :";
        qCDebug(highlighterLog) << text;
        // if there is only a folding begin marker in the line move the current block into the fold
        if (fromStart && toEnd && length <= 1) {
            setFoldingIndent(block, TextBlockUserData::braceDepth(block));
            setFoldingStartIncluded(block, true);
        }
    } else if (region.type() == KSyntaxHighlighting::FoldingRegion::End) {
        const int newBraceDepth = qMax(0, TextBlockUserData::braceDepth(block) - 1);
        qCDebug(highlighterLog) << "Found folding end from '" << offset << "' to '" << length
                                << "' resulting in the bracedepth '" << newBraceDepth << "' in :";
        qCDebug(highlighterLog) << text;
        TextBlockUserData::setBraceDepth(block, newBraceDepth);
        // if the folding end is at the end of the line move the current block into the fold
        if (toEnd)
            setFoldingEndIncluded(block, true);
        else
            setFoldingIndent(block, TextBlockUserData::braceDepth(block));
    }
}

} // TextEditor
