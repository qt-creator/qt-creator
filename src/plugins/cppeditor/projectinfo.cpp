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

#include "projectinfo.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/toolchain.h>

namespace CppEditor {

ProjectInfo::ConstPtr ProjectInfo::create(const ProjectExplorer::ProjectUpdateInfo &updateInfo,
                                     const QVector<ProjectPart::ConstPtr> &projectParts)
{
    return ConstPtr(new ProjectInfo(updateInfo, projectParts));
}

const QVector<ProjectPart::ConstPtr> ProjectInfo::projectParts() const
{
    return m_projectParts;
}

const QSet<QString> ProjectInfo::sourceFiles() const
{
    return m_sourceFiles;
}

bool ProjectInfo::operator ==(const ProjectInfo &other) const
{
    return m_projectName == other.m_projectName
        && m_projectFilePath == other.m_projectFilePath
        && m_buildRoot == other.m_buildRoot
        && m_projectParts == other.m_projectParts
        && m_headerPaths == other.m_headerPaths
        && m_sourceFiles == other.m_sourceFiles
        && m_defines == other.m_defines;
}

bool ProjectInfo::operator !=(const ProjectInfo &other) const
{
    return !operator ==(other);
}

bool ProjectInfo::definesChanged(const ProjectInfo &other) const
{
    return m_defines != other.m_defines;
}

bool ProjectInfo::configurationChanged(const ProjectInfo &other) const
{
    return definesChanged(other) || m_headerPaths != other.m_headerPaths;
}

bool ProjectInfo::configurationOrFilesChanged(const ProjectInfo &other) const
{
    return configurationChanged(other) || m_sourceFiles != other.m_sourceFiles;
}

static QSet<QString> getSourceFiles(const QVector<ProjectPart::ConstPtr> &projectParts)
{
    QSet<QString> sourceFiles;
    for (const ProjectPart::ConstPtr &part : projectParts) {
        for (const ProjectFile &file : qAsConst(part->files))
            sourceFiles.insert(file.path);
    }
    return sourceFiles;
}

static ProjectExplorer::Macros getDefines(const QVector<ProjectPart::ConstPtr> &projectParts)
{
    ProjectExplorer::Macros defines;
    for (const ProjectPart::ConstPtr &part : projectParts) {
        defines.append(part->toolChainMacros);
        defines.append(part->projectMacros);
    }
    return defines;
}

static ProjectExplorer::HeaderPaths getHeaderPaths(
        const QVector<ProjectPart::ConstPtr> &projectParts)
{
    QSet<ProjectExplorer::HeaderPath> uniqueHeaderPaths;
    for (const ProjectPart::ConstPtr &part : projectParts) {
        for (const ProjectExplorer::HeaderPath &headerPath : qAsConst(part->headerPaths))
            uniqueHeaderPaths.insert(headerPath);
    }
    return ProjectExplorer::HeaderPaths(uniqueHeaderPaths.cbegin(), uniqueHeaderPaths.cend());
}

ProjectInfo::ProjectInfo(const ProjectExplorer::ProjectUpdateInfo &updateInfo,
                         const QVector<ProjectPart::ConstPtr> &projectParts)
    : m_projectParts(projectParts),
      m_projectName(updateInfo.projectName),
      m_projectFilePath(updateInfo.projectFilePath),
      m_buildRoot(updateInfo.buildRoot),
      m_headerPaths(getHeaderPaths(projectParts)),
      m_sourceFiles(getSourceFiles(projectParts)),
      m_defines(getDefines(projectParts))
{
}

} // namespace CppEditor
