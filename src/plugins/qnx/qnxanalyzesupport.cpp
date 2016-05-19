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

#include "qnxanalyzesupport.h"

#include "qnxdevice.h"
#include "qnxrunconfiguration.h"
#include "slog2inforunner.h"

#include <debugger/analyzer/analyzerruncontrol.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

QnxAnalyzeSupport::QnxAnalyzeSupport(QnxRunConfiguration *runConfig,
                                     Debugger::AnalyzerRunControl *runControl)
    : QnxAbstractRunSupport(runConfig, runControl)
    , m_runnable(runConfig->runnable().as<StandardRunnable>())
    , m_runControl(runControl)
    , m_qmlPort(-1)
{
    const DeviceApplicationRunner *runner = appRunner();
    connect(runner, &DeviceApplicationRunner::reportError,
            this, &QnxAnalyzeSupport::handleError);
    connect(runner, &DeviceApplicationRunner::remoteProcessStarted,
            this, &QnxAbstractRunSupport::handleRemoteProcessStarted);
    connect(runner, &DeviceApplicationRunner::finished,
            this, &QnxAnalyzeSupport::handleRemoteProcessFinished);
    connect(runner, &DeviceApplicationRunner::reportProgress,
            this, &QnxAnalyzeSupport::handleProgressReport);
    connect(runner, &DeviceApplicationRunner::remoteStdout,
            this, &QnxAnalyzeSupport::handleRemoteOutput);
    connect(runner, &DeviceApplicationRunner::remoteStderr,
            this, &QnxAnalyzeSupport::handleRemoteOutput);

    connect(m_runControl, &Debugger::AnalyzerRunControl::starting,
            this, &QnxAnalyzeSupport::handleAdapterSetupRequested);
    connect(&m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
            this, &QnxAnalyzeSupport::remoteIsRunning);

    IDevice::ConstPtr dev = DeviceKitInformation::device(runConfig->target()->kit());
    QnxDevice::ConstPtr qnxDevice = dev.dynamicCast<const QnxDevice>();

    const QString applicationId = FileName::fromString(runConfig->remoteExecutableFilePath()).fileName();
    m_slog2Info = new Slog2InfoRunner(applicationId, qnxDevice, this);
    connect(m_slog2Info, &Slog2InfoRunner::output,
            this, &QnxAnalyzeSupport::showMessage);
    connect(runner, &DeviceApplicationRunner::remoteProcessStarted,
            m_slog2Info, &Slog2InfoRunner::start);
    if (qnxDevice->qnxVersion() > 0x060500)
        connect(m_slog2Info, &Slog2InfoRunner::commandMissing,
                this, &QnxAnalyzeSupport::printMissingWarning);
}

void QnxAnalyzeSupport::handleAdapterSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Preparing remote side...") + QLatin1Char('\n'), NormalMessageFormat);
    QnxAbstractRunSupport::handleAdapterSetupRequested();
}

void QnxAnalyzeSupport::startExecution()
{
    if (state() == Inactive)
        return;

    if (!setPort(m_qmlPort) && !m_qmlPort.isValid())
        return;

    setState(StartingRemoteProcess);

    StandardRunnable r = m_runnable;
    if (!r.commandLineArguments.isEmpty())
        r.commandLineArguments += QLatin1Char(' ');
    r.commandLineArguments += QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlProfilerServices,
                                                             m_qmlPort);
    appRunner()->start(device(), r);
}

void QnxAnalyzeSupport::handleRemoteProcessFinished(bool success)
{
    if (!m_runControl)
        return;

    if (!success)
        showMessage(tr("The %1 process closed unexpectedly.").arg(m_runnable.executable),
                    NormalMessageFormat);
    m_runControl->notifyRemoteFinished();

    m_slog2Info->stop();
}

void QnxAnalyzeSupport::handleProfilingFinished()
{
    setFinished();
}

void QnxAnalyzeSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), NormalMessageFormat);
}

void QnxAnalyzeSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == Inactive || state() == Running, return);

    showMessage(QString::fromUtf8(output), StdOutFormat);
}

void QnxAnalyzeSupport::handleError(const QString &error)
{
    if (state() == Running) {
        showMessage(error, ErrorMessageFormat);
    } else if (state() != Inactive) {
        showMessage(tr("Initial setup failed: %1").arg(error), NormalMessageFormat);
        setFinished();
    }
}

void QnxAnalyzeSupport::remoteIsRunning()
{
    if (m_runControl)
        m_runControl->notifyRemoteSetupDone(m_qmlPort);
}

void QnxAnalyzeSupport::showMessage(const QString &msg, OutputFormat format)
{
    if (state() != Inactive && m_runControl)
        m_runControl->appendMessage(msg, format);
    m_outputParser.processOutput(msg);
}

void QnxAnalyzeSupport::printMissingWarning()
{
    showMessage(tr("Warning: \"slog2info\" is not found on the device, debug output not available."),
                ErrorMessageFormat);
}

} // namespace Internal
} // namespace Qnx
