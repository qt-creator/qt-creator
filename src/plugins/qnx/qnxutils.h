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
    QnxTarget(const Utils::FilePath &path, const ProjectExplorer::Abi &abi);

    QString shortDescription() const;
    QString cpuDir() const { return m_path.fileName(); }

    Utils::FilePath m_path;
    ProjectExplorer::Abi m_abi;
    Utils::FilePath m_debuggerPath;
};

namespace QnxUtils {
QString cpuDirFromAbi(const ProjectExplorer::Abi &abi);
QString cpuDirShortDescription(const QString &cpuDir);
Utils::EnvironmentItems qnxEnvironmentFromEnvFile(const Utils::FilePath &filePath);
Utils::EnvironmentItems qnxEnvironment(const Utils::FilePath &sdpPath);
QList<QnxTarget> findTargets(const Utils::FilePath &basePath);
ProjectExplorer::Abi convertAbi(const ProjectExplorer::Abi &abi);
ProjectExplorer::Abis convertAbis(const ProjectExplorer::Abis &abis);
}

} // Qnx::Internal
