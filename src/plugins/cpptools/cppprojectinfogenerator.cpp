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
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

namespace CppTools {
namespace Internal {

namespace {

class ToolChainEvaluator
{
public:
    ToolChainEvaluator(ProjectPart &projectPart,
                       Language language,
                       const RawProjectPartFlags &flags,
                       const ToolChainInfo &tcInfo)
        : m_projectPart(projectPart)
        , m_language(language)
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
        m_projectPart.extraCodeModelFlags = m_tcInfo.extraCodeModelFlags;

        m_projectPart.warningFlags = m_flags.warningFlags;

        // For compilation database pass the command line flags directly.
        if (m_projectPart.toolchainType == ProjectExplorer::Constants::COMPILATION_DATABASE_TOOLCHAIN_TYPEID)
            m_projectPart.extraCodeModelFlags = m_flags.commandLineFlags;

        if (m_tcInfo.macroInspectionRunner) {
            auto macroInspectionReport = m_tcInfo.macroInspectionRunner(m_flags.commandLineFlags);
            m_projectPart.toolChainMacros = macroInspectionReport.macros;
            m_projectPart.languageVersion = macroInspectionReport.languageVersion;
        } else { // No compiler set in kit.
            if (m_language == Language::C)
                m_projectPart.languageVersion = ProjectExplorer::LanguageVersion::LatestC;
            else if (m_language == Language::Cxx)
                m_projectPart.languageVersion = ProjectExplorer::LanguageVersion::LatestCxx;
        }

        m_projectPart.languageExtensions = m_flags.languageExtensions;

        addHeaderPaths();
    }

private:
    static ProjectPart::ToolChainWordWidth mapWordWith(unsigned wordWidth)
    {
        return wordWidth == 64
                ? ProjectPart::WordWidth64Bit
                : ProjectPart::WordWidth32Bit;
    }

    void addHeaderPaths()
    {
        if (!m_tcInfo.headerPathsRunner)
            return; // No compiler set in kit.

        const ProjectExplorer::HeaderPaths builtInHeaderPaths
                = m_tcInfo.headerPathsRunner(m_flags.commandLineFlags,
                                             m_tcInfo.sysRootPath);

        ProjectExplorer::HeaderPaths &headerPaths = m_projectPart.headerPaths;
        for (const ProjectExplorer::HeaderPath &header : builtInHeaderPaths) {
            const ProjectExplorer::HeaderPath headerPath{header.path, header.type};
            if (!headerPaths.contains(headerPath))
                headerPaths.push_back(headerPath);
        }
    }

private:
    ProjectPart &m_projectPart;
    const Language m_language;
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
    ProjectInfo projectInfo(m_projectUpdateInfo.project);

    for (const RawProjectPart &rpp : m_projectUpdateInfo.rawProjectParts) {
        if (m_futureInterface.isCanceled())
            return ProjectInfo();

        for (ProjectPart::Ptr part : createProjectParts(rpp))
            projectInfo.appendProjectPart(part);
    }

    return projectInfo;
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
    if (!part->projectConfigFile.isEmpty())
        part->projectMacros += ProjectExplorer::Macro::toMacros(ProjectPart::readProjectConfigFile(part));
    part->headerPaths = rawProjectPart.headerPaths;
    part->precompiledHeaders = rawProjectPart.precompiledHeaders;
    part->selectedForBuilding = rawProjectPart.selectedForBuilding;

    return part;
}

QVector<ProjectPart::Ptr> ProjectInfoGenerator::createProjectParts(const RawProjectPart &rawProjectPart)
{
    using ProjectExplorer::LanguageExtension;

    QVector<ProjectPart::Ptr> result;
    ProjectFileCategorizer cat(rawProjectPart.displayName,
                               rawProjectPart.files,
                               rawProjectPart.fileClassifier);

    if (cat.hasParts()) {
        const ProjectPart::Ptr part = projectPartFromRawProjectPart(rawProjectPart,
                                                                    m_projectUpdateInfo.project);

        if (cat.hasCxxSources()) {
            result << createProjectPart(rawProjectPart,
                                        part,
                                        cat.cxxSources(),
                                        cat.partName("C++"),
                                        Language::Cxx,
                                        LanguageExtension::None);
        }

        if (cat.hasObjcxxSources()) {
            result << createProjectPart(rawProjectPart,
                                        part,
                                        cat.objcxxSources(),
                                        cat.partName("Obj-C++"),
                                        Language::Cxx,
                                        LanguageExtension::ObjectiveC);
        }

        if (cat.hasCSources()) {
            result << createProjectPart(rawProjectPart,
                                        part,
                                        cat.cSources(),
                                        cat.partName("C"),
                                        Language::C,
                                        LanguageExtension::None);
        }

        if (cat.hasObjcSources()) {
            result << createProjectPart(rawProjectPart,
                                        part,
                                        cat.objcSources(),
                                        cat.partName("Obj-C"),
                                        Language::C,
                                        LanguageExtension::ObjectiveC);
        }
    }
    return result;
}

ProjectPart::Ptr ProjectInfoGenerator::createProjectPart(
    const RawProjectPart &rawProjectPart,
    const ProjectPart::Ptr &templateProjectPart,
    const ProjectFiles &projectFiles,
    const QString &partName,
    Language language,
    ProjectExplorer::LanguageExtensions languageExtensions)
{
    ProjectPart::Ptr part(templateProjectPart->copy());
    part->displayName = partName;
    part->files = projectFiles;

    RawProjectPartFlags flags;
    ToolChainInfo tcInfo;
    if (language == Language::C) {
        flags = rawProjectPart.flagsForC;
        tcInfo = m_projectUpdateInfo.cToolChainInfo;
    }
    // Use Cxx toolchain for C projects without C compiler in kit and for C++ code
    if (!tcInfo.isValid()) {
        flags = rawProjectPart.flagsForCxx;
        tcInfo = m_projectUpdateInfo.cxxToolChainInfo;
    }
    // TODO: If no toolchain is set, show a warning
    ToolChainEvaluator evaluator(*part.data(), language, flags, tcInfo);
    evaluator.evaluate();

    part->languageExtensions |= languageExtensions;
    part->updateLanguageFeatures();

    return part;
}

} // namespace Internal
} // namespace CppTools
