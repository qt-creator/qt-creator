// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmdbridgeglobal.h"

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

class tst_CmdBridge;

namespace CmdBridge {

class Client;

class QTCREATOR_CMDBRIDGE_EXPORT FileAccess : public Utils::DeviceFileAccess
{
    friend class ::tst_CmdBridge;
    friend class GoFilePathWatcher;

public:
    ~FileAccess() override;

    Utils::Result<> deployAndInit(
        const Utils::FilePath &libExecPath,
        const Utils::FilePath &remoteRootPath,
        const Utils::Environment &environment);

    Utils::Result<> init(
        const Utils::FilePath &pathToBridge,
        const Utils::Environment &environment,
        bool deleteOnExit);

    Utils::Result<> signalProcess(int pid, Utils::ControlSignal signal) const;

    Utils::Result<Utils::Environment> deviceEnvironment() const override;

protected:
    Utils::Result<> reinit();

    Utils::Result<> iterateDirectory(const Utils::FilePath &filePath,
                          const Utils::FilePath::IterateDirCallback &callBack,
                          const Utils::FileFilter &filter) const override;

    Utils::Result<bool> isExecutableFile(const Utils::FilePath &filePath) const override;
    Utils::Result<bool> isReadableFile(const Utils::FilePath &filePath) const override;
    Utils::Result<bool> isWritableFile(const Utils::FilePath &filePath) const override;
    Utils::Result<bool> isReadableDirectory(const Utils::FilePath &filePath) const override;
    Utils::Result<bool> isWritableDirectory(const Utils::FilePath &filePath) const override;
    Utils::Result<bool> isFile(const Utils::FilePath &filePath) const override;
    Utils::Result<bool> isDirectory(const Utils::FilePath &filePath) const override;
    Utils::Result<bool> isSymLink(const Utils::FilePath &filePath) const override;
    Utils::Result<bool> exists(const Utils::FilePath &filePath) const override;

    Utils::Result<bool> hasHardLinks(const Utils::FilePath &filePath) const override;
    Utils::Result<Utils::FilePathInfo> filePathInfo(const Utils::FilePath &filePath) const override;
    Utils::Result<Utils::FilePath> symLinkTarget(const Utils::FilePath &filePath) const override;
    Utils::Result<QDateTime> lastModified(const Utils::FilePath &filePath) const override;
    Utils::Result<QFile::Permissions> permissions(const Utils::FilePath &filePath) const override;
    Utils::Result<> setPermissions(const Utils::FilePath &filePath, QFile::Permissions) const override;
    Utils::Result<qint64> fileSize(const Utils::FilePath &filePath) const override;
    Utils::Result<QString> owner(const Utils::FilePath &filePath) const override;
    Utils::Result<uint> ownerId(const Utils::FilePath &filePath) const override;
    Utils::Result<QString> group(const Utils::FilePath &filePath) const override;
    Utils::Result<uint> groupId(const Utils::FilePath &filePath) const override;
    Utils::Result<qint64> bytesAvailable(const Utils::FilePath &filePath) const override;
    Utils::Result<QByteArray> fileId(const Utils::FilePath &filePath) const override;

    Utils::Result<QByteArray> fileContents(const Utils::FilePath &filePath,
                                                 qint64 limit,
                                                 qint64 offset) const override;
    Utils::Result<qint64> writeFileContents(const Utils::FilePath &filePath,
                                                  const QByteArray &data) const override;

    Utils::Result<> removeFile(const Utils::FilePath &filePath) const override;
    Utils::Result<> removeRecursively(const Utils::FilePath &filePath) const override;

    Utils::Result<> ensureExistingFile(const Utils::FilePath &filePath) const override;
    Utils::Result<> createDirectory(const Utils::FilePath &filePath) const override;

    Utils::Result<> copyFile(const Utils::FilePath &filePath,
                           const Utils::FilePath &target) const override;
    Utils::Result<> createSymLink(
        const Utils::FilePath &filePath, const Utils::FilePath &symLink) const override;

    Utils::Result<> renameFile(
        const Utils::FilePath &filePath, const Utils::FilePath &target) const override;

    Utils::Result<Utils::FilePath> createTempFile(const Utils::FilePath &filePath) override;
    Utils::Result<Utils::FilePath> createTempDir(const Utils::FilePath &filePath) override;
    Utils::Result<Utils::FilePath> createTemp(const Utils::FilePath &filePath, bool dir);

    Utils::Result<std::unique_ptr<Utils::FilePathWatcher>> watch(
        const Utils::FilePath &filePath) const override;

private:
    std::unique_ptr<CmdBridge::Client> m_client;
    Utils::Environment m_environment;
};

} // namespace CmdBridge
