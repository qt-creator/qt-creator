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

#include "cpprawprojectpart.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>

#include <utils/algorithm.h>

namespace CppTools {

RawProjectPartFlags::RawProjectPartFlags(const ProjectExplorer::ToolChain *toolChain,
                                         const QStringList &commandLineFlags)
{
    // Keep the following cheap/non-blocking for the ui thread. Expensive
    // operations are encapsulated in ToolChainInfo as "runners".
    this->commandLineFlags = commandLineFlags;
    if (toolChain) {
        warningFlags = toolChain->warningFlags(commandLineFlags);
        languageExtensions = toolChain->languageExtensions(commandLineFlags);
    }
}

void RawProjectPart::setDisplayName(const QString &displayName)
{
    this->displayName = displayName;
}

void RawProjectPart::setFiles(const QStringList &files, const FileClassifier &fileClassifier)
{
    this->files = files;
    this->fileClassifier = fileClassifier;
}

static QString trimTrailingSlashes(const QString &path) {
    QString p = path;
    while (p.endsWith('/') && p.count() > 1) {
        p.chop(1);
    }
    return p;
}

ProjectExplorer::HeaderPath RawProjectPart::frameworkDetectionHeuristic(const ProjectExplorer::HeaderPath &header)
{
    QString path = trimTrailingSlashes(header.path);

    if (path.endsWith(".framework")) {
        path = path.left(path.lastIndexOf(QLatin1Char('/')));
        return {path, ProjectExplorer::HeaderPathType::Framework};
    }
    return header;
}

void RawProjectPart::setProjectFileLocation(const QString &projectFile, int line, int column)
{
    this->projectFile = projectFile;
    projectFileLine = line;
    projectFileColumn = column;
}

void RawProjectPart::setConfigFileName(const QString &configFileName)
{
    this->projectConfigFile = configFileName;
}

void RawProjectPart::setBuildSystemTarget(const QString &target)
{
    buildSystemTarget = target;
}

void RawProjectPart::setCallGroupId(const QString &id)
{
    callGroupId = id;
}

void RawProjectPart::setQtVersion(ProjectPart::QtVersion qtVersion)
{
    this->qtVersion = qtVersion;
}

void RawProjectPart::setMacros(const ProjectExplorer::Macros &macros)
{
    this->projectMacros = macros;
}

void RawProjectPart::setHeaderPaths(const ProjectExplorer::HeaderPaths &headerPaths)
{
    this->headerPaths = headerPaths;
}

void RawProjectPart::setIncludePaths(const QStringList &includePaths)
{
    this->headerPaths = Utils::transform<QVector>(includePaths, [](const QString &path) {
        ProjectExplorer::HeaderPath hp(path, ProjectExplorer::HeaderPathType::User);
        return RawProjectPart::frameworkDetectionHeuristic(hp);
    });
}

void RawProjectPart::setPreCompiledHeaders(const QStringList &preCompiledHeaders)
{
    this->precompiledHeaders = preCompiledHeaders;
}

void RawProjectPart::setSelectedForBuilding(bool yesno)
{
    this->selectedForBuilding = yesno;
}

void RawProjectPart::setFlagsForC(const RawProjectPartFlags &flags)
{
    flagsForC = flags;
}

void RawProjectPart::setFlagsForCxx(const RawProjectPartFlags &flags)
{
    flagsForCxx = flags;
}

void RawProjectPart::setBuildTargetType(ProjectPart::BuildTargetType type)
{
    buildTargetType = type;
}

} // namespace CppTools
