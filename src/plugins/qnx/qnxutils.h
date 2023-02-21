// Copyright (C) 2016 BlackBerry Limited. All rights reserved
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abi.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/filepath.h>

namespace Qnx::Internal {

class QnxTarget
{
public:
    QnxTarget(const Utils::FilePath &path, const ProjectExplorer::Abi &abi) :
        m_path(path), m_abi(abi)
    {
    }
    Utils::FilePath m_path;
    ProjectExplorer::Abi m_abi;
};

class QnxUtils
{
public:
    static QString cpuDirFromAbi(const ProjectExplorer::Abi &abi);
    static QString cpuDirShortDescription(const QString &cpuDir);
    static Utils::EnvironmentItems qnxEnvironmentFromEnvFile(const Utils::FilePath &filePath);
    static Utils::FilePath envFilePath(const Utils::FilePath &sdpPath);
    static Utils::EnvironmentItems qnxEnvironment(const Utils::FilePath &sdpPath);
    static QList<QnxTarget> findTargets(const Utils::FilePath &basePath);
    static ProjectExplorer::Abi convertAbi(const ProjectExplorer::Abi &abi);
    static ProjectExplorer::Abis convertAbis(const ProjectExplorer::Abis &abis);
};

} // Qnx::Internal
