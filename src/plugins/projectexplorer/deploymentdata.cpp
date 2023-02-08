// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deploymentdata.h"

#include <utils/algorithm.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

using namespace Utils;

namespace ProjectExplorer {

void DeploymentData::setLocalInstallRoot(const Utils::FilePath &installRoot)
{
    m_localInstallRoot = installRoot;
}

void DeploymentData::addFile(const DeployableFile &file)
{
    m_files << file;
}

void DeploymentData::addFile(const FilePath &localFilePath, const QString &remoteDirectory,
                             DeployableFile::Type type)
{
    addFile(DeployableFile(localFilePath, remoteDirectory, type));
}

DeployableFile DeploymentData::deployableForLocalFile(const Utils::FilePath &localFilePath) const
{
    const DeployableFile f =  Utils::findOrDefault(m_files,
                                                   Utils::equal(&DeployableFile::localFilePath,
                                                                localFilePath));
    if (f.isValid())
        return f;
    const QString localFileName = localFilePath.fileName();
    return Utils::findOrDefault(m_files, [&localFileName](const DeployableFile &d) {
        return d.localFilePath().fileName() == localFileName;
    });
}

bool DeploymentData::operator==(const DeploymentData &other) const
{
    return Utils::toSet(m_files) == Utils::toSet(other.m_files)
            && m_localInstallRoot == other.m_localInstallRoot;
}

QString DeploymentData::addFilesFromDeploymentFile(const FilePath &deploymentFilePath,
                                                   const FilePath &sourceDir_)
{
    const QString sourceDir = sourceDir_.toString();
    const QString sourcePrefix = sourceDir.endsWith('/') ? sourceDir : sourceDir + '/';
    QFile deploymentFile(deploymentFilePath.toString());
    QTextStream deploymentStream;
    QString deploymentPrefix;

    if (!deploymentFile.open(QFile::ReadOnly | QFile::Text))
        return deploymentPrefix;
    deploymentStream.setDevice(&deploymentFile);
    deploymentPrefix = deploymentStream.readLine();
    if (!deploymentPrefix.endsWith('/'))
        deploymentPrefix.append('/');
    if (deploymentStream.device()) {
        while (!deploymentStream.atEnd()) {
            QString line = deploymentStream.readLine();
            if (!line.contains(':'))
                continue;
            int splitPoint = line.lastIndexOf(':');
            QString sourceFile = line.left(splitPoint);
            if (QFileInfo(sourceFile).isRelative())
                sourceFile.prepend(sourcePrefix);
            QString targetFile = line.mid(splitPoint + 1);
            if (QFileInfo(targetFile).isRelative())
                targetFile.prepend(deploymentPrefix);
            addFile(FilePath::fromString(sourceFile), targetFile);
        }
    }
    return deploymentPrefix;
}

} // namespace ProjectExplorer
