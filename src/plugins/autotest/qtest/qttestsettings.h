// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Autotest::Internal {

enum MetricsType
{
    Walltime,
    TickCounter,
    EventCounter,
    CallGrind,
    Perf
};

class QtTestSettings : public Core::PagedSettings
{
public:
    explicit QtTestSettings(Utils::Id settingsId);

    static QString metricsTypeToOption(const MetricsType type);

    Utils::SelectionAspect metrics;
    Utils::BoolAspect noCrashHandler;
    Utils::BoolAspect useXMLOutput;
    Utils::BoolAspect verboseBench;
    Utils::BoolAspect logSignalsSlots;
    Utils::BoolAspect limitWarnings;
    Utils::IntegerAspect maxWarnings;
    Utils::BoolAspect quickCheckForDerivedTests;
};

} // Autotest::Internal
