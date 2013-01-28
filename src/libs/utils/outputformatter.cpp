/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "outputformatter.h"

#include <QPlainTextEdit>
#include <QColor>

#include <QString>

using namespace Utils;

OutputFormatter::OutputFormatter()
    : QObject()
    , m_plainTextEdit(0)
    , m_formats(0)
{

}

OutputFormatter::~OutputFormatter()
{
    delete[] m_formats;
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
    QTextCursor cursor(m_plainTextEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, m_formats[format]);
}

QTextCharFormat OutputFormatter::charFormat(OutputFormat format) const
{
    return m_formats[format];
}

void OutputFormatter::clearLastLine()
{
    QTextCursor cursor(m_plainTextEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
}

QColor OutputFormatter::mixColors(const QColor &a, const QColor &b)
{
    return QColor((a.red() + 2 * b.red()) / 3, (a.green() + 2 * b.green()) / 3,
                  (a.blue() + 2* b.blue()) / 3, (a.alpha() + 2 * b.alpha()) / 3);
}

void OutputFormatter::initFormats()
{
    if (!plainTextEdit())
        return;
    QPalette p = plainTextEdit()->palette();

    QFont boldFont = m_font;
    boldFont.setBold(true);

    m_formats = new QTextCharFormat[NumberOfFormats];

    // NormalMessageFormat
    m_formats[NormalMessageFormat].setFont(boldFont);
    m_formats[NormalMessageFormat].setForeground(mixColors(p.color(QPalette::Text), QColor(Qt::blue)));

    // ErrorMessageFormat
    m_formats[ErrorMessageFormat].setFont(boldFont);
    m_formats[ErrorMessageFormat].setForeground(mixColors(p.color(QPalette::Text), QColor(Qt::red)));

    // StdOutFormat
    m_formats[StdOutFormat].setFont(m_font);
    m_formats[StdOutFormat].setForeground(p.color(QPalette::Text));
    m_formats[StdOutFormatSameLine] = m_formats[StdOutFormat];

    // StdErrFormat
    m_formats[StdErrFormat].setFont(m_font);
    m_formats[StdErrFormat].setForeground(mixColors(p.color(QPalette::Text), QColor(Qt::red)));
    m_formats[StdErrFormatSameLine] = m_formats[StdErrFormat];

    m_formats[DebugFormat].setFont(m_font);
    m_formats[DebugFormat].setForeground(mixColors(p.color(QPalette::Text), QColor(Qt::magenta)));
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
