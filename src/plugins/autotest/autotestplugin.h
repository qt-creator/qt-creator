// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itestframework.h"

#include <extensionsystem/iplugin.h>

#include <utils/filepath.h>

namespace ProjectExplorer {
class Project;
class RunConfiguration;
}

namespace Autotest {
namespace Internal {

class TestProjectSettings;

struct ChoicePair
{
    explicit ChoicePair(const QString &name = {}, const Utils::FilePath &exe = {})
        : displayName(name), executable(exe) {}
    bool matches(const ProjectExplorer::RunConfiguration *rc) const;

    QString displayName;
    Utils::FilePath executable;
};

class AutotestPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AutoTest.json")

public:
    AutotestPlugin();
    ~AutotestPlugin() override;

    void initialize() override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

    static TestProjectSettings *projectSettings(ProjectExplorer::Project *project);
    static TestFrameworks activeTestFrameworks();
    static void updateMenuItemsEnabledState();
    static void cacheRunConfigChoice(const QString &buildTargetKey, const ChoicePair &choice);
    static ChoicePair cachedChoiceFor(const QString &buildTargetKey);
    static void clearChoiceCache();
    static void popupResultsPane();
};

} // namespace Internal
} // namespace Autotest
