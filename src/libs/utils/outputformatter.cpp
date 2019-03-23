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

void OutputFormatter::appendMessage(const QString &text, OutputFormat format)
{
    if (!d->cursor.atEnd() && text.startsWith('\n'))
        d->cursor.movePosition(QTextCursor::End);
    appendMessage(text, d->formats[format]);
}

void OutputFormatter::appendMessage(const QString &text, const QTextCharFormat &format)
{
    foreach (const FormattedText &output, parseAnsi(text, format))
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
    int startPos = 0;
    int crPos = -1;
    while ((crPos = text.indexOf('\r', startPos)) >= 0)  {
        if (text.size() > crPos + 1 && text.at(crPos + 1) == '\n') {
            d->cursor.insertText(text.mid(startPos, crPos - startPos) + '\n', format);
            startPos = crPos + 2;
            continue;
        }
        d->cursor.insertText(text.mid(startPos, crPos - startPos), format);
        d->cursor.clearSelection();
        d->cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        startPos = crPos + 1;
    }
    if (startPos < text.count())
        d->cursor.insertText(text.mid(startPos), format);
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

    // NormalMessageFormat
    d->formats[NormalMessageFormat].setForeground(theme->color(Theme::OutputPanes_NormalMessageTextColor));

    // ErrorMessageFormat
    d->formats[ErrorMessageFormat].setForeground(theme->color(Theme::OutputPanes_ErrorMessageTextColor));

    // LogMessageFormat
    d->formats[LogMessageFormat].setForeground(theme->color(Theme::OutputPanes_WarningMessageTextColor));

    // StdOutFormat
    d->formats[StdOutFormat].setForeground(theme->color(Theme::OutputPanes_StdOutTextColor));
    d->formats[StdOutFormatSameLine] = d->formats[StdOutFormat];

    // StdErrFormat
    d->formats[StdErrFormat].setForeground(theme->color(Theme::OutputPanes_StdErrTextColor));
    d->formats[StdErrFormatSameLine] = d->formats[StdErrFormat];

    d->formats[DebugFormat].setForeground(theme->color(Theme::OutputPanes_DebugTextColor));

    setBoldFontEnabled(d->boldFontEnabled);
}

void OutputFormatter::handleLink(const QString &href)
{
    Q_UNUSED(href);
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

} // namespace Utils
