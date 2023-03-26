// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itemdatacache.h"
#include "testsettings.h"

namespace ProjectExplorer { class Project; }

namespace Autotest {

class ITestFramework;
class ITestTool;

namespace Internal {

class TestProjectSettings : public QObject
{
    Q_OBJECT
public:
    explicit TestProjectSettings(ProjectExplorer::Project *project);
    ~TestProjectSettings();

    void setUseGlobalSettings(bool useGlobal);
    bool useGlobalSettings() const { return m_useGlobalSettings; }
    void setRunAfterBuild(RunAfterBuildMode mode) {m_runAfterBuild = mode; }
    RunAfterBuildMode runAfterBuild() const { return m_runAfterBuild; }
    void setActiveFrameworks(const QHash<ITestFramework *, bool> &enabledFrameworks)
    { m_activeTestFrameworks = enabledFrameworks; }
    QHash<ITestFramework *, bool> activeFrameworks() const { return m_activeTestFrameworks; }
    void activateFramework(const Utils::Id &id, bool activate);
    void setActiveTestTools(const QHash<ITestTool *, bool> &enabledTestTools)
    { m_activeTestTools = enabledTestTools; }
    QHash<ITestTool *, bool> activeTestTools() const { return m_activeTestTools; }
    void activateTestTool(const Utils::Id &id, bool activate);
    Internal::ItemDataCache<Qt::CheckState> *checkStateCache() { return &m_checkStateCache; }
private:
    void load();
    void save();

    ProjectExplorer::Project *m_project;
    bool m_useGlobalSettings = true;
    RunAfterBuildMode m_runAfterBuild = RunAfterBuildMode::None;
    QHash<ITestFramework *, bool> m_activeTestFrameworks;
    QHash<ITestTool *, bool> m_activeTestTools;
    Internal::ItemDataCache<Qt::CheckState> m_checkStateCache;
};

} // namespace Internal
} // namespace Autotest
