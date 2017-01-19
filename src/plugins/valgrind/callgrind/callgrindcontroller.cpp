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
#include "../valgrindprocess.h"

#include <QDebug>
#include <QDir>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>
#include <ssh/sftpchannel.h>

#define CALLGRIND_CONTROL_DEBUG 0

const QLatin1String CALLGRIND_CONTROL_BINARY("callgrind_control");

namespace Valgrind {
namespace Callgrind {

CallgrindController::CallgrindController(QObject *parent)
    : QObject(parent)
    , m_process(0)
    , m_valgrindProc(0)
    , m_lastOption(Unknown)
{
}

CallgrindController::~CallgrindController()
{
    cleanupTempFile();
}

QString toOptionString(CallgrindController::Option option)
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
    if (m_process) {
        emit statusMessage(tr("Previous command has not yet finished."));
        return;
    }
    QTC_ASSERT(m_valgrindProc, return);

    m_process = new ValgrindProcess(m_valgrindProc->device(), this);

    connect(m_process, &ValgrindProcess::finished,
            this, &CallgrindController::processFinished);
    connect(m_process, &ValgrindProcess::error,
            this, &CallgrindController::processError);

    // save back current running operation
    m_lastOption = option;

    const QString optionString = toOptionString(option);

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
    m_process->setProcessChannelMode(QProcess::ForwardedChannels);
#endif
    const int pid = Utils::HostOsInfo::isWindowsHost() ? 0 : m_valgrindProc->pid();
    m_process->setValgrindExecutable(CALLGRIND_CONTROL_BINARY);
    m_process->setValgrindArguments(QStringList() << optionString << QString::number(pid));
    m_process->run(ProjectExplorer::ApplicationLauncher::Gui);
}

void CallgrindController::processError(QProcess::ProcessError)
{
    QTC_ASSERT(m_process, return);
    const QString error = m_process->errorString();
    emit statusMessage(tr("An error occurred while trying to run %1: %2").arg(CALLGRIND_CONTROL_BINARY).arg(error));

    m_process->deleteLater();
    m_process = 0;
}

void CallgrindController::processFinished(int rc, QProcess::ExitStatus status)
{
    QTC_ASSERT(m_process, return);
    const QString error = m_process->errorString();

    m_process->deleteLater(); // Called directly from finished() signal in m_process
    m_process = 0;

    if (rc != 0 || status != QProcess::NormalExit) {
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

void CallgrindController::setValgrindProcess(ValgrindProcess *proc)
{
    m_valgrindProc = proc;
}

void CallgrindController::getLocalDataFile()
{
    QTC_ASSERT(m_valgrindProc, return);

    // we look for callgrind.out.PID, but there may be updated ones called ~.PID.NUM
    QString baseFileName = QString::fromLatin1("callgrind.out.%1").
            arg(m_valgrindProc->pid());
    const QString workingDir = m_valgrindProc->workingDirectory();
    // first, set the to-be-parsed file to callgrind.out.PID
    QString fileName = workingDir.isEmpty() ? baseFileName : (workingDir + QLatin1Char('/') + baseFileName);

    if (!m_valgrindProc->isLocal()) {
        ///TODO: error handling
        emit statusMessage(tr("Downloading remote profile data..."));
        m_ssh = m_valgrindProc->connection();
        // if there are files like callgrind.out.PID.NUM, set it to the most recent one of those
        QString cmd = QString::fromLatin1("ls -t %1* | head -n 1").arg(fileName);
        m_findRemoteFile = m_ssh->createRemoteProcess(cmd.toUtf8());
        connect(m_findRemoteFile.data(), &QSsh::SshRemoteProcess::readyReadStandardOutput,
                this, &CallgrindController::foundRemoteFile);
        m_findRemoteFile->start();
    } else {
        QDir dir(workingDir, QString::fromLatin1("%1.*").arg(baseFileName), QDir::Time);
        QStringList outputFiles = dir.entryList();
        // if there are files like callgrind.out.PID.NUM, set it to the most recent one of those
        if (!outputFiles.isEmpty())
            fileName = workingDir + QLatin1Char('/') + dir.entryList().first();

        emit localParseDataAvailable(fileName);
    }
}

void CallgrindController::foundRemoteFile()
{
    m_remoteFile = m_findRemoteFile->readAllStandardOutput().trimmed();

    m_sftp = m_ssh->createSftpChannel();
    connect(m_sftp.data(), &QSsh::SftpChannel::finished,
            this, &CallgrindController::sftpJobFinished);
    connect(m_sftp.data(), &QSsh::SftpChannel::initialized,
            this, &CallgrindController::sftpInitialized);
    m_sftp->initialize();
}

void CallgrindController::sftpInitialized()
{
    cleanupTempFile();
    Utils::TemporaryFile dataFile("callgrind.out.");
    QTC_ASSERT(dataFile.open(), return);
    m_tempDataFile = dataFile.fileName();
    dataFile.setAutoRemove(false);
    dataFile.close();

    m_downloadJob = m_sftp->downloadFile(QString::fromUtf8(m_remoteFile), m_tempDataFile, QSsh::SftpOverwriteExisting);
}

void CallgrindController::sftpJobFinished(QSsh::SftpJobId job, const QString &error)
{
    QTC_ASSERT(job == m_downloadJob, return);

    m_sftp->closeChannel();

    if (error.isEmpty())
        emit localParseDataAvailable(m_tempDataFile);
}

void CallgrindController::cleanupTempFile()
{
    if (!m_tempDataFile.isEmpty() && QFile::exists(m_tempDataFile))
        QFile::remove(m_tempDataFile);

    m_tempDataFile.clear();
}

} // namespace Callgrind
} // namespace Valgrind
