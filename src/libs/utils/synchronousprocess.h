/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef SYNCHRONOUSPROCESS_H
#define SYNCHRONOUSPROCESS_H

#include "utils_global.h"

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QTextCodec;
class QDebug;
QT_END_NAMESPACE

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

    void setStdOutCodec(QTextCodec *c);
    QTextCodec *stdOutCodec() const;

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

signals:
    void stdOut(const QByteArray &data, bool firstTime);
    void stdErr(const QByteArray &data, bool firstTime);

    void stdOutBuffered(const QString &data, bool firstTime);
    void stdErrBuffered(const QString &data, bool firstTime);

private slots:
    void slotTimeout();
    void finished(int exitCode, QProcess::ExitStatus e);
    void error(QProcess::ProcessError);
    void stdOutReady();
    void stdErrReady();

private:
    void processStdOut(bool emitSignals);
    void processStdErr(bool emitSignals);
    static QString convertStdErr(const QByteArray &);
    QString convertStdOut(const QByteArray &) const;

    SynchronousProcessPrivate *d;
};

} // namespace Utils

#endif
