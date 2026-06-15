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

using ActiveTestFrameworks = QHash<ITestFramework *, bool>;

// Holds the per-project "active" flag for each registered test framework.
// The frameworks are keyed by pointer (they are registered at runtime), so this
// aspect is not serialized through the container; TestProjectSettings reconciles
// it against the registered frameworks in load()/save().
class ActiveTestFrameworksAspect : public Utils::TypedAspect<ActiveTestFrameworks>
{
public:
    explicit ActiveTestFrameworksAspect(Utils::AspectContainer *container = nullptr)
        : TypedAspect(container)
    {}

    void setActive(ITestFramework *framework, bool active)
    {
        ActiveTestFrameworks map = value();
        map.insert(framework, active);
        setValue(map);
    }
};

class TestProjectSettings : public Utils::AspectContainer
{
public:
    explicit TestProjectSettings(ProjectExplorer::Project *project);
    ~TestProjectSettings() override = default;

    RunAfterBuildMode runAfterBuildMode() const;

    QHash<ITestFramework *, bool> activeFrameworks() const { return activeTestFrameworks(); }
    void activateFramework(const Utils::Id &id, bool activate);
    QHash<ITestTool *, bool> activeTestTools() const { return m_activeTestTools; }
    void activateTestTool(const Utils::Id &id, bool activate);
    Internal::ItemDataCache<Qt::CheckState> *checkStateCache() { return &m_checkStateCache; }
    void setPathFilters(const QStringList &filters) { pathFilters.setValue(filters); }
    void addPathFilter(const QString &filter) { setPathFilters(pathFilters() << filter); }

    Utils::BoolAspect useGlobalSettings;   // not {this}: excluded from toMap/fromMap
    Utils::SelectionAspect runAfterBuild{this};
    Utils::BoolAspect limitToFilter{this};
    Utils::StringListAspect pathFilters{this};
    ActiveTestFrameworksAspect activeTestFrameworks{this};

private:
    void load();
    void save();

    ProjectExplorer::Project *m_project;
    QHash<ITestTool *, bool> m_activeTestTools;
    Internal::ItemDataCache<Qt::CheckState> m_checkStateCache;
};

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::ActiveTestFrameworks)
