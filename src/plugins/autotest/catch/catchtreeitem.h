/****************************************************************************
**
** Copyright (C) 2019 Jochen Seemann
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

    explicit CatchTreeItem(ITestFramework *framework, const QString &name = QString(),
                           const QString &filePath = QString(), Type type = Root)
        : TestTreeItem(framework, name, filePath, type) {}

    void setStates(CatchTreeItem::TestStates state) { m_state = state; }
    QString testCasesString() const;

    QVariant data(int column, int role) const override;

    TestTreeItem *copyWithoutChildren() override;
    TestTreeItem *find(const TestParseResult *result) override;
    TestTreeItem *findChild(const TestTreeItem *other) override;
    bool modify(const TestParseResult *result) override;
    TestTreeItem *createParentGroupNode() const override;

    bool canProvideTestConfiguration() const override;
    bool canProvideDebugConfiguration() const override;
    TestConfiguration *testConfiguration() const override;
    TestConfiguration *debugConfiguration() const override;
    QList<TestConfiguration *> getAllTestConfigurations() const override;
    QList<TestConfiguration *> getSelectedTestConfigurations() const override;
    QList<TestConfiguration *> getTestConfigurationsForFile(const Utils::FilePath &fileName) const override;

private:
    QString stateSuffix() const;
    QList<TestConfiguration *> getTestConfigurations(bool ignoreCheckState) const;
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
