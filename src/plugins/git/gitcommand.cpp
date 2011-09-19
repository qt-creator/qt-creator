/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "gitcommand.h"
#include "gitconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/synchronousprocess.h>

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QFuture>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>

Q_DECLARE_METATYPE(QVariant)

namespace Git {
namespace Internal {

static QString msgTermination(int exitCode, const QString &binaryPath, const QStringList &args)
{
    QString cmd = QFileInfo(binaryPath).baseName();
    if (!args.empty()) {
        cmd += QLatin1Char(' ');
        cmd += args.front();
    }
    return exitCode ?
            QCoreApplication::translate("GitCommand", "\n'%1' failed (exit code %2).\n").arg(cmd).arg(exitCode) :
            QCoreApplication::translate("GitCommand", "\n'%1' completed (exit code %2).\n").arg(cmd).arg(exitCode);
}

GitCommand::Job::Job(const QStringList &a, int t) :
    arguments(a),
    timeout(t)
{
    // Finished cookie is emitted via queued slot, needs metatype
    static const int qvMetaId = qRegisterMetaType<QVariant>();
    Q_UNUSED(qvMetaId)
}

GitCommand::GitCommand(const QString &binary,
                       const QString &workingDirectory,
                       const QProcessEnvironment&environment,
                       const QVariant &cookie)  :
    m_binaryPath(binary),
    m_workingDirectory(workingDirectory),
    m_environment(environment),
    m_cookie(cookie),
    m_unixTerminalDisabled(false),
    m_reportTerminationMode(NoReport)
{
}

GitCommand::TerminationReportMode GitCommand::reportTerminationMode() const
{
    return m_reportTerminationMode;
}

void GitCommand::setTerminationReportMode(TerminationReportMode m)
{
    m_reportTerminationMode = m;
}

bool GitCommand::unixTerminalDisabled() const
{
    return m_unixTerminalDisabled;
}

void GitCommand::setUnixTerminalDisabled(bool e)
{
    m_unixTerminalDisabled = e;
}

void GitCommand::addJob(const QStringList &arguments, int timeout)
{
    m_jobs.push_back(Job(arguments, timeout));
}

void GitCommand::execute()
{
    if (Git::Constants::debug)
        qDebug() << "GitCommand::execute" << m_workingDirectory << m_jobs.size();

    if (m_jobs.empty())
        return;

    // For some reason QtConcurrent::run() only works on this
    QFuture<void> task = QtConcurrent::run(this, &GitCommand::run);
    const QString taskName = QLatin1String("Git ") + m_jobs.front().arguments.at(0);

    Core::ICore::instance()->progressManager()->addTask(task, taskName,
                                     QLatin1String("Git.action"));
}

QString GitCommand::msgTimeout(int seconds)
{
    return tr("Error: Git timed out after %1s.").arg(seconds);
}

void GitCommand::run()
{
    if (Git::Constants::debug)
        qDebug() << "GitCommand::run" << m_workingDirectory << m_jobs.size()
                 << "terminal_disabled" << m_unixTerminalDisabled;

    const unsigned processFlags = m_unixTerminalDisabled ?
                            unsigned(Utils::SynchronousProcess::UnixTerminalDisabled) :
                            unsigned(0);
    const QSharedPointer<QProcess> process =
            Utils::SynchronousProcess::createProcess(processFlags);
    if (!m_workingDirectory.isEmpty())
        process->setWorkingDirectory(m_workingDirectory);

    process->setProcessEnvironment(m_environment);

    QByteArray stdOut;
    QByteArray stdErr;
    QString error;

    const int count = m_jobs.size();
    int exitCode = -1;
    bool ok = true;
    for (int j = 0; j < count; j++) {
        if (Git::Constants::debug)
            qDebug() << "GitCommand::run" << j << '/' << count << m_jobs.at(j).arguments;

        process->start(m_binaryPath, m_jobs.at(j).arguments);
        if (!process->waitForStarted()) {
            ok = false;
            error += QString::fromLatin1("Error: \"%1\" could not be started: %2").arg(m_binaryPath, process->errorString());
            break;
        }

        process->closeWriteChannel();
        const int timeOutSeconds = m_jobs.at(j).timeout;
        if (!Utils::SynchronousProcess::readDataFromProcess(*process, timeOutSeconds * 1000,
                                                            &stdOut, &stdErr, false)) {
            Utils::SynchronousProcess::stopProcess(*process);
            ok = false;
            error += msgTimeout(timeOutSeconds);
            break;
        }

        error += QString::fromLocal8Bit(stdErr);
        exitCode = process->exitCode();
        switch (m_reportTerminationMode) {
        case NoReport:
            break;
        case ReportStdout:
            stdOut += msgTermination(exitCode, m_binaryPath, m_jobs.at(j).arguments).toUtf8();
            break;
        case ReportStderr:
            error += msgTermination(exitCode, m_binaryPath, m_jobs.at(j).arguments);
            break;
        }
    }

    // Special hack: Always produce output for diff
    if (ok && stdOut.isEmpty() && m_jobs.front().arguments.at(0) == QLatin1String("diff")) {
        stdOut += "No difference to HEAD";
    } else {
        // @TODO: Remove, see below
        if (ok && m_jobs.front().arguments.at(0) == QLatin1String("status"))
            removeColorCodes(&stdOut);
    }

    if (ok && !stdOut.isEmpty())
        emit outputData(stdOut);

    if (!error.isEmpty())
        emit errorText(error);

    emit finished(ok, exitCode, m_cookie);
    if (ok)
        emit success();
    // As it is used asynchronously, we need to delete ourselves
    this->deleteLater();
}

// Clean output from carriage return and ANSI color codes.
// @TODO: Remove once all relevant commands support "--no-color",
//("status" is  missing it as of git 1.6.2)

void GitCommand::removeColorCodes(QByteArray *data)
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

void GitCommand::setCookie(const QVariant &cookie)
{
    m_cookie = cookie;
}

QVariant GitCommand::cookie() const
{
    return m_cookie;
}


} // namespace Internal
} // namespace Git
