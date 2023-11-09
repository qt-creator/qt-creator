// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itestframework.h"

namespace Autotest::TestFrameworkManager {

void registerTestFramework(ITestFramework *framework);
void registerTestTool(ITestTool *testTool);

ITestFramework *frameworkForId(Utils::Id frameworkId);
ITestTool *testToolForId(Utils::Id testToolId);
ITestTool *testToolForBuildSystemId(Utils::Id buildSystemId);
void activateFrameworksAndToolsFromSettings();
const TestFrameworks registeredFrameworks();
const TestTools registeredTestTools();


} // Autotest::TestFrameworkManager
