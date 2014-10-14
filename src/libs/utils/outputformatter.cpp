/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "ansiescapecodehandler.h"
#include "outputformatter.h"
#include "theme/theme.h"

#include <QPlainTextEdit>

using namespace Utils;

OutputFormatter::OutputFormatter()
    : QObject()
    , m_plainTextEdit(0)
    , m_formats(0)
    , m_escapeCodeHandler(new AnsiEscapeCodeHandler)
    , m_overwriteOutput(false)
{

}

OutputFormatter::~OutputFormatter()
{
    delete[] m_formats;
    delete m_escapeCodeHandler;
}

QPlainTextEdit *OutputFormatter::plainTextEdit() const
{
    return m_plainTextEdit;
}

void OutputFormatter::setPlainTextEdit(QPlainTextEdit *plainText)
{
    m_plainTextEdit = plainText;
    initFormats();
}

void OutputFormatter::appendMessage(const QString &text, OutputFormat format)
{
    appendMessage(text, m_formats[format]);
}

void OutputFormatter::appendMessage(const QString &text, const QTextCharFormat &format)
{
    QTextCursor cursor(m_plainTextEdit->document());
    cursor.movePosition(QTextCursor::End);

    foreach (const FormattedText &output,
             m_escapeCodeHandler->parseText(FormattedText(text, format))) {
        int startPos = 0;
        int crPos = -1;
        while ((crPos = output.text.indexOf(QLatin1Char('\r'), startPos)) >= 0)  {
            append(cursor, output.text.mid(startPos, crPos - startPos), output.format);
            startPos = crPos + 1;
            m_overwriteOutput = true;
        }
        if (startPos < output.text.count())
            append(cursor, output.text.mid(startPos), output.format);
    }
}

QTextCharFormat OutputFormatter::charFormat(OutputFormat format) const
{
    return m_formats[format];
}

void OutputFormatter::append(QTextCursor &cursor, const QString &text,
                             const QTextCharFormat &format)
{
    if (m_overwriteOutput) {
        cursor.clearSelection();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        m_overwriteOutput = false;
    }
    cursor.insertText(text, format);
}

void OutputFormatter::clearLastLine()
{
    QTextCursor cursor(m_plainTextEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
}

void OutputFormatter::initFormats()
{
    if (!plainTextEdit())
        return;
    QPalette p = plainTextEdit()->palette();

    QFont boldFont = m_font;
    boldFont.setBold(true);

    m_formats = new QTextCharFormat[NumberOfFormats];

    Theme *theme = creatorTheme();

    // NormalMessageFormat
    m_formats[NormalMessageFormat].setFont(boldFont);
    m_formats[NormalMessageFormat].setForeground(theme->color(Theme::OutputFormatter_NormalMessageTextColor));

    // ErrorMessageFormat
    m_formats[ErrorMessageFormat].setFont(boldFont);
    m_formats[ErrorMessageFormat].setForeground(theme->color(Theme::OutputFormatter_ErrorMessageTextColor));

    // StdOutFormat
    m_formats[StdOutFormat].setFont(m_font);
    m_formats[StdOutFormat].setForeground(theme->color(Theme::OutputFormatter_StdOutTextColor));
    m_formats[StdOutFormatSameLine] = m_formats[StdOutFormat];

    // StdErrFormat
    m_formats[StdErrFormat].setFont(m_font);
    m_formats[StdErrFormat].setForeground(theme->color(Theme::OutputFormatter_StdErrTextColor));
    m_formats[StdErrFormatSameLine] = m_formats[StdErrFormat];

    m_formats[DebugFormat].setFont(m_font);
    m_formats[DebugFormat].setForeground(theme->color(Theme::OutputFormatter_DebugTextColor));
}

void OutputFormatter::handleLink(const QString &href)
{
    Q_UNUSED(href);
}

QFont OutputFormatter::font() const
{
    return m_font;
}

void OutputFormatter::setFont(const QFont &font)
{
    m_font = font;
    initFormats();
}

void OutputFormatter::flush()
{
    if (m_escapeCodeHandler)
        m_escapeCodeHandler->endFormatScope();
}
