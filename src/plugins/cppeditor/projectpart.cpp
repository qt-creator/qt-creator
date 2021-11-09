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
    return project ? topLevelProject == project->projectFilePath() : !hasProject();
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
    for (const HeaderPath &p : qAsConst(rpp.headerPaths)) {
        const QString cleanPath = QDir::cleanPath(p.path);
        if (seenPaths.insert(cleanPath).second)
            headerPaths << HeaderPath(cleanPath, p.type);
    }

    if (tcInfo.headerPathsRunner) {
        const HeaderPaths builtInHeaderPaths = tcInfo.headerPathsRunner(
                    flags.commandLineFlags, tcInfo.sysRootPath, tcInfo.targetTriple);
        for (const HeaderPath &header : builtInHeaderPaths) {
            if (seenPaths.insert(header.path).second)
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
      includedFiles(rpp.includedFiles),
      precompiledHeaders(rpp.precompiledHeaders),
      headerPaths(getHeaderPaths(rpp, flags, tcInfo)),
      projectMacros(getProjectMacros(rpp)),
      buildSystemTarget(rpp.buildSystemTarget),
      buildTargetType(rpp.buildTargetType),
      selectedForBuilding(rpp.selectedForBuilding),
      toolchainType(tcInfo.type),
      isMsvc2015Toolchain(tcInfo.isMsvc2015ToolChain),
      toolChainTargetTriple(tcInfo.targetTriple),
      toolChainWordWidth(tcInfo.wordWidth == 64 ? ProjectPart::WordWidth64Bit
                                                : ProjectPart::WordWidth32Bit),
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
    const bool hasQt = hasCxx && qtVersion != Utils::QtVersion::None;
    CPlusPlus::LanguageFeatures features;
    features.cxx11Enabled = languageVersion >= Utils::LanguageVersion::CXX11;
    features.cxx14Enabled = languageVersion >= Utils::LanguageVersion::CXX14;
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
