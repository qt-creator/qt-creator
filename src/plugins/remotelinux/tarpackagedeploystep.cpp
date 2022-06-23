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

#include "tarpackagedeploystep.h"

#include "abstractremotelinuxdeployservice.h"
#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "tarpackagecreationstep.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class TarPackageInstaller : public QObject
{
    Q_OBJECT

public:
    TarPackageInstaller();

    void installPackage(const IDeviceConstPtr &deviceConfig,
                        const QString &packageFilePath,
                        bool removePackageFile);
    void cancelInstallation();

signals:
    void stdoutData(const QString &output);
    void stderrData(const QString &output);
    void finished(const QString &errorMsg = QString());

private:
    IDevice::ConstPtr m_device;
    QtcProcess m_installer;
    QtcProcess m_killer;
};

TarPackageInstaller::TarPackageInstaller()
{
    connect(&m_installer, &QtcProcess::readyReadStandardOutput, this, [this] {
        emit stdoutData(QString::fromUtf8(m_installer.readAllStandardOutput()));
    });
    connect(&m_installer, &QtcProcess::readyReadStandardError, this, [this] {
        emit stderrData(QString::fromUtf8(m_installer.readAllStandardError()));
    });
    connect(&m_installer, &QtcProcess::done, this, [this] {
        const QString errorMessage = m_installer.result() == ProcessResult::FinishedWithSuccess
                ? QString() : tr("Installing package failed.") + m_installer.errorString();
        emit finished(errorMessage);
    });
}

void TarPackageInstaller::installPackage(const IDevice::ConstPtr &deviceConfig,
    const QString &packageFilePath, bool removePackageFile)
{
    QTC_ASSERT(m_installer.state() == QProcess::NotRunning, return);

    m_device = deviceConfig;

    QString cmdLine = QLatin1String("cd / && tar xvf ") + packageFilePath;
    if (removePackageFile)
        cmdLine += QLatin1String(" && (rm ") + packageFilePath + QLatin1String(" || :)");
    m_installer.setCommand({m_device->filePath("/bin/sh"), {"-c", cmdLine}});
    m_installer.start();
}

void TarPackageInstaller::cancelInstallation()
{
    QTC_ASSERT(m_installer.state() != QProcess::NotRunning, return);

    m_killer.setCommand({m_device->filePath("/bin/sh"), {"-c", "pkill tar"}});
    m_killer.start();
    m_installer.close();
}

class TarPackageDeployService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT

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

    State m_state = Inactive;
    FileTransfer m_uploader;
    FilePath m_packageFilePath;
    TarPackageInstaller m_installer;
};

TarPackageDeployService::TarPackageDeployService()
{
    connect(&m_uploader, &FileTransfer::done, this,
            &TarPackageDeployService::handleUploadFinished);
    connect(&m_uploader, &FileTransfer::progress, this,
            &TarPackageDeployService::progressMessage);
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
        m_installer.cancelInstallation();
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

    emit progressMessage(tr("Successfully uploaded package file."));
    const QString remoteFilePath = uploadDir() + '/' + m_packageFilePath.fileName();
    m_state = Installing;
    emit progressMessage(tr("Installing package to device..."));
    connect(&m_installer, &TarPackageInstaller::stdoutData,
            this, &AbstractRemoteLinuxDeployService::stdOutData);
    connect(&m_installer, &TarPackageInstaller::stderrData,
            this, &AbstractRemoteLinuxDeployService::stdErrData);
    connect(&m_installer, &TarPackageInstaller::finished,
            this, &TarPackageDeployService::handleInstallationFinished);
    m_installer.installPackage(deviceConfiguration(), remoteFilePath, true);
}

void TarPackageDeployService::handleInstallationFinished(const QString &errorMsg)
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

void TarPackageDeployService::setFinished()
{
    m_state = Inactive;
    m_uploader.stop();
    disconnect(&m_installer, nullptr, this, nullptr);
    handleDeploymentDone();
}

// TarPackageDeployStep

class TarPackageDeployStep : public AbstractRemoteLinuxDeployStep
{
    Q_DECLARE_TR_FUNCTIONS(RemoteLinux::Internal::TarPackageDeployStep)

public:
    TarPackageDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = createDeployService<TarPackageDeployService>();

        setWidgetExpandedByDefault(false);

        setInternalInitializer([this, service] {
            const TarPackageCreationStep *pStep = nullptr;

            for (BuildStep *step : deployConfiguration()->stepList()->steps()) {
                if (step == this)
                    break;
                if ((pStep = qobject_cast<TarPackageCreationStep *>(step)))
                    break;
            }
            if (!pStep)
                return CheckResult::failure(tr("No tarball creation step found."));

            service->setPackageFilePath(pStep->packageFilePath());
            return service->isDeploymentPossible();
        });
    }
};


// TarPackageDeployStepFactory

TarPackageDeployStepFactory::TarPackageDeployStepFactory()
{
    registerStep<TarPackageDeployStep>(Constants::TarPackageDeployStepId);
    setDisplayName(TarPackageDeployStep::tr("Deploy tarball via SFTP upload"));
    setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // Internal
} // RemoteLinux

#include "tarpackagedeploystep.moc"
