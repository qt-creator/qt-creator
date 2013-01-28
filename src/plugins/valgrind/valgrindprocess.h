/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#ifndef VALGRIND_RUNNER_P_H
#define VALGRIND_RUNNER_P_H

#include <utils/qtcprocess.h>
#include <ssh/sshremoteprocess.h>
#include <ssh/sshconnection.h>
#include <utils/outputformat.h>

namespace Valgrind {

/**
 * Abstract process that can be subclassed to supply local and remote valgrind runs
 */
class ValgrindProcess : public QObject
{
    Q_OBJECT

public:
    explicit ValgrindProcess(QObject *parent = 0);

    virtual bool isRunning() const = 0;

    virtual void run(const QString &valgrindExecutable, const QStringList &valgrindArguments,
                     const QString &debuggeeExecutable, const QString &debuggeeArguments) = 0;
    virtual void close() = 0;

    virtual QString errorString() const = 0;
    virtual QProcess::ProcessError error() const = 0;

    virtual void setProcessChannelMode(QProcess::ProcessChannelMode mode) = 0;
    virtual void setWorkingDirectory(const QString &path) = 0;
    virtual QString workingDirectory() const = 0;
    virtual void setEnvironment(const Utils::Environment &environment) = 0;

    virtual qint64 pid() const = 0;

signals:
    void started();
    void finished(int, QProcess::ExitStatus);
    void error(QProcess::ProcessError);
    void processOutput(const QByteArray &, Utils::OutputFormat format);
};

/**
 * Run valgrind on the local machine
 */
class LocalValgrindProcess : public ValgrindProcess
{
    Q_OBJECT

public:
    explicit LocalValgrindProcess(QObject *parent = 0);

    virtual bool isRunning() const;

    virtual void run(const QString &valgrindExecutable, const QStringList &valgrindArguments,
                     const QString &debuggeeExecutable, const QString &debuggeeArguments);
    virtual void close();

    virtual QString errorString() const;
    QProcess::ProcessError error() const;

    virtual void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    virtual void setWorkingDirectory(const QString &path);
    virtual QString workingDirectory() const;
    virtual void setEnvironment(const Utils::Environment &environment);

    virtual qint64 pid() const;

private slots:
    void readyReadStandardError();
    void readyReadStandardOutput();

private:
    Utils::QtcProcess m_process;
    qint64 m_pid;
};

/**
 * Run valgrind on a remote machine via SSH
 */
class RemoteValgrindProcess : public ValgrindProcess
{
    Q_OBJECT

public:
    explicit RemoteValgrindProcess(const QSsh::SshConnectionParameters &sshParams,
                                   QObject *parent = 0);
    explicit RemoteValgrindProcess(QSsh::SshConnection *connection,
                                   QObject *parent = 0);
    ~RemoteValgrindProcess();

    virtual bool isRunning() const;

    virtual void run(const QString &valgrindExecutable, const QStringList &valgrindArguments,
                     const QString &debuggeeExecutable, const QString &debuggeeArguments);
    virtual void close();

    virtual QString errorString() const;
    QProcess::ProcessError error() const;

    virtual void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    virtual void setWorkingDirectory(const QString &path);
    virtual QString workingDirectory() const;
    virtual void setEnvironment(const Utils::Environment &environment);

    virtual qint64 pid() const;

    QSsh::SshConnection *connection() const;

private slots:
    void closed(int);
    void connected();
    void error(QSsh::SshError error);
    void processStarted();
    void findPIDOutputReceived();
    void standardOutput();
    void standardError();

private:
    QSsh::SshConnectionParameters m_params;
    QSsh::SshConnection *m_connection;
    QSsh::SshRemoteProcess::Ptr m_process;
    QString m_workingDir;
    QString m_valgrindExe;
    QStringList m_valgrindArgs;
    QString m_debuggee;
    QString m_debuggeeArgs;
    QString m_errorString;
    QProcess::ProcessError m_error;
    qint64 m_pid;
    QSsh::SshRemoteProcess::Ptr m_findPID;
};

} // namespace Valgrind

#endif // VALGRIND_RUNNER_P_H
