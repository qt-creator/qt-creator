/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectinfo.h"

#include <projectexplorer/project.h>

namespace CppTools {

ProjectInfo::ProjectInfo()
{}

ProjectInfo::ProjectInfo(QPointer<ProjectExplorer::Project> project)
    : m_project(project)
{}

bool ProjectInfo::operator ==(const ProjectInfo &other) const
{
    return m_project == other.m_project
            && m_projectParts == other.m_projectParts
            && m_compilerCallData == other.m_compilerCallData
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

bool ProjectInfo::isValid() const
{
    return !m_project.isNull();
}

QPointer<ProjectExplorer::Project> ProjectInfo::project() const
{
    return m_project;
}

const QList<ProjectPart::Ptr> ProjectInfo::projectParts() const
{
    return m_projectParts;
}

void ProjectInfo::appendProjectPart(const ProjectPart::Ptr &part)
{
    if (part)
        m_projectParts.append(part);
}

void ProjectInfo::finish()
{
    typedef ProjectPartHeaderPath HeaderPath;

    QSet<HeaderPath> incs;
    foreach (const ProjectPart::Ptr &part, m_projectParts) {
        part->updateLanguageFeatures();
        // Update header paths
        foreach (const HeaderPath &hp, part->headerPaths) {
            if (!incs.contains(hp)) {
                incs.insert(hp);
                m_headerPaths += hp;
            }
        }

        // Update source files
        foreach (const ProjectFile &file, part->files)
            m_sourceFiles.insert(file.path);

        // Update defines
        m_defines.append(part->toolchainDefines);
        m_defines.append(part->projectDefines);
        if (!part->projectConfigFile.isEmpty()) {
            m_defines.append('\n');
            m_defines += ProjectPart::readProjectConfigFile(part);
            m_defines.append('\n');
        }
    }
}

const ProjectPartHeaderPaths ProjectInfo::headerPaths() const
{
    return m_headerPaths;
}

const QSet<QString> ProjectInfo::sourceFiles() const
{
    return m_sourceFiles;
}

const QByteArray ProjectInfo::defines() const
{
    return m_defines;
}

void ProjectInfo::setCompilerCallData(const CompilerCallData &data)
{
    m_compilerCallData = data;
}

ProjectInfo::CompilerCallData ProjectInfo::compilerCallData() const
{
    return m_compilerCallData;
}

} // namespace CppTools
