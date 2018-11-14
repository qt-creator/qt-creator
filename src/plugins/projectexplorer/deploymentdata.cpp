/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "deploymentdata.h"

#include <utils/algorithm.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace ProjectExplorer {

void DeploymentData::setLocalInstallRoot(const Utils::FileName &installRoot)
{
    m_localInstallRoot = installRoot;
}

void DeploymentData::addFile(const DeployableFile &file)
{
    for (int i = 0; i < m_files.size(); ++i) {
        if (m_files.at(i).localFilePath() == file.localFilePath()) {
            m_files[i] = file;
            return;
        }
    }
    m_files << file;
}

void DeploymentData::addFile(const QString &localFilePath, const QString &remoteDirectory,
                             DeployableFile::Type type)
{
    addFile(DeployableFile(localFilePath, remoteDirectory, type));
}

DeployableFile DeploymentData::deployableForLocalFile(const QString &localFilePath) const
{
    return Utils::findOrDefault(m_files, [&localFilePath](const DeployableFile &d) {
        return d.localFilePath().toString() == localFilePath;
    });
}

bool DeploymentData::operator==(const DeploymentData &other) const
{
    return m_files.toSet() == other.m_files.toSet()
            && m_localInstallRoot == other.m_localInstallRoot;
}

QString DeploymentData::addFilesFromDeploymentFile(const QString &deploymentFilePath,
                                                   const QString &sourceDir)
{
    const QString sourcePrefix = sourceDir.endsWith('/') ? sourceDir : sourceDir + '/';
    QFile deploymentFile(deploymentFilePath);
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
            QStringList file = line.split(':');
            QString sourceFile = file.at(0);
            if (QFileInfo(sourceFile).isRelative())
                sourceFile.prepend(sourcePrefix);
            QString targetFile = file.at(1);
            if (QFileInfo(targetFile).isRelative())
                targetFile.prepend(deploymentPrefix);
            addFile(sourceFile, targetFile);
        }
    }
    return deploymentPrefix;
}

} // namespace ProjectExplorer
