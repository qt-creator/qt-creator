// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestframework.h"

#include "catchtestsettings.h"

namespace Autotest {
namespace Internal {

class CatchFramework : public ITestFramework
{
public:
    CatchFramework() : ITestFramework(true) {}

    const char *name() const override;
    QString displayName() const override;
    unsigned priority() const override;

protected:
    ITestParser *createTestParser() override;
    ITestTreeItem *createRootNode() override;

private:
    ITestSettings * testSettings() override { return &m_settings; }
    CatchTestSettings m_settings;
    CatchTestSettingsPage m_settingsPage{&m_settings, settingsId()};
};

} // namespace Internal
} // namespace Autotest
