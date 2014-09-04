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
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <QSet>
#include <QTextStream>

using namespace CppTools;
using namespace ProjectExplorer;

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

ProjectPart::Ptr ProjectPart::copy() const
{
    return Ptr(new ProjectPart(*this));
}

QString ProjectPart::id() const
{
    return QDir::fromNativeSeparators(projectFile) + QLatin1Char(' ') + displayName;
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
    foreach (const ProjectFile &file, part->files)
        m_sourceFiles.insert(file.path);

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

const QSet<QString> ProjectInfo::sourceFiles() const
{
    return m_sourceFiles;
}

const QByteArray ProjectInfo::defines() const
{
    return m_defines;
}

namespace {
class ProjectFileCategorizer
{
public:
    ProjectFileCategorizer(const QString &partName, const QStringList &files)
        : m_partName(partName)
    {
        using CppTools::ProjectFile;

        QStringList cHeaders, cxxHeaders;

        foreach (const QString &file, files) {
            switch (ProjectFile::classify(file)) {
            case ProjectFile::CSource: m_cSources += file; break;
            case ProjectFile::CHeader: cHeaders += file; break;
            case ProjectFile::CXXSource: m_cxxSources += file; break;
            case ProjectFile::CXXHeader: cxxHeaders += file; break;
            case ProjectFile::ObjCSource: m_objcSources += file; break;
            case ProjectFile::ObjCXXSource: m_objcxxSources += file; break;
            default:
                continue;
            }
        }

        const bool hasC = !m_cSources.isEmpty();
        const bool hasCxx = !m_cxxSources.isEmpty();
        const bool hasObjc = !m_objcSources.isEmpty();
        const bool hasObjcxx = !m_objcxxSources.isEmpty();

        if (hasObjcxx)
            m_objcxxSources += cxxHeaders + cHeaders;
        if (hasCxx)
            m_cxxSources += cxxHeaders + cHeaders;
        else if (!hasObjcxx)
            m_cxxSources += cxxHeaders;
        if (hasObjc)
            m_objcSources += cHeaders;
        if (hasC || (!hasObjc && !hasObjcxx && !hasCxx))
            m_cSources += cHeaders;

        m_partCount =
                (m_cSources.isEmpty() ? 0 : 1) +
                (m_cxxSources.isEmpty() ? 0 : 1) +
                (m_objcSources.isEmpty() ? 0 : 1) +
                (m_objcxxSources.isEmpty() ? 0 : 1);
    }

    bool hasCSources() const { return !m_cSources.isEmpty(); }
    bool hasCxxSources() const { return !m_cxxSources.isEmpty(); }
    bool hasObjcSources() const { return !m_objcSources.isEmpty(); }
    bool hasObjcxxSources() const { return !m_objcxxSources.isEmpty(); }

    QStringList cSources() const { return m_cSources; }
    QStringList cxxSources() const { return m_cxxSources; }
    QStringList objcSources() const { return m_objcSources; }
    QStringList objcxxSources() const { return m_objcxxSources; }

    bool hasMultipleParts() const { return m_partCount > 1; }
    bool hasNoParts() const { return m_partCount == 0; }

    QString partName(const QString &languageName) const
    {
        if (hasMultipleParts())
            return QString::fromLatin1("%1 (%2)").arg(m_partName).arg(languageName);

        return m_partName;
    }

private:
    QString m_partName;
    QStringList m_cSources, m_cxxSources, m_objcSources, m_objcxxSources;
    int m_partCount;
};
} // anonymous namespace

ProjectPartBuilder::ProjectPartBuilder(ProjectInfo &pInfo)
    : m_templatePart(new ProjectPart)
    , m_pInfo(pInfo)
{
    m_templatePart->project = pInfo.project();
    m_templatePart->displayName = pInfo.project()->displayName();
    m_templatePart->projectFile = pInfo.project()->projectFilePath().toString();
}

void ProjectPartBuilder::setQtVersion(ProjectPart::QtVersion qtVersion)
{
    m_templatePart->qtVersion = qtVersion;
}

void ProjectPartBuilder::setCFlags(const QStringList &flags)
{
    m_cFlags = flags;
}

void ProjectPartBuilder::setCxxFlags(const QStringList &flags)
{
    m_cxxFlags = flags;
}

void ProjectPartBuilder::setDefines(const QByteArray &defines)
{
    m_templatePart->projectDefines = defines;
}

void ProjectPartBuilder::setHeaderPaths(const ProjectPart::HeaderPaths &headerPaths)
{
    m_templatePart->headerPaths = headerPaths;
}

void ProjectPartBuilder::setIncludePaths(const QStringList &includePaths)
{
    m_templatePart->headerPaths.clear();

    foreach (const QString &includeFile, includePaths) {
        ProjectPart::HeaderPath hp(includeFile, ProjectPart::HeaderPath::IncludePath);

        // The simple project managers are utterly ignorant of frameworks on OSX, and won't report
        // framework paths. The work-around is to check if the include path ends in ".framework",
        // and if so, add the parent directory as framework path.
        if (includeFile.endsWith(QLatin1String(".framework"))) {
            const int slashIdx = includeFile.lastIndexOf(QLatin1Char('/'));
            if (slashIdx != -1) {
                hp = ProjectPart::HeaderPath(includeFile.left(slashIdx),
                                             ProjectPart::HeaderPath::FrameworkPath);
                continue;
            }
        }

        m_templatePart->headerPaths += hp;
    }
}

void ProjectPartBuilder::setPreCompiledHeaders(const QStringList &pchs)
{
    m_templatePart->precompiledHeaders = pchs;
}

void ProjectPartBuilder::setProjectFile(const QString &projectFile)
{
    m_templatePart->projectFile = projectFile;
}

void ProjectPartBuilder::setDisplayName(const QString &displayName)
{
    m_templatePart->displayName = displayName;
}

void ProjectPartBuilder::setConfigFileName(const QString &configFileName)
{
    m_templatePart->projectConfigFile = configFileName;
}

QList<Core::Id> ProjectPartBuilder::createProjectPartsForFiles(const QStringList &files)
{
    QList<Core::Id> languages;

    ProjectFileCategorizer cat(m_templatePart->displayName, files);
    if (cat.hasNoParts())
        return languages;

    using CppTools::ProjectFile;
    using CppTools::ProjectPart;

    if (cat.hasCSources()) {
        createProjectPart(cat.cSources(),
                          cat.partName(QCoreApplication::translate("CppTools", "C11")),
                          ProjectPart::C11,
                          ProjectPart::CXX11);
        // TODO: there is no C...
//        languages += ProjectExplorer::Constants::LANG_C;
    }
    if (cat.hasObjcSources()) {
        createProjectPart(cat.objcSources(),
                          cat.partName(QCoreApplication::translate("CppTools", "Obj-C11")),
                          ProjectPart::C11,
                          ProjectPart::CXX11);
        // TODO: there is no Ojective-C...
//        languages += ProjectExplorer::Constants::LANG_OBJC;
    }
    if (cat.hasCxxSources()) {
        createProjectPart(cat.cxxSources(),
                          cat.partName(QCoreApplication::translate("CppTools", "C++11")),
                          ProjectPart::C11,
                          ProjectPart::CXX11);
        languages += ProjectExplorer::Constants::LANG_CXX;
    }
    if (cat.hasObjcxxSources()) {
        createProjectPart(cat.objcxxSources(),
                          cat.partName(QCoreApplication::translate("CppTools", "Obj-C++11")),
                          ProjectPart::C11,
                          ProjectPart::CXX11);
        // TODO: there is no Objective-C++...
        languages += ProjectExplorer::Constants::LANG_CXX;
    }

    return languages;
}

void ProjectPartBuilder::createProjectPart(const QStringList &theSources,
                                           const QString &partName,
                                           ProjectPart::CVersion cVersion,
                                           ProjectPart::CXXVersion cxxVersion)
{
    CppTools::ProjectPart::Ptr part(m_templatePart->copy());
    part->displayName = partName;

    Kit *k = part->project->activeTarget()->kit();
    if (ToolChain *tc = ToolChainKitInformation::toolChain(k))
        part->evaluateToolchain(tc, m_cFlags, m_cxxFlags, SysRootKitInformation::sysRoot(k));

    part->cVersion = cVersion;
    part->cxxVersion = cxxVersion;

    CppTools::ProjectFileAdder adder(part->files);
    foreach (const QString &file, theSources)
        adder.maybeAdd(file);

    m_pInfo.appendProjectPart(part);
}
