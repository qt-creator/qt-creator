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
#include "utils/optional.h"

#include <QPair>
#include <QPlainTextEdit>
#include <QTextCursor>

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
    bool boldFontEnabled = true;
    bool prependCarriageReturn = false;
};

} // namespace Internal

OutputFormatter::OutputFormatter()
    : d(new Internal::OutputFormatterPrivate)
{
}

OutputFormatter::~OutputFormatter()
{
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

void OutputFormatter::doAppendMessage(const QString &text, OutputFormat format)
{
    if (handleMessage(text, format) == Status::NotHandled)
        appendMessageDefault(text, format);
}

OutputFormatter::Status OutputFormatter::handleMessage(const QString &text, OutputFormat format)
{
    Q_UNUSED(text);
    Q_UNUSED(format);
    return Status::NotHandled;
}

void OutputFormatter::doAppendMessage(const QString &text, const QTextCharFormat &format)
{
    const QList<FormattedText> formattedTextList = parseAnsi(text, format);
    for (const FormattedText &output : formattedTextList)
        append(output.text, output.format);
}

QTextCharFormat OutputFormatter::charFormat(OutputFormat format) const
{
    return d->formats[format];
}

QList<FormattedText> OutputFormatter::parseAnsi(const QString &text, const QTextCharFormat &format)
{
    return d->escapeCodeHandler.parseText(FormattedText(text, format));
}

void OutputFormatter::append(const QString &text, const QTextCharFormat &format)
{
    if (!text.isEmpty())
        d->cursor.insertText(text, format);
}

QTextCursor &OutputFormatter::cursor() const
{
    return d->cursor;
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

void OutputFormatter::overrideTextCharFormat(const QTextCharFormat &fmt)
{
    d->formatOverride = fmt;
}

void OutputFormatter::appendMessageDefault(const QString &text, OutputFormat format)
{
    doAppendMessage(text, d->formatOverride ? d->formatOverride.value() : d->formats[format]);
}

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

bool OutputFormatter::handleLink(const QString &href)
{
    Q_UNUSED(href)
    return false;
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

class AggregatingOutputFormatter::Private
{
public:
    QList<OutputFormatter *> formatters;
    OutputFormatter *nextFormatter = nullptr;
};

AggregatingOutputFormatter::AggregatingOutputFormatter() : d(new Private) {}
AggregatingOutputFormatter::~AggregatingOutputFormatter() { delete d; }

void AggregatingOutputFormatter::setFormatters(const QList<OutputFormatter *> &formatters)
{
    for (OutputFormatter * const f : formatters)
        f->setPlainTextEdit(plainTextEdit());
    d->formatters = formatters;
    d->nextFormatter = nullptr;
}

OutputFormatter::Status AggregatingOutputFormatter::handleMessage(const QString &text,
                                                                  OutputFormat format)
{
    if (d->nextFormatter) {
        switch (d->nextFormatter->handleMessage(text, format)) {
        case Status::Done:
            d->nextFormatter = nullptr;
            return Status::Done;
        case Status::InProgress:
            return Status::InProgress;
        case Status::NotHandled:
            QTC_CHECK(false);
            d->nextFormatter = nullptr;
            return Status::NotHandled;
        }
    }
    QTC_CHECK(!d->nextFormatter);
    for (OutputFormatter * const formatter : qAsConst(d->formatters)) {
        switch (formatter->handleMessage(text, format)) {
        case Status::Done:
            return Status::Done;
        case Status::InProgress:
            d->nextFormatter = formatter;
            return Status::InProgress;
        case Status::NotHandled:
            break;
        }
    }
    return Status::NotHandled;
}

bool AggregatingOutputFormatter::handleLink(const QString &href)
{
    for (OutputFormatter * const f : qAsConst(d->formatters)) {
        if (f->handleLink(href))
            return true;
    }
    return false;
}

} // namespace Utils
