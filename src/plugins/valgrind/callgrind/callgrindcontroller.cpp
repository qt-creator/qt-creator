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

#include "callgrindcontroller.h"

#include <utils/temporaryfile.h>
#include <utils/qtcprocess.h>

#include <QDebug>

#define CALLGRIND_CONTROL_DEBUG 0

using namespace ProjectExplorer;
using namespace Utils;

namespace Valgrind {
namespace Callgrind {

const char CALLGRIND_CONTROL_BINARY[] = "callgrind_control";

CallgrindController::CallgrindController() = default;

CallgrindController::~CallgrindController()
{
    cleanupTempFile();
}

static QString toOptionString(CallgrindController::Option option)
{
    /* callgrind_control help from v3.9.0

    Options:
    -h --help        Show this help text
    --version        Show version
    -s --stat        Show statistics
    -b --back        Show stack/back trace
    -e [<A>,...]     Show event counters for <A>,... (default: all)
    --dump[=<s>]     Request a dump optionally using <s> as description
    -z --zero        Zero all event counters
    -k --kill        Kill
    --instr=<on|off> Switch instrumentation state on/off
    */

    switch (option) {
        case CallgrindController::Dump:
            return QLatin1String("--dump");
        case CallgrindController::ResetEventCounters:
            return QLatin1String("--zero");
        case CallgrindController::Pause:
            return QLatin1String("--instr=off");
        case CallgrindController::UnPause:
            return QLatin1String("--instr=on");
        default:
            return QString(); // never reached
    }
}

void CallgrindController::run(Option option)
{
    if (m_controllerProcess) {
        emit statusMessage(tr("Previous command has not yet finished."));
        return;
    }

    // save back current running operation
    m_lastOption = option;

    m_controllerProcess.reset(new QtcProcess);

    switch (option) {
        case CallgrindController::Dump:
            emit statusMessage(tr("Dumping profile data..."));
            break;
        case CallgrindController::ResetEventCounters:
            emit statusMessage(tr("Resetting event counters..."));
            break;
        case CallgrindController::Pause:
            emit statusMessage(tr("Pausing instrumentation..."));
            break;
        case CallgrindController::UnPause:
            emit statusMessage(tr("Unpausing instrumentation..."));
            break;
        default:
            break;
    }

#if CALLGRIND_CONTROL_DEBUG
    m_controllerProcess->setProcessChannelMode(QProcess::ForwardedChannels);
#endif
    connect(m_controllerProcess.get(), &QtcProcess::finished,
            this, &CallgrindController::controllerProcessDone);

    const FilePath control =
            FilePath(CALLGRIND_CONTROL_BINARY).onDevice(m_valgrindRunnable.command.executable());
    m_controllerProcess->setCommand({control, {toOptionString(option), QString::number(m_pid)}});
    m_controllerProcess->setWorkingDirectory(m_valgrindRunnable.workingDirectory);
    m_controllerProcess->setEnvironment(m_valgrindRunnable.environment);
    m_controllerProcess->start();
}

void CallgrindController::setValgrindPid(qint64 pid)
{
    m_pid = pid;
}

void CallgrindController::controllerProcessDone()
{
    const QString error = m_controllerProcess->errorString();
    const ProcessResult result = m_controllerProcess->result();

    m_controllerProcess.release()->deleteLater();

    if (result != ProcessResult::FinishedWithSuccess) {
        emit statusMessage(tr("An error occurred while trying to run %1: %2").arg(CALLGRIND_CONTROL_BINARY).arg(error));
        qWarning() << "Controller exited abnormally:" << error;
        return;
    }

    // this call went fine, we might run another task after this
    switch (m_lastOption) {
        case ResetEventCounters:
            // lets dump the new reset profiling info
            run(Dump);
            return;
        case Pause:
            break;
        case Dump:
            emit statusMessage(tr("Callgrind dumped profiling info"));
            break;
        case UnPause:
            emit statusMessage(tr("Callgrind unpaused."));
            break;
        default:
            break;
    }

    emit finished(m_lastOption);
    m_lastOption = Unknown;
}

void CallgrindController::getLocalDataFile()
{
    cleanupTempFile();
    {
        TemporaryFile dataFile("callgrind.out");
        dataFile.open();
        m_hostOutputFile = FilePath::fromString(dataFile.fileName());
    }

    const auto afterCopy = [this](bool res) {
        QTC_CHECK(res);
        emit localParseDataAvailable(m_hostOutputFile);
    };
    m_valgrindOutputFile.asyncCopyFile(afterCopy, m_hostOutputFile);
}

void CallgrindController::cleanupTempFile()
{
    if (!m_hostOutputFile.isEmpty() && m_hostOutputFile.exists())
        m_hostOutputFile.removeFile();

    m_hostOutputFile.clear();
}

void CallgrindController::setValgrindRunnable(const Runnable &runnable)
{
    m_valgrindRunnable = runnable;
}

} // namespace Callgrind
} // namespace Valgrind
