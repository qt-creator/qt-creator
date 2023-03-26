// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestframework.h"

#include "boosttestsettings.h"

namespace Autotest {
namespace Internal {

class BoostTestFramework : public ITestFramework
{
public:
    BoostTestFramework() : ITestFramework(true) {}

private:
    const char *name() const override;
    QString displayName() const override;
    unsigned priority() const override;
    ITestSettings *testSettings() override { return &m_settings; }
    ITestParser *createTestParser() override;
    ITestTreeItem *createRootNode() override;

    BoostTestSettings m_settings;
    BoostTestSettingsPage m_settingsPage{&m_settings, settingsId()};
};

} // namespace Internal
} // namespace Autotest
