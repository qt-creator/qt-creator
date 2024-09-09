// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmdbridgeglobal.h"

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

#include <QFuture>

class tst_CmdBridge;

namespace CmdBridge {

class Client;

class QTCREATOR_CMDBRIDGE_EXPORT FileAccess : public Utils::DeviceFileAccess
{
    friend class ::tst_CmdBridge;
    friend class GoFilePathWatcher;

public:
    ~FileAccess() override;

    Utils::expected_str<void> deployAndInit(
        const Utils::FilePath &libExecPath, const Utils::FilePath &remoteRootPath);

    Utils::expected_str<void> init(const Utils::FilePath &pathToBridge);

    Utils::expected_str<void> signalProcess(int pid, Utils::ControlSignal signal) const;

    Utils::Environment deviceEnvironment() const override;

protected:
    Utils::expected_str<void> reinit();

    void iterateDirectory(const Utils::FilePath &filePath,
                          const Utils::FilePath::IterateDirCallback &callBack,
                          const Utils::FileFilter &filter) const override;

    bool isExecutableFile(const Utils::FilePath &filePath) const override;
    bool isReadableFile(const Utils::FilePath &filePath) const override;
    bool isWritableFile(const Utils::FilePath &filePath) const override;
    bool isReadableDirectory(const Utils::FilePath &filePath) const override;
    bool isWritableDirectory(const Utils::FilePath &filePath) const override;
    bool isFile(const Utils::FilePath &filePath) const override;
    bool isDirectory(const Utils::FilePath &filePath) const override;
    bool isSymLink(const Utils::FilePath &filePath) const override;
    bool exists(const Utils::FilePath &filePath) const override;

    bool hasHardLinks(const Utils::FilePath &filePath) const override;
    Utils::FilePathInfo filePathInfo(const Utils::FilePath &filePath) const override;
    Utils::FilePath symLinkTarget(const Utils::FilePath &filePath) const override;
    QDateTime lastModified(const Utils::FilePath &filePath) const override;
    QFile::Permissions permissions(const Utils::FilePath &filePath) const override;
    bool setPermissions(const Utils::FilePath &filePath, QFile::Permissions) const override;
    qint64 fileSize(const Utils::FilePath &filePath) const override;
    qint64 bytesAvailable(const Utils::FilePath &filePath) const override;
    QByteArray fileId(const Utils::FilePath &filePath) const override;

    Utils::expected_str<QByteArray> fileContents(const Utils::FilePath &filePath,
                                                 qint64 limit,
                                                 qint64 offset) const override;
    Utils::expected_str<qint64> writeFileContents(const Utils::FilePath &filePath,
                                                  const QByteArray &data) const override;

    Utils::expected_str<void> removeFile(const Utils::FilePath &filePath) const override;
    bool removeRecursively(const Utils::FilePath &filePath, QString *error) const override;

    bool ensureExistingFile(const Utils::FilePath &filePath) const override;
    bool createDirectory(const Utils::FilePath &filePath) const override;

    Utils::expected_str<void> copyFile(const Utils::FilePath &filePath,
                                       const Utils::FilePath &target) const override;

    bool renameFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const override;

    Utils::expected_str<Utils::FilePath> createTempFile(const Utils::FilePath &filePath) override;

    Utils::expected_str<std::unique_ptr<Utils::FilePathWatcher>> watch(
        const Utils::FilePath &filePath) const override;

private:
    std::unique_ptr<CmdBridge::Client> m_client;
    Utils::Environment m_environment;
};

} // namespace CmdBridge
