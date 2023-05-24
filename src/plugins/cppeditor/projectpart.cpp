// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectpart.h"

#include <projectexplorer/project.h>

#include <utils/algorithm.h>

#include <QFile>
#include <QDir>
#include <QTextStream>

using namespace ProjectExplorer;

namespace CppEditor {

QString ProjectPart::id() const
{
    QString projectPartId = projectFileLocation();
    if (!displayName.isEmpty())
        projectPartId.append(QLatin1Char(' ') + displayName);
    return projectPartId;
}

QString ProjectPart::projectFileLocation() const
{
    QString location = QDir::fromNativeSeparators(projectFile);
    if (projectFileLine > 0)
        location += ":" + QString::number(projectFileLine);
    if (projectFileColumn > 0)
        location += ":" + QString::number(projectFileColumn);
    return location;
}

bool ProjectPart::belongsToProject(const ProjectExplorer::Project *project) const
{
    return belongsToProject(project ? project->projectFilePath() : Utils::FilePath());
}

bool ProjectPart::belongsToProject(const Utils::FilePath &project) const
{
    return topLevelProject == project;
}

QByteArray ProjectPart::readProjectConfigFile(const QString &projectConfigFile)
{
    QByteArray result;

    QFile f(projectConfigFile);
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream is(&f);
        result = is.readAll().toUtf8();
        f.close();
    }

    return result;
}

// TODO: Why do we keep the file *and* the resulting macros? Why do we read the file
//       in several places?
static Macros getProjectMacros(const RawProjectPart &rpp)
{
    Macros macros = rpp.projectMacros;
    if (!rpp.projectConfigFile.isEmpty())
        macros += Macro::toMacros(ProjectPart::readProjectConfigFile(rpp.projectConfigFile));
    return macros;
}

static HeaderPaths getHeaderPaths(const RawProjectPart &rpp,
                                  const RawProjectPartFlags &flags,
                                  const ProjectExplorer::ToolChainInfo &tcInfo)
{
    HeaderPaths headerPaths;

    // Prevent duplicate include paths.
    // TODO: Do this once when finalizing the raw project part?
    std::set<QString> seenPaths;
    for (const HeaderPath &p : std::as_const(rpp.headerPaths)) {
        const QString cleanPath = QDir::cleanPath(p.path);
        if (seenPaths.insert(cleanPath).second)
            headerPaths << HeaderPath(cleanPath, p.type);
    }

    if (tcInfo.headerPathsRunner) {
        const HeaderPaths builtInHeaderPaths = tcInfo.headerPathsRunner(
                    flags.commandLineFlags, tcInfo.sysRootPath, tcInfo.targetTriple);
        for (const HeaderPath &header : builtInHeaderPaths) {
            // Prevent projects from adding built-in paths as user paths.
            erase_if(headerPaths, [&header](const HeaderPath &existing) {
                return header.path == existing.path;
            });
            headerPaths.push_back(HeaderPath{header.path, header.type});
        }
    }
    return headerPaths;
}

static ToolChain::MacroInspectionReport getToolchainMacros(
        const RawProjectPartFlags &flags, const ToolChainInfo &tcInfo, Utils::Language language)
{
    ToolChain::MacroInspectionReport report;
    if (tcInfo.macroInspectionRunner) {
        report = tcInfo.macroInspectionRunner(flags.commandLineFlags);
    } else if (language == Utils::Language::C) { // No compiler set in kit.
        report.languageVersion = Utils::LanguageVersion::LatestC;
    } else {
        report.languageVersion = Utils::LanguageVersion::LatestCxx;
    }
    return report;
}

static QStringList getIncludedFiles(const RawProjectPart &rpp, const RawProjectPartFlags &flags)
{
    return !rpp.includedFiles.isEmpty() ? rpp.includedFiles : flags.includedFiles;
}

ProjectPart::ProjectPart(const Utils::FilePath &topLevelProject,
                         const RawProjectPart &rpp,
                         const QString &displayName,
                         const ProjectFiles &files,
                         Utils::Language language,
                         Utils::LanguageExtensions languageExtensions,
                         const RawProjectPartFlags &flags,
                         const ToolChainInfo &tcInfo)
    : topLevelProject(topLevelProject),
      displayName(displayName),
      projectFile(rpp.projectFile),
      projectConfigFile(rpp.projectConfigFile),
      projectFileLine(rpp.projectFileLine),
      projectFileColumn(rpp.projectFileColumn),
      callGroupId(rpp.callGroupId),
      language(language),
      languageExtensions(languageExtensions | flags.languageExtensions),
      qtVersion(rpp.qtVersion),
      files(files),
      includedFiles(getIncludedFiles(rpp, flags)),
      precompiledHeaders(rpp.precompiledHeaders),
      headerPaths(getHeaderPaths(rpp, flags, tcInfo)),
      projectMacros(getProjectMacros(rpp)),
      buildSystemTarget(rpp.buildSystemTarget),
      buildTargetType(rpp.buildTargetType),
      selectedForBuilding(rpp.selectedForBuilding),
      toolchainType(tcInfo.type),
      isMsvc2015Toolchain(tcInfo.isMsvc2015ToolChain),
      toolChainTargetTriple(tcInfo.targetTriple),
      targetTripleIsAuthoritative(tcInfo.targetTripleIsAuthoritative),
      toolChainAbi(tcInfo.abi),
      toolChainInstallDir(tcInfo.installDir),
      compilerFilePath(tcInfo.compilerFilePath),
      warningFlags(flags.warningFlags),
      extraCodeModelFlags(tcInfo.extraCodeModelFlags),
      compilerFlags(flags.commandLineFlags),
      m_macroReport(getToolchainMacros(flags, tcInfo, language)),
      languageFeatures(deriveLanguageFeatures())
{
}

CPlusPlus::LanguageFeatures ProjectPart::deriveLanguageFeatures() const
{
    const bool hasCxx = languageVersion >= Utils::LanguageVersion::CXX98;
    const bool hasQt = hasCxx && qtVersion != Utils::QtMajorVersion::None;
    CPlusPlus::LanguageFeatures features;
    features.cxx11Enabled = languageVersion >= Utils::LanguageVersion::CXX11;
    features.cxx14Enabled = languageVersion >= Utils::LanguageVersion::CXX14;
    features.cxx17Enabled = languageVersion >= Utils::LanguageVersion::CXX17;
    features.cxx20Enabled = languageVersion >= Utils::LanguageVersion::CXX20;
    features.cxxEnabled = hasCxx;
    features.c99Enabled = languageVersion >= Utils::LanguageVersion::C99;
    features.objCEnabled = languageExtensions.testFlag(Utils::LanguageExtension::ObjectiveC);
    features.qtEnabled = hasQt;
    features.qtMocRunEnabled = hasQt;
    if (!hasQt) {
        features.qtKeywordsEnabled = false;
    } else {
        features.qtKeywordsEnabled = !Utils::contains(projectMacros,
                [] (const ProjectExplorer::Macro &macro) { return macro.key == "QT_NO_KEYWORDS"; });
    }
    return features;
}

} // namespace CppEditor
