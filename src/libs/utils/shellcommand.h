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

#pragma once

#include "utils_global.h"

#include "synchronousprocess.h"

#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
class QMutex;
class QStringList;
class QVariant;
class QProcessEnvironment;
template <typename T>
class QFutureInterface;
template <typename T>
class QFuture;
QT_END_NAMESPACE

namespace Utils {

class FileName;
namespace Internal { class ShellCommandPrivate; }

class QTCREATOR_UTILS_EXPORT ProgressParser
{
public:
    ProgressParser();
    virtual ~ProgressParser();

protected:
    virtual void parseProgress(const QString &text) = 0;
    void setProgressAndMaximum(int value, int maximum);

private:
    void setFuture(QFutureInterface<void> *future);

    QFutureInterface<void> *m_future;
    QMutex *m_futureMutex;
    friend class ShellCommand;
};

// Users of this class can either be in the GUI thread or in other threads.
// Use Qt::AutoConnection to always append in the GUI thread (directly or queued)
class QTCREATOR_UTILS_EXPORT OutputProxy : public QObject
{
    Q_OBJECT

    friend class ShellCommand;

signals:
    void append(const QString &text);
    void appendSilently(const QString &text);
    void appendError(const QString &text);
    void appendCommand(const QString &workingDirectory, const Utils::FileName &binary,
                       const QStringList &args);
    void appendMessage(const QString &text);
};

class QTCREATOR_UTILS_EXPORT ShellCommand : public QObject
{
    Q_OBJECT

public:
    // Convenience to synchronously run commands
    enum RunFlags {
        ShowStdOut = 0x1, // Show standard output.
        MergeOutputChannels = 0x2, // see QProcess: Merge stderr/stdout.
        SuppressStdErr = 0x4, // Suppress standard error output.
        SuppressFailMessage = 0x8, // No message about command failure.
        SuppressCommandLogging = 0x10, // No command log entry.
        ShowSuccessMessage = 0x20, // Show message about successful completion of command.
        ForceCLocale = 0x40, // Force C-locale for commands whose output is parsed.
        FullySynchronously = 0x80, // Suppress local event loop (in case UI actions are
                                   // triggered by file watchers).
        SilentOutput = 0x100, // Suppress user notifications about the output happening.
        NoOutput = SuppressStdErr | SuppressFailMessage | SuppressCommandLogging
    };


    ShellCommand(const QString &workingDirectory, const QProcessEnvironment &environment);
    ~ShellCommand() override;

    QString displayName() const;
    void setDisplayName(const QString &name);

    void addJob(const FileName &binary, const QStringList &arguments,
                const QString &workingDirectory = QString(), const ExitCodeInterpreter &interpreter = defaultExitCodeInterpreter);
    void addJob(const FileName &binary, const QStringList &arguments, int timeoutS,
                const QString &workingDirectory = QString(), const ExitCodeInterpreter &interpreter = defaultExitCodeInterpreter);
    void execute(); // Execute tasks asynchronously!
    void abort();
    bool lastExecutionSuccess() const;
    int lastExecutionExitCode() const;

    const QString &defaultWorkingDirectory() const;
    virtual const QProcessEnvironment processEnvironment() const;

    int defaultTimeoutS() const;
    void setDefaultTimeoutS(int timeout);

    unsigned flags() const;
    void addFlags(unsigned f);

    const QVariant &cookie() const;
    void setCookie(const QVariant &cookie);

    QTextCodec *codec() const;
    void setCodec(QTextCodec *codec);

    void setProgressParser(ProgressParser *parser);
    void setProgressiveOutput(bool progressive);

    void setOutputProxyFactory(const std::function<OutputProxy *()> &factory);

    // This is called once per job in a thread.
    // When called from the UI thread it will execute fully synchronously, so no signals will
    // be triggered!
    virtual SynchronousProcessResponse runCommand(const FileName &binary, const QStringList &arguments,
                                                  int timeoutS,
                                                  const QString &workingDirectory = QString(),
                                                  const ExitCodeInterpreter &interpreter = defaultExitCodeInterpreter);

    void cancel();

signals:
    void stdOutText(const QString &);
    void stdErrText(const QString &);
    void finished(bool ok, int exitCode, const QVariant &cookie);
    void success(const QVariant &cookie);

    void terminate(); // Internal

protected:
    virtual unsigned processFlags() const;
    virtual void addTask(QFuture<void> &future);
    QString workDirectory(const QString &wd) const;

private:
    void run(QFutureInterface<void> &future);

    // Run without a event loop in fully blocking mode. No signals will be delivered.
    SynchronousProcessResponse runFullySynchronous(const FileName &binary, const QStringList &arguments,
                                                   QSharedPointer<OutputProxy> proxy,
                                                   int timeoutS, const QString &workingDirectory,
                                                   const ExitCodeInterpreter &interpreter = defaultExitCodeInterpreter);
    // Run with an event loop. Signals will be delivered.
    SynchronousProcessResponse runSynchronous(const FileName &binary, const QStringList &arguments,
                                              QSharedPointer<OutputProxy> proxy,
                                              int timeoutS, const QString &workingDirectory,
                                              const ExitCodeInterpreter &interpreter = defaultExitCodeInterpreter);

    class Internal::ShellCommandPrivate *const d;
};

} // namespace Utils
