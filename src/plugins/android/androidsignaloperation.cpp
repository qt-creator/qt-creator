/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidconfigurations.h"
#include "androidsignaloperation.h"

#include <utils/qtcassert.h>

#include <QProcess>

Android::Internal::AndroidSignalOperation::AndroidSignalOperation()
    : m_adbPath(AndroidConfigurations::currentConfig().adbToolPath().toString())
    , m_adbProcess(new QProcess(this))
    , m_timeout(new QTimer(this))
    , m_state(Idle)
    , m_pid(0)
    , m_signal(0)
{
    m_timeout->setInterval(5000);
    connect(m_timeout, SIGNAL(timeout()), this, SLOT(handleTimeout()));
}

void Android::Internal::AndroidSignalOperation::adbFindRunAsFinished(int exitCode,
                                                                    QProcess::ExitStatus exitStatus)
{
    QTC_ASSERT(m_state == RunAs, return);
    m_timeout->stop();
    m_adbProcess->disconnect(this);

    QString runAs = QString::fromLatin1(m_adbProcess->readAllStandardOutput());
    if (exitStatus != QProcess::NormalExit) {
        m_errorMessage = QLatin1String(" adb Exit code: ") + QString::number(exitCode);
        QString adbError = m_adbProcess->errorString();
        if (!adbError.isEmpty())
            m_errorMessage += QLatin1String(" adb process error: ") + adbError;
    }
    if (runAs.isEmpty() || !m_errorMessage.isEmpty()) {
        m_errorMessage = QLatin1String("Can not find User for process: ")
                + QString::number(m_pid)
                + m_errorMessage;
        m_state = Idle;
        emit finished(m_errorMessage);
    } else {
        connect(m_adbProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                SLOT(adbKillFinished(int,QProcess::ExitStatus)));
        m_state = Kill;
        m_timeout->start();
        m_adbProcess->start(m_adbPath, QStringList()
                            << QLatin1String("shell")
                            << QLatin1String("run-as")
                            << runAs
                            << QLatin1String("kill")
                            << QString::fromLatin1("-%1").arg(m_signal)
                            << QString::number(m_pid));
    }
}

void Android::Internal::AndroidSignalOperation::adbKillFinished(int exitCode,
                                                                QProcess::ExitStatus exitStatus)
{
    QTC_ASSERT(m_state == Kill, return);
    m_timeout->stop();
    m_adbProcess->disconnect(this);

    if (exitStatus != QProcess::NormalExit) {
        m_errorMessage = QLatin1String(" adb process exit code: ") + QString::number(exitCode);
        QString adbError = m_adbProcess->errorString();
        if (!adbError.isEmpty())
            m_errorMessage += QLatin1String(" adb process error: ") + adbError;
    } else {
        m_errorMessage = QString::fromLatin1(m_adbProcess->readAllStandardError());
    }
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage = QLatin1String("Can not kill process: ") + QString::number(m_pid)
                + m_errorMessage;
    }
    m_state = Idle;
    emit finished(m_errorMessage);
}

void Android::Internal::AndroidSignalOperation::handleTimeout()
{
    m_adbProcess->disconnect(this);
    m_adbProcess->kill();
    m_timeout->stop();
    m_state = Idle;
    m_errorMessage = QLatin1String("adb process timed out");
    emit finished(m_errorMessage);
}

void Android::Internal::AndroidSignalOperation::signalOperationViaADB(int pid, int signal)
{
    QTC_ASSERT(m_state == Idle, return);
    m_adbProcess->disconnect(this);
    m_pid = pid;
    m_signal = signal;
    connect(m_adbProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
            SLOT(adbFindRunAsFinished(int,QProcess::ExitStatus)));
    m_state = RunAs;
    m_timeout->start();
    m_adbProcess->start(m_adbPath, QStringList()
                        << QLatin1String("shell")
                        << QLatin1String("cat")
                        << QString::fromLatin1("/proc/%1/cmdline").arg(m_pid));
}

void Android::Internal::AndroidSignalOperation::killProcess(int pid)
{
    signalOperationViaADB(pid, 9);
}

void Android::Internal::AndroidSignalOperation::killProcess(const QString &filePath)
{
    Q_UNUSED(filePath);
    m_errorMessage = QLatin1String("The android signal operation does "
                                   "not support killing by filepath.");
    emit finished(m_errorMessage);
}

void Android::Internal::AndroidSignalOperation::interruptProcess(int pid)
{
    signalOperationViaADB(pid, 2);
}

void Android::Internal::AndroidSignalOperation::interruptProcess(const QString &filePath)
{
    Q_UNUSED(filePath);
    m_errorMessage = QLatin1String("The android signal operation does "
                                   "not support interrupting by filepath.");
    emit finished(m_errorMessage);
}
