// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "rsyncdeploystep.h"

#include "abstractremotelinuxdeploystep.h"
#include "abstractremotelinuxdeployservice.h"
#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class RsyncDeployService : public AbstractRemoteLinuxDeployService
{
public:
    RsyncDeployService()
    {
        connect(&m_mkdir, &QtcProcess::done, this, [this] {
            if (m_mkdir.result() != ProcessResult::FinishedWithSuccess) {
                QString finalMessage = m_mkdir.errorString();
                const QString stdErr = m_mkdir.cleanedStdErr();
                if (!stdErr.isEmpty()) {
                    if (!finalMessage.isEmpty())
                        finalMessage += '\n';
                    finalMessage += stdErr;
                }
                emit errorMessage(Tr::tr("Deploy via rsync: failed to create remote directories:")
                                  + '\n' + finalMessage);
                setFinished();
                return;
            }
            deployFiles();
        });
        connect(&m_mkdir, &QtcProcess::readyReadStandardError, this, [this] {
            emit stdErrData(QString::fromLocal8Bit(m_mkdir.readAllStandardError()));
        });
        connect(&m_fileTransfer, &FileTransfer::progress,
                this, &AbstractRemoteLinuxDeployService::stdOutData);
        connect(&m_fileTransfer, &FileTransfer::done, this, [this](const ProcessResultData &result) {
            auto notifyError = [this](const QString &message) {
                emit errorMessage(message);
                setFinished();
            };
            if (result.m_error == QProcess::FailedToStart)
                notifyError(Tr::tr("rsync failed to start: %1").arg(result.m_errorString));
            else if (result.m_exitStatus == QProcess::CrashExit)
                notifyError(Tr::tr("rsync crashed."));
            else if (result.m_exitCode != 0)
                notifyError(Tr::tr("rsync failed with exit code %1.").arg(result.m_exitCode));
            else
                setFinished();
        });
    }

    void setDeployableFiles(const QList<DeployableFile> &files);
    void setIgnoreMissingFiles(bool ignore) { m_ignoreMissingFiles = ignore; }
    void setFlags(const QString &flags) { m_flags = flags; }

private:
    bool isDeploymentNecessary() const override;

    void doDeploy() override;
    void stopDeployment() override { setFinished(); };

    void createRemoteDirectories();
    void deployFiles();
    void setFinished();

    mutable FilesToTransfer m_files;
    bool m_ignoreMissingFiles = false;
    QString m_flags;
    QtcProcess m_mkdir;
    FileTransfer m_fileTransfer;
};

void RsyncDeployService::setDeployableFiles(const QList<DeployableFile> &files)
{
    for (const DeployableFile &f : files)
        m_files.append({f.localFilePath(), deviceConfiguration()->filePath(f.remoteFilePath())});
}

bool RsyncDeployService::isDeploymentNecessary() const
{
    if (m_ignoreMissingFiles)
        Utils::erase(m_files, [](const FileToTransfer &file) { return !file.m_source.exists(); });
    return !m_files.empty();
}

void RsyncDeployService::doDeploy()
{
    createRemoteDirectories();
}

void RsyncDeployService::createRemoteDirectories()
{
    QStringList remoteDirs;
    for (const FileToTransfer &file : qAsConst(m_files))
        remoteDirs << file.m_target.parentDir().path();
    remoteDirs.sort();
    remoteDirs.removeDuplicates();

    m_mkdir.setCommand({deviceConfiguration()->filePath("mkdir"), QStringList("-p") + remoteDirs});
    m_mkdir.start();
}

void RsyncDeployService::deployFiles()
{
    m_fileTransfer.setTransferMethod(FileTransferMethod::Rsync);
    m_fileTransfer.setRsyncFlags(m_flags);
    m_fileTransfer.setFilesToTransfer(m_files);
    m_fileTransfer.start();
}

void RsyncDeployService::setFinished()
{
    m_mkdir.close();
    m_fileTransfer.stop();
    handleDeploymentDone();
}

// RsyncDeployStep

class RsyncDeployStep : public AbstractRemoteLinuxDeployStep
{
public:
    RsyncDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = createDeployService<Internal::RsyncDeployService>();

        auto flags = addAspect<StringAspect>();
        flags->setDisplayStyle(StringAspect::LineEditDisplay);
        flags->setSettingsKey("RemoteLinux.RsyncDeployStep.Flags");
        flags->setLabelText(Tr::tr("Flags:"));
        flags->setValue(FileTransferSetupData::defaultRsyncFlags());

        auto ignoreMissingFiles = addAspect<BoolAspect>();
        ignoreMissingFiles->setSettingsKey("RemoteLinux.RsyncDeployStep.IgnoreMissingFiles");
        ignoreMissingFiles->setLabel(Tr::tr("Ignore missing files:"),
                                     BoolAspect::LabelPlacement::InExtraLabel);
        ignoreMissingFiles->setValue(false);

        setInternalInitializer([service, flags, ignoreMissingFiles] {
            service->setIgnoreMissingFiles(ignoreMissingFiles->value());
            service->setFlags(flags->value());
            return service->isDeploymentPossible();
        });

        setRunPreparer([this, service] {
            service->setDeployableFiles(target()->deploymentData().allFiles());
        });
    }
};

// RsyncDeployStepFactory

RsyncDeployStepFactory::RsyncDeployStepFactory()
{
    registerStep<RsyncDeployStep>(Constants::RsyncDeployStepId);
    setDisplayName(Tr::tr("Deploy files via rsync"));
    setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // RemoteLinux::Internal
