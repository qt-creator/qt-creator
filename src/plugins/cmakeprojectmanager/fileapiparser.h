// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeconfigitem.h"

#include "fileapidataextractor.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectmacro.h>

#include <QVersionNumber>

#include <vector>

QT_BEGIN_NAMESPACE
template <typename Ret>
class QFuture;
QT_END_NAMESPACE

namespace CMakeProjectManager::Internal {

namespace FileApiDetails {

class ReplyObject
{
public:
    QString kind;
    QString file;
    std::pair<int, int> version;
};

class ReplyFileContents
{
public:
    QString generator;
    bool isMultiConfig = false;
    QString cmakeExecutable;
    QString ctestExecutable;
    QString cmakeRoot;

    QList<ReplyObject> replies;
    QVersionNumber cmakeVersion;

    Utils::FilePath jsonFile(const QString &kind, const Utils::FilePath &replyDir) const;
};

struct PathPair
{
    QString from;
    QString to;
};

class Installer
{
public:
    QString component;
    QString destination;
    std::vector<PathPair> paths;
    QString type;
    bool isExcludeFromAll = false;
    bool isForAllComponents = false;
    bool isOptional = false;
    QString targetId;
    int targetIndex = -1;
    bool targetIsImportLibrary = false;
    QString targetInstallNamelink;
    QString exportName;
    struct ExportTarget {
        QString id;
        int index;
    };
    std::vector<ExportTarget> exportTargets;
    QString runtimeDependencySetName;
    QString runtimeDependencySetType;
    QString fileSetName;
    QString fileSetType;
    QStringList fileSetDirectories;
    struct FileSetTarget {
        QString id;
        int index;
    };
    FileSetTarget fileSetTarget;
    struct CxxModuleBmiTarget {
        QString id;
        int index;
    };
    CxxModuleBmiTarget cxxModuleBmiTarget;
    QString scriptFile;
    int backtrace = -1;
};

class BacktraceNode
{
public:
    int file = -1;
    int line = -1;
    int command = -1;
    int parent = -1;
};

class BacktraceInfo
{
public:
    std::vector<QString> commands;
    std::vector<QString> files;
    std::vector<BacktraceNode> nodes;
};

class DirectoryInfo
{
public:
    QString buildPath;
    QString sourcePath;
    int parent = -1;
    int project = -1;
    std::vector<int> children;
    std::vector<int> targets;
    bool hasInstallRule = false;
    int codemodelVersionMajor = 2;
    int codemodelVersionMinor = 0;
    QString jsonFile;
    std::vector<Installer> installers;
    BacktraceInfo backtraceGraph;
};

class ProjectInfo
{
public:
    QString name;
    int parent = -1;
    std::vector<int> children;
    std::vector<int> directories;
    std::vector<int> targets;
    std::vector<int> abstractTargets;
};

class TargetInfo
{
public:
    // From codemodel file:
    QString name;
    QString id;
    int directory = -1;
    int project = -1;
    QString jsonFile;
    bool imported = false;
    bool local = false;
    bool abstract = false;
    bool symbolic = false;
};

class ConfigurationInfo
{
public:
    QString name;
    std::vector<DirectoryInfo> directories;
    std::vector<ProjectInfo> projects;
    std::vector<TargetInfo> targets;
    std::vector<TargetInfo> abstractTargets;
};

class DirectoryDetails
{
public:
    int codemodelVersionMajor = 2;
    int codemodelVersionMinor = 0;
    QString sourcePath;
    QString buildPath;
    std::vector<Installer> installers;
    BacktraceInfo backtraceGraph;
};

class InstallDestination
{
public:
    QString path;
    int backtrace;
};

class FragmentInfo
{
public:
    QString fragment;
    QString role;
};

class LinkInfo
{
public:
    QString language;
    std::vector<FragmentInfo> fragments;
    bool isLto = false;
    QString sysroot;
};

class ArchiveInfo
{
public:
    std::vector<FragmentInfo> fragments;
    bool isLto = false;
};

class DependencyInfo
{
public:
    QString targetId;
    int backtrace;
};

class SourceInfo
{
public:
    Utils::FilePath path;
    int compileGroup = -1;
    int sourceGroup = -1;
    int backtrace = -1;
    bool isGenerated = false;
};

class IncludeInfo
{
public:
    ProjectExplorer::HeaderPath path;
    int backtrace = 0;
};

class DefineInfo
{
public:
    ProjectExplorer::Macro define;
    int backtrace = 0;
};

class CompileInfo
{
public:
    std::vector<int> sources;
    QString language;
    QStringList fragments;
    std::vector<IncludeInfo> includes;
    std::vector<DefineInfo> defines;
    QString sysroot;
};

class ToolchainCompiler
{
public:
    QString path;
    QString id;
    QString version;
    QString target;
    struct Implicit {
        QStringList includeDirectories;
        QStringList linkDirectories;
        QStringList linkFrameworkDirectories;
        QStringList linkLibraries;
    } implicit;
};

class Toolchain
{
public:
    QString language;
    ToolchainCompiler compiler;
    QStringList sourceFileExtensions;
};

class ConfigureLog
{
public:
    QString path;
    QStringList eventKindNames;
};

class FileSetInfo
{
public:
    QString name;
    QString type;
    QString visibility;
    QStringList baseDirectories;
};

class PrecompileHeaderInfo
{
public:
    QString header;
    int backtrace;
};

class DebuggerInfo
{
public:
    QString workingDirectory;
};

class TargetDetails
{
public:
    QString name;
    QString id;
    QString type;
    QString folderTargetProperty;
    Utils::FilePath sourceDir;
    Utils::FilePath buildDir;
    int backtrace = -1;
    bool isGeneratorProvided = false;
    QString nameOnDisk;
    Utils::FilePaths artifacts;
    QString installPrefix;
    std::vector<InstallDestination> installDestination;
    QList<ProjectExplorer::LauncherInfo> launcherInfos;
    std::optional<LinkInfo> link;
    std::optional<ArchiveInfo> archive;
    std::vector<DependencyInfo> dependencies;
    std::vector<DependencyInfo> linkLibraries;
    std::vector<DependencyInfo> interfaceLinkLibraries;
    std::vector<DependencyInfo> compileDependencies;
    std::vector<DependencyInfo> interfaceCompileDependencies;
    std::vector<DependencyInfo> objectDependencies;
    std::vector<DependencyInfo> orderDependencies;
    std::vector<SourceInfo> sources;
    std::vector<QString> sourceGroups;
    std::vector<CompileInfo> compileGroups;
    BacktraceInfo backtraceGraph;
    bool imported = false;
    bool local = false;
    bool abstract = false;
    bool symbolic = false;
    DebuggerInfo debugger;
    std::vector<FileSetInfo> fileSets;
    std::vector<PrecompileHeaderInfo> precompileHeaders;
};

} // namespace FileApiDetails

class FileApiData
{
public:
    FileApiDetails::ReplyFileContents replyFile;
    CMakeConfig cache;
    std::vector<CMakeFileInfo> cmakeFiles;
    FileApiDetails::ConfigurationInfo codemodel;
    std::vector<FileApiDetails::TargetDetails> targetDetails;
    std::vector<FileApiDetails::DirectoryDetails> directoryDetails;
    std::vector<FileApiDetails::Toolchain> toolchains;
    FileApiDetails::ConfigureLog configureLog;
};

class FileApiParser
{
public:
    static FileApiData parseData(const QFuture<void> &future,
                                 const Utils::FilePath &replyFilePath,
                                 const Utils::FilePath &buildDir,
                                 const QString &cmakeBuildType,
                                 QString &errorMessage);

    static bool setupCMakeFileApi(const Utils::FilePath &buildDirectory);

    static Utils::FilePath cmakeReplyDirectory(const Utils::FilePath &buildDirectory);
    static Utils::FilePaths cmakeQueryFilePaths(const Utils::FilePath &buildDirectory);

    static Utils::FilePath scanForCMakeReplyFile(const Utils::FilePath &buildDirectory);
};

} // CMakeProjectManager::Internal
