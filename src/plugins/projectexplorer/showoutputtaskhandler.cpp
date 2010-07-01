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

#include "showoutputtaskhandler.h"

#include "projectexplorerconstants.h"
#include "task.h"

#include "compileoutputwindow.h"

#include <QtGui/QAction>

using namespace ProjectExplorer::Internal;

ShowOutputTaskHandler::ShowOutputTaskHandler(CompileOutputWindow *window) :
    ITaskHandler(QLatin1String(Constants::SHOW_TASK_OUTPUT)),
    m_window(window)
{
    Q_ASSERT(m_window);
}

bool ShowOutputTaskHandler::canHandle(const ProjectExplorer::Task &task)
{
    return m_window->knowsPositionOf(task);
}

void ShowOutputTaskHandler::handle(const ProjectExplorer::Task &task)
{
    Q_ASSERT(canHandle(task));
    m_window->popup(); // popup first as this does move the visible area!
    m_window->showPositionOf(task);
}

QAction *ShowOutputTaskHandler::createAction(QObject *parent)
{
    QAction *outputAction = new QAction(tr("Show &Output"), parent);
    outputAction->setToolTip(tr("Show output generating this issue."));
    return outputAction;
}
