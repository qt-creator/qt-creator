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
#include "vcsbaseplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <utils/synchronousprocess.h>
#include <utils/runextensions.h>

#include <QDebug>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFuture>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QCoreApplication>
#include <QVariant>
#include <QStringList>
#include <QTextCodec>

Q_DECLARE_METATYPE(QVariant)

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
    unsigned m_flags;
    QTextCodec *m_codec;
    const QString m_sshPasswordPrompt;

    QList<Job> m_jobs;

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
    m_flags(0),
    m_codec(0),
    m_sshPasswordPrompt(VcsBasePlugin::sshPrompt()),
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

unsigned Command::flags() const
{
    return d->m_flags;
}

void Command::addFlags(unsigned f)
{
    d->m_flags |= f;
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
    QFuture<void> task = QtConcurrent::run(&Command::run, this);
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

void Command::run(QFutureInterface<void> &future)
{
    // Check that the binary path is not empty
    if (binaryPath().trimmed().isEmpty()) {
        emit errorText(tr("Unable to start process, binary is empty"));
        return;
    }

    QString stdOut;
    QString stdErr;

    const int count = d->m_jobs.size();
    d->m_lastExecExitCode = -1;
    d->m_lastExecSuccess = true;
    for (int j = 0; j < count; j++) {
        const int timeOutSeconds = d->m_jobs.at(j).timeout;
        Utils::SynchronousProcessResponse resp =
                VcsBasePlugin::runVcs(d->m_workingDirectory, d->m_binaryPath,
                                      d->m_jobs.at(j).arguments,
                                      timeOutSeconds >= 0 ? timeOutSeconds * 1000 : -1,
                                      d->m_environment, d->m_sshPasswordPrompt,
                                      d->m_flags, d->m_codec);
        stdOut += resp.stdOut;
        stdErr += resp.stdErr;
        d->m_lastExecExitCode = resp.exitCode;
        d->m_lastExecSuccess = resp.result == Utils::SynchronousProcessResponse::Finished;
        if (!d->m_lastExecSuccess)
            break;
    }

    if (!future.isCanceled()) {
        emit output(stdOut);
        if (!stdErr.isEmpty())
            emit errorText(stdErr);

        emit finished(d->m_lastExecSuccess, d->m_lastExecExitCode, cookie());
        if (d->m_lastExecSuccess)
            emit success(cookie());
    }

    // As it is used asynchronously, we need to delete ourselves
    this->deleteLater();
}

const QVariant &Command::cookie() const
{
    return d->m_cookie;
}

void Command::setCookie(const QVariant &cookie)
{
    d->m_cookie = cookie;
}

QTextCodec *Command::codec() const
{
    return d->m_codec;
}

void Command::setCodec(QTextCodec *codec)
{
    d->m_codec = codec;
}

} // namespace VcsBase
