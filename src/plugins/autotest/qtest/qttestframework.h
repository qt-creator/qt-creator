// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestframework.h"

#include "qttestsettings.h"

namespace Autotest::Internal {

class QtTestFramework : public ITestFramework
{
public:
    QtTestFramework() : ITestFramework(true) {}

    QStringList testNameForSymbolName(const QString &symbolName) const override;
private:
    const char *name() const override;
    QString displayName() const override;
    unsigned priority() const override;
    ITestParser *createTestParser() override;
    ITestTreeItem *createRootNode() override;
    ITestSettings *testSettings() override { return &m_settings; }

    QtTestSettings m_settings{settingsId()};
};

} // Autotest::Internal
