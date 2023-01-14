// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testresult.h"
#include "qttestconstants.h"

namespace Autotest {

class TestTreeItem;

namespace Internal {

class QtTestResult : public TestResult
{
public:
    QtTestResult(const QString &id, const QString &name, const Utils::FilePath &projectFile,
                 TestType type, const QString &functionName = {}, const QString &dataTag = {});
};

} // namespace Internal
} // namespace Autotest
