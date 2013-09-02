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

#include "blackberrylogprocessrunner.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

using namespace Qnx::Internal;

BlackBerryLogProcessRunner::BlackBerryLogProcessRunner(QObject *parent, const QString& appId, const BlackBerryDeviceConfiguration::ConstPtr &device)
    : QObject(parent)
    , m_currentLogs(false)
    , m_slog2infoFound(false)
    , m_tailProcess(0)
    , m_slog2infoProcess(0)
    , m_testSlog2Process(0)
    , m_launchDateTimeProcess(0)
{
    Q_ASSERT(!appId.isEmpty() && device);

    m_appId = appId;
    m_device = device;

    // The BlackBerry device always uses key authentication
    m_sshParams = m_device->sshParameters();
    m_sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;

    m_tailCommand = QLatin1String("tail -c +1 -f /accounts/1000/appdata/") + m_appId
            + QLatin1String("/logs/log");
    m_slog2infoCommand = QString::fromLatin1("slog2info -w -b ") + m_appId;
}

void BlackBerryLogProcessRunner::start()
{
    if (!m_testSlog2Process) {
        m_testSlog2Process = new QSsh::SshRemoteProcessRunner(this);
        connect(m_testSlog2Process, SIGNAL(processClosed(int)), this, SLOT(handleSlog2infoFound()));
    }

    m_testSlog2Process->run("slog2info", m_sshParams);
}

void BlackBerryLogProcessRunner::handleTailOutput()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    const QString message = QString::fromLatin1(process->readAllStandardOutput());
    // if slog2info found then m_tailProcess should only display 'launching' error logs
    m_slog2infoFound ? output(message, Utils::StdErrFormat) : output(message, Utils::StdOutFormat);
}

void BlackBerryLogProcessRunner::handleSlog2infoOutput()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    const QString message = QString::fromLatin1(process->readAllStandardOutput());
    const QStringList multiLine = message.split(QLatin1Char('\n'));
    Q_FOREACH (const QString &line, multiLine) {
        // Note: This is useless if/once slog2info -b displays only logs from recent launches
        if (!m_launchDateTime.isNull())
        {
            // Check if logs are from the recent launch
            if (!m_currentLogs) {
                QDateTime dateTime = QDateTime::fromString(line.split(m_appId).first().mid(4).trimmed(),
                                                           QString::fromLatin1("dd HH:mm:ss.zzz"));

                m_currentLogs = dateTime >= m_launchDateTime;
                if (!m_currentLogs)
                    continue;
            }
        }

        // The line could be a part of a previous log message that contains a '\n'
        // In that case only the message body is displayed
        if (!line.contains(m_appId) && !line.isEmpty()) {
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

void BlackBerryLogProcessRunner::handleLogError()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    const QString message = QString::fromLatin1(process->readAllStandardError());
    emit output(message, Utils::StdErrFormat);
}

void BlackBerryLogProcessRunner::handleTailProcessConnectionError()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    emit output(tr("Cannot show debug output. Error: %1").arg(process->lastConnectionErrorString()),
                Utils::StdErrFormat);
}

void BlackBerryLogProcessRunner::handleSlog2infoProcessConnectionError()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    emit output(tr("Cannot show slog2info output. Error: %1").arg(process->lastConnectionErrorString()),
                Utils::StdErrFormat);
}

bool BlackBerryLogProcessRunner::showQtMessage(const QString &pattern, const QString &line)
{
    const int index = line.indexOf(pattern);
    if (index != -1) {
        const QString str = line.right(line.length()-index-pattern.length()) + QLatin1Char('\n');
        emit output(str, Utils::StdOutFormat);
        return true;
    }
    return false;
}

void BlackBerryLogProcessRunner::killProcessRunner(QSsh::SshRemoteProcessRunner *processRunner, const QString &command)
{
    QTC_ASSERT(!command.isEmpty(), return);

    QString killCommand = m_device->processSupport()->killProcessByNameCommandLine(command);

    QSsh::SshRemoteProcessRunner *slayProcess = new QSsh::SshRemoteProcessRunner(this);
    connect(slayProcess, SIGNAL(processClosed(int)), this, SIGNAL(finished()));
    slayProcess->run(killCommand.toLatin1(), m_sshParams);

    processRunner->cancel();

    delete processRunner;
}

void BlackBerryLogProcessRunner::handleSlog2infoFound()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    m_slog2infoFound = (process->processExitCode() == 0);

    readLaunchTime();
}

void BlackBerryLogProcessRunner::readLaunchTime()
{
    m_launchDateTimeProcess = new QSsh::SshRemoteProcessRunner(this);
    connect(m_launchDateTimeProcess, SIGNAL(processClosed(int)),
            this, SLOT(readApplicationLog()));

    m_launchDateTimeProcess->run("date +\"%d %H:%M:%S\"", m_sshParams);
}

void BlackBerryLogProcessRunner::readApplicationLog()
{
    QSsh::SshRemoteProcessRunner *process = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    QTC_ASSERT(process, return);

    if (m_tailProcess && m_tailProcess->isProcessRunning())
        return;

    QTC_CHECK(!m_appId.isEmpty());

    if (!m_tailProcess) {
        m_tailProcess = new QSsh::SshRemoteProcessRunner(this);
        connect(m_tailProcess, SIGNAL(readyReadStandardOutput()),
                this, SLOT(handleTailOutput()));
        connect(m_tailProcess, SIGNAL(readyReadStandardError()),
                this, SLOT(handleLogError()));
        connect(m_tailProcess, SIGNAL(connectionError()),
                this, SLOT(handleTailProcessConnectionError()));
    }

    m_tailProcess->run(m_tailCommand.toLatin1(), m_sshParams);

    if (!m_slog2infoFound || (m_slog2infoProcess && m_slog2infoProcess->isProcessRunning()))
        return;

    if (!m_slog2infoProcess) {
        m_launchDateTime = QDateTime::fromString(QString::fromLatin1(process->readAllStandardOutput()).trimmed(),
                                                 QString::fromLatin1("dd HH:mm:ss"));
        m_slog2infoProcess = new QSsh::SshRemoteProcessRunner(this);
        connect(m_slog2infoProcess, SIGNAL(readyReadStandardOutput()),
                this, SLOT(handleSlog2infoOutput()));
        connect(m_slog2infoProcess, SIGNAL(readyReadStandardError()),
                this, SLOT(handleLogError()));
        connect(m_slog2infoProcess, SIGNAL(connectionError()),
                this, SLOT(handleSlog2infoProcessConnectionError()));
    }

    m_slog2infoProcess->run(m_slog2infoCommand.toLatin1(), m_sshParams);
}

void BlackBerryLogProcessRunner::stop()
{
    if (m_testSlog2Process && m_testSlog2Process->isProcessRunning()) {
        m_testSlog2Process->cancel();
        delete m_testSlog2Process;
        m_testSlog2Process = 0;
    }

    if (m_tailProcess && m_tailProcess->isProcessRunning()) {
        killProcessRunner(m_tailProcess, m_tailCommand);
        m_tailProcess = 0;
    }

    if (m_slog2infoFound) {
        if (m_slog2infoProcess && m_slog2infoProcess->isProcessRunning()) {
            killProcessRunner(m_slog2infoProcess, m_slog2infoCommand);
            m_slog2infoProcess = 0;
        }
    }

    emit finished();
}
