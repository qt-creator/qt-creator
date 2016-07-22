/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion and Hugues Delorme
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

#include "shellcommand.h"

#include "fileutils.h"
#include "qtcassert.h"
#include "runextensions.h"

#include <QFileInfo>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QStringList>
#include <QTextCodec>
#include <QThread>
#include <QVariant>

/*!
    \fn void Utils::ProgressParser::parseProgress(const QString &text)

    Reimplement to parse progress as it appears in the standard output.
    If a progress string is detected, call \c setProgressAndMaximum() to update
    the progress bar accordingly.

    \sa Utils::ProgressParser::setProgressAndMaximum()
*/

/*!
    \fn void Utils::ProgressParser::setProgressAndMaximum(int value, int maximum)

    Sets progress \a value and \a maximum for current command. Called by \c parseProgress()
    when a progress string is detected.
*/

namespace Utils {
namespace Internal {

class ShellCommandPrivate
{
public:
    struct Job {
        explicit Job(const QString &wd, const Utils::FileName &b, const QStringList &a, int t,
                     const ExitCodeInterpreter &interpreter);

        QString workingDirectory;
        Utils::FileName binary;
        QStringList arguments;
        ExitCodeInterpreter exitCodeInterpreter;
        int timeoutS;
    };

    ShellCommandPrivate(const QString &defaultWorkingDirectory,
                        const QProcessEnvironment &environment);
    ~ShellCommandPrivate();

    std::function<OutputProxy *()> m_proxyFactory = []() { return new OutputProxy; };
    QString m_displayName;
    const QString m_defaultWorkingDirectory;
    const QProcessEnvironment m_environment;
    QVariant m_cookie;
    QTextCodec *m_codec = nullptr;
    ProgressParser *m_progressParser = nullptr;
    QFutureWatcher<void> m_watcher;
    QList<Job> m_jobs;

    unsigned m_flags = 0;
    int m_defaultTimeoutS = 10;
    int m_lastExecExitCode = -1;

    bool m_lastExecSuccess = false;
    bool m_progressiveOutput = false;
    bool m_hadOutput = false;
    bool m_aborted = false;
};

ShellCommandPrivate::ShellCommandPrivate(const QString &defaultWorkingDirectory,
                                         const QProcessEnvironment &environment) :
    m_defaultWorkingDirectory(defaultWorkingDirectory),
    m_environment(environment)
{ }

ShellCommandPrivate::~ShellCommandPrivate()
{
    delete m_progressParser;
}

ShellCommandPrivate::Job::Job(const QString &wd, const Utils::FileName &b, const QStringList &a,
                              int t, const ExitCodeInterpreter &interpreter) :
    workingDirectory(wd),
    binary(b),
    arguments(a),
    exitCodeInterpreter(interpreter),
    timeoutS(t)
{
    // Finished cookie is emitted via queued slot, needs metatype
    static const int qvMetaId = qRegisterMetaType<QVariant>();
    Q_UNUSED(qvMetaId)
}

} // namespace Internal

ShellCommand::ShellCommand(const QString &workingDirectory,
                           const QProcessEnvironment &environment) :
    d(new Internal::ShellCommandPrivate(workingDirectory, environment))
{
    connect(&d->m_watcher, &QFutureWatcher<void>::canceled, this, &ShellCommand::cancel);
}

ShellCommand::~ShellCommand()
{
    delete d;
}

QString ShellCommand::displayName() const
{
    if (!d->m_displayName.isEmpty())
        return d->m_displayName;
    if (!d->m_jobs.isEmpty()) {
        const Internal::ShellCommandPrivate::Job &job = d->m_jobs.at(0);
        QString result = job.binary.toFileInfo().baseName();
        if (!result.isEmpty())
            result[0] = result.at(0).toTitleCase();
        else
            result = tr("UNKNOWN");

        if (!job.arguments.isEmpty())
            result += QLatin1Char(' ') + job.arguments.at(0);

        return result;
    }
    return tr("Unknown");
}

void ShellCommand::setDisplayName(const QString &name)
{
    d->m_displayName = name;
}

const QString &ShellCommand::defaultWorkingDirectory() const
{
    return d->m_defaultWorkingDirectory;
}

const QProcessEnvironment ShellCommand::processEnvironment() const
{
    return d->m_environment;
}

int ShellCommand::defaultTimeoutS() const
{
    return d->m_defaultTimeoutS;
}

void ShellCommand::setDefaultTimeoutS(int timeout)
{
    d->m_defaultTimeoutS = timeout;
}

unsigned ShellCommand::flags() const
{
    return d->m_flags;
}

void ShellCommand::addFlags(unsigned f)
{
    d->m_flags |= f;
}

void ShellCommand::addJob(const Utils::FileName &binary, const QStringList &arguments,
                          const QString &workingDirectory, const ExitCodeInterpreter &interpreter)
{
    addJob(binary, arguments, defaultTimeoutS(), workingDirectory, interpreter);
}

void ShellCommand::addJob(const Utils::FileName &binary, const QStringList &arguments, int timeoutS,
                          const QString &workingDirectory, const ExitCodeInterpreter &interpreter)
{
    d->m_jobs.push_back(Internal::ShellCommandPrivate::Job(workDirectory(workingDirectory), binary,
                                                           arguments, timeoutS, interpreter));
}

void ShellCommand::execute()
{
    d->m_lastExecSuccess = false;
    d->m_lastExecExitCode = -1;

    if (d->m_jobs.empty())
        return;

    QFuture<void> task = Utils::runAsync(&ShellCommand::run, this);
    d->m_watcher.setFuture(task);
    if (!(d->m_flags & SuppressCommandLogging))
        addTask(task);
}

void ShellCommand::abort()
{
    d->m_aborted = true;
    d->m_watcher.future().cancel();
}

void ShellCommand::cancel()
{
    emit terminate();
}

unsigned ShellCommand::processFlags() const
{
    return 0;
}

void ShellCommand::addTask(QFuture<void> &future)
{
    Q_UNUSED(future);
}

QString ShellCommand::workDirectory(const QString &wd) const
{
    if (!wd.isEmpty())
        return wd;
    return defaultWorkingDirectory();
}

bool ShellCommand::lastExecutionSuccess() const
{
    return d->m_lastExecSuccess;
}

int ShellCommand::lastExecutionExitCode() const
{
    return d->m_lastExecExitCode;
}

void ShellCommand::run(QFutureInterface<void> &future)
{
    // Check that the binary path is not empty
    QTC_ASSERT(!d->m_jobs.isEmpty(), return);

    QString stdOut;
    QString stdErr;

    if (d->m_progressParser)
        d->m_progressParser->setFuture(&future);
    else
        future.setProgressRange(0, 1);
    const int count = d->m_jobs.size();
    d->m_lastExecExitCode = -1;
    d->m_lastExecSuccess = true;
    for (int j = 0; j < count; j++) {
        const Internal::ShellCommandPrivate::Job &job = d->m_jobs.at(j);
        Utils::SynchronousProcessResponse resp
                = runCommand(job.binary, job.arguments, job.timeoutS, job.workingDirectory,
                             job.exitCodeInterpreter);
        stdOut += resp.stdOut();
        stdErr += resp.stdErr();
        d->m_lastExecExitCode = resp.exitCode;
        d->m_lastExecSuccess = resp.result == Utils::SynchronousProcessResponse::Finished;
        if (!d->m_lastExecSuccess)
            break;
    }

    if (!d->m_aborted) {
        if (!d->m_progressiveOutput) {
            emit stdOutText(stdOut);
            if (!stdErr.isEmpty())
                emit stdErrText(stdErr);
        }

        emit finished(d->m_lastExecSuccess, d->m_lastExecExitCode, cookie());
        if (d->m_lastExecSuccess) {
            emit success(cookie());
            future.setProgressValue(future.progressMaximum());
        } else {
            future.cancel(); // sets the progress indicator red
        }
    }

    if (d->m_progressParser)
        d->m_progressParser->setFuture(0);
    // As it is used asynchronously, we need to delete ourselves
    this->deleteLater();
}

Utils::SynchronousProcessResponse ShellCommand::runCommand(const Utils::FileName &binary,
                                                           const QStringList &arguments, int timeoutS,
                                                           const QString &workingDirectory,
                                                           const ExitCodeInterpreter &interpreter)
{
    Utils::SynchronousProcessResponse response;

    const QString dir = workDirectory(workingDirectory);

    if (binary.isEmpty()) {
        response.result = Utils::SynchronousProcessResponse::StartFailed;
        return response;
    }

    QSharedPointer<OutputProxy> proxy(d->m_proxyFactory());

    if (!(d->m_flags & SuppressCommandLogging))
        proxy->appendCommand(dir, binary, arguments);

    if (d->m_flags & FullySynchronously || QThread::currentThread() == QCoreApplication::instance()->thread())
        response = runFullySynchronous(binary, arguments, proxy, timeoutS, dir, interpreter);
    else
        response = runSynchronous(binary, arguments, proxy, timeoutS, dir, interpreter);

    if (!d->m_aborted) {
        // Success/Fail message in appropriate window?
        if (response.result == Utils::SynchronousProcessResponse::Finished) {
            if (d->m_flags & ShowSuccessMessage)
                proxy->appendMessage(response.exitMessage(binary.toUserOutput(), timeoutS));
        } else if (!(d->m_flags & SuppressFailMessage)) {
            proxy->appendError(response.exitMessage(binary.toUserOutput(), timeoutS));
        }
    }

    return response;
}

Utils::SynchronousProcessResponse ShellCommand::runFullySynchronous(const Utils::FileName &binary,
                                                                    const QStringList &arguments,
                                                                    QSharedPointer<OutputProxy> proxy,
                                                                    int timeoutS,
                                                                    const QString &workingDirectory,
                                                                    const ExitCodeInterpreter &interpreter)
{
    // Set up process
    Utils::SynchronousProcess process;
    process.setFlags(processFlags());
    const QString dir = workDirectory(workingDirectory);
    if (!dir.isEmpty())
        process.setWorkingDirectory(dir);
    process.setProcessEnvironment(processEnvironment());
    if (d->m_flags & MergeOutputChannels)
        process.setProcessChannelMode(QProcess::MergedChannels);
    if (d->m_codec)
        process.setCodec(d->m_codec);
    process.setTimeoutS(timeoutS);
    process.setExitCodeInterpreter(interpreter);

    SynchronousProcessResponse resp = process.runBlocking(binary.toString(), arguments);

    if (!d->m_aborted) {
        const QString stdErr = resp.stdErr();
        if (!stdErr.isEmpty() && !(d->m_flags & SuppressStdErr))
            proxy->append(stdErr);

        const QString stdOut = resp.stdOut();
        if (!stdOut.isEmpty() && d->m_flags & ShowStdOut) {
            if (d->m_flags & SilentOutput)
                proxy->appendSilently(stdOut);
            else
                proxy->append(stdOut);
        }
    }

    return resp;
}

SynchronousProcessResponse ShellCommand::runSynchronous(const FileName &binary,
                                                        const QStringList &arguments,
                                                        QSharedPointer<OutputProxy> proxy,
                                                        int timeoutS,
                                                        const QString &workingDirectory,
                                                        const ExitCodeInterpreter &interpreter)
{
    Utils::SynchronousProcess process;
    process.setExitCodeInterpreter(interpreter);
    connect(this, &ShellCommand::terminate, &process, &Utils::SynchronousProcess::terminate);
    process.setProcessEnvironment(processEnvironment());
    process.setTimeoutS(timeoutS);
    if (d->m_codec)
        process.setCodec(d->m_codec);
    process.setFlags(processFlags());
    const QString dir = workDirectory(workingDirectory);
    if (!dir.isEmpty())
        process.setWorkingDirectory(dir);
    process.setProcessEnvironment(processEnvironment());
    // connect stderr to the output window if desired
    if (d->m_flags & MergeOutputChannels) {
        process.setProcessChannelMode(QProcess::MergedChannels);
    } else if (d->m_progressiveOutput
               || !(d->m_flags & SuppressStdErr)) {
        process.setStdErrBufferedSignalsEnabled(true);
        connect(&process, &Utils::SynchronousProcess::stdErrBuffered,
                this, [this, proxy](const QString &text)
        {
            if (!(d->m_flags & SuppressStdErr))
                proxy->appendError(text);
            if (d->m_progressiveOutput)
                emit stdErrText(text);
        });
    }

    // connect stdout to the output window if desired
    if (d->m_progressParser || d->m_progressiveOutput || (d->m_flags & ShowStdOut)) {
        process.setStdOutBufferedSignalsEnabled(true);
        connect(&process, &Utils::SynchronousProcess::stdOutBuffered,
                this, [this, proxy](const QString &text)
        {
            if (d->m_progressParser)
                d->m_progressParser->parseProgress(text);
            if (d->m_flags & ShowStdOut)
                proxy->append(text);
            if (d->m_progressiveOutput) {
                emit stdOutText(text);
                d->m_hadOutput = true;
            }
        });
    }

    process.setTimeOutMessageBoxEnabled(true);

    if (d->m_codec)
        process.setCodec(d->m_codec);
    process.setTimeoutS(timeoutS);
    process.setExitCodeInterpreter(interpreter);

    return process.run(binary.toString(), arguments);
}

const QVariant &ShellCommand::cookie() const
{
    return d->m_cookie;
}

void ShellCommand::setCookie(const QVariant &cookie)
{
    d->m_cookie = cookie;
}

QTextCodec *ShellCommand::codec() const
{
    return d->m_codec;
}

void ShellCommand::setCodec(QTextCodec *codec)
{
    d->m_codec = codec;
}

//! Use \a parser to parse progress data from stdout. Command takes ownership of \a parser
void ShellCommand::setProgressParser(ProgressParser *parser)
{
    QTC_ASSERT(!d->m_progressParser, return);
    d->m_progressParser = parser;
}

void ShellCommand::setProgressiveOutput(bool progressive)
{
    d->m_progressiveOutput = progressive;
}

void ShellCommand::setOutputProxyFactory(const std::function<OutputProxy *()> &factory)
{
    d->m_proxyFactory = factory;
}

ProgressParser::ProgressParser() :
    m_future(0),
    m_futureMutex(new QMutex)
{ }

ProgressParser::~ProgressParser()
{
    delete m_futureMutex;
}

void ProgressParser::setProgressAndMaximum(int value, int maximum)
{
    QMutexLocker lock(m_futureMutex);
    if (!m_future)
        return;
    m_future->setProgressRange(0, maximum);
    m_future->setProgressValue(value);
}

void ProgressParser::setFuture(QFutureInterface<void> *future)
{
    QMutexLocker lock(m_futureMutex);
    m_future = future;
}

} // namespace Utils
