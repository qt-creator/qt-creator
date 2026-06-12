// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itemdatacache.h"
#include "testsettings.h"

#include <projectexplorer/useglobalaspect.h>

#include <utils/aspects.h>

namespace ProjectExplorer { class Project; }

namespace Autotest {

class ITestFramework;
class ITestTool;

namespace Internal {

using ActiveTestFrameworks = QHash<ITestFramework *, bool>;
using ActiveTestTools = QHash<ITestTool *, bool>;

class ActiveTestFrameworksAspect : public Utils::TypedAspect<ActiveTestFrameworks>
{
public:
    explicit ActiveTestFrameworksAspect(Utils::AspectContainer *container);

    void setActive(ITestFramework *framework, bool active);
};

class ActiveTestToolsAspect : public Utils::TypedAspect<ActiveTestTools>
{
public:
    explicit ActiveTestToolsAspect(Utils::AspectContainer *container);

    void setActive(ITestTool *testTool, bool active);
};

class TestProjectSettings : public Utils::AspectContainer
{
public:
    explicit TestProjectSettings(ProjectExplorer::Project *project);
    ~TestProjectSettings() override = default;

    RunAfterBuildMode runAfterBuildMode() const;

    QHash<ITestFramework *, bool> activeFrameworks() const { return activeTestFrameworks(); }
    void activateFramework(const Utils::Id &id, bool activate);
    void activateTestTool(const Utils::Id &id, bool activate);
    Internal::ItemDataCache<Qt::CheckState> *checkStateCache() { return &m_checkStateCache; }
    void setPathFilters(const QStringList &filters) { pathFilters.setValue(filters); }
    void addPathFilter(const QString &filter) { setPathFilters(pathFilters() << filter); }

    ProjectExplorer::UseGlobalAspect useGlobalSettings; // not {this}: excluded from toMap/fromMap
    Utils::SelectionAspect runAfterBuild{this};
    Utils::BoolAspect limitToFilter{this};
    Utils::StringListAspect pathFilters{this};
    ActiveTestFrameworksAspect activeTestFrameworks{this};
    ActiveTestToolsAspect activeTestTools{this};

private:
    void load();
    void save();

    ProjectExplorer::Project *m_project;
    Internal::ItemDataCache<Qt::CheckState> m_checkStateCache{Qt::Checked};
};

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::ActiveTestFrameworks)
Q_DECLARE_METATYPE(Autotest::Internal::ActiveTestTools)
