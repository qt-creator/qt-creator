// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildtargettype.h"
#include "headerpath.h"
#include "projectexplorer_export.h"
#include "projectmacro.h"

// this include style is forced for the cpp unit test mocks
#include <projectexplorer/toolchain.h>

#include <utils/cpplanguage_details.h>
#include <utils/environment.h>
#include <utils/store.h>

#include <functional>

namespace ProjectExplorer {

class BuildSystem;
class Kit;
class Project;

void PROJECTEXPLORER_EXPORT addTargetFlagForIos(
        QStringList &cFlags,
        QStringList &cxxFlags,
        const BuildSystem *bs,
        const std::function<QString()> &getDeploymentTarget
        );

class PROJECTEXPLORER_EXPORT RawProjectPartFlags
{
public:
    RawProjectPartFlags() = default;
    RawProjectPartFlags(const Toolchain *toolChain, const QStringList &commandLineFlags,
                        const Utils::FilePath &includeFileBaseDir);

public:
    QStringList commandLineFlags;
    // The following are deduced from commandLineFlags.
    Utils::WarningFlags warningFlags = Utils::WarningFlags::Default;
    Utils::LanguageExtensions languageExtensions = Utils::LanguageExtension::None;
    Utils::FilePaths includedFiles;
};

class PROJECTEXPLORER_EXPORT RawProjectPart
{
public:
    void setDisplayName(const QString &displayName);

    void setProjectFileLocation(const Utils::FilePath &projectFile, int line = -1, int column = -1);
    void setConfigFileName(const QString &configFileName);
    void setCallGroupId(const QString &id);

    // FileIsActive and GetMimeType must be thread-safe.
    using FileIsActive = std::function<bool(const Utils::FilePath &filePath)>;
    using GetMimeType = std::function<QString(const Utils::FilePath &filePath)>;
    void setFiles(const Utils::FilePaths &files,
                  const FileIsActive &fileIsActive = {},
                  const GetMimeType &getMimeType = {});
    static HeaderPath frameworkDetectionHeuristic(const HeaderPath &header);
    void setHeaderPaths(const HeaderPaths &headerPaths);
    void setIncludePaths(const Utils::FilePaths &includePaths);
    void setPreCompiledHeaders(const Utils::FilePaths &preCompiledHeaders);
    void setIncludedFiles(const Utils::FilePaths &files);

    void setBuildSystemTarget(const QString &target);
    void setBuildTargetType(BuildTargetType type);
    void setSelectedForBuilding(bool yesno);

    void setFlagsForC(const RawProjectPartFlags &flags);
    void setFlagsForCxx(const RawProjectPartFlags &flags);

    void setMacros(const Macros &macros);
    void setQtVersion(Utils::QtMajorVersion qtVersion);

public:
    QString displayName;

    Utils::FilePath projectFile;
    int projectFileLine = -1;
    int projectFileColumn = -1;
    QString callGroupId;

    // Files
    Utils::FilePaths files;
    FileIsActive fileIsActive;
    GetMimeType getMimeType;
    Utils::FilePaths precompiledHeaders;
    Utils::FilePaths includedFiles;
    HeaderPaths headerPaths;
    QString projectConfigFile; // Generic Project Manager only

    // Build system
    QString buildSystemTarget;
    BuildTargetType buildTargetType = BuildTargetType::Unknown;
    bool selectedForBuilding = true;

    // Flags
    RawProjectPartFlags flagsForC;
    RawProjectPartFlags flagsForCxx;

    // Misc
    Macros projectMacros;
    Utils::QtMajorVersion qtVersion = Utils::QtMajorVersion::Unknown;
};

using RawProjectParts = QList<RawProjectPart>;

class PROJECTEXPLORER_EXPORT KitInfo
{
public:
    explicit KitInfo(Kit *kit);

    bool isValid() const;

    Kit *kit = nullptr;
    Toolchain *cToolchain = nullptr;
    Toolchain *cxxToolchain = nullptr;

    Utils::QtMajorVersion projectPartQtVersion = Utils::QtMajorVersion::None;

    Utils::FilePath sysRootPath;
};

class PROJECTEXPLORER_EXPORT ToolchainInfo
{
public:
    ToolchainInfo() = default;
    ToolchainInfo(const ProjectExplorer::Toolchain *toolChain,
                  const Utils::FilePath &sysRootPath,
                  const Utils::Environment &env);

    bool isValid() const { return type.isValid(); }

public:
    Utils::Id type;
    bool isMsvc2015Toolchain = false;
    bool targetTripleIsAuthoritative = false;
    Abi abi;
    QString targetTriple;
    Utils::FilePath compilerFilePath;
    Utils::FilePath installDir;
    QStringList extraCodeModelFlags;

    Utils::FilePath sysRootPath; // For headerPathsRunner.
    ProjectExplorer::Toolchain::BuiltInHeaderPathsRunner headerPathsRunner;
    ProjectExplorer::Toolchain::MacroInspectionRunner macroInspectionRunner;
};

class PROJECTEXPLORER_EXPORT ProjectUpdateInfo
{
public:
    using RppGenerator = std::function<RawProjectParts()>;

    ProjectUpdateInfo() = default;
    ProjectUpdateInfo(Project *project,
                      const KitInfo &kitInfo,
                      const Utils::Environment &env,
                      const RawProjectParts &rawProjectParts,
                      const RppGenerator &rppGenerator = {});

public:
    QString projectName;
    Utils::FilePath projectFilePath;
    Utils::FilePath buildRoot;
    RawProjectParts rawProjectParts;
    RppGenerator rppGenerator;
    Utils::Store cppSettings;

    ToolchainInfo cToolchainInfo;
    ToolchainInfo cxxToolchainInfo;
};

using CppSettingsRetriever = std::function<Utils::Store(const Project *)>;
void PROJECTEXPLORER_EXPORT provideCppSettingsRetriever(const CppSettingsRetriever &retriever);

} // namespace ProjectExplorer
