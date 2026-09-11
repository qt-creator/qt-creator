// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include "cmaketool.h"

#include <cmakelang/cmakedoc.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspect.h>

#include <utils/filepath.h>
#include <utils/id.h>

#include <QObject>

#include <memory>
#include <optional>
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

    // Reads them before they are asked for, so that whoever asks does not
    // wait for the process and the files of the Help of CMake that reading
    // them takes.
    static void readKeywords();

    // What readKeywords() has read so far: nothing while the reading is
    // still going on, so that asking for them never waits for it.
    // keywordsRead() tells when there are some.
    static std::optional<CMakeKeywords> keywordsIfRead();

    static CMakeTool *defaultCMakeTool();
    static void setDefaultCMakeTool(const Utils::Id &id);
    static CMakeTool *findByCommand(const Utils::FilePath &command);
    static CMakeTool *findById(const Utils::Id &id);
    static Utils::Id idForExecutable(const Utils::FilePath &cmakeExecutable);
    static Utils::FilePath executableForId(const Utils::Id id);

    static void notifyAboutUpdate(CMakeTool *);
    static void restoreCMakeTools();

    static void updateDocumentation();

    // What the documentation of that name says, read from the file that
    // carries it: a reStructuredText file of the Help of CMake, or a CMake
    // module that documents itself in a ".rst:" comment.  Null where the
    // file says nothing about the name.
    static CMakeLang::Documentation documentation(const QString &name,
                                                  const Utils::FilePath &file);

    // The same as Markdown, the way a tooltip shows it.
    static QString toolTip(const QString &name, const Utils::FilePath &file);

    static Utils::FilePath mappedFilePath(ProjectExplorer::Project *project, const Utils::FilePath &path);

    void removeDetectedCMake(
        const QString &detectionSource, const ProjectExplorer::LogCallback &logCallback);

signals:
    void cmakeAdded(const Utils::Id &id);
    void cmakeRemoved(const Utils::Id &id);
    void cmakeUpdated(const Utils::Id &id);
    void cmakeToolsLoaded();
    void defaultCMakeChanged();
    void keywordsRead();

private:
    static void saveCMakeTools();
    static void ensureDefaultCMakeToolIsValid();
    void handleDeviceToolDetectionRequest(
        Utils::Id devId, const Utils::FilePaths &searchPaths, quint64 token,
        const ProjectExplorer::ToolDetectionLogger &logger);
};

namespace Internal { void setupCMakeToolManager(QObject *guard); }

} // namespace CMakeProjectManager

Q_DECLARE_METATYPE(QString *)
