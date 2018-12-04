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

#include "clangformatutils.h"

#include <clang/Format/Format.h>
#include <clang/Tooling/Core/Replacement.h>

#include <cpptools/cppmodelmanager.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/hostosinfo.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

#include <llvm/Config/llvm-config.h>

#include <QDir>
#include <QFileInfo>
#include <QTextBlock>

#include <fstream>

using namespace clang;
using namespace format;
using namespace llvm;
using namespace tooling;
using namespace ProjectExplorer;
using namespace TextEditor;

namespace ClangFormat {

namespace {

void adjustFormatStyleForLineBreak(format::FormatStyle &style)
{
    style.DisableFormat = false;
    style.ColumnLimit = 0;
#ifdef KEEP_LINE_BREAKS_FOR_NON_EMPTY_LINES_BACKPORTED
    style.KeepLineBreaksForNonEmptyLines = true;
#endif
    style.MaxEmptyLinesToKeep = 2;
    style.SortIncludes = false;
    style.SortUsingDeclarations = false;
}

StringRef clearExtraNewline(StringRef text)
{
    while (text.startswith("\n\n"))
        text = text.drop_front();
    return text;
}

Replacements filteredReplacements(const Replacements &replacements,
                                  int offset,
                                  int extraOffsetToAdd,
                                  bool onlyIndention)
{
    Replacements filtered;
    for (const Replacement &replacement : replacements) {
        int replacementOffset = static_cast<int>(replacement.getOffset());
        if (onlyIndention && replacementOffset != offset - 1)
            continue;

        if (replacementOffset + 1 >= offset)
            replacementOffset += extraOffsetToAdd;

        StringRef text = onlyIndention ? clearExtraNewline(replacement.getReplacementText())
                                       : replacement.getReplacementText();

        Error error = filtered.add(Replacement(replacement.getFilePath(),
                                               static_cast<unsigned int>(replacementOffset),
                                               replacement.getLength(),
                                               text));
        // Throws if error is not checked.
        if (error)
            break;
    }
    return filtered;
}

void trimFirstNonEmptyBlock(const QTextBlock &currentBlock)
{
    QTextBlock prevBlock = currentBlock.previous();
    while (prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty())
        prevBlock = prevBlock.previous();

    if (prevBlock.text().trimmed().isEmpty())
        return;

    const QString initialText = prevBlock.text();
    if (!initialText.at(initialText.size() - 1).isSpace())
        return;

    auto lastNonSpace = std::find_if_not(initialText.rbegin(),
                                         initialText.rend(),
                                         [](const QChar &letter) {
        return letter.isSpace();
    });
    const int extraSpaceCount = static_cast<int>(std::distance(initialText.rbegin(), lastNonSpace));

    QTextCursor cursor(prevBlock);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                        initialText.size() - extraSpaceCount);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, extraSpaceCount);
    cursor.removeSelectedText();
    cursor.endEditBlock();
}

void trimCurrentBlock(const QTextBlock &currentBlock)
{
    if (currentBlock.text().trimmed().isEmpty()) {
        // Clear the block containing only spaces
        QTextCursor cursor(currentBlock);
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.endEditBlock();
    }
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

void modifyToIndentEmptyLines(QByteArray &buffer, int &offset, int &length, const QTextBlock &block)
{
    const QString blockText = block.text().trimmed();
    const bool closingParenBlock = blockText.startsWith(')');
    if (!blockText.isEmpty() && !closingParenBlock)
        return;

    //This extra text works for the most cases.
    QByteArray extraText("a;");

    // Search for previous character
    QTextBlock prevBlock = block.previous();
    while (prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty())
        prevBlock = prevBlock.previous();
    if (prevBlock.text().endsWith(','))
        extraText = "int a,";

    if (closingParenBlock) {
        if (prevBlock.text().endsWith(','))
            extraText = "int a";
        else
            extraText = "&& a";
    }

    length += extraText.length();
    buffer.insert(offset, extraText);
}

static const int kMaxLinesFromCurrentBlock = 200;

Replacements replacements(const Utils::FileName &fileName,
                          QByteArray buffer,
                          int utf8Offset,
                          int utf8Length,
                          const QTextBlock *block = nullptr,
                          const QChar &typedChar = QChar::Null)
{
    FormatStyle style = styleForFile(fileName);

    int extraOffset = 0;
    if (block) {
        if (block->blockNumber() > kMaxLinesFromCurrentBlock) {
            extraOffset = Utils::Text::utf8NthLineOffset(
                        block->document(), buffer, block->blockNumber() - kMaxLinesFromCurrentBlock);
        }
        int endOffset = Utils::Text::utf8NthLineOffset(
            block->document(), buffer, block->blockNumber() + kMaxLinesFromCurrentBlock);
        if (endOffset == -1)
            endOffset = buffer.size();

        buffer = buffer.mid(extraOffset, endOffset - extraOffset);
        utf8Offset -= extraOffset;

        const int emptySpaceLength = previousEmptyLinesLength(*block);
        utf8Offset -= emptySpaceLength;
        buffer.remove(utf8Offset, emptySpaceLength);

        extraOffset += emptySpaceLength;

        adjustFormatStyleForLineBreak(style);
        if (typedChar == QChar::Null)
            modifyToIndentEmptyLines(buffer, utf8Offset, utf8Length, *block);
    }

    std::vector<Range> ranges{{static_cast<unsigned int>(utf8Offset),
                               static_cast<unsigned int>(utf8Length)}};
    FormattingAttemptStatus status;

    Replacements replacements = reformat(style, buffer.data(), ranges,
                                         fileName.toString().toStdString(), &status);

    if (!status.FormatComplete)
        Replacements();

    return filteredReplacements(replacements,
                                utf8Offset,
                                extraOffset,
                                block);
}

Utils::LineColumn utf16LineColumn(const QTextBlock &block,
                                  int blockOffsetUtf8,
                                  const QByteArray &utf8Buffer,
                                  int utf8Offset)
{
    // If lastIndexOf('\n') returns -1 then we are fine to add 1 and get 0 offset.
    const int lineStartUtf8Offset = utf8Buffer.lastIndexOf('\n', utf8Offset - 1) + 1;
    int line = block.blockNumber() + 1; // Init with the line corresponding the block.

    if (utf8Offset < blockOffsetUtf8) {
        line -= static_cast<int>(std::count(utf8Buffer.begin() + lineStartUtf8Offset,
                                            utf8Buffer.begin() + blockOffsetUtf8,
                                            '\n'));
    } else {
        line += static_cast<int>(std::count(utf8Buffer.begin() + blockOffsetUtf8,
                                            utf8Buffer.begin() + lineStartUtf8Offset,
                                            '\n'));
    }

    const QByteArray lineText = utf8Buffer.mid(lineStartUtf8Offset,
                                               utf8Offset - lineStartUtf8Offset);
    return Utils::LineColumn(line, QString::fromUtf8(lineText).size() + 1);
}

tooling::Replacements utf16Replacements(const QTextBlock &block,
                                        int blockOffsetUtf8,
                                        const QByteArray &utf8Buffer,
                                        const tooling::Replacements &replacements)
{
    tooling::Replacements convertedReplacements;
    for (const Replacement &replacement : replacements) {
        const Utils::LineColumn lineColUtf16 = utf16LineColumn(
                    block, blockOffsetUtf8, utf8Buffer, static_cast<int>(replacement.getOffset()));
        if (!lineColUtf16.isValid())
            continue;
        const int utf16Offset = Utils::Text::positionInText(block.document(),
                                                            lineColUtf16.line,
                                                            lineColUtf16.column);
        const int utf16Length = QString::fromUtf8(
                    utf8Buffer.mid(static_cast<int>(replacement.getOffset()),
                                   static_cast<int>(replacement.getLength()))).size();
        Error error = convertedReplacements.add(
                    Replacement(replacement.getFilePath(),
                                static_cast<unsigned int>(utf16Offset),
                                static_cast<unsigned int>(utf16Length),
                                replacement.getReplacementText()));
        // Throws if error is not checked.
        if (error)
            break;
    }

    return convertedReplacements;
}

void applyReplacements(const QTextBlock &block,
                       int blockOffsetUtf8,
                       const QByteArray &utf8Buffer,
                       const tooling::Replacements &replacements)
{
    if (replacements.empty())
        return;

    tooling::Replacements convertedReplacements = utf16Replacements(block,
                                                                    blockOffsetUtf8,
                                                                    utf8Buffer,
                                                                    replacements);

    int fullOffsetShift = 0;
    QTextCursor editCursor(block);
    for (const Replacement &replacement : convertedReplacements) {
        const QString replacementString = QString::fromStdString(replacement.getReplacementText());
        const int replacementLength = static_cast<int>(replacement.getLength());
        editCursor.beginEditBlock();
        editCursor.setPosition(static_cast<int>(replacement.getOffset()) + fullOffsetShift);
        editCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                replacementLength);
        editCursor.removeSelectedText();
        editCursor.insertText(replacementString);
        editCursor.endEditBlock();
        fullOffsetShift += replacementString.length() - replacementLength;
    }
}

QString selectedLines(QTextDocument *doc, const QTextBlock &startBlock, const QTextBlock &endBlock)
{
    QString text = Utils::Text::textAt(
                QTextCursor(doc),
                startBlock.position(),
                std::max(0, endBlock.position() + endBlock.length() - startBlock.position() - 1));
    while (!text.isEmpty() && text.rbegin()->isSpace())
        text.chop(1);
    return text;
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
                                 bool /*autoTriggered*/)
{
    if (cursor.hasSelection()) {
        // Calling currentBlock.next() might be unsafe because we change the document.
        // Let's operate with block numbers instead.
        const int startNumber = doc->findBlock(cursor.selectionStart()).blockNumber();
        const int endNumber = doc->findBlock(cursor.selectionEnd()).blockNumber();
        for (int currentBlockNumber = startNumber; currentBlockNumber <= endNumber;
             ++currentBlockNumber) {
            const QTextBlock currentBlock = doc->findBlockByNumber(currentBlockNumber);
            if (currentBlock.isValid()) {
                const int blocksAmount = doc->blockCount();
                indentBlock(doc, currentBlock, typedChar, tabSettings);
                QTC_CHECK(blocksAmount == doc->blockCount()
                          && "ClangFormat plugin indentation changed the amount of blocks.");
            }
        }
    } else {
        indentBlock(doc, cursor.block(), typedChar, tabSettings);
    }
}

void ClangFormatIndenter::format(QTextDocument *doc,
                                 const QTextCursor &cursor,
                                 const TextEditor::TabSettings &/*tabSettings*/)
{
    TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget();
    if (!editor)
        return;

    const Utils::FileName fileName = editor->textDocument()->filePath();
    int utf8Offset;
    int utf8Length;
    const QByteArray buffer = doc->toPlainText().toUtf8();
    if (cursor.hasSelection()) {
        const QTextBlock start = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd());
        utf8Offset = Utils::Text::utf8NthLineOffset(doc, buffer, start.blockNumber() + 1);
        QTC_ASSERT(utf8Offset >= 0, return;);
        utf8Length = selectedLines(doc, start, end).toUtf8().size();
        applyReplacements(start,
                          utf8Offset,
                          buffer,
                          replacements(fileName, buffer, utf8Offset, utf8Length));
    } else {
        const QTextBlock block = cursor.block();
        utf8Offset = Utils::Text::utf8NthLineOffset(doc, buffer, block.blockNumber() + 1);
        QTC_ASSERT(utf8Offset >= 0, return;);
        utf8Length = block.text().toUtf8().size();
        applyReplacements(block,
                          utf8Offset,
                          buffer,
                          replacements(fileName, buffer, utf8Offset, utf8Length));
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

    const Utils::FileName fileName = editor->textDocument()->filePath();
    trimFirstNonEmptyBlock(block);
    trimCurrentBlock(block);
    const QByteArray buffer = doc->toPlainText().toUtf8();
    const int utf8Offset = Utils::Text::utf8NthLineOffset(doc, buffer, block.blockNumber() + 1);
    QTC_ASSERT(utf8Offset >= 0, return;);

    applyReplacements(block,
                      utf8Offset,
                      buffer,
                      replacements(fileName, buffer, utf8Offset, 0, &block, typedChar));
}

int ClangFormatIndenter::indentFor(const QTextBlock &block, const TextEditor::TabSettings &)
{
    TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget();
    if (!editor)
        return -1;

    const Utils::FileName fileName = editor->textDocument()->filePath();
    trimFirstNonEmptyBlock(block);
    trimCurrentBlock(block);
    const QTextDocument *doc = block.document();
    const QByteArray buffer = doc->toPlainText().toUtf8();
    const int utf8Offset = Utils::Text::utf8NthLineOffset(doc, buffer, block.blockNumber() + 1);
    QTC_ASSERT(utf8Offset >= 0, return 0;);

    Replacements toReplace = replacements(fileName, buffer, utf8Offset, 0, &block);

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
    FormatStyle style = currentProjectStyle();
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

    tabSettings.m_tabSize = static_cast<int>(style.TabWidth);
    tabSettings.m_indentSize = static_cast<int>(style.IndentWidth);

    if (style.AlignAfterOpenBracket == FormatStyle::BAS_DontAlign)
        tabSettings.m_continuationAlignBehavior = TabSettings::NoContinuationAlign;
    else
        tabSettings.m_continuationAlignBehavior = TabSettings::ContinuationAlignWithIndent;

    return tabSettings;
}

} // namespace ClangFormat
