// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testtreeitem.h"

namespace Autotest {
namespace Internal {

class BoostTestParseResult;

class BoostTestTreeItem : public TestTreeItem
{
public:
    enum TestState
    {
        Enabled             = 0x00,
        Disabled            = 0x01,
        ExplicitlyEnabled   = 0x02,

        Parameterized       = 0x10,
        Fixture             = 0x20,
        Templated           = 0x40,
    };
    Q_FLAGS(TestState)
    Q_DECLARE_FLAGS(TestStates, TestState)

    explicit BoostTestTreeItem(ITestFramework *framework,
                               const QString &name = {},
                               const Utils::FilePath &filePath = {},
                               Type type = Root)
        : TestTreeItem(framework, name, filePath, type)
    {}

public:
    TestTreeItem *copyWithoutChildren() override;
    QVariant data(int column, int role) const override;
    TestTreeItem *find(const TestParseResult *result) override;
    TestTreeItem *findChild(const TestTreeItem *other) override;
    bool modify(const TestParseResult *result) override;
    TestTreeItem *createParentGroupNode() const override;

    void setFullName(const QString &fullName) { m_fullName = fullName; }
    QString fullName() const { return m_fullName; }
    void setStates(TestStates states) { m_state = states; }
    void setState(TestState state) { m_state |= state; }
    TestStates state() const { return m_state; }

    QList<ITestConfiguration *> getAllTestConfigurations() const override;
    QList<ITestConfiguration *> getSelectedTestConfigurations() const override;
    QList<ITestConfiguration *> getFailedTestConfigurations() const override;
    bool canProvideTestConfiguration() const override { return type() != Root; }
    bool canProvideDebugConfiguration() const override { return canProvideTestConfiguration(); }
    ITestConfiguration *testConfiguration() const override;
    ITestConfiguration *debugConfiguration() const override;

private:
    QString nameSuffix() const;
    bool enabled() const;
    TestTreeItem *findChildByNameStateAndFile(const QString &name,
                                              BoostTestTreeItem::TestStates state,
                                              const Utils::FilePath &proFile) const;
    QString prependWithParentsSuitePaths(const QString &testName) const;
    QList<ITestConfiguration *> getTestConfigurations(
            std::function<bool(BoostTestTreeItem *)> predicate) const;
    bool modifyTestContent(const BoostTestParseResult *result);
    TestStates m_state = Enabled;
    QString m_fullName;
};

struct BoostTestInfo
{
    QString fullName; // formatted like UNIX path
    BoostTestTreeItem::TestStates state;
    int line;
};

typedef QVector<BoostTestInfo> BoostTestInfoList;

class BoostTestCodeLocationAndType : public TestCodeLocationAndType
{
public:
    BoostTestTreeItem::TestStates m_state;
    BoostTestInfoList m_suitesState;
};

typedef QVector<BoostTestCodeLocationAndType> BoostTestCodeLocationList;


} // namespace Internal
} // namespace Autotest
