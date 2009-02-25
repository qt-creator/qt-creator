/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "messageoutputwindow.h"

#include <QtGui/QTextEdit>

using namespace Core::Internal;

MessageOutputWindow::MessageOutputWindow()
{
    m_widget = new QTextEdit;
    m_widget->setReadOnly(true);
    m_widget->setFrameStyle(QFrame::NoFrame);
}

MessageOutputWindow::~MessageOutputWindow()
{
    delete m_widget;
}

bool MessageOutputWindow::hasFocus()
{
    return m_widget->hasFocus();
}

bool MessageOutputWindow::canFocus()
{
    return true;
}

void MessageOutputWindow::setFocus()
{
    m_widget->setFocus();
}

void MessageOutputWindow::clearContents()
{
    m_widget->clear();
}

QWidget *MessageOutputWindow::outputWidget(QWidget *parent)
{
    m_widget->setParent(parent);
    return m_widget;
}

QString MessageOutputWindow::name() const
{
    return tr("General");
}

void MessageOutputWindow::visibilityChanged(bool /*b*/)
{
}

void MessageOutputWindow::append(const QString &text)
{
    m_widget->append(text);
}

int MessageOutputWindow::priorityInStatusBar() const
{
    return -1;
}
