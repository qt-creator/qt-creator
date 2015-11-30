/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion and Hugues Delorme
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

#include "shellcommand.h"

#include "fileutils.h"
#include "synchronousprocess.h"
#include "runextensions.h"
#include "qtcassert.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QCoreApplication>
#include <QVariant>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QStringList>
#include <QTextCodec>
#include <QMutex>

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
                     Utils::ExitCodeInterpreter *interpreter = 0);

        QString workingDirectory;
        Utils::FileName binary;
        QStringList arguments;
        int timeoutS;
        Utils::ExitCodeInterpreter *exitCodeInterpreter;
    };

    ShellCommandPrivate(const QString &defaultWorkingDirectory,
                        const QProcessEnvironment &environment);
    ~ShellCommandPrivate();

    std::function<OutputProxy *()> m_proxyFactory = []() { return new OutputProxy; };
    QString m_displayName;
    const QString m_defaultWorkingDirectory;
    const QProcessEnvironment m_environment;
    QVariant m_cookie;
    int m_defaultTimeoutS;
    unsigned m_flags;
    QTextCodec *m_codec;
    ProgressParser *m_progressParser;
    bool m_progressiveOutput;
    bool m_hadOutput;
    bool m_aborted;
    QFutureWatcher<void> m_watcher;

    QList<Job> m_jobs;

    bool m_lastExecSuccess;
    int m_lastExecExitCode;
};

ShellCommandPrivate::ShellCommandPrivate(const QString &defaultWorkingDirectory,
                                         const QProcessEnvironment &environment) :
    m_defaultWorkingDirectory(defaultWorkingDirectory),
    m_environment(environment),
    m_defaultTimeoutS(10),
    m_flags(0),
    m_codec(0),
    m_progressParser(0),
    m_progressiveOutput(false),
    m_hadOutput(false),
    m_aborted(false),
    m_lastExecSuccess(false),
    m_lastExecExitCode(-1)
{ }

ShellCommandPrivate::~ShellCommandPrivate()
{
    delete m_progressParser;
}

ShellCommandPrivate::Job::Job(const QString &wd, const Utils::FileName &b, const QStringList &a,
                              int t, Utils::ExitCodeInterpreter *interpreter) :
    workingDirectory(wd),
    binary(b),
    arguments(a),
    timeoutS(t),
    exitCodeInterpreter(interpreter)
{
    // Finished cookie is emitted via queued slot, needs metatype
    static const int qvMetaId = qRegisterMetaType<QVariant>();
    Q_UNUSED(qvMetaId)
}

} // namespace Internal

ShellCommand::ShellCommand(const QString &workingDirectory,
                           const QProcessEnvironment &environment) :
    d(new Internal::ShellCommandPrivate(workingDirectory, environment))
{ }

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
                          const QString &workingDirectory, Utils::ExitCodeInterpreter *interpreter)
{
    addJob(binary, arguments, defaultTimeoutS(), workingDirectory, interpreter);
}

void ShellCommand::addJob(const Utils::FileName &binary, const QStringList &arguments, int timeoutS,
                          const QString &workingDirectory, Utils::ExitCodeInterpreter *interpreter)
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

    // For some reason QtConcurrent::run() only works on this
    QFuture<void> task = QtConcurrent::run(&ShellCommand::run, this);
    d->m_watcher.setFuture(task);
    connect(&d->m_watcher, &QFutureWatcher<void>::canceled, this, &ShellCommand::cancel);

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
        stdOut += resp.stdOut;
        stdErr += resp.stdErr;
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
        if (d->m_lastExecSuccess)
            emit success(cookie());
        future.setProgressValue(future.progressMaximum());
    }

    if (d->m_progressParser)
        d->m_progressParser->setFuture(0);
    // As it is used asynchronously, we need to delete ourselves
    this->deleteLater();
}

Utils::SynchronousProcessResponse ShellCommand::runCommand(const Utils::FileName &binary,
                                                           const QStringList &arguments, int timeoutS,
                                                           const QString &workingDirectory,
                                                           Utils::ExitCodeInterpreter *interpreter)
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

    if (d->m_flags & FullySynchronously) {
        response = runSynchronous(binary, arguments, timeoutS, dir, interpreter);
    } else {
        Utils::SynchronousProcess process;
        process.setExitCodeInterpreter(interpreter);
        connect(this, &ShellCommand::terminate, &process, &Utils::SynchronousProcess::terminate);
        process.setWorkingDirectory(dir);

        process.setProcessEnvironment(processEnvironment());
        process.setTimeoutS(timeoutS);
        if (d->m_codec)
            process.setCodec(d->m_codec);

        process.setFlags(processFlags());

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
        if (d->m_progressParser || d->m_progressiveOutput
                || (d->m_flags & ShowStdOut)) {
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

        // Run!
        response = process.run(binary.toString(), arguments);
    }

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

Utils::SynchronousProcessResponse ShellCommand::runSynchronous(const Utils::FileName &binary,
                                                               const QStringList &arguments,
                                                               int timeoutS,
                                                               const QString &workingDirectory,
                                                               Utils::ExitCodeInterpreter *interpreter)
{
    Utils::SynchronousProcessResponse response;

    QScopedPointer<OutputProxy> proxy(d->m_proxyFactory());

    // Set up process
    QSharedPointer<QProcess> process = Utils::SynchronousProcess::createProcess(processFlags());
    if (!d->m_defaultWorkingDirectory.isEmpty())
        process->setWorkingDirectory(workDirectory(workingDirectory));
    process->setProcessEnvironment(processEnvironment());
    if (d->m_flags & MergeOutputChannels)
        process->setProcessChannelMode(QProcess::MergedChannels);

    // Start
    process->start(binary.toString(), arguments, QIODevice::ReadOnly);
    process->closeWriteChannel();
    if (!process->waitForStarted()) {
        response.result = Utils::SynchronousProcessResponse::StartFailed;
        return response;
    }

    // process output
    QByteArray stdOut;
    QByteArray stdErr;
    const bool timedOut =
            !Utils::SynchronousProcess::readDataFromProcess(*process.data(), timeoutS,
                                                            &stdOut, &stdErr, true);

    if (!d->m_aborted) {
        if (!stdErr.isEmpty()) {
            response.stdErr = Utils::SynchronousProcess::normalizeNewlines(
                        d->m_codec ? d->m_codec->toUnicode(stdErr) : QString::fromLocal8Bit(stdErr));
            if (!(d->m_flags & SuppressStdErr))
                proxy->append(response.stdErr);
        }

        if (!stdOut.isEmpty()) {
            response.stdOut = Utils::SynchronousProcess::normalizeNewlines(
                        d->m_codec ? d->m_codec->toUnicode(stdOut) : QString::fromLocal8Bit(stdOut));
            if (d->m_flags & ShowStdOut) {
                if (d->m_flags & SilentOutput)
                    proxy->appendSilently(response.stdOut);
                else
                    proxy->append(response.stdOut);
            }
        }
    }

    Utils::ExitCodeInterpreter defaultInterpreter(this);
    Utils::ExitCodeInterpreter *currentInterpreter = interpreter ? interpreter : &defaultInterpreter;
    // Result
    if (timedOut)
        response.result = Utils::SynchronousProcessResponse::Hang;
    else if (process->exitStatus() != QProcess::NormalExit)
        response.result = Utils::SynchronousProcessResponse::TerminatedAbnormally;
    else
        response.result = currentInterpreter->interpretExitCode(process->exitCode());
    return response;
}

bool ShellCommand::runFullySynchronous(const Utils::FileName &binary, const QStringList &arguments,
                                       int timeoutS, QByteArray *outputData, QByteArray *errorData,
                                       const QString &workingDirectory)
{
    QTC_ASSERT(!binary.isEmpty(), return false);

    QScopedPointer<OutputProxy> proxy(d->m_proxyFactory());
    const QString dir = workDirectory(workingDirectory);

    if (!(d->m_flags & SuppressCommandLogging))
        proxy->appendCommand(dir, binary, arguments);

    QProcess process;
    process.setWorkingDirectory(dir);
    process.setProcessEnvironment(d->m_environment);

    process.start(binary.toString(), arguments);
    process.closeWriteChannel();
    if (!process.waitForStarted()) {
        if (errorData) {
            const QString msg = QString::fromLatin1("Unable to execute \"%1\": %2:")
                    .arg(binary.toUserOutput(), process.errorString());
            *errorData = msg.toLocal8Bit();
        }
        return false;
    }

    if (!Utils::SynchronousProcess::readDataFromProcess(process, timeoutS, outputData, errorData, true)) {
        if (errorData)
            errorData->append(tr("Error: Executable timed out after %1 s.").arg(timeoutS).toLocal8Bit());
        Utils::SynchronousProcess::stopProcess(process);
        return false;
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
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
