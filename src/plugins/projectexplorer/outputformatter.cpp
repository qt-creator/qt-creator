/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "outputformatter.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QtGui/QPlainTextEdit>
#include <QtGui/QColor>

#include <QtCore/QString>

using namespace ProjectExplorer;
using namespace TextEditor;

OutputFormatter::OutputFormatter()
    : QObject()
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
    QPalette p = plainTextEdit()->palette();

    FontSettings fs = TextEditorSettings::instance()->fontSettings();
    QFont font = fs.font();
    QFont boldFont = font;
    boldFont.setBold(true);

    m_formats = new QTextCharFormat[NumberOfFormats];

    // NormalMessageFormat
    m_formats[NormalMessageFormat].setFont(boldFont);
    m_formats[NormalMessageFormat].setForeground(mixColors(p.color(QPalette::Text), QColor(Qt::blue)));

    // ErrorMessageFormat
    m_formats[ErrorMessageFormat].setFont(boldFont);
    m_formats[ErrorMessageFormat].setForeground(mixColors(p.color(QPalette::Text), QColor(Qt::red)));

    // StdOutFormat
    m_formats[StdOutFormat].setFont(font);
    m_formats[StdOutFormat].setForeground(p.color(QPalette::Text));
    m_formats[StdOutFormatSameLine] = m_formats[StdOutFormat];

    // StdErrFormat
    m_formats[StdErrFormat].setFont(font);
    m_formats[StdErrFormat].setForeground(mixColors(p.color(QPalette::Text), QColor(Qt::red)));
    m_formats[StdErrFormatSameLine] = m_formats[StdErrFormat];
}

void OutputFormatter::handleLink(const QString &href)
{
    Q_UNUSED(href);
}
