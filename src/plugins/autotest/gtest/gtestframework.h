// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestframework.h"

#include "gtestconstants.h"

namespace Autotest::Internal {

class GTestFramework : public ITestFramework
{
public:
    GTestFramework();

    Utils::IntegerAspect iterations{this};
    Utils::IntegerAspect seed{this};
    Utils::BoolAspect runDisabled{this};
    Utils::BoolAspect shuffle{this};
    Utils::BoolAspect repeat{this};
    Utils::BoolAspect throwOnFailure{this};
    Utils::BoolAspect breakOnFailure{this};
    Utils::SelectionAspect groupMode{this};
    Utils::StringAspect gtestFilter{this};

    static GTest::Constants::GroupMode staticGroupMode();
    static QString currentGTestFilter();

    QStringList testNameForSymbolName(const QString &symbolName) const override;

    QString groupingToolTip() const override;
    ITestParser *createTestParser() override;
    ITestTreeItem *createRootNode() override;

    void readSettings() final;
};

GTestFramework &theGTestFramework();

} // Autotest::Internal
