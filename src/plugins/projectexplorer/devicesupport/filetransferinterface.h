// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <utils/filepath.h>

namespace Utils { class ProcessResultData; }

namespace ProjectExplorer {

enum class FileTransferMethod {
    Sftp,
    Rsync,
    GenericCopy,
    Default = Sftp
};

class PROJECTEXPLORER_EXPORT FileToTransfer
{
public:
    Utils::FilePath m_source;
    Utils::FilePath m_target;
};

using FilesToTransfer = QList<FileToTransfer>;

class PROJECTEXPLORER_EXPORT FileTransferSetupData
{
public:
    FilesToTransfer m_files; // When empty, do test instead of a real transfer
    FileTransferMethod m_method = FileTransferMethod::Default;
    QString m_rsyncFlags = defaultRsyncFlags();

    static QString defaultRsyncFlags();
};

class PROJECTEXPLORER_EXPORT FileTransferInterface : public QObject
{
    Q_OBJECT

signals:
    void progress(const QString &progressMessage);
    void done(const Utils::ProcessResultData &resultData);

protected:
    FileTransferInterface(const FileTransferSetupData &setupData)
        : m_setup(setupData) {}

    void startFailed(const QString &errorString);

    const FileTransferSetupData m_setup;

private:
    FileTransferInterface() = delete;

    virtual void start() = 0;

    friend class FileTransferPrivate;
};

} // namespace ProjectExplorer
