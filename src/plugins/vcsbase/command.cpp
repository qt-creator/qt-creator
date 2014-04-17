/**************************************************************************
**
** Copyright (c) 2014 Brian McGillion and Hugues Delorme
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

#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/icore.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <utils/synchronousprocess.h>
#include <utils/runextensions.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QCoreApplication>
#include <QVariant>
#include <QStringList>
#include <QTextCodec>
#include <QMutex>

Q_DECLARE_METATYPE(QVariant)

enum { debugExecution = 0 };

/*!
    \fn void VcsBase::ProgressParser::parseProgress(const QString &text)

    Reimplement to parse progress as it appears in the standard output.
    If a progress string is detected, call \c setProgressAndMaximum() to update
    the progress bar accordingly.

    \sa VcsBase::ProgressParser::setProgressAndMaximum()
*/

/*!
    \fn void VcsBase::ProgressParser::setProgressAndMaximum(int value, int maximum)

    Sets progress \a value and \a maximum for current command. Called by \c parseProgress()
    when a progress string is detected.
*/

namespace VcsBase {
namespace Internal {

class CommandPrivate
{
public:
    struct Job {
        explicit Job(const QStringList &a, int t, Utils::ExitCodeInterpreter *interpreter = 0);

        QStringList arguments;
        int timeout;
        Utils::ExitCodeInterpreter *exitCodeInterpreter;
    };

    CommandPrivate(const QString &binary,
                   const QString &workingDirectory,
                   const QProcessEnvironment &environment);
    ~CommandPrivate();

    const QString m_binaryPath;
    const QString m_workingDirectory;
    const QProcessEnvironment m_environment;
    QVariant m_cookie;
    int m_defaultTimeout;
    unsigned m_flags;
    QTextCodec *m_codec;
    const QString m_sshPasswordPrompt;
    ProgressParser *m_progressParser;
    VcsBase::VcsBaseOutputWindow *m_outputWindow;
    bool m_progressiveOutput;
    bool m_hadOutput;
    bool m_preventRepositoryChanged;
    bool m_aborted;
    QFutureWatcher<void> m_watcher;

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
    m_defaultTimeout(10),
    m_flags(0),
    m_codec(0),
    m_sshPasswordPrompt(VcsBasePlugin::sshPrompt()),
    m_progressParser(0),
    m_outputWindow(VcsBase::VcsBaseOutputWindow::instance()),
    m_progressiveOutput(false),
    m_hadOutput(false),
    m_preventRepositoryChanged(false),
    m_aborted(false),
    m_lastExecSuccess(false),
    m_lastExecExitCode(-1)
{
}

CommandPrivate::~CommandPrivate()
{
    delete m_progressParser;
}

CommandPrivate::Job::Job(const QStringList &a, int t, Utils::ExitCodeInterpreter *interpreter) :
    arguments(a),
    timeout(t),
    exitCodeInterpreter(interpreter)
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
    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()),
            this, SLOT(coreAboutToClose()));
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

unsigned Command::flags() const
{
    return d->m_flags;
}

void Command::addFlags(unsigned f)
{
    d->m_flags |= f;
}

void Command::addJob(const QStringList &arguments, Utils::ExitCodeInterpreter *interpreter)
{
    addJob(arguments, defaultTimeout(), interpreter);
}

void Command::addJob(const QStringList &arguments, int timeout, Utils::ExitCodeInterpreter *interpreter)
{
    d->m_jobs.push_back(Internal::CommandPrivate::Job(arguments, timeout, interpreter));
}

void Command::execute()
{
    d->m_lastExecSuccess = false;
    d->m_lastExecExitCode = -1;

    if (d->m_jobs.empty())
        return;

    // For some reason QtConcurrent::run() only works on this
    QFuture<void> task = QtConcurrent::run(&Command::run, this);
    d->m_watcher.setFuture(task);
    connect(&d->m_watcher, SIGNAL(canceled()), this, SLOT(cancel()));
    QString binary = QFileInfo(d->m_binaryPath).baseName();
    if (!binary.isEmpty())
        binary = binary.replace(0, 1, binary[0].toUpper()); // Upper the first letter
    const QString taskName = binary + QLatin1Char(' ') + d->m_jobs.front().arguments.at(0);

    Core::ProgressManager::addTask(task, taskName,
        Core::Id::fromString(binary + QLatin1String(".action")));
}

void Command::abort()
{
    d->m_aborted = true;
    d->m_watcher.future().cancel();
}

void Command::cancel()
{
    emit terminate();
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

    if (d->m_progressParser)
        d->m_progressParser->setFuture(&future);
    else
        future.setProgressRange(0, 1);
    const int count = d->m_jobs.size();
    d->m_lastExecExitCode = -1;
    d->m_lastExecSuccess = true;
    for (int j = 0; j < count; j++) {
        const Internal::CommandPrivate::Job &job = d->m_jobs.at(j);
        const int timeOutSeconds = job.timeout;
        Utils::SynchronousProcessResponse resp = runVcs(
                    job.arguments,
                    timeOutSeconds >= 0 ? timeOutSeconds * 1000 : -1,
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
            emit output(stdOut);
            if (!stdErr.isEmpty())
                emit errorText(stdErr);
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

class OutputProxy : public QObject
{
    Q_OBJECT

    friend class Command;

public:
    OutputProxy()
    {
        // Users of this class can either be in the GUI thread or in other threads.
        // Use Qt::AutoConnection to always append in the GUI thread (directly or queued)
        VcsBase::VcsBaseOutputWindow *outputWindow = VcsBase::VcsBaseOutputWindow::instance();
        connect(this, SIGNAL(append(QString)), outputWindow, SLOT(append(QString)));
        connect(this, SIGNAL(appendSilently(QString)), outputWindow, SLOT(appendSilently(QString)));
        connect(this, SIGNAL(appendError(QString)), outputWindow, SLOT(appendError(QString)));
        connect(this, SIGNAL(appendCommand(QString,QString,QStringList)),
                outputWindow, SLOT(appendCommand(QString,QString,QStringList)));
        connect(this, SIGNAL(appendMessage(QString)), outputWindow, SLOT(appendMessage(QString)));
    }

signals:
    void append(const QString &text);
    void appendSilently(const QString &text);
    void appendError(const QString &text);
    void appendCommand(const QString &workingDirectory,
                       const QString &binary,
                       const QStringList &args);
    void appendMessage(const QString &text);
};

Utils::SynchronousProcessResponse Command::runVcs(const QStringList &arguments, int timeoutMS,
                                                  Utils::ExitCodeInterpreter *interpreter)
{
    Utils::SynchronousProcessResponse response;
    OutputProxy outputProxy;

    if (d->m_binaryPath.isEmpty()) {
        response.result = Utils::SynchronousProcessResponse::StartFailed;
        return response;
    }

    if (!(d->m_flags & VcsBasePlugin::SuppressCommandLogging))
        emit outputProxy.appendCommand(d->m_workingDirectory, d->m_binaryPath, arguments);

    const bool sshPromptConfigured = !d->m_sshPasswordPrompt.isEmpty();
    if (debugExecution) {
        QDebug nsp = qDebug().nospace();
        nsp << "Command::runVcs" << d->m_workingDirectory << d->m_binaryPath << arguments
                << timeoutMS;
        if (d->m_flags & VcsBasePlugin::ShowStdOutInLogWindow)
            nsp << "stdout";
        if (d->m_flags & VcsBasePlugin::SuppressStdErrInLogWindow)
            nsp << "suppress_stderr";
        if (d->m_flags & VcsBasePlugin::SuppressFailMessageInLogWindow)
            nsp << "suppress_fail_msg";
        if (d->m_flags & VcsBasePlugin::MergeOutputChannels)
            nsp << "merge_channels";
        if (d->m_flags & VcsBasePlugin::SshPasswordPrompt)
            nsp << "ssh (" << sshPromptConfigured << ')';
        if (d->m_flags & VcsBasePlugin::SuppressCommandLogging)
            nsp << "suppress_log";
        if (d->m_flags & VcsBasePlugin::ForceCLocale)
            nsp << "c_locale";
        if (d->m_flags & VcsBasePlugin::FullySynchronously)
            nsp << "fully_synchronously";
        if (d->m_flags & VcsBasePlugin::ExpectRepoChanges)
            nsp << "expect_repo_changes";
        if (d->m_codec)
            nsp << " Codec: " << d->m_codec->name();
    }

    // TODO tell the document manager about expected repository changes
    //    if (d->m_flags & ExpectRepoChanges)
    //        Core::DocumentManager::expectDirectoryChange(d->m_workingDirectory);
    if (d->m_flags & VcsBasePlugin::FullySynchronously) {
        response = runSynchronous(arguments, timeoutMS, interpreter);
    } else {
        Utils::SynchronousProcess process;
        process.setExitCodeInterpreter(interpreter);
        connect(this, SIGNAL(terminate()), &process, SLOT(terminate()));
        if (!d->m_workingDirectory.isEmpty())
            process.setWorkingDirectory(d->m_workingDirectory);

        QProcessEnvironment env = d->m_environment;
        VcsBasePlugin::setProcessEnvironment(&env,
                                             (d->m_flags & VcsBasePlugin::ForceCLocale),
                                             d->m_sshPasswordPrompt);
        process.setProcessEnvironment(env);
        process.setTimeout(timeoutMS);
        if (d->m_codec)
            process.setCodec(d->m_codec);

        // Suppress terminal on UNIX for ssh prompts if it is configured.
        if (sshPromptConfigured && (d->m_flags & VcsBasePlugin::SshPasswordPrompt))
            process.setFlags(Utils::SynchronousProcess::UnixTerminalDisabled);

        // connect stderr to the output window if desired
        if (d->m_flags & VcsBasePlugin::MergeOutputChannels) {
            process.setProcessChannelMode(QProcess::MergedChannels);
        } else if (d->m_progressiveOutput
                   || !(d->m_flags & VcsBasePlugin::SuppressStdErrInLogWindow)) {
            process.setStdErrBufferedSignalsEnabled(true);
            connect(&process, SIGNAL(stdErrBuffered(QString,bool)),
                    this, SLOT(bufferedError(QString)));
        }

        // connect stdout to the output window if desired
        if (d->m_progressParser || d->m_progressiveOutput
                || (d->m_flags & VcsBasePlugin::ShowStdOutInLogWindow)) {
            process.setStdOutBufferedSignalsEnabled(true);
            connect(&process, SIGNAL(stdOutBuffered(QString,bool)), this, SLOT(bufferedOutput(QString)));
        }

        process.setTimeOutMessageBoxEnabled(true);

        // Run!
        response = process.run(d->m_binaryPath, arguments);
    }

    if (!d->m_aborted) {
        // Success/Fail message in appropriate window?
        if (response.result == Utils::SynchronousProcessResponse::Finished) {
            if (d->m_flags & VcsBasePlugin::ShowSuccessMessage)
                emit outputProxy.appendMessage(response.exitMessage(d->m_binaryPath, timeoutMS));
        } else if (!(d->m_flags & VcsBasePlugin::SuppressFailMessageInLogWindow)) {
            emit outputProxy.appendError(response.exitMessage(d->m_binaryPath, timeoutMS));
        }
    }
    emitRepositoryChanged();

    return response;
}

Utils::SynchronousProcessResponse Command::runSynchronous(const QStringList &arguments, int timeoutMS,
                                                          Utils::ExitCodeInterpreter *interpreter)
{
    Utils::SynchronousProcessResponse response;

    // Set up process
    unsigned processFlags = 0;
    if (!d->m_sshPasswordPrompt.isEmpty() && (d->m_flags & VcsBasePlugin::SshPasswordPrompt))
        processFlags |= Utils::SynchronousProcess::UnixTerminalDisabled;
    QSharedPointer<QProcess> process = Utils::SynchronousProcess::createProcess(processFlags);
    if (!d->m_workingDirectory.isEmpty())
        process->setWorkingDirectory(d->m_workingDirectory);
    QProcessEnvironment env = d->m_environment;
    VcsBasePlugin::setProcessEnvironment(&env,
                                         (d->m_flags & VcsBasePlugin::ForceCLocale),
                                         d->m_sshPasswordPrompt);
    process->setProcessEnvironment(env);
    if (d->m_flags & VcsBasePlugin::MergeOutputChannels)
        process->setProcessChannelMode(QProcess::MergedChannels);

    // Start
    process->start(d->m_binaryPath, arguments, QIODevice::ReadOnly);
    process->closeWriteChannel();
    if (!process->waitForStarted()) {
        response.result = Utils::SynchronousProcessResponse::StartFailed;
        return response;
    }

    // process output
    QByteArray stdOut;
    QByteArray stdErr;
    const bool timedOut =
            !Utils::SynchronousProcess::readDataFromProcess(*process.data(), timeoutMS,
                                                            &stdOut, &stdErr, true);

    if (!d->m_aborted) {
        OutputProxy outputProxy;
        if (!stdErr.isEmpty()) {
            response.stdErr = Utils::SynchronousProcess::normalizeNewlines(
                        d->m_codec ? d->m_codec->toUnicode(stdErr) : QString::fromLocal8Bit(stdErr));
            if (!(d->m_flags & VcsBasePlugin::SuppressStdErrInLogWindow))
                emit outputProxy.append(response.stdErr);
        }

        if (!stdOut.isEmpty()) {
            response.stdOut = Utils::SynchronousProcess::normalizeNewlines(
                        d->m_codec ? d->m_codec->toUnicode(stdOut) : QString::fromLocal8Bit(stdOut));
            if (d->m_flags & VcsBasePlugin::ShowStdOutInLogWindow) {
                if (d->m_flags & VcsBasePlugin::SilentOutput)
                    emit outputProxy.appendSilently(response.stdOut);
                else
                    emit outputProxy.append(response.stdOut);
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

void Command::emitRepositoryChanged()
{
    if (d->m_preventRepositoryChanged || !(d->m_flags & VcsBasePlugin::ExpectRepoChanges))
        return;
    // TODO tell the document manager that the directory now received all expected changes
    // Core::DocumentManager::unexpectDirectoryChange(d->m_workingDirectory);
    Core::VcsManager::emitRepositoryChanged(d->m_workingDirectory);
}

bool Command::runFullySynchronous(const QStringList &arguments, int timeoutMS,
                                  QByteArray *outputData, QByteArray *errorData)
{
    if (d->m_binaryPath.isEmpty())
        return false;

    OutputProxy outputProxy;
    if (!(d->m_flags & VcsBasePlugin::SuppressCommandLogging))
        emit outputProxy.appendCommand(d->m_workingDirectory, d->m_binaryPath, arguments);

    // TODO tell the document manager about expected repository changes
    // if (d->m_flags & ExpectRepoChanges)
    //    Core::DocumentManager::expectDirectoryChange(workingDirectory);
    QProcess process;
    process.setWorkingDirectory(d->m_workingDirectory);
    process.setProcessEnvironment(d->m_environment);

    process.start(d->m_binaryPath, arguments);
    process.closeWriteChannel();
    if (!process.waitForStarted()) {
        if (errorData) {
            const QString msg = QString::fromLatin1("Unable to execute \"%1\": %2:")
                                .arg(d->m_binaryPath, process.errorString());
            *errorData = msg.toLocal8Bit();
        }
        return false;
    }

    if (!Utils::SynchronousProcess::readDataFromProcess(process, timeoutMS, outputData, errorData, true)) {
        if (errorData)
            errorData->append(tr("Error: Executable timed out after %1s.").arg(timeoutMS / 1000).toLocal8Bit());
        Utils::SynchronousProcess::stopProcess(process);
        return false;
    }

    emitRepositoryChanged();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void Command::bufferedOutput(const QString &text)
{
    if (d->m_progressParser)
        d->m_progressParser->parseProgress(text);
    if (d->m_flags & VcsBasePlugin::ShowStdOutInLogWindow)
        d->m_outputWindow->append(text);
    if (d->m_progressiveOutput) {
        emit output(text);
        d->m_hadOutput = true;
    }
}

void Command::bufferedError(const QString &text)
{
    if (!(d->m_flags & VcsBasePlugin::SuppressStdErrInLogWindow))
        d->m_outputWindow->appendError(text);
    if (d->m_progressiveOutput)
        emit errorText(text);
}

void Command::coreAboutToClose()
{
    d->m_preventRepositoryChanged = true;
    abort();
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

//! Use \a parser to parse progress data from stdout. Command takes ownership of \a parser
void Command::setProgressParser(ProgressParser *parser)
{
    QTC_ASSERT(!d->m_progressParser, return);
    d->m_progressParser = parser;
}

void Command::setProgressiveOutput(bool progressive)
{
    d->m_progressiveOutput = progressive;
}

ProgressParser::ProgressParser() :
    m_future(0),
    m_futureMutex(new QMutex)
{
}

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

} // namespace VcsBase

#include "command.moc"
