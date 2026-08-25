// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deploymentdata.h"

#include <utils/algorithm.h>
#include <utils/result.h>

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
                                                   const FilePath &sourceDir)
{
    const Result<QByteArray> contents = deploymentFilePath.fileContents();
    if (!contents)
        return {};

    QByteArray data = *contents;
    QTextStream deploymentStream(&data, QIODevice::ReadOnly);
    QString deploymentPrefix = deploymentStream.readLine();
    if (!deploymentPrefix.endsWith('/'))
        deploymentPrefix.append('/');

    while (!deploymentStream.atEnd()) {
        const QString line = deploymentStream.readLine();
        const int splitPoint = line.lastIndexOf(':');
        if (splitPoint < 0)
            continue;

        QString targetFile = line.mid(splitPoint + 1);
        if (FilePath::fromUserInput(targetFile).isRelativePath())
            targetFile.prepend(deploymentPrefix);

        addFile(sourceDir.resolvePath(line.left(splitPoint)), targetFile);
    }
    return deploymentPrefix;
}

} // namespace ProjectExplorer
