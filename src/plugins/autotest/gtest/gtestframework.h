// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestframework.h"
#include "gtestconstants.h"
#include "gtestsettings.h"

namespace Autotest::Internal {

class GTestFramework : public ITestFramework
{
public:
    GTestFramework();

    static GTest::Constants::GroupMode groupMode();
    static QString currentGTestFilter();

    QStringList testNameForSymbolName(const QString &symbolName) const override;
private:
    const char *name() const override;
    QString displayName() const override;
    unsigned priority() const override;
    QString groupingToolTip() const override;
    ITestSettings *testSettings() override { return &m_settings; }
    ITestParser *createTestParser() override;
    ITestTreeItem *createRootNode() override;

    GTestSettings m_settings{settingsId()};
};

} // Autotest::Internal
