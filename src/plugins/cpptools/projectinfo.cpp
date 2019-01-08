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

#include "cppkitinfo.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>

namespace CppTools {

ToolChainInfo::ToolChainInfo(const ProjectExplorer::ToolChain *toolChain,
                             const QString &sysRootPath)
{
    if (toolChain) {
        // Keep the following cheap/non-blocking for the ui thread...
        type = toolChain->typeId();
        isMsvc2015ToolChain
                = toolChain->targetAbi().osFlavor() == ProjectExplorer::Abi::WindowsMsvc2015Flavor;
        wordWidth = toolChain->targetAbi().wordWidth();
        targetTriple = toolChain->originalTargetTriple();
        extraCodeModelFlags = toolChain->extraCodeModelFlags();

        // ...and save the potentially expensive operations for later so that
        // they can be run from a worker thread.
        this->sysRootPath = sysRootPath;
        headerPathsRunner = toolChain->createBuiltInHeaderPathsRunner();
        macroInspectionRunner = toolChain->createMacroInspectionRunner();
    }
}

ProjectUpdateInfo::ProjectUpdateInfo(ProjectExplorer::Project *project,
                                     const KitInfo &kitInfo,
                                     const RawProjectParts &rawProjectParts)
    : project(project)
    , rawProjectParts(rawProjectParts)
    , cToolChain(kitInfo.cToolChain)
    , cxxToolChain(kitInfo.cxxToolChain)
    , cToolChainInfo(ToolChainInfo(cToolChain, kitInfo.sysRootPath))
    , cxxToolChainInfo(ToolChainInfo(cxxToolChain, kitInfo.sysRootPath))
{
}

ProjectInfo::ProjectInfo(QPointer<ProjectExplorer::Project> project)
    : m_project(project)
{
}

bool ProjectInfo::isValid() const
{
    return !m_project.isNull();
}

QPointer<ProjectExplorer::Project> ProjectInfo::project() const
{
    return m_project;
}

const QVector<ProjectPart::Ptr> ProjectInfo::projectParts() const
{
    return m_projectParts;
}

const QSet<QString> ProjectInfo::sourceFiles() const
{
    return m_sourceFiles;
}

bool ProjectInfo::operator ==(const ProjectInfo &other) const
{
    return m_project == other.m_project
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

void ProjectInfo::appendProjectPart(const ProjectPart::Ptr &projectPart)
{
    if (projectPart)
        m_projectParts.append(projectPart);
}

void ProjectInfo::finish()
{
    QSet<ProjectExplorer::HeaderPath> uniqueHeaderPaths;

    foreach (const ProjectPart::Ptr &part, m_projectParts) {
        // Update header paths
        foreach (const ProjectExplorer::HeaderPath &headerPath, part->headerPaths) {
            const int count = uniqueHeaderPaths.count();
            uniqueHeaderPaths.insert(headerPath);
            if (count < uniqueHeaderPaths.count())
                m_headerPaths += headerPath;
        }

        // Update source files
        foreach (const ProjectFile &file, part->files)
            m_sourceFiles.insert(file.path);

        // Update defines
        m_defines.append(part->toolChainMacros);
        m_defines.append(part->projectMacros);
        if (!part->projectConfigFile.isEmpty())
            m_defines += ProjectExplorer::Macro::toMacros(ProjectPart::readProjectConfigFile(part));
    }
}

} // namespace CppTools
