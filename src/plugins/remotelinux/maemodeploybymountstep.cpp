/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "maemodeploybymountstep.h"

#include "deploymentinfo.h"
#include "maemodeploymentmounter.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemopackageinstaller.h"
#include "maemoremotecopyfacility.h"
#include "qt4maemodeployconfiguration.h"
#include "qt4maemotarget.h"

#include <projectexplorer/project.h>
#include <utils/ssh/sshconnection.h>

#include <QtCore/QFileInfo>

#define ASSERT_BASE_STATE(state) ASSERT_STATE_GENERIC(BaseState, state, baseState())
#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(ExtendedState, state, m_extendedState)

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {

AbstractMaemoDeployByMountStep::AbstractMaemoDeployByMountStep(BuildStepList *parent,
    const QString &id)
        : AbstractMaemoDeployStep(parent, id)
{
    ctor();
}

AbstractMaemoDeployByMountStep::AbstractMaemoDeployByMountStep(BuildStepList *parent,
    AbstractMaemoDeployByMountStep *other)
    : AbstractMaemoDeployStep(parent, other)
{
    ctor();
}

void AbstractMaemoDeployByMountStep::ctor()
{
    m_extendedState= Inactive;

    m_mounter = new MaemoDeploymentMounter(this);
    connect(m_mounter, SIGNAL(setupDone()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(tearDownDone()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SLOT(handleProgressReport(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), this,
        SLOT(handleMountDebugOutput(QString)));
}

void AbstractMaemoDeployByMountStep::stopInternal()
{
    ASSERT_STATE(QList<ExtendedState>() << Mounting << Installing
        << Unmounting);

    switch (m_extendedState) {
    case Installing:
        cancelInstallation();
        unmount();
        break;
    case Mounting:
    case Unmounting:
        break; // Nothing to do here.
    case Inactive:
        setDeploymentFinished();
        break;
    default:
        qFatal("Missing switch case in %s.", Q_FUNC_INFO);
    }
}

void AbstractMaemoDeployByMountStep::startInternal()
{
    Q_ASSERT(m_extendedState == Inactive);

    mount();
}

void AbstractMaemoDeployByMountStep::handleMounted()
{
    ASSERT_BASE_STATE(QList<BaseState>() << Deploying << StopRequested);
    ASSERT_STATE(Mounting);

    if (baseState() == StopRequested) {
        unmount();
        return;
    }

    writeOutput(tr("Installing package to device..."));
    m_extendedState = Installing;
    deploy();
}

void AbstractMaemoDeployByMountStep::handleUnmounted()
{
    ASSERT_BASE_STATE(QList<BaseState>() << Deploying << StopRequested);
    ASSERT_STATE(Unmounting);

    setFinished();
}

void AbstractMaemoDeployByMountStep::handleMountError(const QString &errorMsg)
{
    ASSERT_BASE_STATE(QList<BaseState>() << Deploying << StopRequested);
    ASSERT_STATE(QList<ExtendedState>() << Mounting << Unmounting);

    raiseError(errorMsg);
    setFinished();
}

void AbstractMaemoDeployByMountStep::handleMountDebugOutput(const QString &output)
{
    ASSERT_BASE_STATE(QList<BaseState>() << Deploying << StopRequested);

    if (m_extendedState != Inactive)
        writeOutput(output, ErrorOutput, DontAppendNewline);
}

void AbstractMaemoDeployByMountStep::mount()
{
    m_extendedState = Mounting;
    m_mounter->setupMounts(connection(), deviceConfiguration(),
        mountSpecifications(), qt4BuildConfiguration());
}

QString AbstractMaemoDeployByMountStep::deployMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(connection()->connectionParameters().userName)
        + QLatin1String("/deployMountPoint_") + target()->project()->displayName();
}

void AbstractMaemoDeployByMountStep::handleInstallationFinished(const QString &errorMsg)
{
    ASSERT_BASE_STATE(QList<BaseState>() << Deploying << StopRequested);

    if (baseState() == StopRequested) {
        unmount();
        return;
    }

    if (m_extendedState != Installing)
        return;

    if (errorMsg.isEmpty())
        handleInstallationSuccess();
    else
        raiseError(errorMsg);
    unmount();
}

void AbstractMaemoDeployByMountStep::unmount()
{
    m_extendedState = Unmounting;
    m_mounter->tearDownMounts();
}

void AbstractMaemoDeployByMountStep::setFinished()
{
    m_extendedState = Inactive;
    setDeploymentFinished();
}


MaemoMountAndInstallDeployStep::MaemoMountAndInstallDeployStep(BuildStepList *bc)
    : AbstractMaemoDeployByMountStep(bc, Id)
{
    ctor();
}

MaemoMountAndInstallDeployStep::MaemoMountAndInstallDeployStep(BuildStepList *bc,
    MaemoMountAndInstallDeployStep *other)
        : AbstractMaemoDeployByMountStep(bc, other)
{
    ctor();
}

void MaemoMountAndInstallDeployStep::ctor()
{
    // MaemoMountAndInstallDeployStep default display name
    setDefaultDisplayName(displayName());

    if (qobject_cast<AbstractDebBasedQt4MaemoTarget *>(target()))
        m_installer = new MaemoDebianPackageInstaller(this);
    else
        m_installer = new MaemoRpmPackageInstaller(this);
    connect(m_installer, SIGNAL(stdoutData(QString)),
        SLOT(handleRemoteStdout(QString)));
    connect(m_installer, SIGNAL(stderrData(QString)),
        SLOT(handleRemoteStderr(QString)));
    connect(m_installer, SIGNAL(finished(QString)),
        SLOT(handleInstallationFinished(QString)));
}

const AbstractMaemoPackageCreationStep *MaemoMountAndInstallDeployStep::packagingStep() const
{
    return MaemoGlobal::earlierBuildStep<MaemoDebianPackageCreationStep>(maemoDeployConfig(), this);
}

bool MaemoMountAndInstallDeployStep::isDeploymentPossibleInternal(QString &whyNot) const
{
    if (!packagingStep()) {
        whyNot = tr("No matching packaging step found.");
        return false;
    }
    return true;
}

bool MaemoMountAndInstallDeployStep::isDeploymentNeeded(const QString &hostName) const
{
    const AbstractMaemoPackageCreationStep * const pStep = packagingStep();
    Q_ASSERT(pStep);
    const DeployableFile d(pStep->packageFilePath(), QString());
    return currentlyNeedsDeployment(hostName, d);
}

QList<MaemoMountSpecification> MaemoMountAndInstallDeployStep::mountSpecifications() const
{
    const QString localDir
        = QFileInfo(packagingStep()->packageFilePath()).absolutePath();
    return QList<MaemoMountSpecification>()
        << MaemoMountSpecification(localDir, deployMountPoint());
}

void MaemoMountAndInstallDeployStep::deploy()
{
    const QString remoteFilePath = deployMountPoint() + QLatin1Char('/')
        + QFileInfo(packagingStep()->packageFilePath()).fileName();
    m_installer->installPackage(connection(), deviceConfiguration(), remoteFilePath, false);
}

void MaemoMountAndInstallDeployStep::cancelInstallation()
{
    m_installer->cancelInstallation();
}

void MaemoMountAndInstallDeployStep::handleInstallationSuccess()
{
    setDeployed(connection()->connectionParameters().host,
        DeployableFile(packagingStep()->packageFilePath(), QString()));
    writeOutput(tr("Package installed."));
}

const QString MaemoMountAndInstallDeployStep::Id("MaemoMountAndInstallDeployStep");
QString MaemoMountAndInstallDeployStep::displayName()
{
    return tr("Deploy package via UTFS mount");
}

MaemoMountAndCopyDeployStep::MaemoMountAndCopyDeployStep(BuildStepList *bc)
    : AbstractMaemoDeployByMountStep(bc, Id)
{
    ctor();
}

MaemoMountAndCopyDeployStep::MaemoMountAndCopyDeployStep(BuildStepList *bc,
    MaemoMountAndCopyDeployStep *other)
        : AbstractMaemoDeployByMountStep(bc, other)
{
    ctor();
}

void MaemoMountAndCopyDeployStep::ctor()
{
    // MaemoMountAndCopyDeployStep default display name
    setDefaultDisplayName(displayName());

    m_copyFacility = new MaemoRemoteCopyFacility(this);
    connect(m_copyFacility, SIGNAL(stdoutData(QString)),
        SLOT(handleRemoteStdout(QString)));
    connect(m_copyFacility, SIGNAL(stderrData(QString)),
        SLOT(handleRemoteStderr(QString)));
    connect(m_copyFacility, SIGNAL(progress(QString)),
        SLOT(handleProgressReport(QString)));
    connect(m_copyFacility, SIGNAL(fileCopied(DeployableFile)),
        SLOT(handleFileCopied(DeployableFile)));
    connect(m_copyFacility, SIGNAL(finished(QString)),
        SLOT(handleInstallationFinished(QString)));
}

bool MaemoMountAndCopyDeployStep::isDeploymentPossibleInternal(QString &) const
{
    return true;
}

bool MaemoMountAndCopyDeployStep::isDeploymentNeeded(const QString &hostName) const
{
    m_filesToCopy.clear();
    const QSharedPointer<DeploymentInfo> deploymentInfo
        = maemoDeployConfig()->deploymentInfo();
    const int deployableCount = deploymentInfo->deployableCount();
    for (int i = 0; i < deployableCount; ++i) {
        const DeployableFile &d = deploymentInfo->deployableAt(i);
        if (currentlyNeedsDeployment(hostName, d)
                || QFileInfo(d.localFilePath).isDir()) {
            m_filesToCopy << d;
        }
    }
    return !m_filesToCopy.isEmpty();
}

QList<MaemoMountSpecification> MaemoMountAndCopyDeployStep::mountSpecifications() const
{
    QList<MaemoMountSpecification> mountSpecs;
#ifdef Q_OS_WIN
    bool drivesToMount[26];
    qFill(drivesToMount, drivesToMount + sizeof drivesToMount / sizeof drivesToMount[0], false);
    for (int i = 0; i < m_filesToCopy.count(); ++i) {
        const QString localDir
            = QFileInfo(m_filesToCopy.at(i).localFilePath).canonicalPath();
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
#else
    mountSpecs << MaemoMountSpecification(QLatin1String("/"),
        deployMountPoint());
#endif
    return mountSpecs;
}

void MaemoMountAndCopyDeployStep::deploy()
{
    m_copyFacility->copyFiles(connection(), deviceConfiguration(),
        m_filesToCopy, deployMountPoint());
}

void MaemoMountAndCopyDeployStep::handleFileCopied(const DeployableFile &deployable)
{
    setDeployed(connection()->connectionParameters().host, deployable);
}

void MaemoMountAndCopyDeployStep::cancelInstallation()
{
    m_copyFacility->cancel();
}

void MaemoMountAndCopyDeployStep::handleInstallationSuccess()
{
    writeOutput(tr("All files copied."));
}

const QString MaemoMountAndCopyDeployStep::Id("MaemoMountAndCopyDeployStep");

QString MaemoMountAndCopyDeployStep::displayName()
{
    return tr("Deploy files via UTFS mount");
}

} // namespace Internal
} // namespace RemoteLinux
