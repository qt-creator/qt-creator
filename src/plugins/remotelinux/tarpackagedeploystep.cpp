// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "tarpackagedeploystep.h"

#include "abstractremotelinuxdeployservice.h"
#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class TarPackageDeployService : public AbstractRemoteLinuxDeployService
{
public:
    TarPackageDeployService();
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

    void installPackage(const IDeviceConstPtr &deviceConfig, const QString &packageFilePath);
    void cancelInstallation();

    State m_state = Inactive;
    FileTransfer m_uploader;
    FilePath m_packageFilePath;

    IDevice::ConstPtr m_device;
    QtcProcess m_installer;
    QtcProcess m_killer;
};

TarPackageDeployService::TarPackageDeployService()
{
    connect(&m_uploader, &FileTransfer::done, this,
            &TarPackageDeployService::handleUploadFinished);
    connect(&m_uploader, &FileTransfer::progress, this,
            &TarPackageDeployService::progressMessage);

    connect(&m_installer, &QtcProcess::readyReadStandardOutput, this, [this] {
        emit stdOutData(QString::fromUtf8(m_installer.readAllStandardOutput()));
    });
    connect(&m_installer, &QtcProcess::readyReadStandardError, this, [this] {
        emit stdErrData(QString::fromUtf8(m_installer.readAllStandardError()));
    });
    connect(&m_installer, &QtcProcess::done, this, [this] {
        const QString errorMessage = m_installer.result() == ProcessResult::FinishedWithSuccess
                ? QString() : Tr::tr("Installing package failed.") + m_installer.errorString();
        handleInstallationFinished(errorMessage);
    });
}

void TarPackageDeployService::installPackage(const IDevice::ConstPtr &deviceConfig,
                                             const QString &packageFilePath)
{
    QTC_ASSERT(m_installer.state() == QProcess::NotRunning, return);

    m_device = deviceConfig;

    const QString cmdLine = QLatin1String("cd / && tar xvf ") + packageFilePath
            + " && (rm " + packageFilePath + " || :)";
    m_installer.setCommand({m_device->filePath("/bin/sh"), {"-c", cmdLine}});
    m_installer.start();
}

void TarPackageDeployService::cancelInstallation()
{
    QTC_ASSERT(m_installer.state() != QProcess::NotRunning, return);

    m_killer.setCommand({m_device->filePath("/bin/sh"), {"-c", "pkill tar"}});
    m_killer.start();
    m_installer.close();
}

void TarPackageDeployService::setPackageFilePath(const FilePath &filePath)
{
    m_packageFilePath = filePath;
}

QString TarPackageDeployService::uploadDir() const
{
    return QLatin1String("/tmp");
}

bool TarPackageDeployService::isDeploymentNecessary() const
{
    return hasLocalFileChanged(DeployableFile(m_packageFilePath, {}));
}

void TarPackageDeployService::doDeploy()
{
    QTC_ASSERT(m_state == Inactive, return);

    m_state = Uploading;

    const QString remoteFilePath = uploadDir() + QLatin1Char('/') + m_packageFilePath.fileName();
    const FilesToTransfer files {{m_packageFilePath,
                    deviceConfiguration()->filePath(remoteFilePath)}};
    m_uploader.setFilesToTransfer(files);
    m_uploader.start();
}

void TarPackageDeployService::stopDeployment()
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
        cancelInstallation();
        setFinished();
        break;
    }
}

void TarPackageDeployService::handleUploadFinished(const ProcessResultData &resultData)
{
    QTC_ASSERT(m_state == Uploading, return);

    if (resultData.m_error != QProcess::UnknownError) {
        emit errorMessage(resultData.m_errorString);
        setFinished();
        return;
    }

    emit progressMessage(Tr::tr("Successfully uploaded package file."));
    const QString remoteFilePath = uploadDir() + '/' + m_packageFilePath.fileName();
    m_state = Installing;
    emit progressMessage(Tr::tr("Installing package to device..."));

    installPackage(deviceConfiguration(), remoteFilePath);
}

void TarPackageDeployService::handleInstallationFinished(const QString &errorMsg)
{
    QTC_ASSERT(m_state == Installing, return);

    if (errorMsg.isEmpty()) {
        saveDeploymentTimeStamp(DeployableFile(m_packageFilePath, {}), {});
        emit progressMessage(Tr::tr("Package installed."));
    } else {
        emit errorMessage(errorMsg);
    }
    setFinished();
}

void TarPackageDeployService::setFinished()
{
    m_state = Inactive;
    m_uploader.stop();
    m_installer.close();
    handleDeploymentDone();
}

// TarPackageDeployStep

class TarPackageDeployStep : public AbstractRemoteLinuxDeployStep
{
public:
    TarPackageDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = new TarPackageDeployService;
        setDeployService(service);

        setWidgetExpandedByDefault(false);

        setInternalInitializer([this, service] {
            const BuildStep *tarCreationStep = nullptr;

            for (BuildStep *step : deployConfiguration()->stepList()->steps()) {
                if (step == this)
                    break;
                if (step->id() == Constants::TarPackageCreationStepId) {
                    tarCreationStep = step;
                    break;
                }
            }
            if (!tarCreationStep)
                return CheckResult::failure(Tr::tr("No tarball creation step found."));

            const FilePath tarFile =
                    FilePath::fromVariant(tarCreationStep->data(Constants::TarPackageFilePathId));
            service->setPackageFilePath(tarFile);
            return service->isDeploymentPossible();
        });
    }
};


// TarPackageDeployStepFactory

TarPackageDeployStepFactory::TarPackageDeployStepFactory()
{
    registerStep<TarPackageDeployStep>(Constants::TarPackageDeployStepId);
    setDisplayName(Tr::tr("Deploy tarball via SFTP upload"));
    setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // RemoteLinux::Internal
