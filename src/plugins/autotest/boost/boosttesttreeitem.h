/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
                               const QString &name = QString(),
                               const QString &filePath = QString(),
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

    QList<TestConfiguration *> getAllTestConfigurations() const override;
    QList<TestConfiguration *> getSelectedTestConfigurations() const override;
    bool canProvideTestConfiguration() const override { return type() != Root; }
    bool canProvideDebugConfiguration() const override { return canProvideTestConfiguration(); }
    TestConfiguration *testConfiguration() const override;
    TestConfiguration *debugConfiguration() const override;

private:
    QString nameSuffix() const;
    bool enabled() const;
    TestTreeItem *findChildByNameStateAndFile(const QString &name,
                                              BoostTestTreeItem::TestStates state,
                                              const QString &proFile) const;
    QString prependWithParentsSuitePaths(const QString &testName) const;
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
