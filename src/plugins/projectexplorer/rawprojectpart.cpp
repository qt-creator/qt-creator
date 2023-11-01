// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rawprojectpart.h"

#include "abi.h"
#include "buildconfiguration.h"
#include "buildsystem.h"
#include "kitaspects.h"
#include "project.h"
#include "target.h"

#include <ios/iosconstants.h>

#include <utils/algorithm.h>

namespace ProjectExplorer {

RawProjectPartFlags::RawProjectPartFlags(const ToolChain *toolChain,
                                         const QStringList &commandLineFlags,
                                         const Utils::FilePath &includeFileBaseDir)
{
    // Keep the following cheap/non-blocking for the ui thread. Expensive
    // operations are encapsulated in ToolChainInfo as "runners".
    this->commandLineFlags = commandLineFlags;
    if (toolChain) {
        warningFlags = toolChain->warningFlags(commandLineFlags);
        languageExtensions = toolChain->languageExtensions(commandLineFlags);
        includedFiles = Utils::transform(toolChain->includedFiles(commandLineFlags, includeFileBaseDir),
                                         &Utils::FilePath::toFSPathString);
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

static QString trimTrailingSlashes(const QString &path)
{
    QString p = path;
    while (p.endsWith('/') && p.size() > 1) {
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

void RawProjectPart::setQtVersion(Utils::QtMajorVersion qtVersion)
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
    sysRootPath = SysRootKitAspect::sysRoot(kit);
}

bool KitInfo::isValid() const
{
    return kit;
}

ToolChainInfo::ToolChainInfo(const ToolChain *toolChain,
                             const Utils::FilePath &sysRootPath,
                             const Utils::Environment &env)
{
    if (toolChain) {
        // Keep the following cheap/non-blocking for the ui thread...
        type = toolChain->typeId();
        isMsvc2015ToolChain = toolChain->targetAbi().osFlavor() == Abi::WindowsMsvc2015Flavor;
        abi = toolChain->targetAbi();
        targetTriple = toolChain->effectiveCodeModelTargetTriple();
        targetTripleIsAuthoritative = !toolChain->explicitCodeModelTargetTriple().isEmpty();
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

// We do not get the -target flag from qmake or cmake on macOS; see QTCREATORBUG-28278.
void addTargetFlagForIos(QStringList &cFlags, QStringList &cxxFlags, const BuildSystem *bs,
                         const std::function<QString ()> &getDeploymentTarget)
{
    const Utils::Id deviceType = DeviceTypeKitAspect::deviceTypeId(bs->target()->kit());
    if (deviceType != Ios::Constants::IOS_DEVICE_TYPE
            && deviceType != Ios::Constants::IOS_SIMULATOR_TYPE) {
        return;
    }
    const bool isSim = deviceType == Ios::Constants::IOS_SIMULATOR_TYPE;
    QString targetTriple(QLatin1String(isSim ? "x86_64" : "arm64"));
    targetTriple.append("-apple-ios").append(getDeploymentTarget());
    if (isSim)
        targetTriple.append("-simulator");
    const auto addTargetFlag = [&targetTriple](QStringList &flags) {
        if (!flags.contains("-target") && !Utils::contains(flags,
                    [](const QString &flag) { return flag.startsWith("--target="); })) {
            flags << "-target" << targetTriple;
        }
    };
    addTargetFlag(cxxFlags);
    addTargetFlag(cFlags);
}

} // namespace ProjectExplorer
