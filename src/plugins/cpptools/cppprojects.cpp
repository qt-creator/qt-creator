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

#include "cppprojects.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QSet>
#include <QTextStream>

using namespace CppTools;
using namespace CppTools::Internal;
using namespace ProjectExplorer;

ProjectPart::ProjectPart()
    : project(0)
    , languageVersion(CXX14)
    , languageExtensions(NoExtensions)
    , qtVersion(UnknownQt)
    , warningFlags(ToolChain::WarningsDefault)
    , selectedForBuilding(true)
{
}

/*!
    \brief Retrieves info from concrete compiler using it's flags.

    \param tc Either nullptr or toolchain for project's active target.
    \param cxxflags C++ or Objective-C++ flags.
    \param cflags C or ObjectiveC flags if possible, \a cxxflags otherwise.
*/
void ProjectPart::evaluateToolchain(const ToolChain *tc,
                                    const QStringList &commandLineFlags,
                                    const Utils::FileName &sysRoot)
{
    if (!tc)
        return;

    using namespace ProjectExplorer;
    ToolChain::CompilerFlags flags = tc->compilerFlags(commandLineFlags);

    if (flags & ToolChain::StandardC11)
        languageVersion = C11;
    else if (flags & ToolChain::StandardC99)
        languageVersion = C99;
    else if (flags & ToolChain::StandardCxx17)
        languageVersion = CXX17;
    else if (flags & ToolChain::StandardCxx14)
        languageVersion = CXX14;
    else if (flags & ToolChain::StandardCxx11)
        languageVersion = CXX11;
    else if (flags & ToolChain::StandardCxx98)
        languageVersion = CXX98;
    else
        languageVersion = CXX11;

    if (flags & ToolChain::BorlandExtensions)
        languageExtensions |= BorlandExtensions;
    if (flags & ToolChain::GnuExtensions)
        languageExtensions |= GnuExtensions;
    if (flags & ToolChain::MicrosoftExtensions)
        languageExtensions |= MicrosoftExtensions;
    if (flags & ToolChain::OpenMP)
        languageExtensions |= OpenMPExtensions;
    if (flags & ToolChain::ObjectiveC)
        languageExtensions |= ObjectiveCExtensions;

    warningFlags = tc->warningFlags(commandLineFlags);

    const QList<ProjectExplorer::HeaderPath> headers = tc->systemHeaderPaths(commandLineFlags, sysRoot);
    foreach (const ProjectExplorer::HeaderPath &header, headers) {
        headerPaths << ProjectPart::HeaderPath(header.path(),
                                header.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath
                                    ? ProjectPart::HeaderPath::FrameworkPath
                                    : ProjectPart::HeaderPath::IncludePath);
    }

    toolchainDefines = tc->predefinedMacros(commandLineFlags);
    toolchainType = tc->typeId();
    updateLanguageFeatures();
}

void ProjectPart::updateLanguageFeatures()
{
    const bool hasQt = qtVersion != NoQt;
    languageFeatures.cxx11Enabled = languageVersion >= CXX11;
    languageFeatures.qtEnabled = hasQt;
    languageFeatures.qtMocRunEnabled = hasQt;
    if (!hasQt) {
        languageFeatures.qtKeywordsEnabled = false;
    } else {
        const QByteArray noKeywordsMacro = "#define QT_NO_KEYWORDS";
        const int noKeywordsIndex = projectDefines.indexOf(noKeywordsMacro);
        if (noKeywordsIndex == -1) {
            languageFeatures.qtKeywordsEnabled = true;
        } else {
            const char nextChar = projectDefines.at(noKeywordsIndex + noKeywordsMacro.length());
            // Detect "#define QT_NO_KEYWORDS" and "#define QT_NO_KEYWORDS 1", but exclude
            // "#define QT_NO_KEYWORDS_FOO"
            languageFeatures.qtKeywordsEnabled = nextChar != '\n' && nextChar != ' ';
        }
    }
}

ProjectPart::Ptr ProjectPart::copy() const
{
    return Ptr(new ProjectPart(*this));
}

QString ProjectPart::id() const
{
    QString projectPartId = QDir::fromNativeSeparators(projectFile);
    if (!displayName.isEmpty())
        projectPartId.append(QLatin1Char(' ') + displayName);
    return projectPartId;
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

ProjectInfo::ProjectInfo(QPointer<Project> project)
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

QPointer<Project> ProjectInfo::project() const
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
    typedef ProjectPart::HeaderPath HeaderPath;

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

void ProjectInfo::setCompilerCallData(const CompilerCallData &data)
{
    m_compilerCallData = data;
}

ProjectInfo::CompilerCallData ProjectInfo::compilerCallData() const
{
    return m_compilerCallData;
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

void ProjectPartBuilder::createProjectPart(const QStringList &theSources,
                                           const QString &partName,
                                           ProjectPart::LanguageVersion languageVersion,
                                           ProjectPart::LanguageExtensions languageExtensions)
{
    ProjectPart::Ptr part(m_templatePart->copy());
    part->displayName = partName;

    QTC_ASSERT(part->project, return);
    if (Target *activeTarget = part->project->activeTarget()) {
        if (Kit *kit = activeTarget->kit()) {
            if (ToolChain *toolChain = ToolChainKitInformation::toolChain(kit)) {
                const QStringList flags = languageVersion >= ProjectPart::CXX98 ? m_cxxFlags
                                                                                : m_cFlags;
                part->evaluateToolchain(toolChain, flags, SysRootKitInformation::sysRoot(kit));
            }
        }
    }

    part->languageExtensions |= languageExtensions;

    ProjectFileAdder adder(part->files);
    foreach (const QString &file, theSources)
        adder.maybeAdd(file);

    m_pInfo.appendProjectPart(part);
}


CompilerOptionsBuilder::CompilerOptionsBuilder(const ProjectPart::Ptr &projectPart)
    : m_projectPart(projectPart)
{
}

QStringList CompilerOptionsBuilder::options() const
{
    return m_options;
}

void CompilerOptionsBuilder::add(const QString &option)
{
    m_options.append(option);
}

QString CompilerOptionsBuilder::defineLineToDefineOption(const QByteArray &defineLine)
{
    QByteArray str = defineLine.mid(8);
    int spaceIdx = str.indexOf(' ');
    const QString option = defineOption();
    const bool hasValue = spaceIdx != -1;
    QString arg = option + QLatin1String(str.left(hasValue ? spaceIdx : str.size()) + '=');
    if (hasValue)
        arg += QLatin1String(str.mid(spaceIdx + 1));
    return arg;
}

void CompilerOptionsBuilder::addDefine(const QByteArray &defineLine)
{
    m_options.append(defineLineToDefineOption(defineLine));
}

void CompilerOptionsBuilder::addHeaderPathOptions()
{
    typedef ProjectPart::HeaderPath HeaderPath;
    const QString defaultPrefix = includeOption();

    QStringList result;

    foreach (const HeaderPath &headerPath , m_projectPart->headerPaths) {
        if (headerPath.path.isEmpty())
            continue;

        if (excludeHeaderPath(headerPath.path))
            continue;

        QString prefix;
        switch (headerPath.type) {
        case HeaderPath::FrameworkPath:
            prefix = QLatin1String("-F");
            break;
        default: // This shouldn't happen, but let's be nice..:
            // intentional fall-through:
        case HeaderPath::IncludePath:
            prefix = defaultPrefix;
            break;
        }

        result.append(prefix + headerPath.path);
    }

    m_options.append(result);
}

void CompilerOptionsBuilder::addToolchainAndProjectDefines()
{
    QByteArray extendedDefines = m_projectPart->toolchainDefines + m_projectPart->projectDefines;
    QStringList result;

    foreach (QByteArray def, extendedDefines.split('\n')) {
        if (def.isEmpty() || excludeDefineLine(def))
            continue;

        const QString defineOption = defineLineToDefineOption(def);
        if (!result.contains(defineOption))
            result.append(defineOption);
    }

    m_options.append(result);
}

static QStringList createLanguageOptionGcc(ProjectFile::Kind fileKind, bool objcExt)
{
    QStringList opts;

    switch (fileKind) {
    case ProjectFile::Unclassified:
        break;
    case ProjectFile::CHeader:
        if (objcExt)
            opts += QLatin1String("objective-c-header");
        else
            opts += QLatin1String("c-header");
        break;

    case ProjectFile::CXXHeader:
    default:
        if (!objcExt) {
            opts += QLatin1String("c++-header");
            break;
        } // else: fall-through!
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
        opts += QLatin1String("objective-c++-header");
        break;

    case ProjectFile::CSource:
        if (!objcExt) {
            opts += QLatin1String("c");
            break;
        } // else: fall-through!
    case ProjectFile::ObjCSource:
        opts += QLatin1String("objective-c");
        break;

    case ProjectFile::CXXSource:
        if (!objcExt) {
            opts += QLatin1String("c++");
            break;
        } // else: fall-through!
    case ProjectFile::ObjCXXSource:
        opts += QLatin1String("objective-c++");
        break;

    case ProjectFile::OpenCLSource:
        opts += QLatin1String("cl");
        break;
    case ProjectFile::CudaSource:
        opts += QLatin1String("cuda");
        break;
    }

    if (!opts.isEmpty())
        opts.prepend(QLatin1String("-x"));

    return opts;
}

void CompilerOptionsBuilder::addLanguageOption(ProjectFile::Kind fileKind)
{
    const bool objcExt = m_projectPart->languageExtensions & ProjectPart::ObjectiveCExtensions;
    const QStringList options = createLanguageOptionGcc(fileKind, objcExt);
    m_options.append(options);
}

void CompilerOptionsBuilder::addOptionsForLanguage(bool checkForBorlandExtensions)
{
    QStringList opts;
    const ProjectPart::LanguageExtensions languageExtensions = m_projectPart->languageExtensions;
    const bool gnuExtensions = languageExtensions & ProjectPart::GnuExtensions;
    switch (m_projectPart->languageVersion) {
    case ProjectPart::C89:
        opts << (gnuExtensions ? QLatin1String("-std=gnu89") : QLatin1String("-std=c89"));
        break;
    case ProjectPart::C99:
        opts << (gnuExtensions ? QLatin1String("-std=gnu99") : QLatin1String("-std=c99"));
        break;
    case ProjectPart::C11:
        opts << (gnuExtensions ? QLatin1String("-std=gnu11") : QLatin1String("-std=c11"));
        break;
    case ProjectPart::CXX11:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++11") : QLatin1String("-std=c++11"));
        break;
    case ProjectPart::CXX98:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++98") : QLatin1String("-std=c++98"));
        break;
    case ProjectPart::CXX03:
        // Clang 3.6 does not know -std=gnu++03.
        opts << QLatin1String("-std=c++03");
        break;
    case ProjectPart::CXX14:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++14") : QLatin1String("-std=c++14"));
        break;
    case ProjectPart::CXX17:
        // TODO: Change to (probably) "gnu++17"/"c++17" at some point in the future.
        opts << (gnuExtensions ? QLatin1String("-std=gnu++1z") : QLatin1String("-std=c++1z"));
        break;
    }

    if (languageExtensions & ProjectPart::MicrosoftExtensions)
        opts << QLatin1String("-fms-extensions");

    if (checkForBorlandExtensions && (languageExtensions & ProjectPart::BorlandExtensions))
        opts << QLatin1String("-fborland-extensions");

    m_options.append(opts);
}

QString CompilerOptionsBuilder::includeOption() const
{
    return QLatin1String("-I");
}

QString CompilerOptionsBuilder::defineOption() const
{
    return QLatin1String("-D");
}

bool CompilerOptionsBuilder::excludeDefineLine(const QByteArray &defineLine) const
{
    // This is a quick fix for QTCREATORBUG-11501.
    // TODO: do a proper fix, see QTCREATORBUG-11709.
    if (defineLine.startsWith("#define __cplusplus"))
        return true;

    // gcc 4.9 has:
    //    #define __has_include(STR) __has_include__(STR)
    //    #define __has_include_next(STR) __has_include_next__(STR)
    // The right-hand sides are gcc built-ins that clang does not understand, and they'd
    // override clang's own (non-macro, it seems) definitions of the symbols on the left-hand
    // side.
    const bool isGccToolchain = m_projectPart->toolchainType == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;
    if (isGccToolchain && defineLine.contains("has_include"))
        return true;

    return false;
}

bool CompilerOptionsBuilder::excludeHeaderPath(const QString &headerPath) const
{
    Q_UNUSED(headerPath);
    return false;
}
