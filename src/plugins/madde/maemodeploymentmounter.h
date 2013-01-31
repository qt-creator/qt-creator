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

#ifndef MAEMODEPLOYMENTMOUNTER_H
#define MAEMODEPLOYMENTMOUNTER_H

#include "maemomountspecification.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/portlist.h>

namespace ProjectExplorer { class Kit; }
namespace QSsh { class SshConnection; }
namespace Utils { class FileName; }

namespace Madde {
namespace Internal {
class MaemoRemoteMounter;

class MaemoDeploymentMounter : public QObject
{
    Q_OBJECT

public:
    explicit MaemoDeploymentMounter(QObject *parent = 0);
    ~MaemoDeploymentMounter();

    // Connection must be in connected state.
    void setupMounts(QSsh::SshConnection *connection,
        const QList<MaemoMountSpecification> &mountSpecs,
        const ProjectExplorer::Kit *k);
    void tearDownMounts();

signals:
    void debugOutput(const QString &output);
    void setupDone();
    void tearDownDone();
    void error(const QString &error);
    void reportProgress(const QString &message);

private slots:
    void handleMounted();
    void handleUnmounted();
    void handleMountError(const QString &errorMsg);
    void handleConnectionError();

private:
    enum State {
        Inactive, UnmountingOldDirs, UnmountingCurrentDirs,
        Mounting, Mounted, UnmountingCurrentMounts
    };

    void unmount();
    void setupMounter();
    void setState(State newState);

    State m_state;
    QSsh::SshConnection *m_connection;
    ProjectExplorer::IDevice::ConstPtr m_devConf;
    MaemoRemoteMounter * const m_mounter;
    QList<MaemoMountSpecification> m_mountSpecs;
    const ProjectExplorer::Kit *m_kit;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMODEPLOYMENTMOUNTER_H
