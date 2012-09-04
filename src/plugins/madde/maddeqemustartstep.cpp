/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
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
#include "maddeqemustartstep.h"

#include "maemoqemumanager.h"

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <remotelinux/abstractremotelinuxdeployservice.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {
class MaddeQemuStartService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    MaddeQemuStartService(QObject *parent = 0) : AbstractRemoteLinuxDeployService(parent) {}

private:
    bool isDeploymentNecessary() const { return true; }

    void doDeviceSetup()
    {
        emit progressMessage(tr("Checking whether to start Qemu..."));
        if (deviceConfiguration()->machineType() == IDevice::Hardware) {
            emit progressMessage(tr("Target device is not an emulator. Nothing to do."));
            handleDeviceSetupDone(true);
            return;
        }

        if (MaemoQemuManager::instance().qemuIsRunning()) {
            emit progressMessage(tr("Qemu is already running. Nothing to do."));
            handleDeviceSetupDone(true);
            return;
        }

        MaemoQemuRuntime rt;
        const int qtId = QtSupport::QtKitInformation::qtVersionId(profile());
        if (MaemoQemuManager::instance().runtimeForQtVersion(qtId, &rt)) {
            MaemoQemuManager::instance().startRuntime();
            emit errorMessage(tr("Cannot deploy: Qemu was not running. "
                "It has now been started up for you, but it will take "
                "a bit of time until it is ready. Please try again then."));
        } else {
            emit errorMessage(tr("Cannot deploy: You want to deploy to Qemu, but it is not enabled "
                "for this Qt version."));
        }
        handleDeviceSetupDone(false);
    }

    void stopDeviceSetup() { handleDeviceSetupDone(false); }
    void doDeploy() { handleDeploymentDone(); }
    void stopDeployment() { handleDeploymentDone(); }
};

MaddeQemuStartStep::MaddeQemuStartStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
    setDefaultDisplayName(stepDisplayName());
}

MaddeQemuStartStep::MaddeQemuStartStep(BuildStepList *bsl, MaddeQemuStartStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

AbstractRemoteLinuxDeployService *MaddeQemuStartStep::deployService() const
{
    return m_service;
}

bool MaddeQemuStartStep::initInternal(QString *error)
{
    return deployService()->isDeploymentPossible(error);
}

void MaddeQemuStartStep::ctor()
{
    m_service = new MaddeQemuStartService(this);
}

Core::Id MaddeQemuStartStep::stepId()
{
    return Core::Id("Madde.MaddeQemuCheckStep");
}

QString MaddeQemuStartStep::stepDisplayName()
{
    return tr("Start Qemu, if necessary");
}

} // namespace Internal
} // namespace Madde

#include "maddeqemustartstep.moc"
