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

#include "qmakeparser.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/taskwindow.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::Task;

QMakeParser::QMakeParser()
{
    setObjectName(QLatin1String("QMakeParser"));
}

void QMakeParser::stdError(const QString &line)
{
    QString lne(line.trimmed());
    if (lne.startsWith(QLatin1String("Project ERROR:"))) {
        const QString description = lne.mid(15);
        emit addTask(Task(Task::Error,
                          description,
                          QString() /* filename */,
                          -1 /* linenumber */,
                          ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        return;
    }
    IOutputParser::stdError(line);
}
