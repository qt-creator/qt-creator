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

#include "uploadandinstalltarpackagestep.h"

#include "remotelinux_constants.h"
#include "remotelinuxpackageinstaller.h"
#include "tarpackagecreationstep.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <utils/processinterface.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class UploadAndInstallTarPackageService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT

public:
    UploadAndInstallTarPackageService();
    void setPackageFilePath(const FilePath &filePath);

private:
    enum State { Inactive, Uploading, Installing };

    void handleUploadFinished(const ProcessResultData &resultData);
    void handleInstallationFinished(const QString &errorMsg);

    QString uploadDir() const; // Defaults to remote user's home directory.

    bool isDeploymentNecessary() const override;
    void doDeploy() override;
    void stopDeployment() override;

    void setFinished();

    State m_state = Inactive;
    FileTransfer m_uploader;
    FilePath m_packageFilePath;
    RemoteLinuxTarPackageInstaller m_installer;
};

UploadAndInstallTarPackageService::UploadAndInstallTarPackageService()
{
    connect(&m_uploader, &FileTransfer::done, this,
            &UploadAndInstallTarPackageService::handleUploadFinished);
    connect(&m_uploader, &FileTransfer::progress, this,
            &UploadAndInstallTarPackageService::progressMessage);
}

void UploadAndInstallTarPackageService::setPackageFilePath(const FilePath &filePath)
{
    m_packageFilePath = filePath;
}

QString UploadAndInstallTarPackageService::uploadDir() const
{
    return QLatin1String("/tmp");
}

bool UploadAndInstallTarPackageService::isDeploymentNecessary() const
{
    return hasLocalFileChanged(DeployableFile(m_packageFilePath, {}));
}

void UploadAndInstallTarPackageService::doDeploy()
{
    QTC_ASSERT(m_state == Inactive, return);

    m_state = Uploading;

    const QString remoteFilePath = uploadDir() + QLatin1Char('/') + m_packageFilePath.fileName();
    const FilesToTransfer files {{m_packageFilePath,
                    deviceConfiguration()->filePath(remoteFilePath)}};
    m_uploader.setFilesToTransfer(files);
    m_uploader.start();
}

void UploadAndInstallTarPackageService::stopDeployment()
{
    switch (m_state) {
    case Inactive:
        qWarning("%s: Unexpected state 'Inactive'.", Q_FUNC_INFO);
        break;
    case Uploading:
        m_uploader.stop();
        setFinished();
        break;
    case Installing:
        m_installer.cancelInstallation();
        setFinished();
        break;
    }
}

void UploadAndInstallTarPackageService::handleUploadFinished(const ProcessResultData &resultData)
{
    QTC_ASSERT(m_state == Uploading, return);

    if (resultData.m_error != QProcess::UnknownError) {
        emit errorMessage(resultData.m_errorString);
        setFinished();
        return;
    }

    emit progressMessage(tr("Successfully uploaded package file."));
    const QString remoteFilePath = uploadDir() + '/' + m_packageFilePath.fileName();
    m_state = Installing;
    emit progressMessage(tr("Installing package to device..."));
    connect(&m_installer, &AbstractRemoteLinuxPackageInstaller::stdoutData,
            this, &AbstractRemoteLinuxDeployService::stdOutData);
    connect(&m_installer, &AbstractRemoteLinuxPackageInstaller::stderrData,
            this, &AbstractRemoteLinuxDeployService::stdErrData);
    connect(&m_installer, &AbstractRemoteLinuxPackageInstaller::finished,
            this, &UploadAndInstallTarPackageService::handleInstallationFinished);
    m_installer.installPackage(deviceConfiguration(), remoteFilePath, true);
}

void UploadAndInstallTarPackageService::handleInstallationFinished(const QString &errorMsg)
{
    QTC_ASSERT(m_state == Installing, return);

    if (errorMsg.isEmpty()) {
        saveDeploymentTimeStamp(DeployableFile(m_packageFilePath, {}), {});
        emit progressMessage(tr("Package installed."));
    } else {
        emit errorMessage(errorMsg);
    }
    setFinished();
}

void UploadAndInstallTarPackageService::setFinished()
{
    m_state = Inactive;
    m_uploader.stop();
    disconnect(&m_installer, nullptr, this, nullptr);
    handleDeploymentDone();
}

} // namespace Internal

using namespace Internal;

UploadAndInstallTarPackageStep::UploadAndInstallTarPackageStep(BuildStepList *bsl, Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = createDeployService<UploadAndInstallTarPackageService>();

    setWidgetExpandedByDefault(false);

    setInternalInitializer([this, service] {
        const TarPackageCreationStep *pStep = nullptr;

        for (BuildStep *step : deployConfiguration()->stepList()->steps()) {
            if (step == this)
                break;
            if ((pStep = dynamic_cast<TarPackageCreationStep *>(step)))
                break;
        }
        if (!pStep)
            return CheckResult::failure(tr("No tarball creation step found."));

        service->setPackageFilePath(pStep->packageFilePath());
        return service->isDeploymentPossible();
    });
}

Id UploadAndInstallTarPackageStep::stepId()
{
    return Constants::UploadAndInstallTarPackageStepId;
}

QString UploadAndInstallTarPackageStep::displayName()
{
    return tr("Deploy tarball via SFTP upload");
}

} //namespace RemoteLinux

#include "uploadandinstalltarpackagestep.moc"
