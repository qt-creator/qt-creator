// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace Autotest {
namespace Internal {

enum MetricsType
{
    Walltime,
    TickCounter,
    EventCounter,
    CallGrind,
    Perf
};

class QtTestSettings : public Utils::AspectContainer
{
public:
    QtTestSettings();

    static QString metricsTypeToOption(const MetricsType type);

    Utils::SelectionAspect metrics;
    Utils::BoolAspect noCrashHandler;
    Utils::BoolAspect useXMLOutput;
    Utils::BoolAspect verboseBench;
    Utils::BoolAspect logSignalsSlots;
    Utils::BoolAspect limitWarnings;
    Utils::IntegerAspect maxWarnings;
};

class QtTestSettingsPage final : public Core::IOptionsPage
{
public:
    QtTestSettingsPage(QtTestSettings *settings, Utils::Id settingsId);
};

} // namespace Internal
} // namespace Autotest
