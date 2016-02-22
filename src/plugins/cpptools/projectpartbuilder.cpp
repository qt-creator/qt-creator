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

#include "projectpartbuilder.h"

#include "cppprojectfile.h"
#include "cpptoolsconstants.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

namespace CppTools {

namespace {
class ProjectFileCategorizer
{
public:
    ProjectFileCategorizer(const QString &partName,
                           const QStringList &files,
                           ProjectPartBuilder::FileClassifier fileClassifier
                                = ProjectPartBuilder::FileClassifier())
        : m_partName(partName)
    {
        using CppTools::ProjectFile;

        QStringList cHeaders, cxxHeaders;

        foreach (const QString &file, files) {
            const ProjectFile::Kind kind = fileClassifier
                    ? fileClassifier(file)
                    : ProjectFile::classify(file);

            switch (kind) {
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

void ProjectPartBuilder::setHeaderPaths(const ProjectPartHeaderPaths &headerPaths)
{
    m_templatePart->headerPaths = headerPaths;
}

void ProjectPartBuilder::setIncludePaths(const QStringList &includePaths)
{
    m_templatePart->headerPaths.clear();

    foreach (const QString &includeFile, includePaths) {
        ProjectPartHeaderPath hp(includeFile, ProjectPartHeaderPath::IncludePath);

        // The simple project managers are utterly ignorant of frameworks on OSX, and won't report
        // framework paths. The work-around is to check if the include path ends in ".framework",
        // and if so, add the parent directory as framework path.
        if (includeFile.endsWith(QLatin1String(".framework"))) {
            const int slashIdx = includeFile.lastIndexOf(QLatin1Char('/'));
            if (slashIdx != -1) {
                hp = ProjectPartHeaderPath(includeFile.left(slashIdx),
                                             ProjectPartHeaderPath::FrameworkPath);
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

QList<Core::Id> ProjectPartBuilder::createProjectPartsForFiles(const QStringList &files,
                                                               FileClassifier fileClassifier)
{
    QList<Core::Id> languages;

    ProjectFileCategorizer cat(m_templatePart->displayName, files, fileClassifier);
    if (cat.hasNoParts())
        return languages;

    using CppTools::ProjectFile;
    using CppTools::ProjectPart;

    if (cat.hasCSources()) {
        createProjectPart(cat.cSources(),
                          cat.partName(QCoreApplication::translate("CppTools", "C11")),
                          ProjectPart::C11,
                          ProjectPart::NoExtensions);
        // TODO: there is no C...
//        languages += ProjectExplorer::Constants::LANG_C;
    }
    if (cat.hasObjcSources()) {
        createProjectPart(cat.objcSources(),
                          cat.partName(QCoreApplication::translate("CppTools", "Obj-C11")),
                          ProjectPart::C11,
                          ProjectPart::ObjectiveCExtensions);
        // TODO: there is no Ojective-C...
//        languages += ProjectExplorer::Constants::LANG_OBJC;
    }
    if (cat.hasCxxSources()) {
        createProjectPart(cat.cxxSources(),
                          cat.partName(QCoreApplication::translate("CppTools", "C++11")),
                          ProjectPart::CXX11,
                          ProjectPart::NoExtensions);
        languages += ProjectExplorer::Constants::LANG_CXX;
    }
    if (cat.hasObjcxxSources()) {
        createProjectPart(cat.objcxxSources(),
                          cat.partName(QCoreApplication::translate("CppTools", "Obj-C++11")),
                          ProjectPart::CXX11,
                          ProjectPart::ObjectiveCExtensions);
        // TODO: there is no Objective-C++...
        languages += ProjectExplorer::Constants::LANG_CXX;
    }

    return languages;
}

namespace {

ProjectPartHeaderPath toProjectPartHeaderPath(const ProjectExplorer::HeaderPath &headerPath)
{
    const ProjectPartHeaderPath::Type headerPathType =
        headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath
            ? ProjectPartHeaderPath::FrameworkPath
            : ProjectPartHeaderPath::IncludePath;

    return ProjectPartHeaderPath(headerPath.path(), headerPathType);
}

}

/*!
    \brief Retrieves info from concrete compiler using it's flags.

    \param projectPart Project part which can never be an null pointer.
    \param toolChain Either nullptr or toolchain for project's active target.
    \param cxxflags C++ or Objective-C++ flags.
    \param cflags C or ObjectiveC flags if possible, \a cxxflags otherwise.
*/
void ProjectPartBuilder::evaluateProjectPartToolchain(
        ProjectPart *projectPart,
        const ProjectExplorer::ToolChain *toolChain,
        const QStringList &commandLineFlags,
        const Utils::FileName &sysRoot)
{
    if (toolChain == nullptr)
        return;

    using namespace ProjectExplorer;
    ToolChain::CompilerFlags flags = toolChain->compilerFlags(commandLineFlags);
    auto &languageVersion = projectPart->languageVersion;

    if (flags & ToolChain::StandardC11)
        languageVersion = ProjectPart::C11;
    else if (flags & ToolChain::StandardC99)
        languageVersion = ProjectPart::C99;
    else if (flags & ToolChain::StandardCxx17)
        languageVersion = ProjectPart::CXX17;
    else if (flags & ToolChain::StandardCxx14)
        languageVersion = ProjectPart::CXX14;
    else if (flags & ToolChain::StandardCxx11)
        languageVersion = ProjectPart::CXX11;
    else if (flags & ToolChain::StandardCxx98)
        languageVersion = ProjectPart::CXX98;
    else
        languageVersion = ProjectPart::CXX11;

    auto &languageExtensions = projectPart->languageExtensions;

    if (flags & ToolChain::BorlandExtensions)
        languageExtensions |= ProjectPart::BorlandExtensions;
    if (flags & ToolChain::GnuExtensions)
        languageExtensions |= ProjectPart::GnuExtensions;
    if (flags & ToolChain::MicrosoftExtensions)
        languageExtensions |= ProjectPart::MicrosoftExtensions;
    if (flags & ToolChain::OpenMP)
        languageExtensions |= ProjectPart::OpenMPExtensions;
    if (flags & ToolChain::ObjectiveC)
        languageExtensions |= ProjectPart::ObjectiveCExtensions;

    projectPart->warningFlags = toolChain->warningFlags(commandLineFlags);

    const QList<ProjectExplorer::HeaderPath> headers = toolChain->systemHeaderPaths(commandLineFlags, sysRoot);
    foreach (const ProjectExplorer::HeaderPath &header, headers) {
        const ProjectPartHeaderPath headerPath = toProjectPartHeaderPath(header);
        if (!projectPart->headerPaths.contains(headerPath))
            projectPart->headerPaths.push_back(headerPath);
    }

    projectPart->toolchainDefines = toolChain->predefinedMacros(commandLineFlags);
    projectPart->toolchainType = toolChain->typeId();
    projectPart->updateLanguageFeatures();
}

namespace Internal {

class ProjectFileAdder
{
public:
    ProjectFileAdder(QVector<ProjectFile> &files);
    ~ProjectFileAdder();

    bool maybeAdd(const QString &path);

private:

    void addMapping(const char *mimeName, ProjectFile::Kind kind);

    QVector<ProjectFile> &m_files;
    QHash<QString, ProjectFile::Kind> m_mimeNameMapping;
};

ProjectFileAdder::ProjectFileAdder(QVector<ProjectFile> &files)
    : m_files(files)
{
    addMapping(CppTools::Constants::C_SOURCE_MIMETYPE, ProjectFile::CSource);
    addMapping(CppTools::Constants::C_HEADER_MIMETYPE, ProjectFile::CHeader);
    addMapping(CppTools::Constants::CPP_SOURCE_MIMETYPE, ProjectFile::CXXSource);
    addMapping(CppTools::Constants::CPP_HEADER_MIMETYPE, ProjectFile::CXXHeader);
    addMapping(CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE, ProjectFile::ObjCSource);
    addMapping(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE, ProjectFile::ObjCXXSource);
}

ProjectFileAdder::~ProjectFileAdder()
{
}

bool ProjectFileAdder::maybeAdd(const QString &path)
{
    Utils::MimeDatabase mdb;
    const Utils::MimeType mt = mdb.mimeTypeForFile(path);
    if (m_mimeNameMapping.contains(mt.name())) {
        m_files << ProjectFile(path, m_mimeNameMapping.value(mt.name()));
        return true;
    }
    return false;
}

void ProjectFileAdder::addMapping(const char *mimeName, ProjectFile::Kind kind)
{
    Utils::MimeDatabase mdb;
    Utils::MimeType mimeType = mdb.mimeTypeForName(QLatin1String(mimeName));
    if (mimeType.isValid())
        m_mimeNameMapping.insert(mimeType.name(), kind);
}
}

void ProjectPartBuilder::createProjectPart(const QStringList &theSources,
                                           const QString &partName,
                                           ProjectPart::LanguageVersion languageVersion,
                                           ProjectPart::LanguageExtensions languageExtensions)
{
    ProjectPart::Ptr part(m_templatePart->copy());
    part->displayName = partName;

    QTC_ASSERT(part->project, return);
    if (ProjectExplorer::Target *activeTarget = part->project->activeTarget()) {
        if (ProjectExplorer::Kit *kit = activeTarget->kit()) {
            if (ProjectExplorer::ToolChain *toolChain = ProjectExplorer::ToolChainKitInformation::toolChain(kit)) {
                const QStringList flags = languageVersion >= ProjectPart::CXX98 ? m_cxxFlags
                                                                                : m_cFlags;
                evaluateProjectPartToolchain(part.data(),
                                             toolChain,
                                             flags,
                                             ProjectExplorer::SysRootKitInformation::sysRoot(kit));
            }
        }
    }

    part->languageExtensions |= languageExtensions;

    Internal::ProjectFileAdder adder(part->files);
    foreach (const QString &file, theSources)
        adder.maybeAdd(file);

    m_pInfo.appendProjectPart(part);
}

} // namespace CppTools
