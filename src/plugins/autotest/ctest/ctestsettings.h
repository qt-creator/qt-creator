// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace Autotest {
namespace Internal {

class CTestSettings : public Utils::AspectContainer
{
public:
    CTestSettings();

    QStringList activeSettingsAsOptions() const;

    Utils::IntegerAspect repetitionCount;
    Utils::SelectionAspect repetitionMode;
    Utils::SelectionAspect outputMode;
    Utils::BoolAspect outputOnFail;
    Utils::BoolAspect stopOnFailure;
    Utils::BoolAspect scheduleRandom;
    Utils::BoolAspect repeat;
    // FIXME.. this makes the outputreader fail to get all results correctly for visual display
    Utils::BoolAspect parallel;
    Utils::IntegerAspect jobs;
    Utils::BoolAspect testLoad;
    Utils::IntegerAspect threshold;
};

class CTestSettingsPage final : public Core::IOptionsPage
{
public:
    CTestSettingsPage(CTestSettings *settings, Utils::Id settingsId);
};

} // namespace Internal
} // namespace Autotest

