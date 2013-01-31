/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion and Hugues Delorme
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

#include "command.h"
#include "vcsbaseconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/synchronousprocess.h>

#include <QDebug>
#include <QProcess>
#include <QFuture>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QCoreApplication>

Q_DECLARE_METATYPE(QVariant)

static QString msgTermination(int exitCode, const QString &binaryPath, const QStringList &args)
{
    QString cmd = QFileInfo(binaryPath).baseName();
    if (!args.empty()) {
        cmd += QLatin1Char(' ');
        cmd += args.front();
    }
    return exitCode ?
                QCoreApplication::translate("VcsCommand", "\n'%1' failed (exit code %2).\n").arg(cmd).arg(exitCode) :
                QCoreApplication::translate("VcsCommand", "\n'%1' completed (exit code %2).\n").arg(cmd).arg(exitCode);
}

namespace VcsBase {
namespace Internal {

class CommandPrivate
{
public:
    struct Job {
        explicit Job(const QStringList &a, int t);

        QStringList arguments;
        int timeout;
    };

    CommandPrivate(const QString &binary,
                   const QString &workingDirectory,
                   const QProcessEnvironment &environment);

    const QString m_binaryPath;
    const QString m_workingDirectory;
    const QProcessEnvironment m_environment;
    QVariant m_cookie;
    bool m_unixTerminalDisabled;
    int m_defaultTimeout;

    QList<Job> m_jobs;
    Command::TerminationReportMode m_reportTerminationMode;

    bool m_lastExecSuccess;
    int m_lastExecExitCode;
};

CommandPrivate::CommandPrivate(const QString &binary,
                               const QString &workingDirectory,
                               const QProcessEnvironment &environment) :
    m_binaryPath(binary),
    m_workingDirectory(workingDirectory),
    m_environment(environment),
    m_unixTerminalDisabled(false),
    m_defaultTimeout(10),
    m_reportTerminationMode(Command::NoReport),
    m_lastExecSuccess(false),
    m_lastExecExitCode(-1)
{
}

CommandPrivate::Job::Job(const QStringList &a, int t) :
    arguments(a),
    timeout(t)
{
    // Finished cookie is emitted via queued slot, needs metatype
    static const int qvMetaId = qRegisterMetaType<QVariant>();
    Q_UNUSED(qvMetaId)
}

} // namespace Internal

Command::Command(const QString &binary,
                 const QString &workingDirectory,
                 const QProcessEnvironment &environment) :
    d(new Internal::CommandPrivate(binary, workingDirectory, environment))
{
}

Command::~Command()
{
    delete d;
}

const QString &Command::binaryPath() const
{
    return d->m_binaryPath;
}

const QString &Command::workingDirectory() const
{
    return d->m_workingDirectory;
}

const QProcessEnvironment &Command::processEnvironment() const
{
    return d->m_environment;
}

Command::TerminationReportMode Command::reportTerminationMode() const
{
    return d->m_reportTerminationMode;
}

void Command::setTerminationReportMode(TerminationReportMode m)
{
    d->m_reportTerminationMode = m;
}

int Command::defaultTimeout() const
{
    return d->m_defaultTimeout;
}

void Command::setDefaultTimeout(int timeout)
{
    d->m_defaultTimeout = timeout;
}

bool Command::unixTerminalDisabled() const
{
    return d->m_unixTerminalDisabled;
}

void Command::setUnixTerminalDisabled(bool e)
{
    d->m_unixTerminalDisabled = e;
}

void Command::addJob(const QStringList &arguments)
{
    addJob(arguments, defaultTimeout());
}

void Command::addJob(const QStringList &arguments, int timeout)
{
    d->m_jobs.push_back(Internal::CommandPrivate::Job(arguments, timeout));
}

void Command::execute()
{
    d->m_lastExecSuccess = false;
    d->m_lastExecExitCode = -1;

    if (d->m_jobs.empty())
        return;

    // For some reason QtConcurrent::run() only works on this
    QFuture<void> task = QtConcurrent::run(this, &Command::run);
    QString binary = QFileInfo(d->m_binaryPath).baseName();
    if (!binary.isEmpty())
        binary = binary.replace(0, 1, binary[0].toUpper()); // Upper the first letter
    const QString taskName = binary + QLatin1Char(' ') + d->m_jobs.front().arguments.at(0);

    Core::ICore::progressManager()->addTask(task, taskName, binary + QLatin1String(".action"));
}

bool Command::lastExecutionSuccess() const
{
    return d->m_lastExecSuccess;
}

int Command::lastExecutionExitCode() const
{
    return d->m_lastExecExitCode;
}

QString Command::msgTimeout(int seconds)
{
    return tr("Error: VCS timed out after %1s.").arg(seconds);
}

void Command::run()
{
    // Check that the binary path is not empty
    if (binaryPath().trimmed().isEmpty()) {
        emit errorText(tr("Unable to start process, binary is empty"));
        return;
    }

    const unsigned processFlags = unixTerminalDisabled() ?
                unsigned(Utils::SynchronousProcess::UnixTerminalDisabled) :
                unsigned(0);
    const QSharedPointer<QProcess> process = Utils::SynchronousProcess::createProcess(processFlags);
    if (!workingDirectory().isEmpty())
        process->setWorkingDirectory(workingDirectory());

    process->setProcessEnvironment(processEnvironment());

    QByteArray stdOut;
    QByteArray stdErr;
    QString error;

    const int count = d->m_jobs.size();
    int exitCode = -1;
    bool ok = true;
    for (int j = 0; j < count; j++) {
        process->start(binaryPath(), d->m_jobs.at(j).arguments);
        if (!process->waitForStarted()) {
            ok = false;
            error += QString::fromLatin1("Error: \"%1\" could not be started: %2")
                    .arg(binaryPath(), process->errorString());
            break;
        }

        process->closeWriteChannel();
        const int timeOutSeconds = d->m_jobs.at(j).timeout;
        if (!Utils::SynchronousProcess::readDataFromProcess(*process, timeOutSeconds * 1000,
                                                            &stdOut, &stdErr, false)) {
            Utils::SynchronousProcess::stopProcess(*process);
            ok = false;
            error += msgTimeout(timeOutSeconds);
            break;
        }

        error += QString::fromLocal8Bit(stdErr);
        exitCode = process->exitCode();
        switch (reportTerminationMode()) {
        case NoReport:
            break;
        case ReportStdout:
            stdOut += msgTermination(exitCode, binaryPath(), d->m_jobs.at(j).arguments).toUtf8();
            break;
        case ReportStderr:
            error += msgTermination(exitCode, binaryPath(), d->m_jobs.at(j).arguments);
            break;
        }
    }

    // Special hack: Always produce output for diff
    if (ok && stdOut.isEmpty() && d->m_jobs.front().arguments.at(0) == QLatin1String("diff")) {
        stdOut += "No difference to HEAD";
    } else {
        // @TODO: Remove, see below
        if (ok && d->m_jobs.front().arguments.at(0) == QLatin1String("status"))
            removeColorCodes(&stdOut);
    }

    d->m_lastExecSuccess = ok;
    d->m_lastExecExitCode = exitCode;

    if (ok && !stdOut.isEmpty())
        emit outputData(stdOut);

    if (!error.isEmpty())
        emit errorText(error);

    emit finished(ok, exitCode, cookie());
    if (ok)
        emit success(cookie());
    // As it is used asynchronously, we need to delete ourselves
    this->deleteLater();
}

// Clean output from carriage return and ANSI color codes.
// @TODO: Remove once all relevant commands support "--no-color",
//("status" is  missing it as of git 1.6.2)

void Command::removeColorCodes(QByteArray *data)
{
    // Remove ansi color codes that look like "ESC[<stuff>m"
    const QByteArray ansiColorEscape("\033[");
    int escapePos = 0;
    while (true) {
        const int nextEscapePos = data->indexOf(ansiColorEscape, escapePos);
        if (nextEscapePos == -1)
            break;
        const int endEscapePos = data->indexOf('m', nextEscapePos + ansiColorEscape.size());
        if (endEscapePos != -1) {
            data->remove(nextEscapePos, endEscapePos - nextEscapePos + 1);
            escapePos = nextEscapePos;
        } else {
            escapePos = nextEscapePos + ansiColorEscape.size();
        }
    }
}

const QVariant &Command::cookie() const
{
    return d->m_cookie;
}

void Command::setCookie(const QVariant &cookie)
{
    d->m_cookie = cookie;
}

} // namespace VcsBase
