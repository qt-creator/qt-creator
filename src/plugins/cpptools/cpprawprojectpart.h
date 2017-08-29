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

#pragma once

#include "cpptools_global.h"
#include "projectpart.h"

#include <projectexplorer/toolchain.h>

#include <functional>

namespace CppTools {

class CPPTOOLS_EXPORT RawProjectPartFlags
{
public:
    RawProjectPartFlags() = default;
    RawProjectPartFlags(const ProjectExplorer::ToolChain *toolChain,
                        const QStringList &commandLineFlags);

public:
    QStringList commandLineFlags;
    // The following are deduced from commandLineFlags.
    ProjectExplorer::WarningFlags warningFlags = ProjectExplorer::WarningFlags::Default;
    ProjectExplorer::ToolChain::CompilerFlags compilerFlags
        = ProjectExplorer::ToolChain::CompilerFlag::NoFlags;
};

class CPPTOOLS_EXPORT RawProjectPart
{
public:
    RawProjectPart() {}

    void setDisplayName(const QString &displayName);

    // FileClassifier must be thread-safe.
    using FileClassifier = std::function<ProjectFile::Kind (const QString &filePath)>;
    void setFiles(const QStringList &files, FileClassifier fileClassifier = FileClassifier());

    void setProjectFileLocation(const QString &projectFile, int line = -1, int column = -1);
    void setConfigFileName(const QString &configFileName);
    void setCallGroupId(const QString &id);
    void setBuildSystemTarget(const QString &target);

    void setQtVersion(ProjectPart::QtVersion qtVersion);

    void setMacros(const ProjectExplorer::Macros &macros);
    void setHeaderPaths(const ProjectPartHeaderPaths &headerPaths);
    void setIncludePaths(const QStringList &includePaths);

    void setPreCompiledHeaders(const QStringList &preCompiledHeaders);

    void setSelectedForBuilding(bool yesno);

    void setFlagsForC(const RawProjectPartFlags &flags);
    void setFlagsForCxx(const RawProjectPartFlags &flags);

    void setBuildTargetType(ProjectPart::BuildTargetType type);
public:
    QString displayName;
    QString projectFile;
    int projectFileLine = -1;
    int projectFileColumn = -1;
    QString projectConfigFile; // currently only used by the Generic Project Manager
    QString callGroupId;
    QString buildSystemTarget;
    QStringList precompiledHeaders;
    ProjectPartHeaderPaths headerPaths;
    ProjectExplorer::Macros projectMacros;
    ProjectPart::QtVersion qtVersion = ProjectPart::UnknownQt;
    bool selectedForBuilding = true;

    RawProjectPartFlags flagsForC;
    RawProjectPartFlags flagsForCxx;

    QStringList files;
    FileClassifier fileClassifier;
    ProjectPart::BuildTargetType buildTargetType = ProjectPart::BuildTargetType::Unknown;
};

using RawProjectParts = QVector<RawProjectPart>;

} // namespace CppTools
