// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testtreeitem.h"

namespace Autotest::Internal {

class QtTestTreeItem : public TestTreeItem
{
public:
    explicit QtTestTreeItem(ITestFramework *framework, const QString &name = {},
                            const Utils::FilePath &filePath = {}, Type type = Root);

    TestTreeItem *copyWithoutChildren() override;
    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;
    Qt::CheckState checked() const override;
    bool canProvideTestConfiguration() const override;
    bool canProvideDebugConfiguration() const override;
    ITestConfiguration *testConfiguration() const override;
    ITestConfiguration *debugConfiguration() const override;
    QList<ITestConfiguration *> getAllTestConfigurations() const override;
    QList<ITestConfiguration *> getSelectedTestConfigurations() const override;
    QList<ITestConfiguration *> getFailedTestConfigurations() const override;
    QList<ITestConfiguration *> getTestConfigurationsForFile(const Utils::FilePath &fileName) const override;
    TestTreeItem *find(const TestParseResult *result) override;
    TestTreeItem *findChild(const TestTreeItem *other) override;
    bool modify(const TestParseResult *result) override;
    void setInherited(bool inherited) { m_inherited = inherited; }
    bool inherited() const { return m_inherited; }
    void setRunsMultipleTestcases(bool multiTest) { m_multiTest = multiTest; }
    bool runsMultipleTestcases() const { return m_multiTest; }
    TestTreeItem *createParentGroupNode() const override;
    bool isGroupable() const override;
private:
    QVariant linkForTreeItem() const;
    TestTreeItem *findChildByFileNameAndType(const Utils::FilePath &file, const QString &name,
                                             Type type) const;
    TestTreeItem *findChildByNameAndInheritanceAndMultiTest(const QString &name, bool inherited,
                                                            bool multiTest) const;
    QString nameSuffix() const;
    bool m_inherited = false;
    bool m_multiTest = false;
};

class QtTestCodeLocationAndType : public TestCodeLocationAndType
{
public:
    bool m_inherited = false;
};

typedef QVector<QtTestCodeLocationAndType> QtTestCodeLocationList;

} // namespace Autotest::Internal
