// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <utils/filepath.h>
#include <utils/processenums.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QMutex;
class QVariant;
template <typename T>
class QFuture;
template <typename T>
class QFutureInterface;
class QTextCodec;
QT_END_NAMESPACE

namespace Utils {
class CommandLine;
class Environment;
class QtcProcess;
}

namespace VcsBase {

namespace Internal { class VcsCommandPrivate; }

class VCSBASE_EXPORT ProgressParser
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
    QMutex *m_futureMutex = nullptr;
    friend class Internal::VcsCommandPrivate;
};

class VCSBASE_EXPORT CommandResult
{
public:
    CommandResult() = default;
    CommandResult(const Utils::QtcProcess &process);
    CommandResult(Utils::ProcessResult result, const QString &exitMessage)
        : m_result(result), m_exitMessage(exitMessage) {}

    Utils::ProcessResult result() const { return m_result; }
    int exitCode() const { return m_exitCode; }
    QString exitMessage() const { return m_exitMessage; }

    QString cleanedStdOut() const { return m_cleanedStdOut; }
    QString cleanedStdErr() const { return m_cleanedStdErr; }

    QByteArray rawStdOut() const { return m_rawStdOut; }

private:
    Utils::ProcessResult m_result = Utils::ProcessResult::StartFailed;
    int m_exitCode = 0;
    QString m_exitMessage;

    QString m_cleanedStdOut;
    QString m_cleanedStdErr;

    QByteArray m_rawStdOut;
};

class VCSBASE_EXPORT VcsCommand final : public QObject
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
        SilentOutput = 0x80, // Suppress user notifications about the output happening.
        UseEventLoop = 0x100, // Use event loop when executed in UI thread.
        ExpectRepoChanges = 0x200, // Expect changes in repository by the command
        NoOutput = SuppressStdErr | SuppressFailMessage | SuppressCommandLogging
    };

    VcsCommand(const Utils::FilePath &workingDirectory, const Utils::Environment &environment);
    ~VcsCommand() override;

    void setDisplayName(const QString &name);

    void addJob(const Utils::CommandLine &command, int timeoutS,
                const Utils::FilePath &workingDirectory = {},
                const Utils::ExitCodeInterpreter &interpreter = {});
    void start();

    void addFlags(unsigned f);

    void setCodec(QTextCodec *codec);

    void setProgressParser(ProgressParser *parser);
    void setProgressiveOutput(bool progressive);

    static CommandResult runBlocking(const Utils::FilePath &workingDirectory,
                                     const Utils::Environment &environmentconst,
                                     const Utils::CommandLine &command, unsigned flags,
                                     int timeoutS, QTextCodec *codec);
    void cancel();

    QString cleanedStdOut() const;
    QString cleanedStdErr() const;
    Utils::ProcessResult result() const;

signals:
    void stdOutText(const QString &);
    void stdErrText(const QString &);
    void done();

    void append(const QString &text);
    void appendSilently(const QString &text);
    void appendError(const QString &text);
    void appendCommand(const Utils::FilePath &workingDirectory, const Utils::CommandLine &command);
    void appendMessage(const QString &text);

    void runCommandFinished(const Utils::FilePath &workingDirectory);

private:
    CommandResult runBlockingHelper(const Utils::CommandLine &command, int timeoutS);
    void postRunCommand(const Utils::FilePath &workingDirectory);

    class Internal::VcsCommandPrivate *const d;
};

} // namespace Utils
