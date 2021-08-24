/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "rawprojectpart.h"

#include "abi.h"
#include "buildconfiguration.h"
#include "kitinformation.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "target.h"

#include <utils/algorithm.h>

namespace ProjectExplorer {

RawProjectPartFlags::RawProjectPartFlags(const ToolChain *toolChain,
                                         const QStringList &commandLineFlags,
                                         const QString &includeFileBaseDir)
{
    // Keep the following cheap/non-blocking for the ui thread. Expensive
    // operations are encapsulated in ToolChainInfo as "runners".
    this->commandLineFlags = commandLineFlags;
    if (toolChain) {
        warningFlags = toolChain->warningFlags(commandLineFlags);
        languageExtensions = toolChain->languageExtensions(commandLineFlags);
        includedFiles = toolChain->includedFiles(commandLineFlags, includeFileBaseDir);
    }
}

void RawProjectPart::setDisplayName(const QString &displayName)
{
    this->displayName = displayName;
}

void RawProjectPart::setFiles(const QStringList &files,
                              const FileIsActive &fileIsActive,
                              const GetMimeType &getMimeType)
{
    this->files = files;
    this->fileIsActive = fileIsActive;
    this->getMimeType = getMimeType;
}

static QString trimTrailingSlashes(const QString &path) {
    QString p = path;
    while (p.endsWith('/') && p.count() > 1) {
        p.chop(1);
    }
    return p;
}

HeaderPath RawProjectPart::frameworkDetectionHeuristic(const HeaderPath &header)
{
    QString path = trimTrailingSlashes(header.path);

    if (path.endsWith(".framework"))
        return HeaderPath::makeFramework(path.left(path.lastIndexOf('/')));
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

void RawProjectPart::setQtVersion(Utils::QtVersion qtVersion)
{
    this->qtVersion = qtVersion;
}

void RawProjectPart::setMacros(const Macros &macros)
{
    this->projectMacros = macros;
}

void RawProjectPart::setHeaderPaths(const HeaderPaths &headerPaths)
{
    this->headerPaths = headerPaths;
}

void RawProjectPart::setIncludePaths(const QStringList &includePaths)
{
    this->headerPaths = Utils::transform<QVector>(includePaths, [](const QString &path) {
        return RawProjectPart::frameworkDetectionHeuristic(HeaderPath::makeUser(path));
    });
}

void RawProjectPart::setPreCompiledHeaders(const QStringList &preCompiledHeaders)
{
    this->precompiledHeaders = preCompiledHeaders;
}

void RawProjectPart::setIncludedFiles(const QStringList &files)
{
     includedFiles = files;
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

void RawProjectPart::setBuildTargetType(BuildTargetType type)
{
    buildTargetType = type;
}

KitInfo::KitInfo(Kit *kit)
    : kit(kit)
{
    // Toolchains
    if (kit) {
        cToolChain = ToolChainKitAspect::cToolChain(kit);
        cxxToolChain = ToolChainKitAspect::cxxToolChain(kit);
    }

    // Sysroot
    sysRootPath = SysRootKitAspect::sysRoot(kit).toString();
}

bool KitInfo::isValid() const
{
    return kit;
}

ToolChainInfo::ToolChainInfo(const ToolChain *toolChain,
                             const QString &sysRootPath,
                             const Utils::Environment &env)
{
    if (toolChain) {
        // Keep the following cheap/non-blocking for the ui thread...
        type = toolChain->typeId();
        isMsvc2015ToolChain = toolChain->targetAbi().osFlavor() == Abi::WindowsMsvc2015Flavor;
        wordWidth = toolChain->targetAbi().wordWidth();
        targetTriple = toolChain->originalTargetTriple();
        extraCodeModelFlags = toolChain->extraCodeModelFlags();
        installDir = toolChain->installDir();
        compilerFilePath = toolChain->compilerCommand();

        // ...and save the potentially expensive operations for later so that
        // they can be run from a worker thread.
        this->sysRootPath = sysRootPath;
        headerPathsRunner = toolChain->createBuiltInHeaderPathsRunner(env);
        macroInspectionRunner = toolChain->createMacroInspectionRunner();
    }
}

ProjectUpdateInfo::ProjectUpdateInfo(Project *project,
                                     const KitInfo &kitInfo,
                                     const Utils::Environment &env,
                                     const RawProjectParts &rawProjectParts,
                                     const RppGenerator &rppGenerator)
    : rawProjectParts(rawProjectParts)
    , rppGenerator(rppGenerator)
    , cToolChainInfo(ToolChainInfo(kitInfo.cToolChain, kitInfo.sysRootPath, env))
    , cxxToolChainInfo(ToolChainInfo(kitInfo.cxxToolChain, kitInfo.sysRootPath, env))
{
    if (project) {
        projectName = project->displayName();
        projectFilePath = project->projectFilePath();
        if (project->activeTarget() && project->activeTarget()->activeBuildConfiguration())
            buildRoot = project->activeTarget()->activeBuildConfiguration()->buildDirectory();
    }
}

} // namespace ProjectExplorer
