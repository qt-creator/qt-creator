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
#include "projectexplorer_global.h"
#include "projectmacro.h"

#include <utils/cpplanguage_details.h>

#include <functional>

namespace ProjectExplorer {

class ToolChain;

class PROJECTEXPLORER_EXPORT RawProjectPartFlags
{
public:
    RawProjectPartFlags() = default;
    RawProjectPartFlags(const ToolChain *toolChain, const QStringList &commandLineFlags);

public:
    QStringList commandLineFlags;
    // The following are deduced from commandLineFlags.
    WarningFlags warningFlags = WarningFlags::Default;
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

} // namespace ProjectExplorer
