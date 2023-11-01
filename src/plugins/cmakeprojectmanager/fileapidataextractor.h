// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakebuildtarget.h"
#include "cmakeprojectnodes.h"
#include "3rdparty/cmake/cmListFileCache.h"

#include <projectexplorer/rawprojectpart.h>

#include <utils/filepath.h>

#include <QList>
#include <QSet>
#include <QString>

#include <memory>
#include <optional>

namespace CMakeProjectManager::Internal {

class FileApiData;

class CMakeFileInfo
{
public:
    bool operator==(const CMakeFileInfo& other) const { return path == other.path; }
    friend size_t qHash(const CMakeFileInfo &info, uint seed = 0) { return qHash(info.path, seed); }

    bool operator<(const CMakeFileInfo &other) const { return path < other.path; }

    Utils::FilePath path;
    bool isCMake = false;
    bool isCMakeListsDotTxt = false;
    bool isExternal = false;
    bool isGenerated = false;
    cmListFile cmakeListFile;
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

FileApiQtcData extractData(const QFuture<void> &cancelFuture, FileApiData &input,
                           const Utils::FilePath &sourceDir, const Utils::FilePath &buildDir);

} // CMakeProjectManager::Internal
