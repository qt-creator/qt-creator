// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itemdatacache.h"
#include "testsettings.h"

#include <utils/aspects.h>

namespace ProjectExplorer { class Project; }

namespace Autotest {

class ITestFramework;
class ITestTool;

namespace Internal {

class TestProjectSettings : public Utils::AspectContainer
{
public:
    explicit TestProjectSettings(ProjectExplorer::Project *project);
    ~TestProjectSettings() override = default;

    RunAfterBuildMode runAfterBuildMode() const;

    QHash<ITestFramework *, bool> activeFrameworks() const { return m_activeTestFrameworks; }
    void activateFramework(const Utils::Id &id, bool activate);
    QHash<ITestTool *, bool> activeTestTools() const { return m_activeTestTools; }
    void activateTestTool(const Utils::Id &id, bool activate);
    Internal::ItemDataCache<Qt::CheckState> *checkStateCache() { return &m_checkStateCache; }
    const QStringList pathFilters() const { return m_pathFilters; }
    void setPathFilters(const QStringList &filters) { m_pathFilters = filters; }
    void addPathFilter(const QString &filter) { m_pathFilters.append(filter); }

    Utils::BoolAspect useGlobalSettings;   // not {this}: excluded from toMap/fromMap
    Utils::SelectionAspect runAfterBuild{this};
    Utils::BoolAspect limitToFilter{this};

private:
    void load();
    void save();

    ProjectExplorer::Project *m_project;
    QHash<ITestFramework *, bool> m_activeTestFrameworks;
    QHash<ITestTool *, bool> m_activeTestTools;
    QStringList m_pathFilters;
    Internal::ItemDataCache<Qt::CheckState> m_checkStateCache;
};

} // namespace Internal
} // namespace Autotest
