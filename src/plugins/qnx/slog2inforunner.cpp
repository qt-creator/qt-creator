/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com), KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "slog2inforunner.h"

#include "qnxdeviceprocess.h"

#include <projectexplorer/runnables.h>
#include <utils/qtcassert.h>

#include <QRegExp>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

Slog2InfoRunner::Slog2InfoRunner(const QString &applicationId,
                                 const RemoteLinux::LinuxDevice::ConstPtr &device, QObject *parent)
    : QObject(parent)
    , m_applicationId(applicationId)
    , m_found(false)
    , m_currentLogs(false)
{
    // See QTCREATORBUG-10712 for details.
    // We need to limit length of ApplicationId to 63 otherwise it would not match one in slog2info.
    m_applicationId.truncate(63);

    m_testProcess = new QnxDeviceProcess(device, this);
    connect(m_testProcess, &DeviceProcess::finished, this, &Slog2InfoRunner::handleTestProcessCompleted);

    m_launchDateTimeProcess = new SshDeviceProcess(device, this);
    connect(m_launchDateTimeProcess, &DeviceProcess::finished, this, &Slog2InfoRunner::launchSlog2Info);

    m_logProcess = new QnxDeviceProcess(device, this);
    connect(m_logProcess, &DeviceProcess::readyReadStandardOutput, this, &Slog2InfoRunner::readLogStandardOutput);
    connect(m_logProcess, &DeviceProcess::readyReadStandardError, this, &Slog2InfoRunner::readLogStandardError);
    connect(m_logProcess, &DeviceProcess::error, this, &Slog2InfoRunner::handleLogError);
    connect(m_logProcess, &DeviceProcess::started, this, &Slog2InfoRunner::started);
    connect(m_logProcess, &DeviceProcess::finished, this, &Slog2InfoRunner::finished);
}

void Slog2InfoRunner::start()
{
    StandardRunnable r;
    r.executable = QLatin1String("slog2info");
    m_testProcess->start(r);
}

void Slog2InfoRunner::stop()
{
    if (m_testProcess->state() == QProcess::Running)
        m_testProcess->kill();

    if (m_logProcess->state() == QProcess::Running) {
        m_logProcess->kill();
        processLog(true);
    }
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
    StandardRunnable r;
    r.executable = QLatin1String("date");
    r.commandLineArguments = QLatin1String("+\"%d %H:%M:%S\"");
    m_launchDateTimeProcess->start(r);
}

void Slog2InfoRunner::launchSlog2Info()
{
    QTC_CHECK(!m_applicationId.isEmpty());
    QTC_CHECK(m_found);

    if (m_logProcess->state() == QProcess::Running)
        return;

    m_launchDateTime = QDateTime::fromString(QString::fromLatin1(m_launchDateTimeProcess->readAllStandardOutput()).trimmed(),
                                             QString::fromLatin1("dd HH:mm:ss"));

    StandardRunnable r;
    r.executable = QLatin1String("slog2info");
    r.commandLineArguments = QLatin1String("-w");
    m_logProcess->start(r);
}

void Slog2InfoRunner::readLogStandardOutput()
{
    processLog(false);
}

void Slog2InfoRunner::processLog(bool force)
{
    QString input = QString::fromLatin1(m_logProcess->readAllStandardOutput());
    QStringList lines = input.split(QLatin1Char('\n'));
    if (lines.isEmpty())
        return;
    lines.first().prepend(m_remainingData);
    if (force)
        m_remainingData.clear();
    else
        m_remainingData = lines.takeLast();
    foreach (const QString &line, lines)
        processLogLine(line);
}

void Slog2InfoRunner::processLogLine(const QString &line)
{
    // The "(\\s+\\S+)?" represents a named buffer. If message has noname (aka empty) buffer
    // then the message might get cut for the first number in the message.
    // The "\\s+(\\b.*)?$" represents a space followed by a message. We are unable to determinate
    // how many spaces represent separators and how many are a part of the messages, so resulting
    // messages has all whitespaces at the beginning of the message trimmed.
    static QRegExp regexp(QLatin1String(
        "^[a-zA-Z]+\\s+([0-9]+ [0-9]+:[0-9]+:[0-9]+.[0-9]+)\\s+(\\S+)(\\s+(\\S+))?\\s+([0-9]+)\\s+(.*)?$"));

    if (!regexp.exactMatch(line) || regexp.captureCount() != 6)
        return;

    // Note: This is useless if/once slog2info -b displays only logs from recent launches
    if (!m_launchDateTime.isNull()) {
        // Check if logs are from the recent launch
        if (!m_currentLogs) {
            QDateTime dateTime = QDateTime::fromString(regexp.cap(1),
                                                       QLatin1String("dd HH:mm:ss.zzz"));
            m_currentLogs = dateTime >= m_launchDateTime;
            if (!m_currentLogs)
                return;
        }
    }

    QString applicationId = regexp.cap(2);
    if (!applicationId.startsWith(m_applicationId))
        return;

    QString bufferName = regexp.cap(4);
    int bufferId = regexp.cap(5).toInt();
    // filtering out standard BB10 messages
    if (bufferName == QLatin1String("default") && bufferId == 8900)
        return;

    emit output(regexp.cap(6).trimmed() + QLatin1Char('\n'), Utils::StdOutFormat);
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

} // namespace Internal
} // namespace Qnx
