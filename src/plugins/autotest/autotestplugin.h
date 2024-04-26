// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itestframework.h"

#include <utils/filepath.h>

namespace ProjectExplorer {
class Project;
class RunConfiguration;
}

namespace Autotest::Internal {

class TestProjectSettings;

struct ChoicePair
{
    explicit ChoicePair(const QString &name = {}, const Utils::FilePath &exe = {})
        : displayName(name), executable(exe) {}
    bool matches(const ProjectExplorer::RunConfiguration *rc) const;

    QString displayName;
    Utils::FilePath executable;
};

TestProjectSettings *projectSettings(ProjectExplorer::Project *project);
TestFrameworks activeTestFrameworks();
void updateMenuItemsEnabledState();
void cacheRunConfigChoice(const QString &buildTargetKey, const ChoicePair &choice);
ChoicePair cachedChoiceFor(const QString &buildTargetKey);
void clearChoiceCache();
void popupResultsPane();
QString wildcardPatternFromString(const QString &original);

} // Autotest::Internal
