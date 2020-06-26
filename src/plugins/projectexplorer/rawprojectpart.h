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

#pragma once

#include "buildtargettype.h"
#include "headerpath.h"
#include "projectexplorer_export.h"
#include "projectmacro.h"

// this include style is forced for the cpp unit test mocks
#include <projectexplorer/toolchain.h>

#include <utils/cpplanguage_details.h>
#include <utils/environment.h>
#include <utils/fileutils.h>

#include <QPointer>

#include <functional>

namespace ProjectExplorer {

class Kit;
class Project;

class PROJECTEXPLORER_EXPORT RawProjectPartFlags
{
public:
    RawProjectPartFlags() = default;
    RawProjectPartFlags(const ToolChain *toolChain, const QStringList &commandLineFlags);

public:
    QStringList commandLineFlags;
    // The following are deduced from commandLineFlags.
    Utils::WarningFlags warningFlags = Utils::WarningFlags::Default;
    Utils::LanguageExtensions languageExtensions = Utils::LanguageExtension::None;
};

class PROJECTEXPLORER_EXPORT RawProjectPart
{
public:
    void setDisplayName(const QString &displayName);

    void setProjectFileLocation(const QString &projectFile, int line = -1, int column = -1);
    void setConfigFileName(const QString &configFileName);
    void setCallGroupId(const QString &id);

    // FileIsActive and GetMimeType must be thread-safe.
    using FileIsActive = std::function<bool(const QString &filePath)>;
    using GetMimeType = std::function<QString(const QString &filePath)>;
    void setFiles(const QStringList &files,
                  const FileIsActive &fileIsActive = {},
                  const GetMimeType &getMimeType = {});
    static HeaderPath frameworkDetectionHeuristic(const HeaderPath &header);
    void setHeaderPaths(const HeaderPaths &headerPaths);
    void setIncludePaths(const QStringList &includePaths);
    void setPreCompiledHeaders(const QStringList &preCompiledHeaders);

    void setBuildSystemTarget(const QString &target);
    void setBuildTargetType(BuildTargetType type);
    void setSelectedForBuilding(bool yesno);

    void setFlagsForC(const RawProjectPartFlags &flags);
    void setFlagsForCxx(const RawProjectPartFlags &flags);

    void setMacros(const Macros &macros);
    void setQtVersion(Utils::QtVersion qtVersion);

public:
    QString displayName;

    QString projectFile;
    int projectFileLine = -1;
    int projectFileColumn = -1;
    QString callGroupId;

    // Files
    QStringList files;
    FileIsActive fileIsActive;
    GetMimeType getMimeType;
    QStringList precompiledHeaders;
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
    Utils::QtVersion qtVersion = Utils::QtVersion::Unknown;
};

using RawProjectParts = QVector<RawProjectPart>;

class PROJECTEXPLORER_EXPORT KitInfo
{
public:
    explicit KitInfo(Kit *kit);

    bool isValid() const;

    Kit *kit = nullptr;
    ToolChain *cToolChain = nullptr;
    ToolChain *cxxToolChain = nullptr;

    Utils::QtVersion projectPartQtVersion = Utils::QtVersion::None;

    QString sysRootPath;
};

class PROJECTEXPLORER_EXPORT ToolChainInfo
{
public:
    ToolChainInfo() = default;
    ToolChainInfo(const ProjectExplorer::ToolChain *toolChain,
                  const QString &sysRootPath,
                  const Utils::Environment &env);

    bool isValid() const { return type.isValid(); }

public:
    Utils::Id type;
    bool isMsvc2015ToolChain = false;
    unsigned wordWidth = 0;
    QString targetTriple;
    Utils::FilePath installDir;
    QStringList extraCodeModelFlags;

    QString sysRootPath; // For headerPathsRunner.
    ProjectExplorer::ToolChain::BuiltInHeaderPathsRunner headerPathsRunner;
    ProjectExplorer::ToolChain::MacroInspectionRunner macroInspectionRunner;
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
    bool isValid() const;

public:
    QPointer<Project> project;
    RawProjectParts rawProjectParts;
    RppGenerator rppGenerator;

    const ToolChain *cToolChain = nullptr;
    const ToolChain *cxxToolChain = nullptr;

    ToolChainInfo cToolChainInfo;
    ToolChainInfo cxxToolChainInfo;
};

} // namespace ProjectExplorer
