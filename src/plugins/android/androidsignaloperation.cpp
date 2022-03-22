/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "androidconfigurations.h"
#include "androidsignaloperation.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace Android {
namespace Internal {

AndroidSignalOperation::AndroidSignalOperation()
    : m_adbPath(AndroidConfigurations::currentConfig().adbToolPath())
    , m_timeout(new QTimer(this))
{
    m_timeout->setInterval(5000);
    connect(m_timeout, &QTimer::timeout, this, &AndroidSignalOperation::handleTimeout);
}

void AndroidSignalOperation::adbFindRunAsFinished()
{
    QTC_ASSERT(m_state == RunAs, return);
    m_timeout->stop();

    QString runAs = QString::fromLatin1(m_adbProcess->readAllStandardOutput());
    if (m_adbProcess->exitStatus() != QProcess::NormalExit) {
        m_errorMessage = QLatin1String(" adb Exit code: ") + QString::number(m_adbProcess->exitCode());
        QString adbError = m_adbProcess->errorString();
        if (!adbError.isEmpty())
            m_errorMessage += QLatin1String(" adb process error: ") + adbError;
    }
    m_adbProcess.release()->deleteLater();
    if (runAs.isEmpty() || !m_errorMessage.isEmpty()) {
        m_errorMessage = QLatin1String("Cannot find User for process: ")
                + QString::number(m_pid)
                + m_errorMessage;
        m_state = Idle;
        emit finished(m_errorMessage);
    } else {
        startAdbProcess(Kill, {m_adbPath, {"shell", "run-as", runAs, "kill",
                                           QString("-%1").arg(m_signal), QString::number(m_pid)}},
                        [this] { adbKillFinished(); });
    }
}

void AndroidSignalOperation::adbKillFinished()
{
    QTC_ASSERT(m_state == Kill, return);
    m_timeout->stop();

    if (m_adbProcess->exitStatus() != QProcess::NormalExit) {
        m_errorMessage = QLatin1String(" adb process exit code: ") + QString::number(m_adbProcess->exitCode());
        QString adbError = m_adbProcess->errorString();
        if (!adbError.isEmpty())
            m_errorMessage += QLatin1String(" adb process error: ") + adbError;
    } else {
        m_errorMessage = QString::fromLatin1(m_adbProcess->readAllStandardError());
    }
    m_adbProcess.release()->deleteLater();
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage = QLatin1String("Cannot kill process: ") + QString::number(m_pid)
                + m_errorMessage;
    }
    m_state = Idle;
    emit finished(m_errorMessage);
}

void AndroidSignalOperation::handleTimeout()
{
    m_adbProcess.reset();
    m_timeout->stop();
    m_state = Idle;
    m_errorMessage = QLatin1String("adb process timed out");
    emit finished(m_errorMessage);
}

void AndroidSignalOperation::signalOperationViaADB(qint64 pid, int signal)
{
    QTC_ASSERT(m_state == Idle, return);
    m_pid = pid;
    m_signal = signal;
    startAdbProcess(RunAs, {m_adbPath, {"shell", "cat", QString("/proc/%1/cmdline").arg(m_pid)}},
                    [this] { adbFindRunAsFinished(); });
}

void AndroidSignalOperation::startAdbProcess(State state, const Utils::CommandLine &commandLine,
                                             FinishHandler handler)
{
    m_state = state;
    m_timeout->start();
    m_adbProcess.reset(new QtcProcess);
    connect(m_adbProcess.get(), &QtcProcess::finished, this, handler);
    m_adbProcess->setCommand(commandLine);
    m_adbProcess->start();
}

void AndroidSignalOperation::killProcess(qint64 pid)
{
    signalOperationViaADB(pid, 9);
}

void AndroidSignalOperation::killProcess(const QString &filePath)
{
    Q_UNUSED(filePath)
    m_errorMessage = QLatin1String("The android signal operation does "
                                   "not support killing by filepath.");
    emit finished(m_errorMessage);
}

void AndroidSignalOperation::interruptProcess(qint64 pid)
{
    signalOperationViaADB(pid, 2);
}

void AndroidSignalOperation::interruptProcess(const QString &filePath)
{
    Q_UNUSED(filePath)
    m_errorMessage = QLatin1String("The android signal operation does "
                                   "not support interrupting by filepath.");
    emit finished(m_errorMessage);
}

} // Internal
} // Android
