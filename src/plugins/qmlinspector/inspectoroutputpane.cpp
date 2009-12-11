/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "inspectoroutputpane.h"

#include <QtGui/qtextedit.h>

InspectorOutputPane::InspectorOutputPane(QObject *parent)
    : Core::IOutputPane(parent),
      m_textEdit(new QTextEdit)
{
}

InspectorOutputPane::~InspectorOutputPane()
{
    delete m_textEdit;
}

QWidget *InspectorOutputPane::outputWidget(QWidget *parent)
{
    Q_UNUSED(parent);
    return m_textEdit;
}

QList<QWidget*> InspectorOutputPane::toolBarWidgets() const
{
    return QList<QWidget *>();
}

QString InspectorOutputPane::name() const
{
    return tr("Inspector Output");
}

int InspectorOutputPane::priorityInStatusBar() const
{
    return 1;
}

void InspectorOutputPane::clearContents()
{
    m_textEdit->clear();
}

void InspectorOutputPane::visibilityChanged(bool visible)
{
    Q_UNUSED(visible);
}

void InspectorOutputPane::setFocus()
{
    m_textEdit->setFocus();
}

bool InspectorOutputPane::hasFocus()
{
    return m_textEdit->hasFocus();
}

bool InspectorOutputPane::canFocus()
{
    return true;
}

bool InspectorOutputPane::canNavigate()
{
    return false;
}

bool InspectorOutputPane::canNext()
{
    return false;
}

bool InspectorOutputPane::canPrevious()
{
    return false;
}

void InspectorOutputPane::goToNext()
{
}

void InspectorOutputPane::goToPrev()
{
}

void InspectorOutputPane::addOutput(RunControl *, const QString &text)
{
    m_textEdit->insertPlainText(text);
    m_textEdit->moveCursor(QTextCursor::End);
}

void InspectorOutputPane::addOutputInline(RunControl *, const QString &text)
{
    m_textEdit->insertPlainText(text);
    m_textEdit->moveCursor(QTextCursor::End);
}

void InspectorOutputPane::addErrorOutput(RunControl *, const QString &text)
{
    m_textEdit->append(text);
    m_textEdit->moveCursor(QTextCursor::End);
}

void InspectorOutputPane::addInspectorStatus(const QString &text)
{
    m_textEdit->setTextColor(Qt::darkGreen);
    m_textEdit->append(text);
    m_textEdit->moveCursor(QTextCursor::End);
    m_textEdit->setTextColor(Qt::black);
}

