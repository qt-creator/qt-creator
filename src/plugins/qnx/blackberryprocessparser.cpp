/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberryprocessparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char ERROR_MESSAGE_START[]          = "Error: ";
const char WARNING_MESSAGE_START[]        = "Warning: ";
const char PROGRESS_MESSAGE_START[]       = "Info: Progress ";
const char PID_MESSAGE_START[]            = "result::";
const char APPLICATION_ID_MESSAGE_START[] = "Info: Launching ";

const char AUTHENTICATION_ERROR[]  = "Authentication failed.";
}

BlackBerryProcessParser::BlackBerryProcessParser()
{
    m_messageReplacements[QLatin1String(AUTHENTICATION_ERROR)] =
            tr("Authentication failed. Please make sure the password for the device is correct.");
}

void BlackBerryProcessParser::stdOutput(const QString &line)
{
    parse(line);
    IOutputParser::stdOutput(line);
}

void BlackBerryProcessParser::stdError(const QString &line)
{
    parse(line);
    IOutputParser::stdError(line);
}

void BlackBerryProcessParser::parse(const QString &line)
{
    bool isErrorMessage = line.startsWith(QLatin1String(ERROR_MESSAGE_START));
    bool isWarningMessage = line.startsWith(QLatin1String(WARNING_MESSAGE_START));
    if (isErrorMessage || isWarningMessage)
        parseErrorAndWarningMessage(line, isErrorMessage);
    else if (line.startsWith(QLatin1String(PROGRESS_MESSAGE_START)))
        parseProgress(line);
    else if (line.startsWith(QLatin1String(PID_MESSAGE_START)))
        parsePid(line);
    else if (line.startsWith(QLatin1String(APPLICATION_ID_MESSAGE_START)))
        parseApplicationId(line);
}

void BlackBerryProcessParser::parseErrorAndWarningMessage(const QString &line, bool isErrorMessage)
{
    QString message = line.mid(line.indexOf(QLatin1String(": ")) + 2).trimmed();

    foreach (const QString &key, m_messageReplacements.keys()) {
        if (message.startsWith(key)) {
            message = m_messageReplacements[key];
            break;
        }
    }

    ProjectExplorer::Task task(isErrorMessage ? ProjectExplorer::Task::Error : ProjectExplorer::Task::Warning,
                               message,
                               Utils::FileName(),
                               -1,
                               Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    emit addTask(task);
}

void BlackBerryProcessParser::parseProgress(const QString &line)
{
    const QString startOfLine = QLatin1String(PROGRESS_MESSAGE_START);

    const int percentPos = line.indexOf(QLatin1Char('%'));
    const QString progressStr = line.mid(startOfLine.length(), percentPos - startOfLine.length());

    bool ok;
    const int progress = progressStr.toInt(&ok);
    if (ok)
        emit progressParsed(progress);
}

void BlackBerryProcessParser::parsePid(const QString &line)
{
    int pidIndex = -1;
    if (line.contains(QLatin1String("running"))) // "result::running,<pid>"
        pidIndex = 16;
    else // "result::<pid>"
        pidIndex = 8;

    bool ok;
    const qint64 pid = line.mid(pidIndex).toInt(&ok);
    if (ok)
        emit pidParsed(pid);
}

void BlackBerryProcessParser::parseApplicationId(const QString &line)
{
    const int endOfId = line.indexOf(QLatin1String("..."));
    const QString applicationId = line.mid(16, endOfId - 16);
    emit applicationIdParsed(applicationId);
}
