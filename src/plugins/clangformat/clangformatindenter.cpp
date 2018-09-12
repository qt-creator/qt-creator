/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangformatindenter.h"

#include <clang/Format/Format.h>
#include <clang/Tooling/Core/Replacement.h>

#include <coreplugin/icore.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/hostosinfo.h>

#include <QDir>
#include <QFileInfo>
#include <QTextBlock>

using namespace clang;
using namespace format;
using namespace llvm;
using namespace tooling;
using namespace ProjectExplorer;
using namespace TextEditor;

namespace ClangFormat {
namespace Internal {

namespace {

void adjustFormatStyleForLineBreak(format::FormatStyle &style,
                                   int length,
                                   int prevBlockSize,
                                   bool prevBlockEndsWithPunctuation)
{
    if (length > 0)
        style.ColumnLimit = prevBlockSize;
    style.AlwaysBreakBeforeMultilineStrings = true;
    style.AlwaysBreakTemplateDeclarations = true;
    style.AllowAllParametersOfDeclarationOnNextLine = true;
    style.AllowShortBlocksOnASingleLine = true;
    style.AllowShortCaseLabelsOnASingleLine = true;
    style.AllowShortFunctionsOnASingleLine = FormatStyle::SFS_Empty;
    style.AllowShortIfStatementsOnASingleLine = true;
    style.AllowShortLoopsOnASingleLine = true;
    if (prevBlockEndsWithPunctuation) {
        style.BreakBeforeBinaryOperators = FormatStyle::BOS_None;
        style.BreakBeforeTernaryOperators = false;
        style.BreakConstructorInitializers = FormatStyle::BCIS_AfterColon;
    } else {
        style.BreakBeforeBinaryOperators = FormatStyle::BOS_All;
        style.BreakBeforeTernaryOperators = true;
        style.BreakConstructorInitializers = FormatStyle::BCIS_BeforeComma;
    }
}

Replacements filteredReplacements(const Replacements &replacements,
                                  unsigned int offset,
                                  unsigned int lengthForFilter,
                                  int extraOffsetToAdd,
                                  int prevBlockLength)
{
    Replacements filtered;
    for (const Replacement &replacement : replacements) {
        unsigned int replacementOffset = replacement.getOffset();
        if (replacementOffset > offset + lengthForFilter)
            break;

        if (offset > static_cast<unsigned int>(prevBlockLength)
                && replacementOffset < offset - static_cast<unsigned int>(prevBlockLength))
            continue;

        if (lengthForFilter == 0 && replacement.getReplacementText().find('\n') == std::string::npos
            && filtered.empty()) {
            continue;
        }

        if (replacementOffset + 1 >= offset)
            replacementOffset += static_cast<unsigned int>(extraOffsetToAdd);

        Error error = filtered.add(Replacement(replacement.getFilePath(),
                                               replacementOffset,
                                               replacement.getLength(),
                                               replacement.getReplacementText()));
        // Throws if error is not checked.
        if (error)
            break;
    }
    return filtered;
}

std::string assumedFilePath()
{
    const Project *project = SessionManager::startupProject();
    if (project && project->projectDirectory().appendPath(".clang-format").exists())
        return project->projectDirectory().appendPath("test.cpp").toString().toStdString();

    return QString(Core::ICore::userResourcePath() + "/test.cpp").toStdString();
}

FormatStyle formatStyle()
{
    Expected<FormatStyle> style = format::getStyle("file", assumedFilePath(), "none", "");
    if (style)
        return *style;
    return FormatStyle();
}

Replacements replacements(const std::string &buffer,
                          unsigned int offset,
                          unsigned int length,
                          bool blockFormatting = false,
                          const QChar &typedChar = QChar::Null,
                          int extraOffsetToAdd = 0,
                          int prevBlockLength = 1,
                          bool prevBlockEndsWithPunctuation = false)
{
    FormatStyle style = formatStyle();

    if (blockFormatting && typedChar == QChar::Null)
        adjustFormatStyleForLineBreak(style, length, prevBlockLength, prevBlockEndsWithPunctuation);

    std::vector<Range> ranges{{offset, length}};
    FormattingAttemptStatus status;

    Replacements replacements = reformat(style, buffer, ranges, assumedFilePath(), &status);

    if (!status.FormatComplete)
        Replacements();

    unsigned int lengthForFilter = 0;
    if (!blockFormatting)
        lengthForFilter = length;

    return filteredReplacements(replacements,
                                offset,
                                lengthForFilter,
                                extraOffsetToAdd,
                                prevBlockLength);
}

void applyReplacements(QTextDocument *doc,
                       const std::string &stdStrBuffer,
                       const tooling::Replacements &replacements,
                       int totalShift)
{
    if (replacements.empty())
        return;

    QTextCursor editCursor(doc);
    int fullOffsetDiff = 0;
    for (const Replacement &replacement : replacements) {
        const int utf16Offset
            = QString::fromStdString(stdStrBuffer.substr(0, replacement.getOffset())).length()
              + totalShift + fullOffsetDiff;
        const int utf16Length = QString::fromStdString(stdStrBuffer.substr(replacement.getOffset(),
                                                                           replacement.getLength()))
                                    .length();
        const QString replacementText = QString::fromStdString(replacement.getReplacementText());

        editCursor.beginEditBlock();
        editCursor.setPosition(utf16Offset);
        editCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, utf16Length);
        editCursor.removeSelectedText();
        editCursor.insertText(replacementText);
        editCursor.endEditBlock();
        fullOffsetDiff += replacementText.length() - utf16Length;
    }
}

// Returns offset shift.
int modifyToIndentEmptyLines(QString &buffer, int &offset, int &length, const QTextBlock &block)
{
    //This extra text works for the most cases.
    QString extraText("a;");

    const QString blockText = block.text().trimmed();
    // Search for previous character
    QTextBlock prevBlock = block.previous();
    while (prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty())
        prevBlock = prevBlock.previous();
    if (prevBlock.text().endsWith(','))
        extraText = "int a,";

    const bool closingParenBlock = blockText.startsWith(')');
    if (closingParenBlock) {
        if (prevBlock.text().endsWith(','))
            extraText = "int a";
        else
            extraText = "&& a";
    }

    if (length == 0 || closingParenBlock) {
        length += extraText.length();
        buffer.insert(offset, extraText);
    }

    if (blockText.startsWith('}')) {
        buffer.insert(offset - 1, extraText);
        offset += extraText.size();
        return extraText.size();
    }

    return 0;
}

// Returns first non-empty block (searches from current block backwards).
QTextBlock clearFirstNonEmptyBlockFromExtraSpaces(const QTextBlock &currentBlock)
{
    QTextBlock prevBlock = currentBlock.previous();
    while (prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty())
        prevBlock = prevBlock.previous();

    if (prevBlock.text().trimmed().isEmpty())
        return prevBlock;

    const QString initialText = prevBlock.text();
    if (!initialText.at(initialText.length() - 1).isSpace())
        return prevBlock;

    QTextCursor cursor(prevBlock);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.movePosition(QTextCursor::Left);

    while (cursor.positionInBlock() >= 0 && initialText.at(cursor.positionInBlock()).isSpace())
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    if (cursor.hasSelection())
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.endEditBlock();
    return prevBlock;
}

// Returns the total langth of previous lines with pure whitespace.
int previousEmptyLinesLength(const QTextBlock &currentBlock)
{
    int length{0};
    QTextBlock prevBlock = currentBlock.previous();
    while (prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty()) {
        length += prevBlock.text().length() + 1;
        prevBlock = prevBlock.previous();
    }

    return length;
}

static constexpr const int MinCharactersBeforeCurrentInBuffer = 200;
static constexpr const int MaxCharactersBeforeCurrentInBuffer = 500;

int startOfIndentationBuffer(const QString &buffer, int start)
{
    if (start < MaxCharactersBeforeCurrentInBuffer)
        return 0;

    auto it = buffer.cbegin() + (start - MinCharactersBeforeCurrentInBuffer);
    for (; it != buffer.cbegin() + (start - MaxCharactersBeforeCurrentInBuffer); --it) {
        if (*it == '{') {
            // Find the start of it's line.
            for (auto inner = it;
                 inner != buffer.cbegin() + (start - MaxCharactersBeforeCurrentInBuffer);
                 --inner) {
                if (*inner == '\n')
                    return inner + 1 - buffer.cbegin();
            }
            break;
        }
    }

    return it - buffer.cbegin();
}

int nextEndingScopePosition(const QString &buffer, int start)
{
    if (start >= buffer.size() - 1)
        return buffer.size() - 1;

    for (auto it = buffer.cbegin() + (start + 1); it != buffer.cend(); ++it) {
        if (*it == '}')
            return it - buffer.cbegin();
    }

    return buffer.size() - 1;
}

} // anonymous namespace

bool ClangFormatIndenter::isElectricCharacter(const QChar &ch) const
{
    switch (ch.toLatin1()) {
    case '{':
    case '}':
    case ':':
    case '#':
    case '<':
    case '>':
    case ';':
    case '(':
    case ')':
    case ',':
    case '.':
        return true;
    }
    return false;
}

void ClangFormatIndenter::indent(QTextDocument *doc,
                                 const QTextCursor &cursor,
                                 const QChar &typedChar,
                                 const TabSettings &tabSettings,
                                 bool autoTriggered)
{
    if (typedChar == QChar::Null && (cursor.hasSelection() || !autoTriggered)) {
        TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget();
        int offset;
        int length;
        if (cursor.hasSelection()) {
            const QTextBlock start = doc->findBlock(cursor.selectionStart());
            const QTextBlock end = doc->findBlock(cursor.selectionEnd());
            offset = start.position();
            length = std::max(0, end.position() + end.length() - start.position() - 1);
        } else {
            const QTextBlock block = cursor.block();
            offset = block.position();
            length = std::max(0, block.length() - 1);
        }
        QString buffer = editor->toPlainText();
        const int totalShift = startOfIndentationBuffer(buffer, offset);
        const int cutAtPos = nextEndingScopePosition(buffer, offset + length) + 1;
        buffer = buffer.mid(totalShift, cutAtPos - totalShift);
        offset -= totalShift;
        const std::string stdStrBefore = buffer.left(offset).toStdString();
        const std::string stdStrBuffer = stdStrBefore + buffer.mid(offset).toStdString();
        applyReplacements(doc,
                          stdStrBuffer,
                          replacements(stdStrBuffer, stdStrBefore.length(), length),
                          totalShift);
    } else {
        indentBlock(doc, cursor.block(), typedChar, tabSettings);
    }
}

void ClangFormatIndenter::reindent(QTextDocument *doc,
                                   const QTextCursor &cursor,
                                   const TabSettings &tabSettings)
{
    indent(doc, cursor, QChar::Null, tabSettings);
}

void ClangFormatIndenter::indentBlock(QTextDocument *doc,
                                      const QTextBlock &block,
                                      const QChar &typedChar,
                                      const TabSettings &tabSettings)
{
    Q_UNUSED(tabSettings);

    TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget();
    if (!editor)
        return;

    const QTextBlock prevBlock = clearFirstNonEmptyBlockFromExtraSpaces(block);

    int offset = block.position();
    int length = std::max(0, block.length() - 1);
    QString buffer = editor->toPlainText();

    int emptySpaceLength = previousEmptyLinesLength(block);
    offset -= emptySpaceLength;
    buffer.remove(offset, emptySpaceLength);

    int extraPrevBlockLength{0};
    int prevBlockTextLength{1};
    bool prevBlockEndsWithPunctuation = false;
    if (typedChar == QChar::Null) {
        extraPrevBlockLength = modifyToIndentEmptyLines(buffer, offset, length, block);

        const QString prevBlockText = prevBlock.text();
        prevBlockEndsWithPunctuation = prevBlockText.size() > 0
                                           ? prevBlockText.at(prevBlockText.size() - 1).isPunct()
                                           : false;
        prevBlockTextLength = prevBlockText.size() + 1;
    }

    const int totalShift = startOfIndentationBuffer(buffer, offset);
    const int cutAtPos = nextEndingScopePosition(buffer, offset + length) + 1;
    buffer = buffer.mid(totalShift, cutAtPos - totalShift);
    offset -= totalShift;
    const std::string stdStrBefore = buffer.left(offset).toStdString();
    const std::string stdStrBuffer = stdStrBefore + buffer.mid(offset).toStdString();
    applyReplacements(doc,
                      stdStrBuffer,
                      replacements(stdStrBuffer,
                                   stdStrBefore.length(),
                                   length,
                                   true,
                                   typedChar,
                                   emptySpaceLength - extraPrevBlockLength,
                                   prevBlockTextLength + extraPrevBlockLength,
                                   prevBlockEndsWithPunctuation),
                      totalShift);
}

int ClangFormatIndenter::indentFor(const QTextBlock &block, const TextEditor::TabSettings &)
{
    TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget();
    if (!editor)
        return -1;

    const QTextBlock prevBlock = clearFirstNonEmptyBlockFromExtraSpaces(block);

    int offset = block.position();
    int length = std::max(0, block.length() - 1);
    QString buffer = editor->toPlainText();

    int emptySpaceLength = previousEmptyLinesLength(block);
    offset -= emptySpaceLength;
    buffer.replace(offset, emptySpaceLength, "");
    int extraPrevBlockLength = modifyToIndentEmptyLines(buffer, offset, length, block);

    const QString prevBlockText = prevBlock.text();
    bool prevBlockEndsWithPunctuation = prevBlockText.size() > 0
                                            ? prevBlockText.at(prevBlockText.size() - 1).isPunct()
                                            : false;

    const int totalShift = startOfIndentationBuffer(buffer, offset);
    const int cutAtPos = nextEndingScopePosition(buffer, offset + length) + 1;
    buffer = buffer.mid(totalShift, cutAtPos - totalShift);
    offset -= totalShift;
    const std::string stdStrBefore = buffer.left(offset).toStdString();
    const std::string stdStrBuffer = stdStrBefore + buffer.mid(offset).toStdString();
    Replacements toReplace = replacements(stdStrBuffer,
                                          stdStrBefore.length(),
                                          length,
                                          true,
                                          QChar::Null,
                                          emptySpaceLength - extraPrevBlockLength,
                                          prevBlockText.size() + extraPrevBlockLength + 1,
                                          prevBlockEndsWithPunctuation);

    if (toReplace.empty())
        return -1;

    const Replacement replacement = *toReplace.begin();

    const StringRef text = replacement.getReplacementText();
    size_t afterLineBreak = text.find_last_of('\n');
    afterLineBreak = (afterLineBreak == std::string::npos) ? 0 : afterLineBreak + 1;
    return static_cast<int>(text.size() - afterLineBreak);
}

TabSettings ClangFormatIndenter::tabSettings() const
{
    FormatStyle style = formatStyle();
    TabSettings tabSettings;

    switch (style.UseTab) {
    case FormatStyle::UT_Never:
        tabSettings.m_tabPolicy = TabSettings::SpacesOnlyTabPolicy;
        break;
    case FormatStyle::UT_Always:
        tabSettings.m_tabPolicy = TabSettings::TabsOnlyTabPolicy;
        break;
    default:
        tabSettings.m_tabPolicy = TabSettings::MixedTabPolicy;
    }

    tabSettings.m_tabSize = style.TabWidth;
    tabSettings.m_indentSize = style.IndentWidth;

    if (style.AlignAfterOpenBracket)
        tabSettings.m_continuationAlignBehavior = TabSettings::ContinuationAlignWithSpaces;
    else
        tabSettings.m_continuationAlignBehavior = TabSettings::ContinuationAlignWithIndent;

    return tabSettings;
}

} // namespace Internal
} // namespace ClangFormat
