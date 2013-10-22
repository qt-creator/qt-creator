/**************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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

#include "slog2inforunner.h"

#include <projectexplorer/devicesupport/sshdeviceprocess.h>
#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

Slog2InfoRunner::Slog2InfoRunner(const QString &applicationId, const RemoteLinux::LinuxDevice::ConstPtr &device, QObject *parent)
    : QObject(parent)
    , m_applicationId(applicationId)
    , m_found(false)
    , m_currentLogs(false)
{
    m_testProcess = new ProjectExplorer::SshDeviceProcess(device, this);
    connect(m_testProcess, SIGNAL(finished()), this, SLOT(handleTestProcessCompleted()));

    m_launchDateTimeProcess = new ProjectExplorer::SshDeviceProcess(device, this);
    connect(m_launchDateTimeProcess, SIGNAL(finished()), this, SLOT(launchSlog2Info()));

    m_logProcess = new ProjectExplorer::SshDeviceProcess(device, this);
    connect(m_logProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(readLogStandardOutput()));
    connect(m_logProcess, SIGNAL(readyReadStandardError()), this, SLOT(readLogStandardError()));
    connect(m_logProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(handleLogError()));
    connect(m_logProcess, SIGNAL(started()), this, SIGNAL(started()));
    connect(m_logProcess, SIGNAL(finished()), this, SIGNAL(finished()));
}

void Slog2InfoRunner::start()
{
    m_testProcess->start(QLatin1String("slog2info"), QStringList());
}

void Slog2InfoRunner::stop()
{
    if (m_testProcess->state() == QProcess::Running)
        m_testProcess->kill();

    if (m_logProcess->state() == QProcess::Running)
        m_logProcess->kill();
}

bool Slog2InfoRunner::commandFound() const
{
    return m_found;
}

void Slog2InfoRunner::handleTestProcessCompleted()
{
    m_found = (m_testProcess->exitCode() == 0);
    if (m_found)
        readLaunchTime();
    else
        emit commandMissing();
}

void Slog2InfoRunner::readLaunchTime()
{
    QStringList arguments;
    arguments << QLatin1String("+\"%d %H:%M:%S\"");
    m_launchDateTimeProcess->start(QLatin1String("date"), arguments);
}

void Slog2InfoRunner::launchSlog2Info()
{
    QTC_CHECK(!m_applicationId.isEmpty());
    QTC_CHECK(m_found);

    if (m_logProcess && m_logProcess->state() == QProcess::Running)
        return;

    m_launchDateTime = QDateTime::fromString(QString::fromLatin1(m_launchDateTimeProcess->readAllStandardOutput()).trimmed(),
                                             QString::fromLatin1("dd HH:mm:ss"));

    QStringList arguments;
    arguments << QLatin1String("-w")
              << QLatin1String("-b")
              << m_applicationId;
    m_logProcess->start(QLatin1String("slog2info"), arguments);
}

void Slog2InfoRunner::readLogStandardOutput()
{
    const QString message = QString::fromLatin1(m_logProcess->readAllStandardOutput());
    const QStringList multiLine = message.split(QLatin1Char('\n'));
    Q_FOREACH (const QString &line, multiLine) {
        // Note: This is useless if/once slog2info -b displays only logs from recent launches
        if (!m_launchDateTime.isNull())
        {
            // Check if logs are from the recent launch
            if (!m_currentLogs) {
                QDateTime dateTime = QDateTime::fromString(line.split(m_applicationId).first().mid(4).trimmed(),
                                                           QString::fromLatin1("dd HH:mm:ss.zzz"));

                m_currentLogs = dateTime >= m_launchDateTime;
                if (!m_currentLogs)
                    continue;
            }
        }

        // The line could be a part of a previous log message that contains a '\n'
        // In that case only the message body is displayed
        if (!line.contains(m_applicationId) && !line.isEmpty()) {
            emit output(line + QLatin1Char('\n'), Utils::StdOutFormat);
            continue;
        }

        QStringList validLineBeginnings;
        validLineBeginnings << QLatin1String("qt-msg      0  ")
                            << QLatin1String("qt-msg*     0  ")
                            << QLatin1String("default*  9000  ")
                            << QLatin1String("default   9000  ")
                            << QLatin1String("                           0  ");
        Q_FOREACH (const QString &beginning, validLineBeginnings) {
            if (showQtMessage(beginning, line))
                break;
        }
    }
}

bool Slog2InfoRunner::showQtMessage(const QString &pattern, const QString &line)
{
    const int index = line.indexOf(pattern);
    if (index != -1) {
        const QString str = line.right(line.length()-index-pattern.length()) + QLatin1Char('\n');
        emit output(str, Utils::StdOutFormat);
        return true;
    }
    return false;
}

void Slog2InfoRunner::readLogStandardError()
{
    const QString message = QString::fromLatin1(m_logProcess->readAllStandardError());
    emit output(message, Utils::StdErrFormat);
}

void Slog2InfoRunner::handleLogError()
{
    emit output(tr("Cannot show slog2info output. Error: %1").arg(m_logProcess->errorString()), Utils::StdErrFormat);
}
