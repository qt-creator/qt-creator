// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprojectmanager_global.h"

#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectmanager.h>

#include <utils/fileutils.h>

namespace QmlProjectManager {
namespace ProjectFileContentTools {

struct QMLPROJECTMANAGER_EXPORT Resolution {
    int width;
    int height;
};

const Utils::FilePaths QMLPROJECTMANAGER_EXPORT rootCmakeFiles(ProjectExplorer::Project *project = nullptr);
const QString QMLPROJECTMANAGER_EXPORT readFileContents(const Utils::FilePath &filePath);
const QString QMLPROJECTMANAGER_EXPORT qdsVersion(const Utils::FilePath &projectFilePath);
const QString QMLPROJECTMANAGER_EXPORT qtVersion(const Utils::FilePath &projectFilePath);
const QString QMLPROJECTMANAGER_EXPORT getMainQmlFile(const Utils::FilePath &projectFilePath);
const QString QMLPROJECTMANAGER_EXPORT appQmlFile(const Utils::FilePath &projectFilePath);
const Resolution QMLPROJECTMANAGER_EXPORT resolutionFromConstants(const Utils::FilePath &projectFilePath);

} //ProjectFileContentTools
} //QmlProjectManager

