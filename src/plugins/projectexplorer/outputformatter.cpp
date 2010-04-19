/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "outputformatter.h"

#include <QtGui/QPlainTextEdit>

using namespace ProjectExplorer;

OutputFormatter::OutputFormatter(QObject *parent)
    : QObject(parent)
{
}

QPlainTextEdit *OutputFormatter::plainTextEdit() const
{
    return m_plainTextEdit;
}

void OutputFormatter::setPlainTextEdit(QPlainTextEdit *plainText)
{
    m_plainTextEdit = plainText;
    setParent(m_plainTextEdit);
}

void OutputFormatter::appendApplicationOutput(const QString &text, bool /*onStdErr*/)
{
    QTextCharFormat format;
    format.setForeground(plainTextEdit()->palette().text().color());
    plainTextEdit()->setCurrentCharFormat(format);
    plainTextEdit()->insertPlainText(text);
}

void OutputFormatter::appendMessage(const QString &text, bool isError)
{
    appendApplicationOutput(text, isError);
}

void OutputFormatter::mousePressEvent(QMouseEvent * /*e*/)
{}

void OutputFormatter::mouseReleaseEvent(QMouseEvent * /*e*/)
{}

void OutputFormatter::mouseMoveEvent(QMouseEvent * /*e*/)
{}
