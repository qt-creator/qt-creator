// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestframework.h"

namespace Autotest::Internal {

class CTestTool final : public Autotest::ITestTool
{
public:
    CTestTool();

    Utils::Id buildSystemId() const final;

    ITestTreeItem *createItemFromTestCaseInfo(const ProjectExplorer::TestCaseInfo &tci) final;

    ITestTreeItem *createRootNode() final;

    QStringList activeSettingsAsOptions() const;

    Utils::IntegerAspect repetitionCount{this};
    Utils::SelectionAspect repetitionMode{this};
    Utils::SelectionAspect outputMode{this};
    Utils::BoolAspect outputOnFail{this};
    Utils::BoolAspect stopOnFailure{this};
    Utils::BoolAspect scheduleRandom{this};
    Utils::BoolAspect repeat{this};
    // FIXME.. this makes the outputreader fail to get all results correctly for visual display
    Utils::BoolAspect parallel{this};
    Utils::IntegerAspect jobs{this};
    Utils::BoolAspect testLoad{this};
    Utils::IntegerAspect threshold{this};
};

CTestTool &theCTestTool();

} // Autotest::Internal

