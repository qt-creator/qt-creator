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
    using Utils::LanguageExtension;

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
    Utils::LanguageExtensions languageExtensions)
{
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

    ProjectPart::Ptr part(templateProjectPart->copy());
    part->displayName = partName;
    part->files = projectFiles;
    part->toolchainType = tcInfo.type;
    part->isMsvc2015Toolchain = tcInfo.isMsvc2015ToolChain;
    part->toolChainWordWidth = tcInfo.wordWidth == 64 ? ProjectPart::WordWidth64Bit
                                                      : ProjectPart::WordWidth32Bit;
    part->toolChainTargetTriple = tcInfo.targetTriple;
    part->extraCodeModelFlags = tcInfo.extraCodeModelFlags;
    part->compilerFlags = flags.commandLineFlags;
    part->warningFlags = flags.warningFlags;
    part->language = language;
    part->languageExtensions = flags.languageExtensions;

    // Toolchain macros and language version
    if (tcInfo.macroInspectionRunner) {
        auto macroInspectionReport = tcInfo.macroInspectionRunner(flags.commandLineFlags);
        part->toolChainMacros = macroInspectionReport.macros;
        part->languageVersion = macroInspectionReport.languageVersion;
    // No compiler set in kit.
    } else if (language == Language::C) {
        part->languageVersion = Utils::LanguageVersion::LatestC;
    } else {
        part->languageVersion = Utils::LanguageVersion::LatestCxx;
    }

    // Header paths
    if (tcInfo.headerPathsRunner) {
        const ProjectExplorer::HeaderPaths builtInHeaderPaths
            = tcInfo.headerPathsRunner(flags.commandLineFlags, tcInfo.sysRootPath);

        ProjectExplorer::HeaderPaths &headerPaths = part->headerPaths;
        for (const ProjectExplorer::HeaderPath &header : builtInHeaderPaths) {
            const ProjectExplorer::HeaderPath headerPath{header.path, header.type};
            if (!headerPaths.contains(headerPath))
                headerPaths.push_back(headerPath);
        }
    }

    part->languageExtensions |= languageExtensions;
    part->updateLanguageFeatures();

    return part;
}

} // namespace Internal
} // namespace CppTools
