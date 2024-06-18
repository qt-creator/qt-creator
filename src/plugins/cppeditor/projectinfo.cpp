// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectinfo.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/toolchain.h>

using namespace Utils;

namespace CppEditor {

ProjectInfo::ConstPtr ProjectInfo::create(const ProjectExplorer::ProjectUpdateInfo &updateInfo,
                                     const QVector<ProjectPart::ConstPtr> &projectParts)
{
    return ConstPtr(new ProjectInfo(updateInfo, projectParts));
}

ProjectInfo::ConstPtr ProjectInfo::cloneWithNewSettings(const ConstPtr &pi,
                                                        const CppCodeModelSettings &settings)
{
    return ConstPtr(new ProjectInfo(pi, settings));
}

ProjectExplorer::Project *ProjectInfo::project() const
{
    return ProjectExplorer::ProjectManager::projectWithProjectFilePath(projectFilePath());
}

bool ProjectInfo::operator ==(const ProjectInfo &other) const
{
    return m_projectName == other.m_projectName
        && m_projectFilePath == other.m_projectFilePath
        && m_buildRoot == other.m_buildRoot
        && m_projectParts == other.m_projectParts
        && m_headerPaths == other.m_headerPaths
        && m_sourceFiles == other.m_sourceFiles
        && m_settings == other.m_settings
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
    return definesChanged(other) || m_headerPaths != other.m_headerPaths
        || m_settings != other.settings();
}

bool ProjectInfo::configurationOrFilesChanged(const ProjectInfo &other) const
{
    return configurationChanged(other) || m_sourceFiles != other.m_sourceFiles;
}

static QSet<FilePath> getSourceFiles(const QVector<ProjectPart::ConstPtr> &projectParts)
{
    QSet<FilePath> sourceFiles;
    for (const ProjectPart::ConstPtr &part : projectParts) {
        for (const ProjectFile &file : std::as_const(part->files))
            sourceFiles.insert(file.path);
    }
    return sourceFiles;
}

static ProjectExplorer::Macros getDefines(const QVector<ProjectPart::ConstPtr> &projectParts)
{
    ProjectExplorer::Macros defines;
    for (const ProjectPart::ConstPtr &part : projectParts) {
        defines.append(part->toolchainMacros);
        defines.append(part->projectMacros);
    }
    return defines;
}

static ProjectExplorer::HeaderPaths getHeaderPaths(
        const QVector<ProjectPart::ConstPtr> &projectParts)
{
    QSet<ProjectExplorer::HeaderPath> uniqueHeaderPaths;
    for (const ProjectPart::ConstPtr &part : projectParts) {
        for (const ProjectExplorer::HeaderPath &headerPath : std::as_const(part->headerPaths))
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
      m_defines(getDefines(projectParts)),
      m_settings(updateInfo.cppSettings)
{
}

ProjectInfo::ProjectInfo(const ConstPtr &pi, const CppCodeModelSettings &settings)
    : m_projectParts(pi->projectParts()),
    m_projectName(pi->projectName()),
    m_projectFilePath(pi->projectFilePath()),
    m_buildRoot(pi->buildRoot()),
    m_headerPaths(pi->m_headerPaths),
    m_sourceFiles(pi->sourceFiles()),
    m_defines(pi->m_defines),
    m_settings(settings)
{
}

} // namespace CppEditor
