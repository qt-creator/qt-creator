// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestframework.h"
#include "ctestsettings.h"

namespace Autotest {
namespace Internal {

class CTestTool final : public Autotest::ITestTool
{
public:
    CTestTool() : Autotest::ITestTool(false) {}

    Utils::Id buildSystemId() const final;

    ITestTreeItem *createItemFromTestCaseInfo(const ProjectExplorer::TestCaseInfo &tci) final;

protected:
    const char *name() const final;
    QString displayName() const final;
    ITestTreeItem *createRootNode() final;

private:
    ITestSettings *testSettings() override { return &m_settings; }

    CTestSettings m_settings;
    CTestSettingsPage m_settingsPage{&m_settings, settingsId()};
};

} // namespace Internal
} // namespace Autotest
