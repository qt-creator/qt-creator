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

#include "ansiescapecodehandler.h"
#include "outputformatter.h"
#include "qtcassert.h"
#include "synchronousprocess.h"
#include "theme/theme.h"

#include <QPair>
#include <QPlainTextEdit>
#include <QTextCursor>

#include <numeric>

namespace Utils {

namespace Internal {

class OutputFormatterPrivate
{
public:
    QPlainTextEdit *plainTextEdit = nullptr;
    QTextCharFormat formats[NumberOfFormats];
    QTextCursor cursor;
    AnsiEscapeCodeHandler escapeCodeHandler;
    QPair<QString, OutputFormat> incompleteLine;
    optional<QTextCharFormat> formatOverride;
    QList<OutputLineParser *> lineParsers;
    OutputLineParser *nextParser = nullptr;
    bool boldFontEnabled = true;
    bool prependCarriageReturn = false;
};

} // namespace Internal

OutputLineParser::~OutputLineParser()
{
}

OutputFormatter::OutputFormatter()
    : d(new Internal::OutputFormatterPrivate)
{
}

OutputFormatter::~OutputFormatter()
{
    qDeleteAll(d->lineParsers);
    delete d;
}

QPlainTextEdit *OutputFormatter::plainTextEdit() const
{
    return d->plainTextEdit;
}

void OutputFormatter::setPlainTextEdit(QPlainTextEdit *plainText)
{
    d->plainTextEdit = plainText;
    d->cursor = plainText ? plainText->textCursor() : QTextCursor();
    d->cursor.movePosition(QTextCursor::End);
    initFormats();
}

void OutputFormatter::setLineParsers(const QList<OutputLineParser *> &parsers)
{
    flush();
    qDeleteAll(d->lineParsers);
    d->lineParsers = parsers;
    d->nextParser = nullptr;
}

void OutputFormatter::doAppendMessage(const QString &text, OutputFormat format)
{
    const QTextCharFormat charFmt = charFormat(format);
    const QList<FormattedText> formattedText = parseAnsi(text, charFmt);
    const QString cleanLine = std::accumulate(formattedText.begin(), formattedText.end(), QString(),
            [](const FormattedText &t1, const FormattedText &t2) { return t1.text + t2.text; });
    const OutputLineParser::Result res = handleMessage(cleanLine, format);
    if (res.newContent) {
        append(res.newContent.value(), charFmt);
        return;
    }
    for (const FormattedText &output : linkifiedText(formattedText, res.linkSpecs))
        append(output.text, output.format);
}

OutputLineParser::Result OutputFormatter::handleMessage(const QString &text, OutputFormat format)
{
    if (d->nextParser) {
        const OutputLineParser::Result res = d->nextParser->handleLine(text, format);
        switch (res.status) {
        case OutputLineParser::Status::Done:
            d->nextParser = nullptr;
            return res;
        case OutputLineParser::Status::InProgress:
            return res;
        case OutputLineParser::Status::NotHandled:
            QTC_CHECK(false); // TODO: This case will be legal after the merge
            d->nextParser = nullptr;
            return res;
        }
    }
    QTC_CHECK(!d->nextParser);
    for (OutputLineParser * const parser : qAsConst(d->lineParsers)) {
        const OutputLineParser::Result res = parser->handleLine(text, format);
        switch (res.status) {
        case OutputLineParser::Status::Done:
            return res;
        case OutputLineParser::Status::InProgress:
            d->nextParser = parser;
            return res;
        case OutputLineParser::Status::NotHandled:
            break;
        }
    }
    return OutputLineParser::Status::NotHandled;
}

void OutputFormatter::reset()
{
    for (OutputLineParser * const p : d->lineParsers)
        p->reset();
}

QTextCharFormat OutputFormatter::charFormat(OutputFormat format) const
{
    return d->formatOverride ? d->formatOverride.value() : d->formats[format];
}

QList<FormattedText> OutputFormatter::parseAnsi(const QString &text, const QTextCharFormat &format)
{
    return d->escapeCodeHandler.parseText(FormattedText(text, format));
}

const QList<FormattedText> OutputFormatter::linkifiedText(
        const QList<FormattedText> &text, const OutputLineParser::LinkSpecs &linkSpecs)
{
    if (linkSpecs.isEmpty())
        return text;

    QList<FormattedText> linkified;
    int totalTextLengthSoFar = 0;
    int nextLinkSpecIndex = 0;

    for (const FormattedText &t : text) {

        // There is no more linkification work to be done. Just copy the text as-is.
        if (nextLinkSpecIndex >= linkSpecs.size()) {
            linkified << t;
            continue;
        }

        for (int nextLocalTextPos = 0; nextLocalTextPos < t.text.size(); ) {

            // There are no more links in this part, so copy the rest of the text as-is.
            if (nextLinkSpecIndex >= linkSpecs.size()) {
                linkified << FormattedText(t.text.mid(nextLocalTextPos), t.format);
                totalTextLengthSoFar += t.text.length() - nextLocalTextPos;
                break;
            }

            const OutputLineParser::LinkSpec &linkSpec = linkSpecs.at(nextLinkSpecIndex);
            const int localLinkStartPos = linkSpec.startPos - totalTextLengthSoFar;
            ++nextLinkSpecIndex;

            // We ignore links that would cross format boundaries.
            if (localLinkStartPos < nextLocalTextPos
                    || localLinkStartPos + linkSpec.length > t.text.length()) {
                linkified << FormattedText(t.text.mid(nextLocalTextPos), t.format);
                totalTextLengthSoFar += t.text.length() - nextLocalTextPos;
                break;
            }

            // Now we know we have a link that is fully inside this part of the text.
            // Split the text so that the link part gets the appropriate format.
            const int prefixLength = localLinkStartPos - nextLocalTextPos;
            const QString textBeforeLink = t.text.mid(nextLocalTextPos, prefixLength);
            linkified << FormattedText(textBeforeLink, t.format);
            const QString linkedText = t.text.mid(localLinkStartPos, linkSpec.length);
            linkified << FormattedText(linkedText, linkFormat(t.format, linkSpec.target));
            nextLocalTextPos = localLinkStartPos + linkSpec.length;
            totalTextLengthSoFar += prefixLength + linkSpec.length;
        }
    }
    return linkified;
}

void OutputFormatter::append(const QString &text, const QTextCharFormat &format)
{
    int startPos = 0;
    int crPos = -1;
    while ((crPos = text.indexOf('\r', startPos)) >= 0)  {
        d->cursor.insertText(text.mid(startPos, crPos - startPos), format);
        d->cursor.clearSelection();
        d->cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        startPos = crPos + 1;
    }
    if (startPos < text.count())
        d->cursor.insertText(text.mid(startPos), format);
}

QTextCharFormat OutputFormatter::linkFormat(const QTextCharFormat &inputFormat, const QString &href)
{
    QTextCharFormat result = inputFormat;
    result.setForeground(creatorTheme()->color(Theme::TextColorLink));
    result.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    result.setAnchor(true);
    result.setAnchorHref(href);
    return result;
}

#ifdef WITH_TESTS
void OutputFormatter::overrideTextCharFormat(const QTextCharFormat &fmt)
{
    d->formatOverride = fmt;
}
#endif // WITH_TESTS

void OutputFormatter::clearLastLine()
{
    // Note that this approach will fail if the text edit is not read-only and users
    // have messed with the last line between programmatic inputs.
    // We live with this risk, as all the alternatives are worse.
    if (!d->cursor.atEnd())
        d->cursor.movePosition(QTextCursor::End);
    d->cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    d->cursor.removeSelectedText();
}

void OutputFormatter::initFormats()
{
    if (!plainTextEdit())
        return;

    Theme *theme = creatorTheme();
    d->formats[NormalMessageFormat].setForeground(theme->color(Theme::OutputPanes_NormalMessageTextColor));
    d->formats[ErrorMessageFormat].setForeground(theme->color(Theme::OutputPanes_ErrorMessageTextColor));
    d->formats[LogMessageFormat].setForeground(theme->color(Theme::OutputPanes_WarningMessageTextColor));
    d->formats[StdOutFormat].setForeground(theme->color(Theme::OutputPanes_StdOutTextColor));
    d->formats[StdErrFormat].setForeground(theme->color(Theme::OutputPanes_StdErrTextColor));
    d->formats[DebugFormat].setForeground(theme->color(Theme::OutputPanes_DebugTextColor));
    setBoldFontEnabled(d->boldFontEnabled);
}

void OutputFormatter::flushIncompleteLine()
{
    clearLastLine();
    doAppendMessage(d->incompleteLine.first, d->incompleteLine.second);
    d->incompleteLine.first.clear();
}

void OutputFormatter::dumpIncompleteLine(const QString &line, OutputFormat format)
{
    append(line, charFormat(format));
    d->incompleteLine.first.append(line);
    d->incompleteLine.second = format;
}

void OutputFormatter::handleLink(const QString &href)
{
    for (OutputLineParser * const f : qAsConst(d->lineParsers)) {
        if (f->handleLink(href))
            return;
    }
}

void OutputFormatter::clear()
{
    d->prependCarriageReturn = false;
    d->incompleteLine.first.clear();
    plainTextEdit()->clear();
    reset();
}

void OutputFormatter::setBoldFontEnabled(bool enabled)
{
    d->boldFontEnabled = enabled;
    const QFont::Weight fontWeight = enabled ? QFont::Bold : QFont::Normal;
    d->formats[NormalMessageFormat].setFontWeight(fontWeight);
    d->formats[ErrorMessageFormat].setFontWeight(fontWeight);
}

void OutputFormatter::flush()
{
    if (!d->incompleteLine.first.isEmpty())
        flushIncompleteLine();
    d->escapeCodeHandler.endFormatScope();
    reset();
}

void OutputFormatter::appendMessage(const QString &text, OutputFormat format)
{
    // If we have an existing incomplete line and its format is different from this one,
    // then we consider the two messages unrelated. We re-insert the previous incomplete line,
    // possibly formatted now, and start from scratch with the new input.
    if (!d->incompleteLine.first.isEmpty() && d->incompleteLine.second != format)
        flushIncompleteLine();

    QString out = text;
    if (d->prependCarriageReturn) {
        d->prependCarriageReturn = false;
        out.prepend('\r');
    }
    out = SynchronousProcess::normalizeNewlines(out);
    if (out.endsWith('\r')) {
        d->prependCarriageReturn = true;
        out.chop(1);
    }

    // If the input is a single incomplete line, we do not forward it to the specialized
    // formatting code, but simply dump it as-is. Once it becomes complete or it needs to
    // be flushed for other reasons, we remove the unformatted part and re-insert it, this
    // time with proper formatting.
    if (!out.contains('\n')) {
        dumpIncompleteLine(out, format);
        return;
    }

    // We have at least one complete line, so let's remove the previously dumped
    // incomplete line and prepend it to the first line of our new input.
    if (!d->incompleteLine.first.isEmpty()) {
        clearLastLine();
        out.prepend(d->incompleteLine.first);
        d->incompleteLine.first.clear();
    }

    // Forward all complete lines to the specialized formatting code, and handle a
    // potential trailing incomplete line the same way as above.
    for (int startPos = 0; ;) {
        const int eolPos = out.indexOf('\n', startPos);
        if (eolPos == -1) {
            dumpIncompleteLine(out.mid(startPos), format);
            break;
        }
        doAppendMessage(out.mid(startPos, eolPos - startPos + 1), format);
        startPos = eolPos + 1;
    }
}

} // namespace Utils
