// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tabsettings.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QTextCursor>
#include <QTextDocument>

static const char spacesForTabsKey[] = "SpacesForTabs";
static const char autoDetectKey[] = "AutoDetect";
static const char tabSizeKey[] = "TabSize";
static const char indentSizeKey[] = "IndentSize";
static const char paddingModeKey[] = "PaddingMode";

using namespace Utils;

namespace TextEditor {

TabSettings::TabSettings(TabSettings::TabPolicy tabPolicy,
                         int tabSize,
                         int indentSize,
                         TabSettings::ContinuationAlignBehavior continuationAlignBehavior)
    : m_tabPolicy(tabPolicy)
    , m_tabSize(tabSize)
    , m_indentSize(indentSize)
    , m_continuationAlignBehavior(continuationAlignBehavior)
{
}

Store TabSettings::toMap() const
{
    return {
        {spacesForTabsKey, m_tabPolicy != TabsOnlyTabPolicy},
        {autoDetectKey, m_autoDetect},
        {tabSizeKey, m_tabSize},
        {indentSizeKey, m_indentSize},
        {paddingModeKey, m_continuationAlignBehavior}
    };
}

void TabSettings::fromMap(const Store &map)
{
    const bool spacesForTabs = map.value(spacesForTabsKey, true).toBool();
    m_autoDetect = map.value(autoDetectKey, true).toBool();
    m_tabPolicy = spacesForTabs ? SpacesOnlyTabPolicy : TabsOnlyTabPolicy;
    m_tabSize = map.value(tabSizeKey, m_tabSize).toInt();
    m_indentSize = map.value(indentSizeKey, m_indentSize).toInt();
    m_continuationAlignBehavior = (ContinuationAlignBehavior)
        map.value(paddingModeKey, m_continuationAlignBehavior).toInt();
}

TabSettings TabSettings::autoDetect(const QTextDocument *document) const
{
    QTC_ASSERT(document, return *this);

    if (!m_autoDetect)
        return *this;

    int totalIndentations = 0;
    int indentationWithTabs = 0;
    QMap<int, int> indentCount;
    QMap<int, int> deltaCount;

    auto checkText =
        [this, document, &totalIndentations, &indentCount, &indentationWithTabs](const QTextBlock &block) {
            if (block.length() == 0)
                return 0;
            int pos = block.position();
            bool hasTabs = false;
            int indentation = 0;
            // iterate ove the characters in the document is faster since we do not have to allocate
            // a string for each block text when we are only interested in the first few characters
            QChar c = document->characterAt(pos);
            while (c.isSpace() && c != QChar::ParagraphSeparator) {
                if (c == QChar::Tabulation) {
                    hasTabs = true;
                    indentation += m_tabSize;
                } else {
                    ++indentation;
                }
                c = document->characterAt(++pos);
            }
            // only track indentations that are at least 2 columns wide
            if (indentation > 1) {
                if (hasTabs)
                    ++indentationWithTabs;
                ++indentCount[indentation];
                ++totalIndentations;
                return indentation;
            }
            return 0;
        };

    const int blockCount = document->blockCount();
    bool useDefault = true;
    if (blockCount < 400) {
        // check the indentation of all blocks if the document is shorter than 400 lines
        int previousIndentation = 0;
        for (QTextBlock block = document->firstBlock(); block.isValid(); block = block.next()) {
            const int indentation = checkText(block);
            const int delta = indentation - previousIndentation;
            if (delta != 0)
                ++deltaCount[qAbs(delta)];
            previousIndentation = indentation;
        }
        // We checked all, so if we find any indented line, it makes sense to use it:
        useDefault = totalIndentations == 0;
    } else {
        // scanning the first and last 25 lines specifically since those most probably contain
        // different indentations
        const int startEndDelta = 25;
        int previousStartIndentation = 0;
        int previousEndIndentation = 0;
        for (int i = 0; i < startEndDelta; ++i) {
            int indentation = checkText(document->findBlockByNumber(i));
            int delta = indentation - previousStartIndentation;
            if (delta != 0)
                ++deltaCount[qAbs(delta)];
            previousStartIndentation = indentation;
            indentation = checkText(document->findBlockByNumber(blockCount - 1 - i));
            delta = indentation - previousEndIndentation;
            if (delta != 0)
                ++deltaCount[qAbs(delta)];
            previousEndIndentation = indentation;
        }

        // scan random lines until we have 200 indentations or checked a maximum of 2000 lines
        // to limit the number of checks for large documents
        QRandomGenerator gen(QDateTime::currentMSecsSinceEpoch());
        int checks = 0;
        QMap<int, int> alreadyCheckedIndentations;
        while (totalIndentations < 200) {
            ++checks;
            if (checks > 2000)
                break;
            const int blockNumber = gen.bounded(startEndDelta + 1, blockCount - startEndDelta - 12);
            auto it = alreadyCheckedIndentations.find(blockNumber);
            bool alreadyChecked = it != alreadyCheckedIndentations.end();
            if (!alreadyChecked) {
                it = alreadyCheckedIndentations
                         .insert(blockNumber, checkText(document->findBlockByNumber(blockNumber)));
            };
            int previousIndentation = it.value();
            for (int i = 1; i <= 10; ++i) {
                auto it = alreadyCheckedIndentations.find(blockNumber + i);
                if (it != alreadyCheckedIndentations.end()) {
                    if (alreadyChecked)
                        continue;
                    alreadyChecked = true;
                } else {
                    it = alreadyCheckedIndentations.insert(
                        blockNumber + i, checkText(document->findBlockByNumber(blockNumber + i)));
                }
                const int indentation = it.value();
                const int delta = indentation - previousIndentation;
                if (delta != 0)
                    ++deltaCount[qAbs(delta)];
                previousIndentation = indentation;
            }
        }
        // Don't determine indentation for the whole file from few actually indented lines that we
        // managed to find:
        useDefault = totalIndentations < 3;
    }

    if (useDefault)
        return *this;

    // find the most common delta
    int mostCommonDelta = 0;
    int mostCommonDeltaCount = 0;
    for (auto it = deltaCount.cbegin(); it != deltaCount.cend(); ++it) {
        if (const int count = it.value(); count > mostCommonDeltaCount) {
            mostCommonDeltaCount = count;
            mostCommonDelta = it.key();
        }
    }

    // find the most common indent
    int mostCommonIndent = 0;
    int mostCommonIndentCount = 0;
    for (auto it = indentCount.cbegin(); it != indentCount.cend(); ++it) {
        if (const int count = it.value(); count > mostCommonIndentCount) {
            mostCommonIndentCount = count;
            mostCommonIndent = it.key();
        }
    }

    // check whether the most common delta is a fraction of the most common indent
    if (mostCommonDeltaCount >= 4 && mostCommonIndent % mostCommonDelta == 0) {
        TabSettings result = *this;
        result.m_indentSize = mostCommonDelta;
        double relativeTabCount = double(indentationWithTabs) / double(totalIndentations);
        result.m_tabPolicy = relativeTabCount > 0.5 ? TabSettings::TabsOnlyTabPolicy
                                                    : TabSettings::SpacesOnlyTabPolicy;
        return result;
    }

    for (auto it = indentCount.cbegin(); it != indentCount.cend(); ++it) {
        // check whether the smallest indent is a fraction of the most common indent
        // to filter out some false positives
        if (mostCommonIndent % it.key() == 0) {
            TabSettings result = *this;
            result.m_indentSize = it.key();
            double relativeTabCount = double(indentationWithTabs) / double(totalIndentations);
            result.m_tabPolicy = relativeTabCount > 0.5 ? TabSettings::TabsOnlyTabPolicy
                                                        : TabSettings::SpacesOnlyTabPolicy;
            return result;
        }
    }
    return *this;
}

bool TabSettings::cursorIsAtBeginningOfLine(const QTextCursor &cursor)
{
    QString text = cursor.block().text();
    int fns = firstNonSpace(text);
    return (cursor.position() - cursor.block().position() <= fns);
}

int TabSettings::lineIndentPosition(const QString &text) const
{
    int i = 0;
    while (i < text.size()) {
        if (!text.at(i).isSpace())
            break;
        ++i;
    }
    int column = columnAt(text, i);
    return i - (column % m_indentSize);
}

int TabSettings::firstNonSpace(const QString &text)
{
    int i = 0;
    while (i < text.size()) {
        if (!text.at(i).isSpace())
            return i;
        ++i;
    }
    return i;
}

QString TabSettings::indentationString(const QString &text)
{
    return text.left(firstNonSpace(text));
}


int TabSettings::indentationColumn(const QString &text) const
{
    return columnAt(text, firstNonSpace(text));
}

int TabSettings::maximumPadding(const QString &text)
{
    int fns = firstNonSpace(text);
    int i = fns;
    while (i > 0) {
        if (text.at(i-1) != QLatin1Char(' '))
            break;
        --i;
    }
    return fns - i;
}


int TabSettings::trailingWhitespaces(const QString &text)
{
    int i = 0;
    while (i < text.size()) {
        if (!text.at(text.size()-1-i).isSpace())
            return i;
        ++i;
    }
    return i;
}

void TabSettings::removeTrailingWhitespace(QTextCursor cursor, QTextBlock &block)
{
    if (const int trailing = trailingWhitespaces(block.text())) {
        cursor.setPosition(block.position() + block.length() - 1);
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, trailing);
        cursor.removeSelectedText();
    }
}

bool TabSettings::isIndentationClean(const QTextBlock &block, const int indent) const
{
    int i = 0;
    int spaceCount = 0;
    QString text = block.text();
    while (i < text.size()) {
        QChar c = text.at(i);
        if (!c.isSpace())
            return true;

        if (c == QLatin1Char(' ')) {
            ++spaceCount;
            if (spaceCount == m_tabSize)
                if (m_tabPolicy == TabsOnlyTabPolicy)
                    if ((m_continuationAlignBehavior != ContinuationAlignWithSpaces) || (i < indent))
                        return false;
            if (spaceCount > indent && m_continuationAlignBehavior == NoContinuationAlign)
                return false;
        } else if (c == QLatin1Char('\t')) {
            if (m_tabPolicy == SpacesOnlyTabPolicy || (spaceCount != 0))
                return false;
            if ((m_continuationAlignBehavior != ContinuationAlignWithIndent) && ((i + 1) * m_tabSize > indent))
                return false;
        }
        ++i;
    }
    return true;
}

int TabSettings::columnAt(const QString &text, int position) const
{
    int column = 0;
    for (int i = 0; i < position; ++i) {
        if (text.at(i) == QLatin1Char('\t'))
            column = column - (column % m_tabSize) + m_tabSize;
        else
            ++column;
    }
    return column;
}

int TabSettings::columnAtCursorPosition(const QTextCursor &cursor) const
{
    return columnAt(cursor.block().text(), cursor.positionInBlock());
}

int TabSettings::positionAtColumn(const QString &text, int column, int *offset, bool allowOverstep) const
{
    int col = 0;
    int i = 0;
    int textSize = text.size();
    while ((i < textSize || allowOverstep) && col < column) {
        if (i < textSize && text.at(i) == QLatin1Char('\t'))
            col = col - (col % m_tabSize) + m_tabSize;
        else
            ++col;
        ++i;
    }
    if (offset)
        *offset = column - col;
    return i;
}

int TabSettings::columnCountForText(const QString &text, int startColumn) const
{
    int column = startColumn;
    for (auto c : text) {
        if (c == QLatin1Char('\t'))
            column = column - (column % m_tabSize) + m_tabSize;
        else
            ++column;
    }
    return column - startColumn;
}

int TabSettings::spacesLeftFromPosition(const QString &text, int position)
{
    if (position > text.size())
        return 0;
    int i = position;
    while (i > 0) {
        if (!text.at(i-1).isSpace())
            break;
        --i;
    }
    return position - i;
}

int TabSettings::indentedColumn(int column, bool doIndent) const
{
    int aligned = (column / m_indentSize) * m_indentSize;
    if (doIndent)
        return aligned + m_indentSize;
    if (aligned < column)
        return aligned;
    return qMax(0, aligned - m_indentSize);
}

QString TabSettings::indentationString(int startColumn, int targetColumn, int padding) const
{
    targetColumn = qMax(startColumn, targetColumn);
    if (m_tabPolicy == SpacesOnlyTabPolicy)
        return QString(targetColumn - startColumn, QLatin1Char(' '));

    QString s;
    int alignedStart = startColumn == 0 ? 0 : startColumn - (startColumn % m_tabSize) + m_tabSize;
    if (alignedStart > startColumn && alignedStart <= targetColumn) {
        s += QLatin1Char('\t');
        startColumn = alignedStart;
    }
    if (m_continuationAlignBehavior == NoContinuationAlign) {
        targetColumn -= padding;
        padding = 0;
    } else if (m_continuationAlignBehavior == ContinuationAlignWithIndent) {
        padding = 0;
    }
    const int columns = targetColumn - padding - startColumn;
    const int tabs = columns / m_tabSize;
    s += QString(tabs, QLatin1Char('\t'));
    s += QString(targetColumn - startColumn - tabs * m_tabSize, QLatin1Char(' '));
    return s;
}

void TabSettings::indentLine(const QTextBlock &block, int newIndent, int padding) const
{
    const QString text = block.text();
    const int oldBlockLength = text.size();

    if (m_continuationAlignBehavior == NoContinuationAlign) {
        newIndent -= padding;
        padding = 0;
    } else if (m_continuationAlignBehavior == ContinuationAlignWithIndent) {
        padding = 0;
    }

    // Quickly check whether indenting is required.
    // fixme: after changing "use spaces for tabs" the change was not reflected
    // because of the following optimisation. Commenting it out for now.
//    if (indentationColumn(text) == newIndent)
//        return;

    const QString indentString = indentationString(0, newIndent, padding);

    if (oldBlockLength == indentString.length() && text == indentString)
        return;

    QTextCursor cursor(block);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, firstNonSpace(text));
    cursor.removeSelectedText();
    cursor.insertText(indentString);
    cursor.endEditBlock();
}

void TabSettings::reindentLine(QTextBlock block, int delta) const
{
    const QString text = block.text();
    const int oldBlockLength = text.size();

    int oldIndent = indentationColumn(text);
    int newIndent = qMax(oldIndent + delta, 0);

    if (oldIndent == newIndent)
        return;

    int padding = 0;
    // user likes tabs for spaces and uses tabs for indentation, preserve padding
    if (m_tabPolicy == TabsOnlyTabPolicy && m_tabSize == m_indentSize)
        padding = qMin(maximumPadding(text), newIndent);
    const QString indentString = indentationString(0, newIndent, padding);

    if (oldBlockLength == indentString.length() && text == indentString)
        return;

    QTextCursor cursor(block);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, firstNonSpace(text));
    cursor.removeSelectedText();
    cursor.insertText(indentString);
    cursor.endEditBlock();
}

bool TabSettings::equals(const TabSettings &ts) const
{
    return m_autoDetect == ts.m_autoDetect
        && m_tabPolicy == ts.m_tabPolicy
        && m_tabSize == ts.m_tabSize
        && m_indentSize == ts.m_indentSize
        && m_continuationAlignBehavior == ts.m_continuationAlignBehavior;
}

static TabSettings::Retriever g_retriever = [](const FilePath &) { return TabSettings{}; };

void TabSettings::setRetriever(const Retriever &retriever) { g_retriever = retriever; }

TabSettings TabSettings::settingsForFile(const Utils::FilePath &filePath)
{
    return g_retriever(filePath);
}

} // namespace TextEditor
