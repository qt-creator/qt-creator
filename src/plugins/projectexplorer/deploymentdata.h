// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "deployablefile.h"
#include "projectexplorer_export.h"

#include <utils/commandline.h>
#include <utils/environment.h>

#include <QList>

namespace ProjectExplorer {

enum class DeploymentKnowledge { Perfect, Approximative, Bad };

class PROJECTEXPLORER_EXPORT MakeInstallCommand
{
public:
    Utils::CommandLine command;
    Utils::Environment environment;
};

class PROJECTEXPLORER_EXPORT DeploymentData
{
public:
    void setFileList(const QList<DeployableFile> &files) { m_files = files; }
    QList<DeployableFile> allFiles() const { return m_files; }

    void setLocalInstallRoot(const Utils::FilePath &installRoot);
    Utils::FilePath localInstallRoot() const { return m_localInstallRoot; }

    void addFile(const DeployableFile &file);
    void addFile(const Utils::FilePath &localFilePath, const QString &remoteDirectory,
                 DeployableFile::Type type = DeployableFile::TypeNormal);
    QString addFilesFromDeploymentFile(const Utils::FilePath &deploymentFilePath, const Utils::FilePath &sourceDir);

    int fileCount() const { return m_files.count(); }
    DeployableFile fileAt(int index) const { return m_files.at(index); }
    DeployableFile deployableForLocalFile(const Utils::FilePath &localFilePath) const;

    bool operator==(const DeploymentData &other) const;

private:
    QList<DeployableFile> m_files;
    Utils::FilePath m_localInstallRoot;
};

inline bool operator!=(const DeploymentData &d1, const DeploymentData &d2) { return !(d1 == d2); }

} // namespace ProjectExplorer
