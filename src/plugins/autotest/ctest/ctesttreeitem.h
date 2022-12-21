// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testtreeitem.h"

namespace Autotest {
namespace Internal {

class CTestTreeItem final : public Autotest::ITestTreeItem
{
public:
    CTestTreeItem(ITestBase *testBase, const QString &name, const Utils::FilePath &filepath, Type type);

    QList<ITestConfiguration *> getAllTestConfigurations() const final;
    QList<ITestConfiguration *> getSelectedTestConfigurations() const final;
    QList<ITestConfiguration *> getFailedTestConfigurations() const final;
    ITestConfiguration *testConfiguration() const final;
    bool canProvideTestConfiguration() const final { return true; }

    QVariant data(int column, int role) const final;
private:
    QList<ITestConfiguration *> testConfigurationsFor(const QStringList &selected) const;
};

} // namespace Internal
} // namespace Autotest
