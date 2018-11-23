/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "sftpdefs.h"
#include "ssh_global.h"

#include <QObject>

namespace QSsh {
class SshConnection;

class QSSH_EXPORT SftpSession : public QObject
{
    friend class SshConnection;
    Q_OBJECT
public:
    ~SftpSession() override;
    void start();
    void quit();

    SftpJobId ls(const QString &path);
    SftpJobId createDirectory(const QString &path);
    SftpJobId removeDirectory(const QString &path);
    SftpJobId removeFile(const QString &path);
    SftpJobId rename(const QString &oldPath, const QString &newPath);
    SftpJobId createSoftLink(const QString &filePath, const QString &target);
    SftpJobId uploadFile(const QString &localFilePath, const QString &remoteFilePath);
    SftpJobId downloadFile(const QString &remoteFilePath, const QString &localFilePath);

    enum class State { Inactive, Starting, Running, Closing };
    State state() const;

signals:
    void started();
    void done(const QString &error);
    void commandFinished(SftpJobId job, const QString &error);
    void fileInfoAvailable(SftpJobId job, const QList<SftpFileInfo> &fileInfoList);

private:
    SftpSession(const QStringList &connectionArgs);
    void doStart();
    void handleStdout();
    void handleLsOutput(SftpJobId jobId, const QByteArray &output);

    struct SftpSessionPrivate;
    SftpSessionPrivate * const d;
};

} // namespace QSsh
