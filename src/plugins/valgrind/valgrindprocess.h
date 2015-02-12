/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#ifndef VALGRINDPROCESS_H
#define VALGRINDPROCESS_H

#include <projectexplorer/applicationlauncher.h>

#include <ssh/sshremoteprocess.h>
#include <ssh/sshconnection.h>
#include <utils/osspecificaspects.h>
#include <utils/outputformat.h>

namespace Valgrind {

struct Remote {
    QSsh::SshConnectionParameters m_params;
    QSsh::SshConnection *m_connection;
    QSsh::SshRemoteProcess::Ptr m_process;
    QString m_workingDir;
    QString m_valgrindExe;
    QString m_debuggee;
    QString m_errorString;
    QProcess::ProcessError m_error;
    QSsh::SshRemoteProcess::Ptr m_findPID;
};

/**
 * Process for supplying local and remote valgrind runs
 */
class ValgrindProcess : public QObject
{
    Q_OBJECT

public:
    ValgrindProcess(bool isLocal, const QSsh::SshConnectionParameters &sshParams,
                    QSsh::SshConnection *connection = 0, QObject *parent = 0);

    bool isRunning() const;

    void setValgrindExecutable(const QString &valgrindExecutable);
    void setValgrindArguments(const QStringList &valgrindArguments);
    void setDebuggeeExecutable(const QString &debuggeeExecutable);
    void setDebugeeArguments(const QString &debuggeeArguments);

    void run();
    void close();

    QString errorString() const;
    QProcess::ProcessError processError() const;

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    void setWorkingDirectory(const QString &path);
    QString workingDirectory() const;
    void setEnvironment(const Utils::Environment &environment);
    void setLocalRunMode(ProjectExplorer::ApplicationLauncher::Mode localRunMode);

    qint64 pid() const;
    QSsh::SshConnection *connection() const;
    bool isLocal() const { return m_isLocal; }

signals:
    void started();
    void finished(int, QProcess::ExitStatus);
    void error(QProcess::ProcessError);
    void processOutput(const QString &, Utils::OutputFormat format);
    void localHostAddressRetrieved(const QHostAddress &localHostAddress);

private slots:
    void handleRemoteStderr();
    void handleRemoteStdout();
    void handleError(QSsh::SshError);

    void closed(int);
    void connected();
    void localProcessStarted();
    void remoteProcessStarted();
    void findPIDOutputReceived();

private:
    QString argumentString(Utils::OsType osType) const;

    ProjectExplorer::ApplicationLauncher m_localProcess;

    qint64 m_pid;

    Remote m_remote;
    QString m_valgrindExecutable;
    QStringList m_valgrindArguments;
    QString m_debuggeeExecutable;
    QString m_debuggeeArguments;
    bool m_isLocal;
    ProjectExplorer::ApplicationLauncher::Mode m_localRunMode;
};



} // namespace Valgrind

#endif // VALGRINDPROCESS_H
