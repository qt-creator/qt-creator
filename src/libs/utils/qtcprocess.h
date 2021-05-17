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

#pragma once

#include "utils_global.h"

#include "environment.h"
#include "commandline.h"

#include <QProcess>
#include <QTextCodec>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace Utils {

class CommandLine;
class Environment;

namespace Internal { class QtcProcessPrivate; }

class QTCREATOR_UTILS_EXPORT QtcProcess : public QProcess
{
    Q_OBJECT

public:
    QtcProcess(QObject *parent = nullptr);
    ~QtcProcess();

    enum Result {
        // Finished with return code 0
        Finished,
        // Finished with return code != 0
        FinishedError,
        // Process terminated abnormally (kill)
        TerminatedAbnormally,
        // Executable could not be started
        StartFailed,
        // Hang, no output after time out
        Hang };

    void setEnvironment(const Environment &env);
    const Environment &environment() const;

    void setCommand(const CommandLine &cmdLine);
    const CommandLine &commandLine() const;

    void setUseCtrlCStub(bool enabled);
    void setLowPriority();
    void setDisableUnixTerminal();

    void start();
    void terminate();
    void interrupt();

    /* Timeout for hanging processes (triggers after no more output
     * occurs on stderr/stdout). */
    void setTimeoutS(int timeoutS);

    void setCodec(QTextCodec *c);
    void setTimeOutMessageBoxEnabled(bool);
    void setExitCodeInterpreter(const std::function<QtcProcess::Result(int)> &interpreter);

    // FIXME: This is currently only used in run(), not in start()
    void setWriteData(const QByteArray &writeData);

    // Starts a nested event loop and runs the command
    void run();

    // Starts the command blocking the UI fully
    void runBlocking();

    // FIXME: Remove. Kept for downstream for a while.
    void run(const CommandLine &cmd) { setCommand(cmd); run(); }
    void runBlocking(const CommandLine &cmd) { setCommand(cmd); runBlocking(); }

    void setStdOutCallback(const std::function<void(const QString &)> &callback);
    void setStdErrCallback(const std::function<void(const QString &)> &callback);

    static void setRemoteStartProcessHook(const std::function<void (QtcProcess &)> &hook);

    void setOpenMode(OpenMode mode);

    bool stopProcess();
    bool readDataFromProcess(int timeoutS, QByteArray *stdOut, QByteArray *stdErr,
                             bool showTimeOutMessageBox);

    static QString normalizeNewlines(const QString &text);

    Result result() const;
    void setResult(Result result);

    QByteArray allRawOutput() const;
    QString allOutput() const;

    QString stdOut() const;
    QString stdErr() const;

    QByteArray rawStdOut() const;
    QByteArray rawStdErr() const;

    int exitCode() const;

    QString exitMessage();

    // Helpers to find binaries. Do not use it for other path variables
    // and file types.
    static QString locateBinary(const QString &binary);
    static QString locateBinary(const QString &path, const QString &binary);

private:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void setupChildProcess() override;
#endif
    friend class SynchronousProcess;
    friend QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const QtcProcess &r);

    Internal::QtcProcessPrivate *d = nullptr;

    void setProcessEnvironment(const QProcessEnvironment &environment) = delete;
    QProcessEnvironment processEnvironment() const = delete;
};

using ExitCodeInterpreter = std::function<QtcProcess::Result(int /*exitCode*/)>;

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const QtcProcess &);

class QTCREATOR_UTILS_EXPORT SynchronousProcess : public QtcProcess
{
    Q_OBJECT
public:
    SynchronousProcess();
    ~SynchronousProcess() override;
};

} // namespace Utils
