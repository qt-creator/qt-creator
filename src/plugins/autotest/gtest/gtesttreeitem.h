// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testtreeitem.h"

namespace Autotest {
namespace Internal {

class GTestParseResult;

class GTestTreeItem : public TestTreeItem
{
public:
    enum TestState
    {
        Enabled         = 0x00,
        Disabled        = 0x01,
        Parameterized   = 0x02,
        Typed           = 0x04,
    };

    Q_FLAGS(TestState)
    Q_DECLARE_FLAGS(TestStates, TestState)

    explicit GTestTreeItem(ITestFramework *testFramework, const QString &name = {},
                           const Utils::FilePath &filePath = {}, Type type = Root)
        : TestTreeItem(testFramework, name, filePath, type), m_state(Enabled)
    {}

    TestTreeItem *copyWithoutChildren() override;
    QVariant data(int column, int role) const override;
    bool canProvideTestConfiguration() const override { return type() != Root; }
    bool canProvideDebugConfiguration() const override { return type() != Root; }
    ITestConfiguration *testConfiguration() const override;
    ITestConfiguration *debugConfiguration() const override;
    QList<ITestConfiguration *> getAllTestConfigurations() const override;
    QList<ITestConfiguration *> getSelectedTestConfigurations() const override;
    QList<ITestConfiguration *> getFailedTestConfigurations() const override;
    QList<ITestConfiguration *> getTestConfigurationsForFile(const Utils::FilePath &fileName) const override;
    TestTreeItem *find(const TestParseResult *result) override;
    TestTreeItem *findChild(const TestTreeItem *other) override;
    bool modify(const TestParseResult *result) override;
    TestTreeItem *createParentGroupNode() const override;

    void setStates(TestStates states) { m_state = states; }
    void setState(TestState state) { m_state |= state; }
    TestStates state() const { return m_state; }
    TestTreeItem *findChildByNameStateAndFile(const QString &name,
                                              GTestTreeItem::TestStates state,
                                              const Utils::FilePath &proFile) const;
    QString nameSuffix() const;
    bool isGroupNodeFor(const TestTreeItem *other) const override;
    bool isGroupable() const override;
    TestTreeItem *applyFilters() override;
    bool shouldBeAddedAfterFiltering() const override;
private:
    bool modifyTestSetContent(const GTestParseResult *result);
    QList<ITestConfiguration *> getTestConfigurations(bool ignoreCheckState) const;
    GTestTreeItem::TestStates m_state;
};

class GTestCodeLocationAndType : public TestCodeLocationAndType
{
public:
    GTestTreeItem::TestStates m_state;
};

typedef QVector<GTestCodeLocationAndType> GTestCodeLocationList;

struct GTestCaseSpec
{
    QString testCaseName;
    bool parameterized;
    bool typed;
    bool disabled;
};

} // namespace Internal
} // namespace Autotest
