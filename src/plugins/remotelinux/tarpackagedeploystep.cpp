// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tarpackagedeploystep.h"

#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Utils::Tasking;

namespace RemoteLinux::Internal {

// TarPackageDeployStep

class TarPackageDeployStep : public AbstractRemoteLinuxDeployStep
{
public:
    TarPackageDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        setWidgetExpandedByDefault(false);

        setInternalInitializer([this] {
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
            setPackageFilePath(tarFile);
            return isDeploymentPossible();
        });
    }
    void setPackageFilePath(const FilePath &filePath);

private:
    QString remoteFilePath() const;
    bool isDeploymentNecessary() const final;
    Group deployRecipe() final;
    TaskItem uploadTask();
    TaskItem installTask();

    FilePath m_packageFilePath;
};

void TarPackageDeployStep::setPackageFilePath(const FilePath &filePath)
{
    m_packageFilePath = filePath;
}

QString TarPackageDeployStep::remoteFilePath() const
{
    return QLatin1String("/tmp/") + m_packageFilePath.fileName();
}

bool TarPackageDeployStep::isDeploymentNecessary() const
{
    return hasLocalFileChanged(DeployableFile(m_packageFilePath, {}));
}

TaskItem TarPackageDeployStep::uploadTask()
{
    const auto setupHandler = [this](FileTransfer &transfer) {
        const FilesToTransfer files {{m_packageFilePath,
                        deviceConfiguration()->filePath(remoteFilePath())}};
        transfer.setFilesToTransfer(files);
        connect(&transfer, &FileTransfer::progress, this, &TarPackageDeployStep::addProgressMessage);
        addProgressMessage(Tr::tr("Uploading package to device..."));
    };
    const auto doneHandler = [this](const FileTransfer &) {
        addProgressMessage(Tr::tr("Successfully uploaded package file."));
    };
    const auto errorHandler = [this](const FileTransfer &transfer) {
        const ProcessResultData result = transfer.resultData();
        addErrorMessage(result.m_errorString);
    };
    return Transfer(setupHandler, doneHandler, errorHandler);
}

TaskItem TarPackageDeployStep::installTask()
{
    const auto setupHandler = [this](QtcProcess &process) {
        const QString cmdLine = QLatin1String("cd / && tar xvf ") + remoteFilePath()
                + " && (rm " + remoteFilePath() + " || :)";
        process.setCommand({deviceConfiguration()->filePath("/bin/sh"), {"-c", cmdLine}});
        QtcProcess *proc = &process;
        connect(proc, &QtcProcess::readyReadStandardOutput, this, [this, proc] {
            handleStdOutData(proc->readAllStandardOutput());
        });
        connect(proc, &QtcProcess::readyReadStandardError, this, [this, proc] {
            handleStdErrData(proc->readAllStandardError());
        });
        addProgressMessage(Tr::tr("Installing package to device..."));
    };
    const auto doneHandler = [this](const QtcProcess &) {
        saveDeploymentTimeStamp(DeployableFile(m_packageFilePath, {}), {});
        addProgressMessage(Tr::tr("Successfully installed package file."));
    };
    const auto errorHandler = [this](const QtcProcess &process) {
        addErrorMessage(Tr::tr("Installing package failed.") + process.errorString());
    };
    return Process(setupHandler, doneHandler, errorHandler);
}

Group TarPackageDeployStep::deployRecipe()
{
    return Group { uploadTask(), installTask() };
}


// TarPackageDeployStepFactory

TarPackageDeployStepFactory::TarPackageDeployStepFactory()
{
    registerStep<TarPackageDeployStep>(Constants::TarPackageDeployStepId);
    setDisplayName(Tr::tr("Deploy tarball via SFTP upload"));
    setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // RemoteLinux::Internal
