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

#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/processinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>
#include <utils/threadutils.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

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
            if (dirs.contains(parentDir) || QDir(parentDir.path()).isRoot())
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
        if (resultData.m_error == QProcess::FailedToStart) {
            resultData.m_errorString = Tr::tr("\"%1\" failed to start: %2")
                    .arg(FileTransfer::transferMethodName(m_setup.m_method), resultData.m_errorString);
        } else if (resultData.m_exitStatus != QProcess::NormalExit) {
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
        QStringList options = m_sshParameters.connectionOptions(SshSettings::sshFilePath());
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
        const Id linkDeviceId = Id::fromSetting(m_device.extraData(Constants::LinkDevice));
        const auto linkDevice = DeviceManager::instance()->find(linkDeviceId);
        const bool useConnectionSharing = !linkDevice && SshSettings::connectionSharingEnabled();

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
            resultData.m_error = QProcess::FailedToStart;

        m_connecting = false;
        if (m_connectionHandle) // TODO: should it disconnect from signals first?
            m_connectionHandle.release()->deleteLater();

        if (resultData.m_error != QProcess::UnknownError || m_process.state() != QProcess::NotRunning)
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
        FilePath sftpBinary = SshSettings::sftpFilePath();

        // This is a hack. We only test the last hop here.
        const Id linkDeviceId = Id::fromSetting(device().extraData(Constants::LinkDevice));
        if (const auto linkDevice = DeviceManager::instance()->find(linkDeviceId))
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
        for (auto it = m_setup.m_files.cbegin(); it != m_setup.m_files.cend(); ++it)
            m_batches[it->m_target.parentDir()] << *it;
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

    void startNextBatch()
    {
        process().close();

        const QString sshCmdLine = ProcessArgs::joinArgs(
                    QStringList{SshSettings::sshFilePath().toUserOutput()}
                    << fullConnectionOptions(), OsTypeLinux);
        QStringList options{"-e", sshCmdLine};
        options << ProcessArgs::splitArgs(m_setup.m_rsyncFlags, HostOsInfo::hostOs());

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
        // TODO: Get rsync location from settings?
        process().setCommand(CommandLine("rsync", options));
        process().start();
    }

    // On Windows, rsync is either from msys or cygwin. Neither work with the other's ssh.exe.
    FileToTransfer fixLocalFileOnWindows(const FileToTransfer &file, const QStringList &options) const
    {
        if (!HostOsInfo::isWindowsHost())
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
};

static void createDir(QPromise<Result<>> &promise, const FilePath &pathToCreate)
{
    const Result<> result = pathToCreate.ensureWritableDir();
    promise.addResult(result);

    if (!result)
        promise.future().cancel();
};

static void copyFile(QPromise<Result<>> &promise, const FileToTransfer &file)
{
    const Result<> result = file.m_source.copyFile(file.m_target);
    promise.addResult(result);

    if (!result)
        promise.future().cancel();
};

class GenericTransferImpl : public FileTransferInterface
{
    Tasking::TaskTreeRunner m_taskTree;

public:
    GenericTransferImpl(const FileTransferSetupData &setup)
        : FileTransferInterface(setup)
    {}

private:
    void start() final
    {
        using namespace Tasking;

        const QSet<FilePath> allParentDirs
            = Utils::transform<QSet>(m_setup.m_files, [](const FileToTransfer &f) {
                  return f.m_target.parentDir();
              });

        const LoopList iteratorParentDirs(QList(allParentDirs.cbegin(), allParentDirs.cend()));

        const auto onCreateDirSetup = [iteratorParentDirs](Async<Result<>> &async) {
            async.setConcurrentCallData(createDir, *iteratorParentDirs);
        };

        const auto onCreateDirDone = [this,
                                      iteratorParentDirs](const Async<Result<>> &async) {
            const Result<> result = async.result();
            if (result)
                emit progress(
                    Tr::tr("Created directory: \"%1\".\n").arg(iteratorParentDirs->toUserOutput()));
            else
                emit progress(result.error());
        };

        const LoopList iterator(m_setup.m_files);
        const Storage<int> counterStorage;

        const auto onCopySetup = [iterator](Async<Result<>> &async) {
            async.setConcurrentCallData(copyFile, *iterator);
        };

        const auto onCopyDone = [this, iterator, counterStorage](
                                    const Async<Result<>> &async) {
            const Result<> result = async.result();
            int &counter = *counterStorage;
            ++counter;

            if (result) {
                //: %1/%2 = progress in the form 4/15, %3 and %4 = source and target file paths
                emit progress(Tr::tr("Copied %1/%2: \"%3\" -> \"%4\".\n")
                                  .arg(counter)
                                  .arg(m_setup.m_files.size())
                                  .arg(iterator->m_source.toUserOutput())
                                  .arg(iterator->m_target.toUserOutput()));
            } else {
                emit progress(result.error() + "\n");
            }
        };

        const Group recipe {
            For (iteratorParentDirs) >> Do {
                parallelIdealThreadCountLimit,
                AsyncTask<Result<>>(onCreateDirSetup, onCreateDirDone),
            },
            For (iterator) >> Do {
                parallelLimit(2),
                counterStorage,
                AsyncTask<Result<>>(onCopySetup, onCopyDone),
            },
        };

        m_taskTree.start(recipe, {}, [this](DoneWith result) {
            ProcessResultData resultData;
            if (result != DoneWith::Success) {
                resultData.m_exitCode = -1;
                resultData.m_errorString = Tr::tr("Failed to deploy files.");
            }
            emit done(resultData);
        });
    }
};

FileTransferInterface *createRemoteLinuxFileTransferInterface
    (const LinuxDevice &device, const FileTransferSetupData &setup)
{
    if (Utils::anyOf(setup.m_files,
                     [](const FileToTransfer &f) { return !f.m_source.isLocal(); })) {
        return new GenericTransferImpl(setup);
    }

    switch (setup.m_method) {
    case FileTransferMethod::Sftp:  return new SftpTransferImpl(setup, device.shared_from_this());
    case FileTransferMethod::Rsync: return new RsyncTransferImpl(setup, device.shared_from_this());
    case FileTransferMethod::GenericCopy: return new GenericTransferImpl(setup);
    }
    QTC_CHECK(false);
    return {};
}

} // namespace RemoteLinux::Internal
