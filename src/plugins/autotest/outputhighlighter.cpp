/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "outputhighlighter.h"
#include "testresultspane.h"

#include <utils/algorithm.h>
#include <utils/theme/theme.h>

#include <QRegularExpression>
#include <QTextDocument>

namespace Autotest {
namespace Internal {

class OutputHighlighterBlockData : public QTextBlockUserData
{
public:
    OutputHighlighterBlockData(const QTextCharFormat &format, OutputChannel channel)
        : lastCharFormat(format), outputChannel(channel) {}

    QTextCharFormat lastCharFormat;
    OutputChannel outputChannel;
};

OutputHighlighter::OutputHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
}

static QColor translateCommandlineColor(const QString &cmdlineEscapeString)
{
    const QColor defaultColor = Utils::creatorTheme()->color(Utils::Theme::PaletteWindowText);
    if (cmdlineEscapeString.isEmpty())
        return defaultColor;
    const QRegularExpression regex("^\\d+(;\\d+)*$");
    QRegularExpressionMatch matcher = regex.match(cmdlineEscapeString);
    if (!matcher.hasMatch())
        return defaultColor;

    QList<int> values = Utils::transform(matcher.captured().split(';'),
                                         [](const QString &str) { return str.toInt(); });
    Utils::sort(values);

    bool isBright = false;
    for (int value : values) {
        if (value < 10) { // special formats (bright/bold, underline,..)
            isBright = value == 1;
            continue;
        }
        switch (value) { // so far only foreground
        case 30: return isBright ? QColor(76, 76, 76) : QColor(0, 0, 0);
        case 31: return isBright ? QColor(205, 0, 0) : QColor(180, 0, 0);
        case 32: return isBright ? QColor(0, 205, 0) : QColor(0, 180, 0);
        case 33: return isBright ? QColor(205, 205, 0) : QColor(180, 180, 0);
        case 34: return isBright ? QColor(30, 140, 255) : QColor(70, 130, 180);
        case 35: return isBright ? QColor(205, 0, 205) : QColor(180, 0, 180);
        case 36: return isBright ? QColor(0, 205, 205) : QColor(0, 180, 180);
        case 37: return isBright ? QColor(255, 255, 255) : QColor(230, 230, 230);
        case 39: return defaultColor; // use default color
        default: continue; // ignore others like background
        }
    }
    return defaultColor;
}

void OutputHighlighter::highlightBlock(const QString &text)
{
    struct PositionsAndColors
    {
        PositionsAndColors(const QRegularExpressionMatch &match)
            : start(match.capturedStart())
            , end(match.capturedEnd())
            , length(match.capturedLength())
            , color(translateCommandlineColor(match.captured(1)))
        {}
        int start;      // start of the escape sequence
        int end;        // end of the escape sequence
        int length;     // length of the escape sequence
        QColor color;   // color to be used
    };

    if (text.isEmpty() || currentBlock().userData())
        return;

    auto resultsPane = TestResultsPane::instance();
    OutputChannel channel = resultsPane->channelForBlockNumber(currentBlock().blockNumber());

    const QRegularExpression pattern("\u001B\\[(.*?)m");
    QTextCursor cursor(currentBlock());

    QList<PositionsAndColors> escapeSequences;
    QRegularExpressionMatchIterator it = pattern.globalMatch(text);
    while (it.hasNext())
        escapeSequences.append(PositionsAndColors(it.next()));

    int removed = 0;
    const int blockPosition = currentBlock().position();

    QTextCharFormat modified = startCharFormat();
    cursor.select(QTextCursor::BlockUnderCursor);
    cursor.setCharFormat(modified);

    for (int i = 0, max = escapeSequences.length(); i < max; ++i) {
        auto seq = escapeSequences.at(i);

        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        if (seq.length > 0) {
            // delete color escape sequence
            cursor.setPosition(blockPosition + seq.start - removed);
            cursor.setPosition(blockPosition + seq.end - removed, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            removed += seq.length;
        }

        // highlight it until next sequence starts or EOL
        if (i < max - 1)
            cursor.setPosition(blockPosition + escapeSequences.at(i + 1).start - removed, QTextCursor::KeepAnchor);
        else
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        modified.setForeground(seq.color);
        cursor.setCharFormat(modified);
    }

    currentBlock().setUserData(new OutputHighlighterBlockData(modified, channel));
}

QTextCharFormat OutputHighlighter::startCharFormat() const
{
    QTextBlock current = currentBlock();
    OutputChannel channel = TestResultsPane::instance()->channelForBlockNumber(
                current.blockNumber());

    do {
        if (auto data = static_cast<OutputHighlighterBlockData *>(current.userData())) {
            if (data->outputChannel == channel)
                return data->lastCharFormat;
        }
        current = current.previous();
    } while (current.isValid());

    QTextCharFormat format = currentBlock().charFormat();
    format.setForeground(Utils::creatorTheme()->color(Utils::Theme::PaletteWindowText));
    return format;
}

} // namespace Internal
} // namespace Autotest
