// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace Autotest {
namespace Internal {

class CatchTestSettings : public Utils::AspectContainer
{
public:
    CatchTestSettings();

    Utils::IntegerAspect abortAfter;
    Utils::IntegerAspect benchmarkSamples;
    Utils::IntegerAspect benchmarkResamples;
    Utils::DoubleAspect confidenceInterval;
    Utils::IntegerAspect benchmarkWarmupTime;
    Utils::BoolAspect abortAfterChecked;
    Utils::BoolAspect samplesChecked;
    Utils::BoolAspect resamplesChecked;
    Utils::BoolAspect confidenceIntervalChecked;
    Utils::BoolAspect warmupChecked;
    Utils::BoolAspect noAnalysis;
    Utils::BoolAspect showSuccess;
    Utils::BoolAspect breakOnFailure;
    Utils::BoolAspect noThrow;
    Utils::BoolAspect visibleWhitespace;
    Utils::BoolAspect warnOnEmpty;
};

class CatchTestSettingsPage : public Core::IOptionsPage
{
public:
    CatchTestSettingsPage(CatchTestSettings *settings, Utils::Id settingsId);
};

} // namespace Internal
} // namespace Autotest
