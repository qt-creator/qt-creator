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
#include "maemodeploybymountsteps.h"

#include "maemodeploymentmounter.h"
#include "maemoglobal.h"
#include "maemomountspecification.h"
#include "maemopackagecreationstep.h"
#include "maemopackageinstaller.h"
#include "maemoqemumanager.h"
#include "maemoremotecopyfacility.h"
#include "qt4maemodeployconfiguration.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <remotelinux/abstractremotelinuxdeployservice.h>
#include <remotelinux/linuxdevice.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>

#include <QFileInfo>
#include <QList>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {
class AbstractMaemoDeployByMountService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractMaemoDeployByMountService)
protected:
    AbstractMaemoDeployByMountService(QObject *parent);

    QString deployMountPoint() const;

private slots:
    void handleMounted();
    void handleUnmounted();
    void handleMountError(const QString &errorMsg);
    void handleInstallationFinished(const QString &errorMsg);

private:
    virtual QList<MaemoMountSpecification> mountSpecifications() const=0;
    virtual void doInstall() = 0;
    virtual void cancelInstallation() = 0;
    virtual void handleInstallationSuccess() = 0;

    void doDeviceSetup();
    void stopDeviceSetup();
    void doDeploy();
    void stopDeployment();

    void unmount();
    void setFinished();

    MaemoDeploymentMounter * const m_mounter;
    enum State { Inactive, Mounting, Installing, Unmounting } m_state;
    bool m_stopRequested;
};

class MaemoMountAndInstallPackageService : public AbstractMaemoDeployByMountService
{
    Q_OBJECT

public:
    MaemoMountAndInstallPackageService(QObject *parent);

    void setPackageFilePath(const QString &filePath) { m_packageFilePath = filePath; }

private:
    bool isDeploymentNecessary() const;

    QList<MaemoMountSpecification> mountSpecifications() const;
    void doInstall();
    void cancelInstallation();
    void handleInstallationSuccess();

    MaemoDebianPackageInstaller * const m_installer;
    QString m_packageFilePath;
};

class MaemoMountAndCopyFilesService : public AbstractMaemoDeployByMountService
{
    Q_OBJECT

public:
    MaemoMountAndCopyFilesService(QObject *parent);

    void setDeployableFiles(const QList<DeployableFile> &deployableFiles) {
        m_deployableFiles = deployableFiles;
    }

private:
    bool isDeploymentNecessary() const;

    QList<MaemoMountSpecification> mountSpecifications() const;
    void doInstall();
    void cancelInstallation();
    void handleInstallationSuccess();

    Q_SLOT void handleFileCopied(const ProjectExplorer::DeployableFile &deployable);

    MaemoRemoteCopyFacility * const m_copyFacility;
    mutable QList<DeployableFile> m_filesToCopy;
    QList<DeployableFile> m_deployableFiles;
};


AbstractMaemoDeployByMountService::AbstractMaemoDeployByMountService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent),
      m_mounter(new MaemoDeploymentMounter(this)),
      m_state(Inactive),
      m_stopRequested(false)
{
    connect(m_mounter, SIGNAL(setupDone()), SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(tearDownDone()), SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), SIGNAL(progressMessage(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), SIGNAL(stdErrData(QString)));
}

void AbstractMaemoDeployByMountService::doDeviceSetup()
{
    QTC_ASSERT(m_state == Inactive, return);

    handleDeviceSetupDone(true);
}

void AbstractMaemoDeployByMountService::stopDeviceSetup()
{
    QTC_ASSERT(m_state == Inactive, return);

    handleDeviceSetupDone(false);
}

void AbstractMaemoDeployByMountService::doDeploy()
{
    QTC_ASSERT(m_state == Inactive, return);

    if (!target()) {
        emit errorMessage(tr("Missing target."));
        setFinished();
        return;
    }

    m_state = Mounting;
    m_mounter->setupMounts(connection(), mountSpecifications(), profile());
}

void AbstractMaemoDeployByMountService::stopDeployment()
{
    switch (m_state) {
    case Installing:
        m_stopRequested = true;
        cancelInstallation();

        // TODO: Possibly unsafe, because the mount point may still be in use if the
        // application did not exit immediately.
        unmount();
        break;
    case Mounting:
    case Unmounting:
        m_stopRequested = true;
        break;
    case Inactive:
        qWarning("%s: Unexpected state 'Inactive'.", Q_FUNC_INFO);
        break;
    }
}

void AbstractMaemoDeployByMountService::unmount()
{
    m_state = Unmounting;
    m_mounter->tearDownMounts();
}

void AbstractMaemoDeployByMountService::handleMounted()
{
    QTC_ASSERT(m_state == Mounting, return);

    if (m_stopRequested) {
        unmount();
        return;
    }

    m_state = Installing;
    doInstall();
}

void AbstractMaemoDeployByMountService::handleUnmounted()
{
    QTC_ASSERT(m_state == Unmounting, return);

    setFinished();
}

void AbstractMaemoDeployByMountService::handleMountError(const QString &errorMsg)
{
    QTC_ASSERT(m_state == Mounting, return);

    emit errorMessage(errorMsg);
    setFinished();
}

void AbstractMaemoDeployByMountService::handleInstallationFinished(const QString &errorMsg)
{
    QTC_ASSERT(m_state == Installing, return);

    if (errorMsg.isEmpty())
        handleInstallationSuccess();
    else
        emit errorMessage(errorMsg);
    unmount();
}

void AbstractMaemoDeployByMountService::setFinished()
{
    m_state = Inactive;
    m_stopRequested = false;
    handleDeploymentDone();
}

QString AbstractMaemoDeployByMountService::deployMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(deviceConfiguration()->sshParameters().userName)
        + QLatin1String("/deployMountPoint_")
        + target()->project()->displayName();
}


MaemoMountAndInstallPackageService::MaemoMountAndInstallPackageService(QObject *parent)
    : AbstractMaemoDeployByMountService(parent), m_installer(new MaemoDebianPackageInstaller(this))
{
    connect(m_installer, SIGNAL(stdoutData(QString)), SIGNAL(stdOutData(QString)));
    connect(m_installer, SIGNAL(stderrData(QString)), SIGNAL(stdErrData(QString)));
    connect(m_installer, SIGNAL(finished(QString)), SLOT(handleInstallationFinished(QString)));
}

bool MaemoMountAndInstallPackageService::isDeploymentNecessary() const
{
    return hasChangedSinceLastDeployment(DeployableFile(m_packageFilePath, QString()));
}

QList<MaemoMountSpecification> MaemoMountAndInstallPackageService::mountSpecifications() const
{
    const QString localDir = QFileInfo(m_packageFilePath).absolutePath();
    return QList<MaemoMountSpecification>()
        << MaemoMountSpecification(localDir, deployMountPoint());
}

void MaemoMountAndInstallPackageService::doInstall()
{
    const QString remoteFilePath = deployMountPoint() + QLatin1Char('/')
        + QFileInfo(m_packageFilePath).fileName();
    m_installer->installPackage(deviceConfiguration(), remoteFilePath, false);
}

void MaemoMountAndInstallPackageService::cancelInstallation()
{
    m_installer->cancelInstallation();
}

void MaemoMountAndInstallPackageService::handleInstallationSuccess()
{
    saveDeploymentTimeStamp(DeployableFile(m_packageFilePath, QString()));
    emit progressMessage(tr("Package installed."));
}


MaemoMountAndCopyFilesService::MaemoMountAndCopyFilesService(QObject *parent)
    : AbstractMaemoDeployByMountService(parent), m_copyFacility(new MaemoRemoteCopyFacility(this))
{
    connect(m_copyFacility, SIGNAL(stdoutData(QString)), SIGNAL(stdOutData(QString)));
    connect(m_copyFacility, SIGNAL(stderrData(QString)), SIGNAL(stdErrData(QString)));
    connect(m_copyFacility, SIGNAL(progress(QString)), SIGNAL(progressMessage(QString)));
    connect(m_copyFacility, SIGNAL(fileCopied(ProjectExplorer::DeployableFile)),
        SLOT(handleFileCopied(ProjectExplorer::DeployableFile)));
    connect(m_copyFacility, SIGNAL(finished(QString)), SLOT(handleInstallationFinished(QString)));
}

bool MaemoMountAndCopyFilesService::isDeploymentNecessary() const
{
    m_filesToCopy.clear();
    for (int i = 0; i < m_deployableFiles.count(); ++i) {
        const DeployableFile &d = m_deployableFiles.at(i);
        if (hasChangedSinceLastDeployment(d) || d.localFilePath().toFileInfo().isDir())
            m_filesToCopy << d;
    }
    return !m_filesToCopy.isEmpty();
}

QList<MaemoMountSpecification> MaemoMountAndCopyFilesService::mountSpecifications() const
{
    QList<MaemoMountSpecification> mountSpecs;
    if (Utils::HostOsInfo::isWindowsHost()) {
        bool drivesToMount[26];
        qFill(drivesToMount, drivesToMount + sizeof drivesToMount / sizeof drivesToMount[0], false);
        for (int i = 0; i < m_filesToCopy.count(); ++i) {
            const QString localDir
                    = m_filesToCopy.at(i).localFilePath().toFileInfo().canonicalPath();
            const char driveLetter = localDir.at(0).toLower().toLatin1();
            if (driveLetter < 'a' || driveLetter > 'z') {
                qWarning("Weird: drive letter is '%c'.", driveLetter);
                continue;
            }

            const int index = driveLetter - 'a';
            if (drivesToMount[index])
                continue;

            const QString mountPoint = deployMountPoint() + QLatin1Char('/')
                    + QLatin1Char(driveLetter);
            const MaemoMountSpecification mountSpec(localDir.left(3), mountPoint);
            mountSpecs << mountSpec;
            drivesToMount[index] = true;
        }
    } else {
        mountSpecs << MaemoMountSpecification(QLatin1String("/"), deployMountPoint());
    }
    return mountSpecs;
}

void MaemoMountAndCopyFilesService::doInstall()
{
    m_copyFacility->copyFiles(connection(), deviceConfiguration(), m_filesToCopy,
        deployMountPoint());
}

void MaemoMountAndCopyFilesService::cancelInstallation()
{
    m_copyFacility->cancel();
}

void MaemoMountAndCopyFilesService::handleInstallationSuccess()
{
    emit progressMessage(tr("All files copied."));
}

void MaemoMountAndCopyFilesService::handleFileCopied(const DeployableFile &deployable)
{
    saveDeploymentTimeStamp(deployable);
}

MaemoInstallPackageViaMountStep::MaemoInstallPackageViaMountStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
}

MaemoInstallPackageViaMountStep::MaemoInstallPackageViaMountStep(BuildStepList *bsl,
        MaemoInstallPackageViaMountStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

void MaemoInstallPackageViaMountStep::ctor()
{
    m_deployService = new MaemoMountAndInstallPackageService(this);
    setDefaultDisplayName(displayName());
}

AbstractRemoteLinuxDeployService *MaemoInstallPackageViaMountStep::deployService() const
{
    return m_deployService;
}

bool MaemoInstallPackageViaMountStep::initInternal(QString *error)
{
    const AbstractMaemoPackageCreationStep * const pStep
        = deployConfiguration()->earlierBuildStep<MaemoDebianPackageCreationStep>(this);
    if (!pStep) {
        if (error)
            *error = tr("No Debian package creation step found.");
        return false;
    }
    m_deployService->setPackageFilePath(pStep->packageFilePath());
    return deployService()->isDeploymentPossible(error);
}

Core::Id MaemoInstallPackageViaMountStep::stepId()
{
    return Core::Id("MaemoMountAndInstallDeployStep");
}

QString MaemoInstallPackageViaMountStep::displayName()
{
    return tr("Deploy package via UTFS mount");
}


MaemoCopyFilesViaMountStep::MaemoCopyFilesViaMountStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
}

MaemoCopyFilesViaMountStep::MaemoCopyFilesViaMountStep(BuildStepList *bsl,
        MaemoCopyFilesViaMountStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

void MaemoCopyFilesViaMountStep::ctor()
{
    m_deployService = new MaemoMountAndCopyFilesService(this);
    setDefaultDisplayName(displayName());
}

AbstractRemoteLinuxDeployService *MaemoCopyFilesViaMountStep::deployService() const
{
    return m_deployService;
}

bool MaemoCopyFilesViaMountStep::initInternal(QString *error)
{
    m_deployService->setDeployableFiles(target()->deploymentData().allFiles());
    return deployService()->isDeploymentPossible(error);
}

Core::Id MaemoCopyFilesViaMountStep::stepId()
{
    return Core::Id("MaemoMountAndCopyDeployStep");
}

QString MaemoCopyFilesViaMountStep::displayName()
{
    return tr("Deploy files via UTFS mount");
}

} // namespace Internal
} // namespace Madde

#include "maemodeploybymountsteps.moc"
