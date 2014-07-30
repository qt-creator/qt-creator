/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppprojects.h"

#include <projectexplorer/headerpath.h>

#include <QSet>
#include <QTextStream>

namespace CppTools {

ProjectPart::ProjectPart()
    : project(0)
    , cVersion(C89)
    , cxxVersion(CXX11)
    , cxxExtensions(NoExtensions)
    , qtVersion(UnknownQt)
    , cWarningFlags(ProjectExplorer::ToolChain::WarningsDefault)
    , cxxWarningFlags(ProjectExplorer::ToolChain::WarningsDefault)

{
}

/*!
    \brief Retrieves info from concrete compiler using it's flags.

    \param tc Either nullptr or toolchain for project's active target.
    \param cxxflags C++ or Objective-C++ flags.
    \param cflags C or ObjectiveC flags if possible, \a cxxflags otherwise.
*/
void ProjectPart::evaluateToolchain(const ProjectExplorer::ToolChain *tc,
                                    const QStringList &cxxflags,
                                    const QStringList &cflags,
                                    const Utils::FileName &sysRoot)
{
    if (!tc)
        return;

    using namespace ProjectExplorer;
    ToolChain::CompilerFlags cxx = tc->compilerFlags(cxxflags);
    ToolChain::CompilerFlags c = (cxxflags == cflags)
            ? cxx : tc->compilerFlags(cflags);

    if (c & ToolChain::StandardC11)
        cVersion = C11;
    else if (c & ToolChain::StandardC99)
        cVersion = C99;
    else
        cVersion = C89;

    if (cxx & ToolChain::StandardCxx11)
        cxxVersion = CXX11;
    else
        cxxVersion = CXX98;

    if (cxx & ToolChain::BorlandExtensions)
        cxxExtensions |= BorlandExtensions;
    if (cxx & ToolChain::GnuExtensions)
        cxxExtensions |= GnuExtensions;
    if (cxx & ToolChain::MicrosoftExtensions)
        cxxExtensions |= MicrosoftExtensions;
    if (cxx & ToolChain::OpenMP)
        cxxExtensions |= OpenMPExtensions;

    cWarningFlags = tc->warningFlags(cflags);
    cxxWarningFlags = tc->warningFlags(cxxflags);

    const QList<ProjectExplorer::HeaderPath> headers = tc->systemHeaderPaths(cxxflags, sysRoot);
    foreach (const ProjectExplorer::HeaderPath &header, headers) {
        headerPaths << ProjectPart::HeaderPath(header.path(),
                                header.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath
                                    ? ProjectPart::HeaderPath::FrameworkPath
                                    : ProjectPart::HeaderPath::IncludePath);
    }

    toolchainDefines = tc->predefinedMacros(cxxflags);
}

QByteArray ProjectPart::readProjectConfigFile(const ProjectPart::Ptr &part)
{
    QByteArray result;

    QFile f(part->projectConfigFile);
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream is(&f);
        result = is.readAll().toUtf8();
        f.close();
    }

    return result;
}

ProjectInfo::ProjectInfo()
{}

ProjectInfo::ProjectInfo(QPointer<ProjectExplorer::Project> project)
    : m_project(project)
{}

ProjectInfo::operator bool() const
{
    return !m_project.isNull();
}

bool ProjectInfo::isValid() const
{
    return !m_project.isNull();
}

bool ProjectInfo::isNull() const
{
    return m_project.isNull();
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
    if (!part)
        return;

    m_projectParts.append(part);

    typedef ProjectPart::HeaderPath HeaderPath;

    // Update header paths
    QSet<HeaderPath> incs = QSet<HeaderPath>::fromList(m_headerPaths);
    foreach (const HeaderPath &hp, part->headerPaths) {
        if (!incs.contains(hp)) {
            incs.insert(hp);
            m_headerPaths += hp;
        }
    }

    // Update source files
    QSet<QString> srcs = QSet<QString>::fromList(m_sourceFiles);
    foreach (const ProjectFile &file, part->files)
        srcs.insert(file.path);
    m_sourceFiles = srcs.toList();

    // Update defines
    if (!m_defines.isEmpty())
        m_defines.append('\n');
    m_defines.append(part->toolchainDefines);
    m_defines.append(part->projectDefines);
    if (!part->projectConfigFile.isEmpty()) {
        m_defines.append('\n');
        m_defines += ProjectPart::readProjectConfigFile(part);
        m_defines.append('\n');
    }
}

void ProjectInfo::clearProjectParts()
{
    m_projectParts.clear();
    m_headerPaths.clear();
    m_sourceFiles.clear();
    m_defines.clear();
}

const ProjectPart::HeaderPaths ProjectInfo::headerPaths() const
{
    return m_headerPaths;
}

const QStringList ProjectInfo::sourceFiles() const
{
    return m_sourceFiles;
}

const QByteArray ProjectInfo::defines() const
{
    return m_defines;
}

} // namespace CppTools
