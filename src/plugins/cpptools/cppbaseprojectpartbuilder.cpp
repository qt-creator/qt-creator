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

#include "cppbaseprojectpartbuilder.h"

#include "cppprojectfile.h"
#include "cppprojectfilecategorizer.h"
#include "projectinfo.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

namespace CppTools {

BaseProjectPartBuilder::BaseProjectPartBuilder(ProjectInterface *project,
                                               ProjectInfo &projectInfo)
    : m_project(project)
    , m_projectInfo(projectInfo)
    , m_templatePart(new ProjectPart)
{
    QTC_CHECK(m_project);
    m_templatePart->project = projectInfo.project();
    m_templatePart->displayName = m_project->displayName();
    m_templatePart->projectFile = m_project->projectFilePath();
}

void BaseProjectPartBuilder::setQtVersion(ProjectPart::QtVersion qtVersion)
{
    m_templatePart->qtVersion = qtVersion;
}

void BaseProjectPartBuilder::setCFlags(const QStringList &flags)
{
    m_cFlags = flags;
}

void BaseProjectPartBuilder::setCxxFlags(const QStringList &flags)
{
    m_cxxFlags = flags;
}

void BaseProjectPartBuilder::setDefines(const QByteArray &defines)
{
    m_templatePart->projectDefines = defines;
}

void BaseProjectPartBuilder::setHeaderPaths(const ProjectPartHeaderPaths &headerPaths)
{
    m_templatePart->headerPaths = headerPaths;
}

void BaseProjectPartBuilder::setIncludePaths(const QStringList &includePaths)
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

void BaseProjectPartBuilder::setPreCompiledHeaders(const QStringList &preCompiledHeaders)
{
    m_templatePart->precompiledHeaders = preCompiledHeaders;
}

void BaseProjectPartBuilder::setSelectedForBuilding(bool yesno)
{
    m_templatePart->selectedForBuilding = yesno;
}

void BaseProjectPartBuilder::setProjectFile(const QString &projectFile)
{
    m_templatePart->projectFile = projectFile;
}

void BaseProjectPartBuilder::setDisplayName(const QString &displayName)
{
    m_templatePart->displayName = displayName;
}

void BaseProjectPartBuilder::setConfigFileName(const QString &configFileName)
{
    m_templatePart->projectConfigFile = configFileName;
}

QList<Core::Id> BaseProjectPartBuilder::createProjectPartsForFiles(const QStringList &filePaths,
                                                                   FileClassifier fileClassifier)
{
    QList<Core::Id> languages;

    ProjectFileCategorizer cat(m_templatePart->displayName, filePaths, fileClassifier);

    if (cat.hasParts()) {
        if (cat.hasCxxSources()) {
            languages += ProjectExplorer::Constants::CXX_LANGUAGE_ID;
            createProjectPart(cat.cxxSources(),
                              cat.partName("C++"),
                              ProjectPart::LatestCxxVersion,
                              ProjectPart::NoExtensions);
        }

        if (cat.hasObjcxxSources()) {
            languages += ProjectExplorer::Constants::CXX_LANGUAGE_ID;
            createProjectPart(cat.objcxxSources(),
                              cat.partName("Obj-C++"),
                              ProjectPart::LatestCxxVersion,
                              ProjectPart::ObjectiveCExtensions);
        }

        if (cat.hasCSources()) {
            languages += ProjectExplorer::Constants::C_LANGUAGE_ID;
            createProjectPart(cat.cSources(),
                              cat.partName("C"),
                              ProjectPart::LatestCVersion,
                              ProjectPart::NoExtensions);
        }

        if (cat.hasObjcSources()) {
            languages += ProjectExplorer::Constants::C_LANGUAGE_ID;
            createProjectPart(cat.objcSources(),
                              cat.partName("Obj-C"),
                              ProjectPart::LatestCVersion,
                              ProjectPart::ObjectiveCExtensions);
        }
    }

    return languages;
}

namespace {

class ToolChainEvaluator
{
public:
    ToolChainEvaluator(ProjectPart &projectPart,
                       const ToolChainInterface &toolChain)
        : m_projectPart(projectPart)
        , m_toolChain(toolChain)
        , m_compilerFlags(toolChain.compilerFlags())
    {
    }

    void evaluate()
    {
        mapLanguageVersion();
        mapLanguageExtensions();

        addHeaderPaths();
        m_projectPart.toolchainDefines = m_toolChain.predefinedMacros();

        m_projectPart.warningFlags = m_toolChain.warningFlags();
        m_projectPart.toolchainType = m_toolChain.type();
        m_projectPart.isMsvc2015Toolchain = m_toolChain.isMsvc2015Toolchain();
        m_projectPart.toolChainWordWidth = mapWordWith(m_toolChain.wordWidth());
        m_projectPart.toolChainTargetTriple = m_toolChain.targetTriple();
    }

private:
    static ProjectPart::ToolChainWordWidth mapWordWith(unsigned wordWidth)
    {
        return wordWidth == 64
                ? ProjectPart::WordWidth64Bit
                : ProjectPart::WordWidth32Bit;
    }

    void mapLanguageVersion()
    {
        using namespace ProjectExplorer;
        ProjectPart::LanguageVersion &languageVersion = m_projectPart.languageVersion;

        if (m_compilerFlags & ToolChain::StandardC11)
            languageVersion = ProjectPart::C11;
        else if (m_compilerFlags & ToolChain::StandardC99)
            languageVersion = ProjectPart::C99;
        else if (m_compilerFlags & ToolChain::StandardCxx17)
            languageVersion = ProjectPart::CXX17;
        else if (m_compilerFlags & ToolChain::StandardCxx14)
            languageVersion = ProjectPart::CXX14;
        else if (m_compilerFlags & ToolChain::StandardCxx11)
            languageVersion = ProjectPart::CXX11;
        else if (m_compilerFlags & ToolChain::StandardCxx98)
            languageVersion = ProjectPart::CXX98;
    }

    void mapLanguageExtensions()
    {
        using namespace ProjectExplorer;
        ProjectPart::LanguageExtensions &languageExtensions = m_projectPart.languageExtensions;

        if (m_compilerFlags & ToolChain::BorlandExtensions)
            languageExtensions |= ProjectPart::BorlandExtensions;
        if (m_compilerFlags & ToolChain::GnuExtensions)
            languageExtensions |= ProjectPart::GnuExtensions;
        if (m_compilerFlags & ToolChain::MicrosoftExtensions)
            languageExtensions |= ProjectPart::MicrosoftExtensions;
        if (m_compilerFlags & ToolChain::OpenMP)
            languageExtensions |= ProjectPart::OpenMPExtensions;
        if (m_compilerFlags & ToolChain::ObjectiveC)
            languageExtensions |= ProjectPart::ObjectiveCExtensions;
    }

    static ProjectPartHeaderPath toProjectPartHeaderPath(
            const ProjectExplorer::HeaderPath &headerPath)
    {
        const ProjectPartHeaderPath::Type headerPathType =
            headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath
                ? ProjectPartHeaderPath::FrameworkPath
                : ProjectPartHeaderPath::IncludePath;

        return ProjectPartHeaderPath(headerPath.path(), headerPathType);
    }

    void addHeaderPaths()
    {
        ProjectPartHeaderPaths &headerPaths = m_projectPart.headerPaths;

        foreach (const ProjectExplorer::HeaderPath &header, m_toolChain.systemHeaderPaths()) {
            const ProjectPartHeaderPath headerPath = toProjectPartHeaderPath(header);
            if (!headerPaths.contains(headerPath))
                headerPaths.push_back(headerPath);
        }
    }

private:
    ProjectPart &m_projectPart;

    const ToolChainInterface &m_toolChain;
    const ProjectExplorer::ToolChain::CompilerFlags m_compilerFlags;
};

} // anynomous

void BaseProjectPartBuilder::createProjectPart(const ProjectFiles &projectFiles,
                                               const QString &partName,
                                               ProjectPart::LanguageVersion languageVersion,
                                               ProjectPart::LanguageExtensions languageExtensions)
{
    ProjectPart::Ptr part(m_templatePart->copy());
    part->displayName = partName;
    part->files = projectFiles;
    part->languageVersion = languageVersion;
    QTC_ASSERT(part->project, return);

    // TODO: If not toolchain is set, show a warning
    if (const ToolChainInterfacePtr toolChain = selectToolChain(languageVersion)) {
        ToolChainEvaluator evaluator(*part.data(), *toolChain.get());
        evaluator.evaluate();
    }

    part->languageExtensions |= languageExtensions;
    part->updateLanguageFeatures();

    m_projectInfo.appendProjectPart(part);
}

ToolChainInterfacePtr BaseProjectPartBuilder::selectToolChain(
        ProjectPart::LanguageVersion languageVersion)
{
    ToolChainInterfacePtr toolChain = nullptr;

    if (languageVersion <= ProjectPart::LatestCVersion)
        toolChain = m_project->toolChain(ProjectExplorer::Constants::C_LANGUAGE_ID, m_cFlags);

    if (!toolChain) // Use Cxx toolchain for C projects without C compiler in kit and for C++ code
        toolChain = m_project->toolChain(ProjectExplorer::Constants::CXX_LANGUAGE_ID, m_cxxFlags);

    return toolChain;
}

} // namespace CppTools
