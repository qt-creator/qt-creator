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

#ifndef CALLGRINDCONTROLLER_H
#define CALLGRINDCONTROLLER_H

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

#endif // CALLGRINDCONTROLLER_H
