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
using namespace Utils::Tasking;

namespace RemoteLinux::Internal {

class TarPackageDeployService : public AbstractRemoteLinuxDeployService
{
public:
    void setPackageFilePath(const FilePath &filePath);

private:
    QString remoteFilePath() const;

    bool isDeploymentNecessary() const override;
    void doDeploy() override;
    void stopDeployment() override;
    TaskItem uploadTask();
    TaskItem installTask();

    FilePath m_packageFilePath;
    std::unique_ptr<TaskTree> m_taskTree;
};

void TarPackageDeployService::setPackageFilePath(const FilePath &filePath)
{
    m_packageFilePath = filePath;
}

QString TarPackageDeployService::remoteFilePath() const
{
    return QLatin1String("/tmp/") + m_packageFilePath.fileName();
}

bool TarPackageDeployService::isDeploymentNecessary() const
{
    return hasLocalFileChanged(DeployableFile(m_packageFilePath, {}));
}

TaskItem TarPackageDeployService::uploadTask()
{
    auto setupHandler = [this](FileTransfer &transfer) {
        const FilesToTransfer files {{m_packageFilePath,
                        deviceConfiguration()->filePath(remoteFilePath())}};
        transfer.setFilesToTransfer(files);
        connect(&transfer, &FileTransfer::progress,
                this, &TarPackageDeployService::progressMessage);
        emit progressMessage(Tr::tr("Uploading package to device..."));
    };
    auto doneHandler = [this](const FileTransfer &) {
        emit progressMessage(Tr::tr("Successfully uploaded package file."));
    };
    auto errorHandler = [this](const FileTransfer &transfer) {
        const ProcessResultData result = transfer.resultData();
        emit errorMessage(result.m_errorString);
    };
    return Transfer(setupHandler, doneHandler, errorHandler);
}

TaskItem TarPackageDeployService::installTask()
{
    auto setupHandler = [this](QtcProcess &process) {
        const QString cmdLine = QLatin1String("cd / && tar xvf ") + remoteFilePath()
                + " && (rm " + remoteFilePath() + " || :)";
        process.setCommand({deviceConfiguration()->filePath("/bin/sh"), {"-c", cmdLine}});
        QtcProcess *proc = &process;
        connect(proc, &QtcProcess::readyReadStandardOutput, this, [this, proc] {
            emit stdOutData(QString::fromUtf8(proc->readAllStandardOutput()));
        });
        connect(proc, &QtcProcess::readyReadStandardError, this, [this, proc] {
            emit stdErrData(QString::fromUtf8(proc->readAllStandardError()));
        });
        emit progressMessage(Tr::tr("Installing package to device..."));
    };
    auto doneHandler = [this](const QtcProcess &) {
        saveDeploymentTimeStamp(DeployableFile(m_packageFilePath, {}), {});
        emit progressMessage(Tr::tr("Successfully installed package file."));
    };
    auto errorHandler = [this](const QtcProcess &process) {
        emit errorMessage(Tr::tr("Installing package failed.") + process.errorString());
    };
    return Process(setupHandler, doneHandler, errorHandler);
}

void TarPackageDeployService::doDeploy()
{
    QTC_ASSERT(!m_taskTree, return);

    auto finishHandler = [this] {
        m_taskTree.release()->deleteLater();
        stopDeployment();
    };
    Group root {
        uploadTask(),
        installTask(),
        OnGroupDone(finishHandler),
        OnGroupError(finishHandler),
    };
    m_taskTree.reset(new TaskTree(root));
    m_taskTree->start();
}

void TarPackageDeployService::stopDeployment()
{
    m_taskTree.reset();
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
