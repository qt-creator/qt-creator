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

#include "remotelinuxcheckforfreediskspacestep.h"

#include "abstractremotelinuxdeployservice.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <utils/aspects.h>
#include <utils/fileutils.h>

#include <QScopeGuard>

#include <limits>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

// RemoteLinuxCheckForFreeDiskSpaceService

class RemoteLinuxCheckForFreeDiskSpaceService : public AbstractRemoteLinuxDeployService
{
    Q_DECLARE_TR_FUNCTIONS(RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceService)

public:
    RemoteLinuxCheckForFreeDiskSpaceService() {}

    void setPathToCheck(const QString &path);
    void setRequiredSpaceInBytes(quint64 sizeInBytes);

private:
    void deployAndFinish();
    bool isDeploymentNecessary() const override { return true; }

    CheckResult isDeploymentPossible() const override;
    void doDeviceSetup() final { deployAndFinish(); }
    void stopDeviceSetup() final { }

    void doDeploy() final {}
    void stopDeployment() final {}

    QString m_pathToCheck;
    quint64 m_requiredSpaceInBytes = 0;
};

void RemoteLinuxCheckForFreeDiskSpaceService::setPathToCheck(const QString &path)
{
    m_pathToCheck = path;
}

void RemoteLinuxCheckForFreeDiskSpaceService::setRequiredSpaceInBytes(quint64 sizeInBytes)
{
    m_requiredSpaceInBytes = sizeInBytes;
}

void RemoteLinuxCheckForFreeDiskSpaceService::deployAndFinish()
{
    auto cleanup = qScopeGuard([this] { setFinished(); });
    const FilePath path
            = deviceConfiguration()->mapToGlobalPath(FilePath::fromString(m_pathToCheck));
    const qint64 freeSpace = path.bytesAvailable();
    if (freeSpace < 0) {
        emit errorMessage(tr("Cannot get info about free disk space for \"%1\"")
                .arg(path.toUserOutput()));
        return;
    }

    const qint64 mb = 1024 * 1024;
    const qint64 freeSpaceMB = freeSpace / mb;
    const qint64 requiredSpaceMB = m_requiredSpaceInBytes / mb;

    if (freeSpaceMB < requiredSpaceMB) {
        emit errorMessage(tr("The remote file system has only %n megabytes of free space, "
                "but %1 megabytes are required.", nullptr, freeSpaceMB)
                          .arg(requiredSpaceMB));
        return;
    }

    emit progressMessage(tr("The remote file system has %n megabytes of free space, going ahead.",
                            nullptr, freeSpaceMB));
}

CheckResult RemoteLinuxCheckForFreeDiskSpaceService::isDeploymentPossible() const
{
    if (!m_pathToCheck.startsWith('/')) {
        return CheckResult::failure(
           tr("Cannot check for free disk space: \"%1\" is not an absolute path.")
                    .arg(m_pathToCheck));
    }

    return AbstractRemoteLinuxDeployService::isDeploymentPossible();
}

// RemoteLinuxCheckForFreeDiskSpaceStep

RemoteLinuxCheckForFreeDiskSpaceStep::RemoteLinuxCheckForFreeDiskSpaceStep
    (BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = createDeployService<RemoteLinuxCheckForFreeDiskSpaceService>();

    auto pathToCheckAspect = addAspect<StringAspect>();
    pathToCheckAspect->setSettingsKey("RemoteLinux.CheckForFreeDiskSpaceStep.PathToCheck");
    pathToCheckAspect->setDisplayStyle(StringAspect::LineEditDisplay);
    pathToCheckAspect->setValue("/");
    pathToCheckAspect->setLabelText(tr("Remote path to check for free space:"));

    auto requiredSpaceAspect = addAspect<IntegerAspect>();
    requiredSpaceAspect->setSettingsKey("RemoteLinux.CheckForFreeDiskSpaceStep.RequiredSpace");
    requiredSpaceAspect->setLabel(tr("Required disk space:"));
    requiredSpaceAspect->setDisplayScaleFactor(1024*1024);
    requiredSpaceAspect->setValue(5*1024*1024);
    requiredSpaceAspect->setSuffix(tr("MB"));
    requiredSpaceAspect->setRange(1, std::numeric_limits<int>::max());

    setInternalInitializer([service, pathToCheckAspect, requiredSpaceAspect] {
        service->setPathToCheck(pathToCheckAspect->value());
        service->setRequiredSpaceInBytes(requiredSpaceAspect->value());
        return CheckResult::success();
    });
}

RemoteLinuxCheckForFreeDiskSpaceStep::~RemoteLinuxCheckForFreeDiskSpaceStep() = default;

Id RemoteLinuxCheckForFreeDiskSpaceStep::stepId()
{
    return "RemoteLinux.CheckForFreeDiskSpaceStep";
}

QString RemoteLinuxCheckForFreeDiskSpaceStep::displayName()
{
    return tr("Check for free disk space");
}

} // namespace RemoteLinux
