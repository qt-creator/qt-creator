/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef MAEMOSSHRUNNER_H
#define MAEMOSSHRUNNER_H

#include "maemomountspecification.h"

#include <remotelinux/remotelinuxapplicationrunner.h>

namespace Madde {
namespace Internal {
class MaemoRemoteMounter;
class MaemoRunConfiguration;

class MaemoSshRunner : public RemoteLinux::AbstractRemoteLinuxApplicationRunner
{
    Q_OBJECT
public:
    MaemoSshRunner(QObject *parent, MaemoRunConfiguration *runConfig);
    ~MaemoSshRunner();

signals:
    void mountDebugOutput(const QString &output);

private slots:
    void handleMounted();
    void handleUnmounted();
    void handleMounterError(const QString &errorMsg);

private:
    enum MountState { InactiveMountState, InitialUnmounting, Mounting, Mounted, PostRunUnmounting };

    bool canRun(QString &whyNot) const;
    void doDeviceSetup();
    void doAdditionalInitialCleanup();
    void doAdditionalInitializations();
    void doPostRunCleanup();
    void doAdditionalConnectionErrorHandling();

    void mount();
    void unmount();

    MaemoRemoteMounter * const m_mounter;
    QList<MaemoMountSpecification> m_mountSpecs;
    MountState m_mountState;
    int m_qtId;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOSSHRUNNER_H
