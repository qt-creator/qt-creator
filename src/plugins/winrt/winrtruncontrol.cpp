/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "winrtruncontrol.h"
#include "winrtrunconfiguration.h"
#include "winrtconstants.h"
#include "winrtdevice.h"

#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtkitinformation.h>

using ProjectExplorer::DeviceKitInformation;
using ProjectExplorer::IDevice;
using ProjectExplorer::RunControl;
using ProjectExplorer::RunMode;
using ProjectExplorer::Target;
using Utils::QtcProcess;

namespace WinRt {
namespace Internal {

WinRtRunControl::WinRtRunControl(WinRtRunConfiguration *runConfiguration, RunMode mode)
    : RunControl(runConfiguration, mode)
    , m_state(StoppedState)
    , m_process(0)
{
    Target *target = runConfiguration->target();
    IDevice::ConstPtr device = DeviceKitInformation::device(target->kit());
    m_device = device.dynamicCast<const WinRtDevice>();

    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!qt) {
        appendMessage(tr("The current kit has no Qt version."),
                      Utils::ErrorMessageFormat);
        return;
    }

    m_isWinPhone = (qt->type() == QLatin1String(Constants::WINRT_WINPHONEQT));
    m_runnerFilePath = qt->binPath().toString() + QStringLiteral("/winrtrunner.exe");
    if (!QFile::exists(m_runnerFilePath)) {
        appendMessage(tr("Cannot find winrtrunner.exe in '%1'.").arg(
                          QDir::toNativeSeparators(qt->binPath().toString())),
                      Utils::ErrorMessageFormat);
        return;
    }

    const Utils::FileName proFile = Utils::FileName::fromString(
                target->project()->document()->filePath());
    m_executableFilePath = target->applicationTargets().targetForProject(proFile).toString()
                + QStringLiteral(".exe");   // ### we should not need to append ".exe" here.

    m_arguments = runConfiguration->arguments();
    m_uninstallAfterStop = runConfiguration->uninstallAfterStop();
}

void WinRtRunControl::start()
{
    if (m_state != StoppedState)
        return;
    if (!startWinRtRunner())
        m_state = StoppedState;
}

RunControl::StopResult WinRtRunControl::stop()
{
    if (m_state != StartedState) {
        m_state = StoppedState;
        return StoppedSynchronously;
    }

    QTC_ASSERT(m_process, return StoppedSynchronously);
    m_process->interrupt();
    return AsynchronousStop;
}

bool WinRtRunControl::isRunning() const
{
    return m_state == StartedState;
}

QIcon WinRtRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
}

void WinRtRunControl::cannotRetrieveDebugOutput()
{
    appendMessage(tr("Cannot retrieve debugging output.\n"), Utils::ErrorMessageFormat);
}

void WinRtRunControl::onProcessStarted()
{
    QTC_CHECK(m_state == StartingState);
    m_state = StartedState;
    emit started();
}

void WinRtRunControl::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QTC_ASSERT(m_process, return);
    QTC_CHECK(m_state == StartedState);

    if (exitStatus == QProcess::CrashExit) {
        appendMessage(tr("winrtrunner crashed\n"), Utils::ErrorMessageFormat);
    } else if (exitCode != 0) {
        appendMessage(tr("winrtrunner returned with exit code %1\n").arg(exitCode),
                Utils::ErrorMessageFormat);
    } else {
        appendMessage(tr("winrtrunner finished successfully.\n"), Utils::NormalMessageFormat);
    }

    m_process->disconnect();
    m_process->deleteLater();
    m_process = 0;
    m_state = StoppedState;
    emit finished();
}

void WinRtRunControl::onProcessError()
{
    QTC_ASSERT(m_process, return);
    appendMessage(tr("Error while executing winrtrunner: %1\n").arg(m_process->errorString()),
            Utils::ErrorMessageFormat);
    m_process->disconnect();
    m_process->deleteLater();
    m_process = 0;
    m_state = StoppedState;
    emit finished();
}

void WinRtRunControl::onProcessReadyReadStdOut()
{
    QTC_ASSERT(m_process, return);
    appendMessage(QString::fromLocal8Bit(m_process->readAllStandardOutput()), Utils::StdOutFormat);
}

void WinRtRunControl::onProcessReadyReadStdErr()
{
    QTC_ASSERT(m_process, return);
    appendMessage(QString::fromLocal8Bit(m_process->readAllStandardError()), Utils::StdErrFormat);
}

bool WinRtRunControl::startWinRtRunner()
{
    QString runnerArgs;
    QtcProcess::addArg(&runnerArgs, QStringLiteral("--profile"));
    QtcProcess::addArg(&runnerArgs, m_isWinPhone ? QStringLiteral("xap") : QStringLiteral("appx"));
    if (m_device) {
        QtcProcess::addArg(&runnerArgs, QStringLiteral("--device"));
        QtcProcess::addArg(&runnerArgs, QString::number(m_device->deviceId()));
    }
    QtcProcess::addArgs(&runnerArgs, QStringLiteral("--install --start --stop --wait 0"));
    QtcProcess::addArg(&runnerArgs, m_executableFilePath);
    if (!m_arguments.isEmpty())
        QtcProcess::addArgs(&runnerArgs, m_arguments);

    appendMessage(QStringLiteral("winrtrunner ") + runnerArgs + QLatin1Char('\n'),
                  Utils::NormalMessageFormat);

    QTC_ASSERT(!m_process, m_process->deleteLater());
    m_process = new QtcProcess(this);
    connect(m_process, SIGNAL(started()), SLOT(onProcessStarted()));
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            SLOT(onProcessFinished(int,QProcess::ExitStatus)));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)), SLOT(onProcessError()));
    connect(m_process, SIGNAL(readyReadStandardOutput()), SLOT(onProcessReadyReadStdOut()));
    connect(m_process, SIGNAL(readyReadStandardError()), SLOT(onProcessReadyReadStdErr()));

    m_state = StartingState;
    m_process->setUseCtrlCStub(true);
    m_process->setCommand(m_runnerFilePath, runnerArgs);
    m_process->setWorkingDirectory(QFileInfo(m_executableFilePath).absolutePath());
    m_process->start();
    return true;
}

} // namespace Internal
} // namespace WinRt
