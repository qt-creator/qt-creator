// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testtreeitem.h"

namespace Autotest {
namespace Internal {

class CatchTreeItem : public TestTreeItem
{
public:
    enum TestState
    {
        Normal        = 0x0,
        Parameterized = 0x1,
        Fixture       = 0x2
    };
    Q_FLAGS(TestState)
    Q_DECLARE_FLAGS(TestStates, TestState)

    explicit CatchTreeItem(ITestFramework *testFramework, const QString &name = {},
                           const Utils::FilePath &filePath = {}, Type type = Root)
        : TestTreeItem(testFramework, name, filePath, type) {}

    void setStates(CatchTreeItem::TestStates state) { m_state = state; }
    CatchTreeItem::TestStates states() const { return m_state; }
    QString testCasesString() const;

    QVariant data(int column, int role) const override;

    TestTreeItem *copyWithoutChildren() override;
    TestTreeItem *find(const TestParseResult *result) override;
    TestTreeItem *findChild(const TestTreeItem *other) override;
    bool modify(const TestParseResult *result) override;
    TestTreeItem *createParentGroupNode() const override;

    bool canProvideTestConfiguration() const override;
    bool canProvideDebugConfiguration() const override;
    ITestConfiguration *testConfiguration() const override;
    ITestConfiguration *debugConfiguration() const override;
    QList<ITestConfiguration *> getAllTestConfigurations() const override;
    QList<ITestConfiguration *> getSelectedTestConfigurations() const override;
    QList<ITestConfiguration *> getFailedTestConfigurations() const override;
    QList<ITestConfiguration *> getTestConfigurationsForFile(const Utils::FilePath &fileName) const override;

private:
    QString stateSuffix() const;
    QList<ITestConfiguration *> getTestConfigurations(bool ignoreCheckState) const;
    TestStates m_state = Normal;
};

class CatchTestCodeLocationAndType : public TestCodeLocationAndType
{
public:
    CatchTreeItem::TestStates states = CatchTreeItem::Normal;
    QStringList tags; // TODO: use them for the item
};

typedef QVector<CatchTestCodeLocationAndType> CatchTestCodeLocationList;

} // namespace Internal
} // namespace Autotest
