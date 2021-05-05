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

#include <QProcess>
#include <QTextCodec>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace Utils {

class SynchronousProcessPrivate;
class CommandLine;
class Environment;

/* Result of SynchronousProcess execution */
class QTCREATOR_UTILS_EXPORT SynchronousProcessResponse
{
public:
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

    void clear();

    // Helper to format an exit message.
    QString exitMessage(const QString &binary, int timeoutS) const;

    QByteArray allRawOutput() const;
    QString allOutput() const;

    QString stdOut() const;
    QString stdErr() const;

    Result result = StartFailed;
    int exitCode = -1;

    QByteArray rawStdOut;
    QByteArray rawStdErr;
    QTextCodec *codec = QTextCodec::codecForLocale();
};

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const SynchronousProcessResponse &);

using ExitCodeInterpreter = std::function<SynchronousProcessResponse::Result(int /*exitCode*/)>;
QTCREATOR_UTILS_EXPORT SynchronousProcessResponse::Result defaultExitCodeInterpreter(int code);

class QTCREATOR_UTILS_EXPORT SynchronousProcess : public QObject
{
    Q_OBJECT
public:
    SynchronousProcess();
    ~SynchronousProcess() override;

    /* Timeout for hanging processes (triggers after no more output
     * occurs on stderr/stdout). */
    void setTimeoutS(int timeoutS);
    int timeoutS() const;

    void setCodec(QTextCodec *c);
    QTextCodec *codec() const;

    QProcess::ProcessChannelMode processChannelMode () const;
    void setProcessChannelMode(QProcess::ProcessChannelMode m);

    bool stdOutBufferedSignalsEnabled() const;
    void setStdOutBufferedSignalsEnabled(bool);

    bool stdErrBufferedSignalsEnabled() const;
    void setStdErrBufferedSignalsEnabled(bool);

    bool timeOutMessageBoxEnabled() const;
    void setTimeOutMessageBoxEnabled(bool);

    Environment environment() const;
    void setEnvironment(const Environment &);

    void setWorkingDirectory(const QString &workingDirectory);
    QString workingDirectory() const;

    // Unix: Do not give the child process a terminal for input prompting.
    void setDisableUnixTerminal();

    void setExitCodeInterpreter(const ExitCodeInterpreter &interpreter);
    ExitCodeInterpreter exitCodeInterpreter() const;

    // Starts a nested event loop and runs the command
    SynchronousProcessResponse run(const CommandLine &cmd, const QByteArray &writeData = {});
    // Starts the command blocking the UI fully
    SynchronousProcessResponse runBlocking(const CommandLine &cmd);

signals:
    void stdOutBuffered(const QString &lines, bool firstTime);
    void stdErrBuffered(const QString &lines, bool firstTime);

public slots:
    bool terminate();

private:
    void slotTimeout();
    void finished(int exitCode, QProcess::ExitStatus e);
    void error(QProcess::ProcessError);
    void processStdOut(bool emitSignals);
    void processStdErr(bool emitSignals);

    SynchronousProcessPrivate *d;
};

} // namespace Utils
