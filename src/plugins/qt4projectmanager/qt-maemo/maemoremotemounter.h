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

#include "maemodeviceconfigurations.h"
#include "maemomountspecification.h"

#include <coreplugin/ssh/sftpdefs.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

QT_FORWARD_DECLARE_CLASS(QTimer);

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
    MaemoRemoteMounter(QObject *parent);
    ~MaemoRemoteMounter();
    void setToolchain(const MaemoToolChain *toolchain) { m_toolChain = toolchain; }
    void setPortList(const MaemoPortList &portList) { m_portList = portList; }
    bool addMountSpecification(const MaemoMountSpecification &mountSpec,
        bool mountAsRoot);
    void resetMountSpecifications() { m_mountSpecs.clear(); }
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
    void handleUtfsClientsStarted();
    void handleUtfsClientsFinished(int exitStatus);
    void handleUtfsClientStderr(const QByteArray &output);
    void handleUnmountProcessFinished(int exitStatus);
    void handleUmountStderr(const QByteArray &output);
    void handleUtfsServerError(QProcess::ProcessError procError);
    void handleUtfsServerFinished(int exitCode,
        QProcess::ExitStatus exitStatus);
    void handleUtfsServerTimeout();

private:
    void deployUtfsClient();
    void startUtfsClients();
    void startUtfsServers();
    void killUtfsServer(QProcess *proc);
    void killAllUtfsServers();
    QString utfsClientOnDevice() const;
    QString utfsServer() const;

    const MaemoToolChain *m_toolChain;
    QTimer * const m_utfsServerTimer;

    struct MountInfo {
        MountInfo(const MaemoMountSpecification &m, int port, bool root)
            : mountSpec(m), remotePort(port), mountAsRoot(root) {}
        MaemoMountSpecification mountSpec;
        int remotePort;
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
    MaemoPortList m_portList;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOREMOTEMOUNTER_H
