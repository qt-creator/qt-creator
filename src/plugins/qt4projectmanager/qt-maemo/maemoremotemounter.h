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
class MaemoUsedPortsGatherer;

class MaemoRemoteMounter : public QObject
{
    Q_OBJECT
public:
    MaemoRemoteMounter(QObject *parent);
    ~MaemoRemoteMounter();

    // Must already be connected.
    void setConnection(const QSharedPointer<Core::SshConnection> &connection);

    void setToolchain(const MaemoToolChain *toolChain);
    void addMountSpecification(const MaemoMountSpecification &mountSpec,
        bool mountAsRoot);
    bool hasValidMountSpecifications() const;
    void resetMountSpecifications() { m_mountSpecs.clear(); }
    void mount(MaemoPortList *freePorts,
        const MaemoUsedPortsGatherer *portsGatherer);
    void unmount();
    void stop();

signals:
    void mounted();
    void unmounted();
    void error(const QString &reason);
    void reportProgress(const QString &progressOutput);
    void debugOutput(const QString &output);

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
    void handleUtfsServerStderr();
    void startUtfsServers();

private:
    enum State {
        Inactive, Unmounting, UploaderInitializing, UploadRunning,
        UtfsClientsStarting, UtfsClientsStarted, UtfsServersStarted
    };

    void setState(State newState);

    void deployUtfsClient();
    void startUtfsClients();
    void killUtfsServer(QProcess *proc);
    void killAllUtfsServers();
    QString utfsClientOnDevice() const;
    QString utfsServer() const;

    QTimer * const m_utfsServerTimer;

    struct MountInfo {
        MountInfo(const MaemoMountSpecification &m, bool root)
            : mountSpec(m), mountAsRoot(root), remotePort(-1) {}
        MaemoMountSpecification mountSpec;
        bool mountAsRoot;
        int remotePort;
    };

    QSharedPointer<Core::SshConnection> m_connection;
    QList<MountInfo> m_mountSpecs;
    QSharedPointer<Core::SftpChannel> m_utfsClientUploader;
    QSharedPointer<Core::SshRemoteProcess> m_mountProcess;
    QSharedPointer<Core::SshRemoteProcess> m_unmountProcess;
    Core::SftpJobId m_uploadJobId;

    typedef QSharedPointer<QProcess> ProcPtr;
    QList<ProcPtr> m_utfsServers;

    QByteArray m_utfsClientStderr;
    QByteArray m_umountStderr;
    MaemoPortList *m_freePorts;
    const MaemoUsedPortsGatherer *m_portsGatherer;
    bool m_remoteMountsAllowed;
    QString m_maddeRoot;

    State m_state;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOREMOTEMOUNTER_H
