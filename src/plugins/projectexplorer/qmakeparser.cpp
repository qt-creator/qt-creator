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
#include "projectexplorerconstants.h"
#include "taskwindow.h"

using namespace ProjectExplorer;

QMakeParser::QMakeParser()
{
}

QString QMakeParser::name() const
{
    return QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_QMAKE);
}

void QMakeParser::stdOutput(const QString & line)
{
    Q_UNUSED(line)
}

void QMakeParser::stdError(const QString & line)
{
    QString lne(line.trimmed());
    if (lne.startsWith("Project ERROR:"))
    {
        lne = lne.mid(15);
        emit addToTaskWindow(TaskWindow::Task(TaskWindow::Error,
                                              lne /* description */,
                                              QString() /* filename */,
                                              -1 /* linenumber */,
                                              Constants::TASK_CATEGORY_BUILDSYSTEM));
        return;
    }
}
