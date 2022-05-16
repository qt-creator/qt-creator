/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "rsyncdeploystep.h"

#include "abstractremotelinuxdeployservice.h"
#include "filetransfer.h"
#include "remotelinux_constants.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <ssh/sshconnection.h>
#include <ssh/sshsettings.h>
#include <utils/algorithm.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class RsyncDeployService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    RsyncDeployService(QObject *parent = nullptr) : AbstractRemoteLinuxDeployService(parent)
    {
        connect(&m_mkdir, &QtcProcess::done, this, [this] {
            if (m_mkdir.result() != ProcessResult::FinishedWithSuccess) {
                emit errorMessage(tr("Failed to create remote directories: %1").arg(m_mkdir.stdErr()));
                setFinished();
                return;
            }
            deployFiles();
        });
        connect(&m_fileTransfer, &FileTransfer::progress,
                this, &AbstractRemoteLinuxDeployService::stdOutData);
        connect(&m_fileTransfer, &FileTransfer::done, this, [this](const ProcessResultData &result) {
            auto notifyError = [this](const QString &message) {
                emit errorMessage(message);
                setFinished();
            };
            if (result.m_error == QProcess::FailedToStart)
                notifyError(tr("rsync failed to start: %1").arg(result.m_errorString));
            else if (result.m_exitStatus == QProcess::CrashExit)
                notifyError(tr("rsync crashed."));
            else if (result.m_exitCode != 0)
                notifyError(tr("rsync failed with exit code %1.").arg(result.m_exitCode));
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

    void filterFiles() const;
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
    filterFiles();
    return !m_files.empty();
}

void RsyncDeployService::doDeploy()
{
    createRemoteDirectories();
}

void RsyncDeployService::filterFiles() const
{
    if (!m_ignoreMissingFiles)
        return;

    Utils::erase(m_files, [](const FileToTransfer &file) { return !file.m_source.exists(); });
}

void RsyncDeployService::createRemoteDirectories()
{
    QStringList remoteDirs;
    for (const FileToTransfer &file : qAsConst(m_files))
        remoteDirs << file.m_target.parentDir().path();
    remoteDirs.sort();
    remoteDirs.removeDuplicates();
    m_mkdir.setCommand({deviceConfiguration()->filePath("mkdir"),
             {"-p", ProcessArgs::createUnixArgs(remoteDirs).toString()}});
    m_mkdir.start();
}

void RsyncDeployService::deployFiles()
{
    m_fileTransfer.setDevice(deviceConfiguration());
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

} // namespace Internal

RsyncDeployStep::RsyncDeployStep(BuildStepList *bsl, Utils::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = createDeployService<Internal::RsyncDeployService>();

    auto flags = addAspect<StringAspect>();
    flags->setDisplayStyle(StringAspect::LineEditDisplay);
    flags->setSettingsKey("RemoteLinux.RsyncDeployStep.Flags");
    flags->setLabelText(tr("Flags:"));
    flags->setValue(FileTransfer::defaultRsyncFlags());

    auto ignoreMissingFiles = addAspect<BoolAspect>();
    ignoreMissingFiles->setSettingsKey("RemoteLinux.RsyncDeployStep.IgnoreMissingFiles");
    ignoreMissingFiles->setLabel(tr("Ignore missing files:"),
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

RsyncDeployStep::~RsyncDeployStep() = default;

Utils::Id RsyncDeployStep::stepId()
{
    return Constants::RsyncDeployStepId;
}

QString RsyncDeployStep::displayName()
{
    return tr("Deploy files via rsync");
}

QString RsyncDeployStep::defaultFlags()
{
    return QString("-av");
}

RsyncCommandLine RsyncDeployStep::rsyncCommand(const SshConnection &sshConnection,
                                               const QString &flags)
{
    const QString sshCmdLine = ProcessArgs::joinArgs(
                QStringList{SshSettings::sshFilePath().toUserOutput()}
                << sshConnection.connectionOptions(SshSettings::sshFilePath()), OsTypeLinux);
    const SshConnectionParameters sshParams = sshConnection.connectionParameters();
    return RsyncCommandLine(QStringList{"-e", sshCmdLine, flags},
                            sshParams.userName() + '@' + sshParams.host());
}

} //namespace RemoteLinux

#include <rsyncdeploystep.moc>
