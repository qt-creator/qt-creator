// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeconfigitem.h"

#include "fileapidataextractor.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectmacro.h>

#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>

#include <QString>
#include <QVector>
#include <QVersionNumber>

#include <vector>

QT_BEGIN_NAMESPACE
template <typename Ret>
class QPromise;
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

    QVector<ReplyObject> replies;
    QVersionNumber cmakeVersion;

    Utils::FilePath jsonFile(const QString &kind, const Utils::FilePath &replyDir) const;
};

class Directory
{
public:
    QString buildPath;
    QString sourcePath;
    int parent = -1;
    int project = -1;
    std::vector<int> children;
    std::vector<int> targets;
    bool hasInstallRule = false;
};

class Project
{
public:
    QString name;
    int parent = -1;
    std::vector<int> children;
    std::vector<int> directories;
    std::vector<int> targets;
};

class Target
{
public:
    // From codemodel file:
    QString name;
    QString id;
    int directory = -1;
    int project = -1;
    QString jsonFile;
};

class Configuration
{
public:
    QString name;
    std::vector<Directory> directories;
    std::vector<Project> projects;
    std::vector<Target> targets;
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
    QString path;
    int compileGroup = -1;
    int sourceGroup = -1;
    int backtrace = -1;
    bool isGenerated = false;
};

class IncludeInfo
{
public:
    ProjectExplorer::HeaderPath path;
    int backtrace;
};

class DefineInfo
{
public:
    ProjectExplorer::Macro define;
    int backtrace;
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
    QList<Utils::FilePath> artifacts;
    QString installPrefix;
    std::vector<InstallDestination> installDestination;
    std::optional<LinkInfo> link;
    std::optional<ArchiveInfo> archive;
    std::vector<DependencyInfo> dependencies;
    std::vector<SourceInfo> sources;
    std::vector<QString> sourceGroups;
    std::vector<CompileInfo> compileGroups;
    BacktraceInfo backtraceGraph;
};

} // namespace FileApiDetails

class FileApiData
{
public:
    FileApiDetails::ReplyFileContents replyFile;
    CMakeConfig cache;
    std::vector<CMakeFileInfo> cmakeFiles;
    FileApiDetails::Configuration codemodel;
    std::vector<FileApiDetails::TargetDetails> targetDetails;
};

class FileApiParser
{
public:
    static FileApiData parseData(QPromise<std::shared_ptr<FileApiQtcData>> &promise,
                                 const Utils::FilePath &replyFilePath,
                                 const QString &cmakeBuildType,
                                 QString &errorMessage);

    static bool setupCMakeFileApi(const Utils::FilePath &buildDirectory);

    static Utils::FilePath cmakeReplyDirectory(const Utils::FilePath &buildDirectory);
    static Utils::FilePaths cmakeQueryFilePaths(const Utils::FilePath &buildDirectory);

    static Utils::FilePath scanForCMakeReplyFile(const Utils::FilePath &buildDirectory);
};

} // CMakeProjectManager::Internal
