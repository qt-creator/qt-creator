// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include "cmaketool.h"

#include <projectexplorer/kitaspect.h>

#include <utils/filepath.h>
#include <utils/id.h>

#include <QObject>

#include <memory>
#include <vector>

namespace ProjectExplorer {
class Project;
}

namespace CMakeProjectManager {

class CMakeKeywords;

class CMAKE_EXPORT CMakeToolManager : public QObject
{
    Q_OBJECT
public:
    CMakeToolManager();
    ~CMakeToolManager();

    static CMakeToolManager *instance();

    static QList<CMakeTool *> cmakeTools();

    static bool registerCMakeTool(std::unique_ptr<CMakeTool> &&tool);
    static void deregisterCMakeTool(const Utils::Id &id);
    static std::vector<std::unique_ptr<CMakeTool>> autoDetectCMakeTools(
        const Utils::FilePaths &searchPaths, const Utils::FilePath &rootPath);

    static CMakeKeywords defaultProjectOrDefaultCMakeKeyWords();

    static CMakeTool *defaultCMakeTool();
    static void setDefaultCMakeTool(const Utils::Id &id);
    static CMakeTool *findByCommand(const Utils::FilePath &command);
    static CMakeTool *findById(const Utils::Id &id);
    static Utils::Id idForExecutable(const Utils::FilePath &cmakeExecutable);
    static Utils::FilePath executableForId(const Utils::Id id);

    static void notifyAboutUpdate(CMakeTool *);
    static void restoreCMakeTools();

    static void updateDocumentation();

    static QString toolTipForRstHelpFile(const Utils::FilePath &helpFile);

    static Utils::FilePath mappedFilePath(ProjectExplorer::Project *project, const Utils::FilePath &path);

    void removeDetectedCMake(
        const QString &detectionSource, const ProjectExplorer::LogCallback &logCallback);

signals:
    void cmakeAdded (const Utils::Id &id);
    void cmakeRemoved (const Utils::Id &id);
    void cmakeUpdated (const Utils::Id &id);
    void cmakeToolsChanged ();
    void cmakeToolsLoaded ();
    void defaultCMakeChanged ();

private:
    static void saveCMakeTools();
    static void ensureDefaultCMakeToolIsValid();
    void handleDeviceToolDetectionRequest(Utils::Id devId, const Utils::FilePaths &searchPaths);
};

namespace Internal { void setupCMakeToolManager(QObject *guard); }

} // namespace CMakeProjectManager

Q_DECLARE_METATYPE(QString *)
