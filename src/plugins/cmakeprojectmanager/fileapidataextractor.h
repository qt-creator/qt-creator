// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakebuildtarget.h"
#include "cmakeprojectnodes.h"

#include <projectexplorer/rawprojectpart.h>

#include <utils/fileutils.h>

#include <QList>
#include <QSet>
#include <QString>

#include <memory>
#include <optional>

namespace CMakeProjectManager {
namespace Internal {

class FileApiData;

class CMakeFileInfo
{
public:
    bool operator==(const CMakeFileInfo& other) const { return path == other.path; }
    friend auto qHash(const CMakeFileInfo &info, uint seed = 0) { return info.path.hash(seed); }

    Utils::FilePath path;
    bool isCMake = false;
    bool isCMakeListsDotTxt = false;
    bool isExternal = false;
    bool isGenerated = false;
};

class FileApiQtcData
{
public:
    QString errorMessage;
    CMakeConfig cache;
    QSet<CMakeFileInfo> cmakeFiles;
    QList<CMakeBuildTarget> buildTargets;
    ProjectExplorer::RawProjectParts projectParts;
    std::unique_ptr<CMakeProjectNode> rootProjectNode;
    QString ctestPath;
    bool isMultiConfig = false;
    bool usesAllCapsTargets = false;
};

FileApiQtcData extractData(FileApiData &data,
                           const Utils::FilePath &sourceDirectory,
                           const Utils::FilePath &buildDirectory);

} // namespace Internal
} // namespace CMakeProjectManager
