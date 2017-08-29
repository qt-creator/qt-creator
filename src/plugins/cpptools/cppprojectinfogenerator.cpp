/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "cppprojectinfogenerator.h"

#include "cppprojectfilecategorizer.h"

#include <projectexplorer/headerpath.h>

namespace CppTools {
namespace Internal {

namespace {

class ToolChainEvaluator
{
public:
    ToolChainEvaluator(ProjectPart &projectPart,
                       const RawProjectPartFlags &flags,
                       const ToolChainInfo &tcInfo)
        : m_projectPart(projectPart)
        , m_flags(flags)
        , m_tcInfo(tcInfo)
    {
    }

    void evaluate()
    {
        m_projectPart.toolchainType = m_tcInfo.type;
        m_projectPart.isMsvc2015Toolchain = m_tcInfo.isMsvc2015ToolChain;
        m_projectPart.toolChainWordWidth = mapWordWith(m_tcInfo.wordWidth);
        m_projectPart.toolChainTargetTriple = m_tcInfo.targetTriple;

        m_projectPart.warningFlags = m_flags.warningFlags;

        mapLanguageVersion();
        mapLanguageExtensions();

        addHeaderPaths();
        addDefines();
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

        const ToolChain::CompilerFlags &compilerFlags = m_flags.compilerFlags;
        ProjectPart::LanguageVersion &languageVersion = m_projectPart.languageVersion;

        if (compilerFlags & ToolChain::StandardC11)
            languageVersion = ProjectPart::C11;
        else if (compilerFlags & ToolChain::StandardC99)
            languageVersion = ProjectPart::C99;
        else if (compilerFlags & ToolChain::StandardCxx17)
            languageVersion = ProjectPart::CXX17;
        else if (compilerFlags & ToolChain::StandardCxx14)
            languageVersion = ProjectPart::CXX14;
        else if (compilerFlags & ToolChain::StandardCxx11)
            languageVersion = ProjectPart::CXX11;
        else if (compilerFlags & ToolChain::StandardCxx98)
            languageVersion = ProjectPart::CXX98;
    }

    void mapLanguageExtensions()
    {
        using namespace ProjectExplorer;

        const ToolChain::CompilerFlags &compilerFlags = m_flags.compilerFlags;
        ProjectPart::LanguageExtensions &languageExtensions = m_projectPart.languageExtensions;

        if (compilerFlags & ToolChain::BorlandExtensions)
            languageExtensions |= ProjectPart::BorlandExtensions;
        if (compilerFlags & ToolChain::GnuExtensions)
            languageExtensions |= ProjectPart::GnuExtensions;
        if (compilerFlags & ToolChain::MicrosoftExtensions)
            languageExtensions |= ProjectPart::MicrosoftExtensions;
        if (compilerFlags & ToolChain::OpenMP)
            languageExtensions |= ProjectPart::OpenMPExtensions;
        if (compilerFlags & ToolChain::ObjectiveC)
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
        if (!m_tcInfo.headerPathsRunner)
            return; // No compiler set in kit.

        const QList<ProjectExplorer::HeaderPath> systemHeaderPaths
                = m_tcInfo.headerPathsRunner(m_flags.commandLineFlags,
                                             m_tcInfo.sysRoothPath);

        ProjectPartHeaderPaths &headerPaths = m_projectPart.headerPaths;
        for (const ProjectExplorer::HeaderPath &header : systemHeaderPaths) {
            const ProjectPartHeaderPath headerPath = toProjectPartHeaderPath(header);
            if (!headerPaths.contains(headerPath))
                headerPaths.push_back(headerPath);
        }
    }

    void addDefines()
    {
        if (!m_tcInfo.predefinedMacrosRunner)
            return; // No compiler set in kit.

        m_projectPart.toolChainMacros = m_tcInfo.predefinedMacrosRunner(m_flags.commandLineFlags);
    }

private:
    ProjectPart &m_projectPart;
    const RawProjectPartFlags &m_flags;
    const ToolChainInfo &m_tcInfo;
};

} // anonymous namespace

ProjectInfoGenerator::ProjectInfoGenerator(const QFutureInterface<void> &futureInterface,
                                           const ProjectUpdateInfo &projectUpdateInfo)
    : m_futureInterface(futureInterface)
    , m_projectUpdateInfo(projectUpdateInfo)
{
}

ProjectInfo ProjectInfoGenerator::generate()
{
    m_projectInfo = ProjectInfo(m_projectUpdateInfo.project);

    for (const RawProjectPart &rpp : m_projectUpdateInfo.rawProjectParts) {
        if (m_futureInterface.isCanceled())
            return ProjectInfo();

        createProjectParts(rpp);
    }

    return m_projectInfo;
}

static ProjectPart::Ptr projectPartFromRawProjectPart(const RawProjectPart &rawProjectPart,
                                                      ProjectExplorer::Project *project)
{
    ProjectPart::Ptr part(new ProjectPart);
    part->project = project;
    part->projectFile = rawProjectPart.projectFile;
    part->projectConfigFile = rawProjectPart.projectConfigFile;
    part->projectFileLine = rawProjectPart.projectFileLine;
    part->projectFileColumn = rawProjectPart.projectFileColumn;
    part->callGroupId = rawProjectPart.callGroupId;
    part->buildSystemTarget = rawProjectPart.buildSystemTarget;
    part->buildTargetType = rawProjectPart.buildTargetType;
    part->qtVersion = rawProjectPart.qtVersion;
    part->projectMacros = rawProjectPart.projectMacros;
    part->headerPaths = rawProjectPart.headerPaths;
    part->precompiledHeaders = rawProjectPart.precompiledHeaders;
    part->selectedForBuilding = rawProjectPart.selectedForBuilding;

    return part;
}

void ProjectInfoGenerator::createProjectParts(const RawProjectPart &rawProjectPart)
{
    ProjectFileCategorizer cat(rawProjectPart.displayName,
                               rawProjectPart.files,
                               rawProjectPart.fileClassifier);

    if (cat.hasParts()) {
        const ProjectPart::Ptr part = projectPartFromRawProjectPart(rawProjectPart,
                                                                    m_projectUpdateInfo.project);

        ProjectPart::LanguageVersion defaultVersion = ProjectPart::LatestCxxVersion;
        if (rawProjectPart.qtVersion == ProjectPart::Qt4_8_6AndOlder)
            defaultVersion = ProjectPart::CXX11;
        if (cat.hasCxxSources()) {
            createProjectPart(rawProjectPart,
                              part,
                              cat.cxxSources(),
                              cat.partName("C++"),
                              defaultVersion,
                              ProjectPart::NoExtensions);
        }

        if (cat.hasObjcxxSources()) {
            createProjectPart(rawProjectPart,
                              part,
                              cat.objcxxSources(),
                              cat.partName("Obj-C++"),
                              defaultVersion,
                              ProjectPart::ObjectiveCExtensions);
        }

        if (cat.hasCSources()) {
            createProjectPart(rawProjectPart,
                              part,
                              cat.cSources(),
                              cat.partName("C"),
                              ProjectPart::LatestCVersion,
                              ProjectPart::NoExtensions);
        }

        if (cat.hasObjcSources()) {
            createProjectPart(rawProjectPart,
                              part,
                              cat.objcSources(),
                              cat.partName("Obj-C"),
                              ProjectPart::LatestCVersion,
                              ProjectPart::ObjectiveCExtensions);
        }
    }
}

void ProjectInfoGenerator::createProjectPart(const RawProjectPart &rawProjectPart,
                                             const ProjectPart::Ptr &templateProjectPart,
                                             const ProjectFiles &projectFiles,
                                             const QString &partName,
                                             ProjectPart::LanguageVersion languageVersion,
                                             ProjectPart::LanguageExtensions languageExtensions)
{
    ProjectPart::Ptr part(templateProjectPart->copy());
    part->displayName = partName;
    part->files = projectFiles;
    part->languageVersion = languageVersion;

    RawProjectPartFlags flags;
    ToolChainInfo tcInfo;
    if (languageVersion <= ProjectPart::LatestCVersion) {
        flags = rawProjectPart.flagsForC;
        tcInfo = m_projectUpdateInfo.cToolChainInfo;
    }
    // Use Cxx toolchain for C projects without C compiler in kit and for C++ code
    if (!tcInfo.isValid()) {
        flags = rawProjectPart.flagsForCxx;
        tcInfo = m_projectUpdateInfo.cxxToolChainInfo;
    }
    // TODO: If no toolchain is set, show a warning
    ToolChainEvaluator evaluator(*part.data(), flags, tcInfo);
    evaluator.evaluate();

    part->languageExtensions |= languageExtensions;
    part->updateLanguageFeatures();

    m_projectInfo.appendProjectPart(part);
}

} // namespace Internal
} // namespace CppTools
