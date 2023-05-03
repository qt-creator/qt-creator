// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidsignaloperation.h"

#include <utils/process.h>
#include <utils/qtcassert.h>

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

AndroidSignalOperation::~AndroidSignalOperation() = default;

bool AndroidSignalOperation::handleCrashMessage()
{
    if (m_adbProcess->exitStatus() == QProcess::NormalExit)
        return false;
    m_errorMessage = QLatin1String(" adb process exit code: ") + QString::number(m_adbProcess->exitCode());
    const QString adbError = m_adbProcess->errorString();
    if (!adbError.isEmpty())
        m_errorMessage += QLatin1String(" adb process error: ") + adbError;
    return true;
}

void AndroidSignalOperation::adbFindRunAsFinished()
{
    QTC_ASSERT(m_state == RunAs, return);
    m_timeout->stop();

    handleCrashMessage();
    const QString runAs = QString::fromLatin1(m_adbProcess->readAllRawStandardOutput());
    m_adbProcess.release()->deleteLater();
    if (runAs.isEmpty() || !m_errorMessage.isEmpty()) {
        m_errorMessage.prepend(QLatin1String("Cannot find User for process: ")
                               + QString::number(m_pid));
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

    if (!handleCrashMessage())
        m_errorMessage = QString::fromLatin1(m_adbProcess->readAllRawStandardError());
    m_adbProcess.release()->deleteLater();
    if (!m_errorMessage.isEmpty())
        m_errorMessage.prepend(QLatin1String("Cannot kill process: ") + QString::number(m_pid));
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
    m_adbProcess.reset(new Process);
    connect(m_adbProcess.get(), &Process::done, this, handler);
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
