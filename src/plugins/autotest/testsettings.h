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

    void toSettings() const;
    void fromSettings();

    Utils::IntegerAspect scanThreadLimit{this};
    Utils::IntegerAspect timeout{this};
    Utils::BoolAspect omitInternalMsg{this};
    Utils::BoolAspect omitRunConfigWarn{this};
    Utils::BoolAspect limitResultOutput{this};
    Utils::BoolAspect limitResultDescription{this};
    Utils::IntegerAspect resultDescriptionMaxSize{this};
    Utils::BoolAspect autoScroll{this};
    Utils::BoolAspect processArgs{this};
    Utils::BoolAspect displayApplication{this};
    Utils::BoolAspect popupOnStart{this};
    Utils::BoolAspect popupOnFinish{this};
    Utils::BoolAspect popupOnFail{this};
    Utils::SelectionAspect runAfterBuild{this};

    RunAfterBuildMode runAfterBuildMode() const;
};

TestSettings &testSettings();

} // Autotest::Internal
