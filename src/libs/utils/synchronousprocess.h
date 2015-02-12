/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef SYNCHRONOUSPROCESS_H
#define SYNCHRONOUSPROCESS_H

#include "utils_global.h"

#include <QProcess>
#include <QSharedPointer>
#include <QTextCodec>

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace Utils {

struct SynchronousProcessPrivate;

/* Result of SynchronousProcess execution */
struct QTCREATOR_UTILS_EXPORT SynchronousProcessResponse
{
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

    SynchronousProcessResponse();
    void clear();

    // Helper to format an exit message.
    QString exitMessage(const QString &binary, int timeoutMS) const;

    Result result;
    int exitCode;
    QString stdOut;
    QString stdErr;
};

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const SynchronousProcessResponse &);

class QTCREATOR_UTILS_EXPORT ExitCodeInterpreter : public QObject
{
    Q_OBJECT
public:
    ExitCodeInterpreter(QObject *parent) : QObject(parent) {}
    virtual SynchronousProcessResponse::Result interpretExitCode(int code) const;
};

class QTCREATOR_UTILS_EXPORT SynchronousProcess : public QObject
{
    Q_OBJECT
public:
    enum Flags {
        // Unix: Do not give the child process a terminal for input prompting.
        UnixTerminalDisabled = 0x1
    };

    SynchronousProcess();
    virtual ~SynchronousProcess();

    /* Timeout for hanging processes (triggers after no more output
     * occurs on stderr/stdout). */
    void setTimeout(int timeoutMS);
    int timeout() const;

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

    QStringList environment() const;
    void setEnvironment(const QStringList &);

    void setProcessEnvironment(const QProcessEnvironment &environment);
    QProcessEnvironment processEnvironment() const;

    void setWorkingDirectory(const QString &workingDirectory);
    QString workingDirectory() const;

    unsigned flags() const;
    void setFlags(unsigned);

    void setExitCodeInterpreter(ExitCodeInterpreter *interpreter);
    ExitCodeInterpreter *exitCodeInterpreter() const;

    SynchronousProcessResponse run(const QString &binary, const QStringList &args);

    // Create a (derived) processes with flags applied.
    static QSharedPointer<QProcess> createProcess(unsigned flags);

    // Static helper for running a process synchronously in the foreground with timeout
    // detection similar SynchronousProcess' handling (taking effect after no more output
    // occurs on stderr/stdout as opposed to waitForFinished()). Returns false if a timeout
    // occurs. Checking of the process' exit state/code still has to be done.
    static bool readDataFromProcess(QProcess &p, int timeOutMS,
                                    QByteArray *stdOut = 0, QByteArray *stdErr = 0,
                                    bool timeOutMessageBox = false);
    // Stop a process by first calling terminate() (allowing for signal handling) and
    // then kill().
    static bool stopProcess(QProcess &p);

    // Helpers to find binaries. Do not use it for other path variables
    // and file types.
    static QString locateBinary(const QString &binary);
    static QString locateBinary(const QString &path, const QString &binary);

    static QString normalizeNewlines(const QString &text);

signals:
    void stdOut(const QString &text, bool firstTime);
    void stdErr(const QString &text, bool firstTime);

    void stdOutBuffered(const QString &data, bool firstTime);
    void stdErrBuffered(const QString &data, bool firstTime);

public slots:
    bool terminate();

private slots:
    void slotTimeout();
    void finished(int exitCode, QProcess::ExitStatus e);
    void error(QProcess::ProcessError);
    void stdOutReady();
    void stdErrReady();

private:
    void processStdOut(bool emitSignals);
    void processStdErr(bool emitSignals);
    QString convertOutput(const QByteArray &, QTextCodec::ConverterState *state) const;

    SynchronousProcessPrivate *d;
};

} // namespace Utils

#endif
