// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testresult.h"

namespace Autotest {

class TestTreeItem;

namespace Internal {

class GTestResult : public TestResult
{
public:
    GTestResult(const QString &id, const QString &name, const Utils::FilePath &projectFile,
                const QString &testCaseName = {}, int iteration = 1);
};

} // namespace Internal
} // namespace Autotest
