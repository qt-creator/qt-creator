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

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocess.h>
#include <ssh/sshsettings.h>
#include <utils/algorithm.h>
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
    RsyncDeployService(QObject *parent = nullptr) : AbstractRemoteLinuxDeployService(parent) {}

    void setDeployableFiles(const QList<DeployableFile> &files) { m_deployableFiles = files; }
    void setIgnoreMissingFiles(bool ignore) { m_ignoreMissingFiles = ignore; }

private:
    bool isDeploymentNecessary() const override;

    void doDeviceSetup() override { handleDeviceSetupDone(true); }
    void stopDeviceSetup() override { handleDeviceSetupDone(false); };

    void doDeploy() override;
    void stopDeployment() override { setFinished(); };

    void filterDeployableFiles() const;
    void createRemoteDirectories();
    void deployFiles();
    void deployNextFile();
    void setFinished();

    mutable QList<DeployableFile> m_deployableFiles;
    bool m_ignoreMissingFiles = false;
    SshProcess m_rsync;
    SshRemoteProcessPtr m_mkdir;
};

bool RsyncDeployService::isDeploymentNecessary() const
{
    filterDeployableFiles();
    return !m_deployableFiles.empty();
}

void RsyncDeployService::doDeploy()
{
    createRemoteDirectories();
}

void RsyncDeployService::filterDeployableFiles() const
{
    if (m_ignoreMissingFiles) {
        erase(m_deployableFiles, [](const DeployableFile &f) {
            return !f.localFilePath().exists();
        });
    }
}

void RsyncDeployService::createRemoteDirectories()
{
    QStringList remoteDirs;
    for (const DeployableFile &f : m_deployableFiles)
        remoteDirs << f.remoteDirectory();
    remoteDirs.sort();
    remoteDirs.removeDuplicates();
    m_mkdir = connection()->createRemoteProcess("mkdir -p " + QtcProcess::Arguments
                                                ::createUnixArgs(remoteDirs).toString().toUtf8());
    connect(m_mkdir.get(), &SshRemoteProcess::done, this, [this](const QString &error) {
        QString userError;
        if (!error.isEmpty())
            userError = error;
        if (m_mkdir->exitCode() != 0)
            userError = QString::fromUtf8(m_mkdir->readAllStandardError());
        if (!userError.isEmpty()) {
            emit errorMessage(tr("Failed to create remote directories: %1").arg(userError));
            setFinished();
            return;
        }
        deployFiles();
    });
    m_mkdir->start();
}

void RsyncDeployService::deployFiles()
{
    connect(&m_rsync, &QProcess::readyReadStandardOutput, this, [this] {
        emit progressMessage(QString::fromLocal8Bit(m_rsync.readAllStandardOutput()));
    });
    connect(&m_rsync, &QProcess::readyReadStandardError, this, [this] {
        emit warningMessage(QString::fromLocal8Bit(m_rsync.readAllStandardError()));
    });
    connect(&m_rsync, &QProcess::errorOccurred, this, [this] {
        if (m_rsync.error() == QProcess::FailedToStart) {
            emit errorMessage(tr("rsync failed to start: %1").arg(m_rsync.errorString()));
            setFinished();
        }
    });
    connect(&m_rsync, static_cast<void (QProcess::*)(int)>(&QProcess::finished), this, [this] {
        if (m_rsync.exitStatus() == QProcess::CrashExit) {
            emit errorMessage(tr("rsync crashed."));
            setFinished();
            return;
        }
        if (m_rsync.exitCode() != 0) {
            emit errorMessage(tr("rsync failed with exit code %1.").arg(m_rsync.exitCode()));
            setFinished();
            return;
        }
        deployNextFile();
    });
    deployNextFile();
}

void RsyncDeployService::deployNextFile()
{
    if (m_deployableFiles.empty()) {
        setFinished();
        return;
    }
    const DeployableFile file = m_deployableFiles.takeFirst();
    const RsyncCommandLine cmdLine = RsyncDeployStep::rsyncCommand(*connection());
    const QStringList args = QStringList(cmdLine.options)
            << file.localFilePath().toString()
            << (cmdLine.remoteHostSpec + ':' + file.remoteFilePath());
    m_rsync.start("rsync", args); // TODO: Get rsync location from settings?
}

void RsyncDeployService::setFinished()
{
    if (m_mkdir) {
        m_mkdir->disconnect();
        m_mkdir->kill();
    }
    m_rsync.disconnect();
    m_rsync.kill();
    handleDeploymentDone();
}

} // namespace Internal

class RsyncDeployStep::RsyncDeployStepPrivate
{
public:
    Internal::RsyncDeployService deployService;
    BaseBoolAspect *ignoreMissingFilesAspect;
};

RsyncDeployStep::RsyncDeployStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId()), d(new RsyncDeployStepPrivate)
{
    d->ignoreMissingFilesAspect = addAspect<BaseBoolAspect>();
    d->ignoreMissingFilesAspect
            ->setSettingsKey("RemoteLinux.RsyncDeployStep.IgnoreMissingFiles");
    d->ignoreMissingFilesAspect->setLabel(tr("Ignore missing files"));
    d->ignoreMissingFilesAspect->setValue(false);

    setDefaultDisplayName(displayName());
}

RsyncDeployStep::~RsyncDeployStep()
{
    delete d;
}

bool RsyncDeployStep::initInternal(QString *error)
{
    d->deployService.setDeployableFiles(target()->deploymentData().allFiles());
    d->deployService.setIgnoreMissingFiles(d->ignoreMissingFilesAspect->value());
    return d->deployService.isDeploymentPossible(error);
}

AbstractRemoteLinuxDeployService *RsyncDeployStep::deployService() const
{
    return &d->deployService;
}

Core::Id RsyncDeployStep::stepId()
{
    return "RemoteLinux.RsyncDeployStep";
}

QString RsyncDeployStep::displayName()
{
    return tr("Deploy files via rsync");
}

RsyncCommandLine RsyncDeployStep::rsyncCommand(const SshConnection &sshConnection)
{
    const QString sshCmdLine = QtcProcess::joinArgs(
                QStringList{SshSettings::sshFilePath().toUserOutput()}
                << sshConnection.connectionOptions());
    const SshConnectionParameters sshParams = sshConnection.connectionParameters();
    return RsyncCommandLine(QStringList{"-e", sshCmdLine, "-av"},
                            sshParams.userName() + '@' + sshParams.host());
}

} //namespace RemoteLinux

#include <rsyncdeploystep.moc>
