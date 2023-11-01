// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestframework.h"

namespace Autotest::Internal {

enum MetricsType
{
    Walltime,
    TickCounter,
    EventCounter,
    CallGrind,
    Perf
};

class QtTestFramework : public ITestFramework
{
public:
    QtTestFramework();

    static QString metricsTypeToOption(const MetricsType type);

    Utils::SelectionAspect metrics{this};
    Utils::BoolAspect noCrashHandler{this};
    Utils::BoolAspect useXMLOutput{this};
    Utils::BoolAspect verboseBench{this};
    Utils::BoolAspect logSignalsSlots{this};
    Utils::BoolAspect limitWarnings{this};
    Utils::IntegerAspect maxWarnings{this};
    Utils::BoolAspect quickCheckForDerivedTests{this};

    QStringList testNameForSymbolName(const QString &symbolName) const override;

    ITestParser *createTestParser() override;
    ITestTreeItem *createRootNode() override;
};

QtTestFramework &theQtTestFramework();

} // Autotest::Internal
