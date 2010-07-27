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


#include "signsisparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskwindow.h>

using namespace Qt4ProjectManager;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

SignsisParser::SignsisParser()
{
    m_signSis.setPattern("^(error):([A-Z\\d]+):(.+)$");
    m_signSis.setMinimal(true);
}

void SignsisParser::stdOutput(const QString &line)
{
    QString lne = line.trimmed();

    if (m_signSis.indexIn(lne) > -1) {
        QString errorDescription(m_signSis.cap(3));
        int index = errorDescription.indexOf(QLatin1String("error:"));
        if (index >= 0) {
            stdOutput(errorDescription.mid(index));
            errorDescription = errorDescription.left(index);
        }
        Task task(Task::Error,
                  errorDescription /* description */,
                  QString(), -1,
                  TASK_CATEGORY_BUILDSYSTEM);
        emit addTask(task);
    }
    IOutputParser::stdOutput(line);
}

void SignsisParser::stdError(const QString &line)
{
    stdOutput(line);
    IOutputParser::stdError(line);
}
