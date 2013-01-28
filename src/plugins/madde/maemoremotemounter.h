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

#ifndef MAEMOREMOTEMOUNTER_H
#define MAEMOREMOTEMOUNTER_H

#include "maemomountspecification.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/fileutils.h>

#include <QList>
#include <QObject>
#include <QProcess>
#include <QSharedPointer>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace QSsh { class SshRemoteProcessRunner; }
namespace ProjectExplorer { class DeviceUsedPortsGatherer; }

namespace Madde {
namespace Internal {

class MaemoRemoteMounter : public QObject
{
    Q_OBJECT

public:
    MaemoRemoteMounter(QObject *parent = 0);
    ~MaemoRemoteMounter();

    void setParameters(const ProjectExplorer::IDevice::ConstPtr &devConf,
            const Utils::FileName &maddeRoot);
    void addMountSpecification(const MaemoMountSpecification &mountSpec,
        bool mountAsRoot);
    bool hasValidMountSpecifications() const;
    void resetMountSpecifications() { m_mountSpecs.clear(); }
    void mount();
    void unmount();
    void stop();

signals:
    void mounted();
    void unmounted();
    void error(const QString &reason);
    void reportProgress(const QString &progressOutput);
    void debugOutput(const QString &output);

private slots:
    void handleUtfsClientsStarted();
    void handleUtfsClientsFinished(int exitStatus);
    void handleUnmountProcessFinished(int exitStatus);
    void handleUtfsServerError(QProcess::ProcessError procError);
    void handleUtfsServerFinished(int exitCode,
        QProcess::ExitStatus exitStatus);
    void handleUtfsServerTimeout();
    void handleUtfsServerStderr();
    void startUtfsServers();
    void handlePortsGathererError(const QString &errorMsg);
    void handlePortListReady();

private:
    enum State {
        Inactive, Unmounting, UtfsClientsStarting, UtfsClientsStarted,
            UtfsServersStarted, GatheringPorts
    };

    void setState(State newState);

    void startUtfsClients();
    void killUtfsServer(QProcess *proc);
    void killAllUtfsServers();
    QString utfsClientOnDevice() const;
    Utils::FileName utfsServer() const;

    QTimer * const m_utfsServerTimer;

    struct MountInfo {
        MountInfo(const MaemoMountSpecification &m, bool root)
            : mountSpec(m), mountAsRoot(root), remotePort(-1) {}
        MaemoMountSpecification mountSpec;
        bool mountAsRoot;
        int remotePort;
    };

    ProjectExplorer::IDevice::ConstPtr m_devConf;
    QList<MountInfo> m_mountSpecs;
    QSsh::SshRemoteProcessRunner * const m_mountProcess;
    QSsh::SshRemoteProcessRunner * const m_unmountProcess;

    typedef QSharedPointer<QProcess> ProcPtr;
    QList<ProcPtr> m_utfsServers;

    ProjectExplorer::DeviceUsedPortsGatherer *m_portsGatherer;
    Utils::FileName m_maddeRoot;

    State m_state;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOREMOTEMOUNTER_H
