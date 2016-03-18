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

#include <QObject>

#include <qprocess.h>

#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>
#include <ssh/sftpchannel.h>

namespace Valgrind {

class ValgrindProcess;

namespace Callgrind {

class CallgrindController : public QObject
{
    Q_OBJECT
    Q_ENUMS(Option)

public:
    enum Option {
        Unknown,
        Dump,
        ResetEventCounters,
        Pause, UnPause
    };

    explicit CallgrindController(QObject *parent = 0);
    virtual ~CallgrindController();

    void run(Valgrind::Callgrind::CallgrindController::Option option);

    void setValgrindProcess(ValgrindProcess *process);
    ValgrindProcess *valgrindProcess() { return m_valgrindProc; }

    /**
     * Make data file available locally, triggers @c localParseDataAvailable.
     *
     * If the valgrind process was run remotely, this transparently
     * downloads the data file first and returns a local path.
     */
    void getLocalDataFile();

Q_SIGNALS:
    void finished(Valgrind::Callgrind::CallgrindController::Option option);

    void localParseDataAvailable(const QString &file);

    void statusMessage(const QString &msg);

private Q_SLOTS:
    void processError(QProcess::ProcessError);
    void processFinished(int, QProcess::ExitStatus);

    void foundRemoteFile();
    void sftpInitialized();
    void sftpJobFinished(QSsh::SftpJobId job, const QString &error);

private:
    void cleanupTempFile();

    // callgrind_control process
    Valgrind::ValgrindProcess *m_process;
    // valgrind process
    Valgrind::ValgrindProcess *m_valgrindProc;

    Option m_lastOption;

    // remote callgrind support
    QSsh::SshConnection *m_ssh;
    QString m_tempDataFile;
    QSsh::SshRemoteProcess::Ptr m_findRemoteFile;
    QSsh::SftpChannel::Ptr m_sftp;
    QSsh::SftpJobId m_downloadJob;
    QByteArray m_remoteFile;
};

} // namespace Callgrind
} // namespace Valgrind
