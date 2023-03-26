// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

namespace QmlProjectManager {
namespace GenerateQmlProject {

class QmlProjectFileGenerator {
public:
    bool prepare(const Utils::FilePath &targetDir);
    bool prepareForUiQmlFile(const Utils::FilePath &uiFilePath);
    bool execute();

    const Utils::FilePath targetDir() const;
    const Utils::FilePath targetFile() const;

private:
    const QString createFilteredDirEntries(const QStringList &suffixes) const;
    const QString createDirArrayEntry(const QString &arrayName, const QStringList &relativePaths) const;
    const Utils::FilePath selectTargetFile(const Utils::FilePath &uiFilePath = {});
    bool isStandardStructure(const Utils::FilePath &projectDir) const;
    bool isDirAcceptable(const Utils::FilePath &dir, const Utils::FilePath &uiFile);
    const QString createContentDirEntries(const QString &containerName,
                                          const QStringList &suffixes) const;
    const Utils::FilePath findInDirTree(const Utils::FilePath &dir,
                                        const QStringList &suffixes,
                                        int currentSearchDepth = 0) const;
    const QStringList findContentDirs(const QStringList &suffixes) const;

private:
    Utils::FilePath m_targetDir;
    Utils::FilePath m_targetFile;
};

} // GenerateQmlProject
} // QmlProjectManager
