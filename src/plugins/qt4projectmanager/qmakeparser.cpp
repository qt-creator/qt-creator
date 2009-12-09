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

#include "qmakeparser.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/taskwindow.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::TaskWindow;

QMakeParser::QMakeParser()
{
}

void QMakeParser::stdError(const QString &line)
{
    QString lne(line.trimmed());
    if (lne.startsWith("Project ERROR:")) {
        const QString description = lne.mid(15);
        emit addTask(TaskWindow::Task(TaskWindow::Error,
                                      description,
                                      QString() /* filename */,
                                      -1 /* linenumber */,
                                      ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        return;
    }
    IOutputParser::stdError(line);
}
