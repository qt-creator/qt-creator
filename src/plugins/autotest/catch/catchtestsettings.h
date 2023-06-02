// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Autotest::Internal {

class CatchTestSettings : public Core::PagedSettings
{
public:
    explicit CatchTestSettings(Utils::Id settingsId);

    Utils::IntegerAspect abortAfter{this};
    Utils::IntegerAspect benchmarkSamples{this};
    Utils::IntegerAspect benchmarkResamples{this};
    Utils::DoubleAspect confidenceInterval{this};
    Utils::IntegerAspect benchmarkWarmupTime{this};
    Utils::BoolAspect abortAfterChecked{this};
    Utils::BoolAspect samplesChecked{this};
    Utils::BoolAspect resamplesChecked{this};
    Utils::BoolAspect confidenceIntervalChecked{this};
    Utils::BoolAspect warmupChecked{this};
    Utils::BoolAspect noAnalysis{this};
    Utils::BoolAspect showSuccess{this};
    Utils::BoolAspect breakOnFailure{this};
    Utils::BoolAspect noThrow{this};
    Utils::BoolAspect visibleWhitespace{this};
    Utils::BoolAspect warnOnEmpty{this};
};

} // Autotest::Internal
