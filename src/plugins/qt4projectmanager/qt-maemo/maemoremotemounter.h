/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MAEMOREMOTEMOUNTER_H
#define MAEMOREMOTEMOUNTER_H

#include "maemomountspecification.h"

#include <coreplugin/ssh/sftpdefs.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

QT_FORWARD_DECLARE_CLASS(QProcess);

namespace Core {
class SftpChannel;
class SshConnection;
class SshRemoteProcess;
}

namespace Qt4ProjectManager {
namespace Internal {
class MaemoToolChain;

class MaemoRemoteMounter : public QObject
{
    Q_OBJECT
public:
    MaemoRemoteMounter(QObject *parent, const MaemoToolChain *toolchain);
    ~MaemoRemoteMounter();
    void addMountSpecification(const MaemoMountSpecification &mountSpec,
        bool mountAsRoot);
    void mount();
    void unmount();
    void stop();

    // Must be connected already.
    void setConnection(const QSharedPointer<Core::SshConnection> &connection);

signals:
    void mounted();
    void unmounted();
    void error(const QString &reason);
    void reportProgress(const QString &progressOutput);

private slots:
    void handleUploaderInitialized();
    void handleUploaderInitializationFailed(const QString &reason);
    void handleUploadFinished(Core::SftpJobId jobId, const QString &error);
    void handleUtfsClientsFinished(int exitStatus);
    void handleUnmountProcessFinished(int exitStatus);
    void handleUtfsClientStderr(const QByteArray &output);
    void handleUmountStderr(const QByteArray &output);

private:
    void deployUtfsClient();
    void startUtfsClients();
    void startUtfsServers();
    QString utfsClientOnDevice() const;
    QString utfsServer() const;

    const MaemoToolChain * const m_toolChain;

    struct MountInfo {
        MountInfo(const MaemoMountSpecification &m, bool root)
            : mountSpec(m), mountAsRoot(root) {}
        MaemoMountSpecification mountSpec;
        bool mountAsRoot;
    };

    QSharedPointer<Core::SshConnection> m_connection;
    QList<MountInfo> m_mountSpecs;
    QSharedPointer<Core::SftpChannel> m_utfsClientUploader;
    QSharedPointer<Core::SshRemoteProcess> m_mountProcess;
    QSharedPointer<Core::SshRemoteProcess> m_unmountProcess;
    Core::SftpJobId m_uploadJobId;

    typedef QSharedPointer<QProcess> ProcPtr;
    QList<ProcPtr> m_utfsServers;
    bool m_stop;
    QByteArray m_utfsClientStderr;
    QByteArray m_umountStderr;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOREMOTEMOUNTER_H
