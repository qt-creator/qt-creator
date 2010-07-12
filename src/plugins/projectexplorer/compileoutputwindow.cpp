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

#include "compileoutputwindow.h"
#include "buildmanager.h"

#include <find/basetextfind.h>
#include <aggregation/aggregate.h>

#include <QtGui/QKeyEvent>
#include <QtGui/QIcon>
#include <QtGui/QTextEdit>
#include <QtGui/QScrollBar>
#include <QtGui/QPlainTextEdit>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
const int MAX_LINECOUNT = 10000;
}

CompileOutputWindow::CompileOutputWindow(BuildManager * /*bm*/)
{
    m_outputWindow = new OutputWindow();
    m_outputWindow->setWindowTitle(tr("Compile Output"));
    m_outputWindow->setWindowIcon(QIcon(":/qt4projectmanager/images/window.png"));
    m_outputWindow->setReadOnly(true);

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_outputWindow);
    agg->add(new Find::BaseTextFind(m_outputWindow));

    qRegisterMetaType<QTextCharFormat>("QTextCharFormat");
}

bool CompileOutputWindow::hasFocus()
{
    return m_outputWindow->hasFocus();
}

bool CompileOutputWindow::canFocus()
{
    return true;
}

void CompileOutputWindow::setFocus()
{
    m_outputWindow->setFocus();
}

QWidget *CompileOutputWindow::outputWidget(QWidget *)
{
    return m_outputWindow;
}

void CompileOutputWindow::appendText(const QString &text, const QTextCharFormat &textCharFormat)
{
    m_outputWindow->appendText(text, textCharFormat, MAX_LINECOUNT);
}

void CompileOutputWindow::clearContents()
{
    m_outputWindow->clear();
}

void CompileOutputWindow::visibilityChanged(bool)
{

}

int CompileOutputWindow::priorityInStatusBar() const
{
    return 50;
}

bool CompileOutputWindow::canNext()
{
    return false;
}

bool CompileOutputWindow::canPrevious()
{
    return false;
}

void CompileOutputWindow::goToNext()
{

}

void CompileOutputWindow::goToPrev()
{

}

bool CompileOutputWindow::canNavigate()
{
    return false;
}
