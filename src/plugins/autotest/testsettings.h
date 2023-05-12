// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Autotest::Internal {

enum class RunAfterBuildMode
{
    None,
    All,
    Selected
};

class NonAspectSettings
{
public:
    QHash<Utils::Id, bool> frameworks;
    QHash<Utils::Id, bool> frameworksGrouping;
    QHash<Utils::Id, bool> tools;
};

class TestSettings : public Utils::AspectContainer, public NonAspectSettings
{
public:
    TestSettings();

    static TestSettings *instance();

    void toSettings(QSettings *s) const;
    void fromSettings(QSettings *s);

    Utils::IntegerAspect timeout;
    Utils::BoolAspect omitInternalMsg;
    Utils::BoolAspect omitRunConfigWarn;
    Utils::BoolAspect limitResultOutput;
    Utils::BoolAspect limitResultDescription;
    Utils::IntegerAspect resultDescriptionMaxSize;
    Utils::BoolAspect autoScroll;
    Utils::BoolAspect processArgs;
    Utils::BoolAspect displayApplication;
    Utils::BoolAspect popupOnStart;
    Utils::BoolAspect popupOnFinish;
    Utils::BoolAspect popupOnFail;
    Utils::SelectionAspect runAfterBuild;

    RunAfterBuildMode runAfterBuildMode() const;
};

} // Autotest::Internal
