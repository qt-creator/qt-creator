/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "deployablefile.h"
#include "projectexplorer_export.h"

#include <utils/environment.h>

#include <QList>

namespace ProjectExplorer {

enum class DeploymentKnowledge { Perfect, Approximative, Bad };

class PROJECTEXPLORER_EXPORT MakeInstallCommand
{
public:
    Utils::FilePath command;
    QStringList arguments;
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
    void addFile(const QString &localFilePath, const QString &remoteDirectory,
                 DeployableFile::Type type = DeployableFile::TypeNormal);
    QString addFilesFromDeploymentFile(const QString &deploymentFilePath, const QString &sourceDir);

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
