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
#include "showoutputtaskhandler.h"
#include "task.h"

#include <find/basetextfind.h>
#include <aggregation/aggregate.h>
#include <extensionsystem/pluginmanager.h>

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
const int MAX_LINECOUNT = 10000;
}

CompileOutputWindow::CompileOutputWindow(BuildManager * /*bm*/)
{
    m_textEdit = new QPlainTextEdit();
    m_textEdit->setWindowTitle(tr("Compile Output"));
    m_textEdit->setWindowIcon(QIcon(":/qt4projectmanager/images/window.png"));
    m_textEdit->setReadOnly(true);
    m_textEdit->setFrameStyle(QFrame::NoFrame);
    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_textEdit);
    agg->add(new Find::BaseTextFind(m_textEdit));

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
    return m_textEdit->hasFocus();
}

bool CompileOutputWindow::canFocus()
{
    return true;
}

void CompileOutputWindow::setFocus()
{
    m_textEdit->setFocus();
}

QWidget *CompileOutputWindow::outputWidget(QWidget *)
{
    return m_textEdit;
}

void CompileOutputWindow::appendText(const QString &text, const QTextCharFormat &textCharFormat)
{
    if (m_textEdit->document()->blockCount() > MAX_LINECOUNT)
        return;
    bool shouldScroll = (m_textEdit->verticalScrollBar()->value() ==
                         m_textEdit->verticalScrollBar()->maximum());
    QString textWithNewline = text;
    if (!textWithNewline.endsWith("\n"))
        textWithNewline.append("\n");
    QTextCursor cursor = QTextCursor(m_textEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    cursor.insertText(textWithNewline, textCharFormat);

    if (m_textEdit->document()->blockCount() > MAX_LINECOUNT) {
        QTextCharFormat tmp;
        tmp.setFontWeight(QFont::Bold);
        cursor.insertText(tr("Additional output omitted\n"), tmp);
    }

    cursor.endEditBlock();

    if (shouldScroll) {
        m_textEdit->verticalScrollBar()->setValue(m_textEdit->verticalScrollBar()->maximum());
        m_textEdit->setTextCursor(cursor);
    }
}

void CompileOutputWindow::clearContents()
{
    m_textEdit->clear();
    m_taskPositions.clear();
}

void CompileOutputWindow::visibilityChanged(bool b)
{
    if (b)
        m_textEdit->verticalScrollBar()->setValue(m_textEdit->verticalScrollBar()->maximum());
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
    int blocknumber = m_textEdit->blockCount();
    if (blocknumber > MAX_LINECOUNT)
        return;
    m_taskPositions.insert(task.taskId, blocknumber - 1);
}

bool CompileOutputWindow::knowsPositionOf(const Task &task)
{
    return (m_taskPositions.contains(task.taskId));
}

void CompileOutputWindow::showPositionOf(const Task &task)
{
    int position = m_taskPositions.value(task.taskId);
    QTextCursor newCursor(m_textEdit->document()->findBlockByNumber(position));
    newCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    m_textEdit->setTextCursor(newCursor);
}
