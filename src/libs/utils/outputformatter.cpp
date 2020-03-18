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
#include "synchronousprocess.h"
#include "theme/theme.h"

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
    if (!d->cursor.atEnd() && text.startsWith('\n'))
        d->cursor.movePosition(QTextCursor::End);
    doAppendMessage(text, d->formats[format]);
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

void OutputFormatter::clearLastLine()
{
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

void OutputFormatter::handleLink(const QString &href)
{
    Q_UNUSED(href)
}

void OutputFormatter::clear()
{
    d->prependCarriageReturn = false;
    plainTextEdit()->clear();
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
    d->escapeCodeHandler.endFormatScope();
}

void OutputFormatter::appendMessage(const QString &text, OutputFormat format)
{
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
    doAppendMessage(out, format);
}

} // namespace Utils
