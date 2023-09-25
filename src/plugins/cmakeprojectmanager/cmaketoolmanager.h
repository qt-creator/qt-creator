// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include "cmaketool.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <QObject>

#include <memory>

namespace CMakeProjectManager {

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

    static CMakeTool *defaultProjectOrDefaultCMakeTool();

    static CMakeTool *defaultCMakeTool();
    static void setDefaultCMakeTool(const Utils::Id &id);
    static CMakeTool *findByCommand(const Utils::FilePath &command);
    static CMakeTool *findById(const Utils::Id &id);

    static void notifyAboutUpdate(CMakeTool *);
    static void restoreCMakeTools();

    static void updateDocumentation();

    static QString toolTipForRstHelpFile(const Utils::FilePath &helpFile);

public slots:
    QList<Utils::Id> autoDetectCMakeForDevice(const Utils::FilePaths &searchPaths,
                                  const QString &detectionSource,
                                  QString *logMessage);
    Utils::Id registerCMakeByPath(const Utils::FilePath &cmakePath,
                             const QString &detectionSource);
    void removeDetectedCMake(const QString &detectionSource, QString *logMessage);
    void listDetectedCMake(const QString &detectionSource, QString *logMessage);

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

    static CMakeToolManager *m_instance;
};

} // namespace CMakeProjectManager

Q_DECLARE_METATYPE(QString *)
