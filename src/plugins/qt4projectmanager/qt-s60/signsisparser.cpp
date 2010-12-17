/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
