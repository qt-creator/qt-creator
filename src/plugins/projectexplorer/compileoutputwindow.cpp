/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "compileoutputwindow.h"
#include "buildmanager.h"
#include "showoutputtaskhandler.h"
#include "task.h"

#include <find/basetextfind.h>
#include <aggregation/aggregate.h>
#include <extensionsystem/pluginmanager.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QtGui/QKeyEvent>
#include <QtGui/QIcon>
#include <QtGui/QTextCharFormat>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>
#include <QtGui/QScrollBar>
#include <QtGui/QPlainTextEdit>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
const int MAX_LINECOUNT = 50000;
}

CompileOutputWindow::CompileOutputWindow(BuildManager * /*bm*/)
{
    m_outputWindow = new OutputWindow();
    m_outputWindow->setWindowTitle(tr("Compile Output"));
    m_outputWindow->setWindowIcon(QIcon(QLatin1String(Qt4ProjectManager::Constants::ICON_WINDOW)));
    m_outputWindow->setReadOnly(true);
    m_outputWindow->setUndoRedoEnabled(false);

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_outputWindow);
    agg->add(new Find::BaseTextFind(m_outputWindow));

    qRegisterMetaType<QTextCharFormat>("QTextCharFormat");

    m_handler = new ShowOutputTaskHandler(this);
    ExtensionSystem::PluginManager::instance()->addObject(m_handler);
}

CompileOutputWindow::~CompileOutputWindow()
{
    ExtensionSystem::PluginManager::instance()->removeObject(m_handler);
    delete m_handler;
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

static QColor mix_colors(QColor a, QColor b)
{
    return QColor((a.red() + 2 * b.red()) / 3, (a.green() + 2 * b.green()) / 3,
                  (a.blue() + 2* b.blue()) / 3, (a.alpha() + 2 * b.alpha()) / 3);
}

void CompileOutputWindow::appendText(const QString &text, ProjectExplorer::BuildStep::OutputFormat format)
{
    QPalette p = m_outputWindow->palette();
    QTextCharFormat textFormat;
    switch (format) {
    case BuildStep::NormalOutput:
        textFormat.setForeground(p.color(QPalette::Text));
        textFormat.setFontWeight(QFont::Normal);
        break;
    case BuildStep::ErrorOutput:
        textFormat.setForeground(mix_colors(p.color(QPalette::Text), QColor(Qt::red)));
        textFormat.setFontWeight(QFont::Normal);
        break;
    case BuildStep::MessageOutput:
        textFormat.setForeground(mix_colors(p.color(QPalette::Text), QColor(Qt::blue)));
        break;
    case BuildStep::ErrorMessageOutput:
        textFormat.setForeground(mix_colors(p.color(QPalette::Text), QColor(Qt::red)));
        textFormat.setFontWeight(QFont::Bold);
        break;

    }

    m_outputWindow->appendText(text, textFormat, MAX_LINECOUNT);
}

void CompileOutputWindow::clearContents()
{
    m_outputWindow->clear();
    m_taskPositions.clear();
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

void CompileOutputWindow::registerPositionOf(const Task &task)
{
    int blocknumber = m_outputWindow->blockCount();
    if (blocknumber > MAX_LINECOUNT)
        return;
    m_taskPositions.insert(task.taskId, blocknumber);
}

bool CompileOutputWindow::knowsPositionOf(const Task &task)
{
    return (m_taskPositions.contains(task.taskId));
}

void CompileOutputWindow::showPositionOf(const Task &task)
{
    int position = m_taskPositions.value(task.taskId);
    QTextCursor newCursor(m_outputWindow->document()->findBlockByNumber(position));
    newCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    m_outputWindow->setTextCursor(newCursor);
}
