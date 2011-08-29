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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "testoutputwindow.h"

#include <QtGui/QTextEdit>
#include <QDebug>

TestOutputWindow *m_instance = 0;

QTextEdit *testOutputPane()
{
    if (m_instance)
        return m_instance->m_widget;
    return 0;
}

TestOutputWindow::TestOutputWindow() : m_widget(new QTextEdit)
{
    m_widget->setReadOnly(true);
    m_widget->setFrameStyle(QFrame::NoFrame);
    m_instance = this;
}

TestOutputWindow::~TestOutputWindow()
{
    delete m_widget;
    m_instance = 0;
}

bool TestOutputWindow::hasFocus()
{
    return m_widget->hasFocus();
}

bool TestOutputWindow::canFocus()
{
    return true;
}

void TestOutputWindow::setFocus()
{
    m_widget->setFocus();
}

void TestOutputWindow::clearContents()
{
    m_widget->clear();
}

QWidget *TestOutputWindow::outputWidget(QWidget *parent)
{
    m_widget->setParent(parent);
    return m_widget;
}

QString TestOutputWindow::name() const
{
    return tr("Test Output");
}

void TestOutputWindow::visibilityChanged(bool /*b*/)
{
}

int TestOutputWindow::priorityInStatusBar() const
{
    return 50;
}

bool TestOutputWindow::canNext()
{
    return false;
}

bool TestOutputWindow::canPrevious()
{
    return false;
}

void TestOutputWindow::goToNext()
{

}

void TestOutputWindow::goToPrev()
{

}

bool TestOutputWindow::canNavigate()
{
    return false;
}

