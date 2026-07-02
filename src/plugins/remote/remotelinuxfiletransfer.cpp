// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linuxdevice.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/filetransferinterface.h>
#include <projectexplorer/devicesupport/processlist.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/devicesupport/sshsettings.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>

#include <QDir>
#include <QFileInfo>
#include <utils/async.h>
#include <utils/processinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Remote::Internal {

const QByteArray s_pidMarker = "__qtc";

static SshParameters displayless(const SshParameters &sshParameters)
{
    SshParameters parameters = sshParameters;
    parameters.setX11DisplayName({});
    return parameters;
}

static FilePaths dirsToCreate(const FilesToTransfer &files)
{
    FilePaths dirs;
    for (const FileToTransfer &file : files) {
        FilePath parentDir = file.m_target.parentDir();
        while (true) {
            if (dirs.contains(parentDir) || parentDir.isRootPath())
                break;
            dirs << parentDir;
            parentDir = parentDir.parentDir();
        }
    }
    return sorted(std::move(dirs));
}

static QByteArray transferCommand(bool link)
{
    return link ? "ln -s" : "put -R";
}

class SshTransferInterface : public FileTransferInterface
{
protected:
    SshTransferInterface(const FileTransferSetupData &setup, const DeviceConstRef &device)
        : FileTransferInterface(setup)
        , m_device(device)
        , m_process(this)
    {
        SshParameters::setupSshEnvironment(&m_process);
        connect(&m_process, &Process::readyReadStandardOutput, this, [this] {
            emit progress(QString::fromLocal8Bit(m_process.readAllRawStandardOutput()));
        });
        connect(&m_process, &Process::done, this, &SshTransferInterface::doneImpl);
    }

    DeviceConstRef device() const { return m_device; }

    bool handleError()
    {
        ProcessResultData resultData = m_process.resultData();
        if (resultData.m_error == ProcessError::FailedToStart) {
            resultData.m_errorString = Tr::tr("\"%1\" failed to start: %2")
                    .arg(FileTransfer::transferMethodName(m_setup.m_method), resultData.m_errorString);
        } else if (resultData.m_exitStatus != ProcessExitStatus::NormalExit) {
            resultData.m_errorString = Tr::tr("\"%1\" crashed.")
                    .arg(FileTransfer::transferMethodName(m_setup.m_method));
        } else if (resultData.m_exitCode != 0) {
            resultData.m_errorString = QString::fromLocal8Bit(m_process.readAllRawStandardError());
        } else {
            return false;
        }
        emit done(resultData);
        return true;
    }

    void handleDone()
    {
        if (!handleError())
            emit done(m_process.resultData());
    }

    QStringList fullConnectionOptions() const
    {
        QStringList options = m_sshParameters.connectionOptions(sshSettings().sshFilePath());
        if (!m_socketFilePath.isEmpty())
            options << "-o" << ("ControlPath=" + m_socketFilePath);
        return options;
    }

    QString host() const { return m_sshParameters.host(); }
    QString userAtHost() const { return m_sshParameters.userAtHost(); }

    Process &process() { return m_process; }

private:
    virtual void startImpl() = 0;
    virtual void doneImpl() = 0;

    void start() final
    {
        m_sshParameters = displayless(m_device.sshParameters());
        const Id linkDeviceId = m_device.linkDeviceId();
        const auto linkDevice = DeviceManager::find(linkDeviceId);
        const bool useConnectionSharing = !linkDevice && sshSettings().useConnectionSharing();

        if (useConnectionSharing) {
            m_connecting = true;
            m_connectionHandle.reset(new SshConnectionHandle(m_device));
            m_connectionHandle->setParent(this);
            connect(m_connectionHandle.get(), &SshConnectionHandle::connected,
                    this, &SshTransferInterface::handleConnected);
            connect(m_connectionHandle.get(), &SshConnectionHandle::disconnected,
                    this, &SshTransferInterface::handleDisconnected);
            auto linuxDevice = std::dynamic_pointer_cast<const LinuxDevice>(m_device.lock());
            QTC_ASSERT(linuxDevice, startFailed("No Linux device"); return);
            linuxDevice->attachToSharedConnection(m_connectionHandle.get(), m_sshParameters);
        } else {
            startImpl();
        }
    }

    void handleConnected(const QString &socketFilePath)
    {
        m_connecting = false;
        m_socketFilePath = socketFilePath;
        startImpl();
    }

    void handleDisconnected(const ProcessResultData &result)
    {
        ProcessResultData resultData = result;
        if (m_connecting)
            resultData.m_error = ProcessError::FailedToStart;

        m_connecting = false;
        if (m_connectionHandle) // TODO: should it disconnect from signals first?
            m_connectionHandle.release()->deleteLater();

        if (resultData.m_error != ProcessError::UnknownError || m_process.state() != ProcessState::NotRunning)
            emit done(resultData); // TODO: don't emit done() on process finished afterwards
    }

    DeviceConstRef m_device;
    SshParameters m_sshParameters;

    // ssh shared connection related
    std::unique_ptr<SshConnectionHandle> m_connectionHandle;
    QString m_socketFilePath;
    bool m_connecting = false;

    Process m_process;
};

class SftpTransferImpl : public SshTransferInterface
{
public:
    SftpTransferImpl(const FileTransferSetupData &setup, const DeviceConstRef &device)
        : SshTransferInterface(setup, device)
    {}

private:
    void startImpl() final
    {
        FilePath sftpBinary = sshSettings().sftpFilePath();

        // This is a hack. We only test the last hop here.
        const Id linkDeviceId = device().linkDeviceId();
        if (const auto linkDevice = DeviceManager::find(linkDeviceId))
            sftpBinary = linkDevice->filePath(sftpBinary.fileName()).searchInPath();

        if (!sftpBinary.exists()) {
            startFailed(Tr::tr("\"sftp\" binary \"%1\" does not exist.")
                            .arg(sftpBinary.toUserOutput()));
            return;
        }

        QByteArray batchData;

        const FilePaths dirs = dirsToCreate(m_setup.m_files);
        for (const FilePath &dir : dirs) {
            if (!dir.exists())
                batchData += "-mkdir " + ProcessArgs::quoteArgUnix(dir.path()).toLocal8Bit() + '\n';
        }

        for (const FileToTransfer &file : m_setup.m_files) {
            FilePath sourceFileOrLinkTarget = file.m_source;
            bool link = false;

            const QFileInfo fi(file.m_source.toFileInfo());
            if (fi.isSymLink()) {
                link = true;
                batchData += "-rm " + ProcessArgs::quoteArgUnix(
                                          file.m_target.path()).toLocal8Bit() + '\n';
                // see QTBUG-5817.
                sourceFileOrLinkTarget =
                    sourceFileOrLinkTarget.withNewPath(fi.dir().relativeFilePath(fi.symLinkTarget()));
            }

            const QByteArray source = ProcessArgs::quoteArgUnix(sourceFileOrLinkTarget.path())
                                          .toLocal8Bit();
            const QByteArray target = ProcessArgs::quoteArgUnix(file.m_target.path()).toLocal8Bit();

            batchData += transferCommand(link) + ' ' + source + ' ' + target + '\n';
            if (file.m_targetPermissions == FilePermissions::ForceExecutable)
                batchData += "chmod 1775 " + target + '\n';
        }
        process().setCommand({sftpBinary, {fullConnectionOptions(), "-b", "-", host()}});
        process().setWriteData(batchData);
        process().start();
    }

    void doneImpl() final { handleDone(); }
};

class RsyncTransferImpl : public SshTransferInterface
{
public:
    RsyncTransferImpl(const FileTransferSetupData &setup, const DeviceConstRef &device)
        : SshTransferInterface(setup, device)
    { }

private:
    void startImpl() final
    {
        // Note: This assumes that files do not get renamed when transferring.
        for (auto it = m_setup.m_files.cbegin(); it != m_setup.m_files.cend(); ++it) {
            const FilePath source = it->m_source;
            if (m_rsync.isEmpty()) { // haven't figured out the rsync tool yet, so do it now
                const IDevice::ConstPtr device = DeviceManager::deviceForPath(source);
                if (QTC_GUARD(device)) {
                    const FilePath rsyncDeviceTool = device->deviceToolPath(
                        ProjectExplorer::Constants::RSYNC_TOOL_ID);
                    m_rsync = rsyncDeviceTool.isEmpty() ? device->filePath("rsync") // fallback
                                                        : rsyncDeviceTool;
                }
            }
            m_batches[it->m_target.parentDir()] << *it;
        }
        if (m_rsync.isEmpty()) // fallback
            m_rsync = "rsync";
        startNextBatch();
    }

    void doneImpl() final
    {
        if (m_batches.isEmpty())
            return handleDone();

        if (handleError())
            return;

        startNextBatch();
    }

    FilePath sshCommand()
    {
        const IDevice::ConstPtr device = DeviceManager::deviceForPath(m_rsync);
        if (device) {
            const FilePath sshDeviceTool = device->deviceToolPath(
                ProjectExplorer::Constants::SSH_TOOL_ID);
            if (!sshDeviceTool.isEmpty())
                return sshDeviceTool;
        }
        if (m_rsync.isLocal())
            return sshSettings().sshFilePath();
        if (device)
            return device->filePath("ssh");
        return "ssh";
    }

    void startNextBatch()
    {
        process().close();

        QStringList options;
        const FilePath ssh = sshCommand();
        if (!ssh.isEmpty()) {
            const QString sshCmdLine = ProcessArgs::joinArgs(
                QStringList{ssh.nativePath()} << fullConnectionOptions(), OsTypeLinux);
            options << QStringList{"-e", sshCmdLine};
        }
        options << ProcessArgs::splitArgs(m_setup.m_rsyncFlags, m_rsync.osType());

        if (!m_batches.isEmpty()) { // NormalRun
            const auto batchIt = m_batches.begin();
            for (auto filesIt = batchIt->cbegin(); filesIt != batchIt->cend(); ++filesIt) {
                const FileToTransfer fixedFile = fixLocalFileOnWindows(*filesIt, options);
                options << fixedFile.m_source.path();
            }
            options << fixedRemotePath(batchIt.key(), userAtHost());
            m_batches.erase(batchIt);
        } else { // TestRun
            options << "-n" << "--exclude=*" << (userAtHost() + ":/tmp");
        }
        process().setCommand(CommandLine(m_rsync, options));
        process().start();
    }

    // On Windows, rsync is either from msys or cygwin. Neither work with the other's ssh.exe.
    FileToTransfer fixLocalFileOnWindows(const FileToTransfer &file, const QStringList &options) const
    {
        if (m_rsync.osType() != OsType::OsTypeWindows)
            return file;

        QString localFilePath = file.m_source.path();
        localFilePath = '/' + localFilePath.at(0) + localFilePath.mid(2);
        if (anyOf(options, [](const QString &opt) {
                return opt.contains("cygwin", Qt::CaseInsensitive); })) {
            localFilePath.prepend("/cygdrive");
        }

        FileToTransfer fixedFile = file;
        fixedFile.m_source = fixedFile.m_source.withNewPath(localFilePath);
        return fixedFile;
    }

    QString fixedRemotePath(const FilePath &file, const QString &remoteHost) const
    {
        return remoteHost + ':' + file.path();
    }

    QHash<FilePath, FilesToTransfer> m_batches;
    FilePath m_rsync;
};

FileTransferInterface *createRemoteLinuxFileTransferInterface
    (const LinuxDevice &device, const FileTransferSetupData &setup)
{
    const bool isLocal = Utils::allOf(setup.m_files, [](const FileToTransfer &f) {
        return f.m_source.isLocal();
    });

    if (setup.m_method == FileTransferMethod::Rsync)
        return new RsyncTransferImpl(setup, device.shared_from_this());
    if (setup.m_method == FileTransferMethod::Sftp && isLocal /* not supported for remote src */)
        return new SftpTransferImpl(setup, device.shared_from_this());
    return createGenericFileTransferInterface(setup);
}

} // namespace Remote::Internal
