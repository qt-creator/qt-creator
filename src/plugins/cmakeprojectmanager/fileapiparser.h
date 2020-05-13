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

#pragma once

#include "cmakeconfigitem.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectmacro.h>

#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QString>
#include <QVector>

#include <vector>

namespace CMakeProjectManager {
namespace Internal {

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
    QString cmakeExecutable;
    QString cmakeRoot;

    QVector<ReplyObject> replies;

    QString jsonFile(const QString &kind, const QDir &replyDir) const;
};

class CMakeFileInfo
{
public:
    QString path;
    bool isCMake = false;
    bool isCMakeListsDotTxt = false;
    bool isExternal = false;
    bool isGenerated = false;
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
    Utils::optional<LinkInfo> link;
    Utils::optional<ArchiveInfo> archive;
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
    std::vector<FileApiDetails::CMakeFileInfo> cmakeFiles;
    std::vector<FileApiDetails::Configuration> codemodel;
    std::vector<FileApiDetails::TargetDetails> targetDetails;
};

class FileApiParser
{
public:
    static FileApiData parseData(const QFileInfo &replyFileInfo, QString &errorMessage);

    static bool setupCMakeFileApi(const Utils::FilePath &buildDirectory,
                                  Utils::FileSystemWatcher &watcher);

    static QStringList cmakeQueryFilePaths(const Utils::FilePath &buildDirectory);

    static QFileInfo scanForCMakeReplyFile(const Utils::FilePath &buildDirectory);
};

} // namespace Internal
} // namespace CMakeProjectManager
