/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

    explicit GTestTreeItem(const QString &name = QString(), const QString &filePath = QString(),
                           Type type = Root) : TestTreeItem(name, filePath, type), m_state(Enabled) {}

    QVariant data(int column, int role) const override;
    bool canProvideTestConfiguration() const override { return type() != Root; }
    bool canProvideDebugConfiguration() const override { return type() != Root; }
    TestConfiguration *testConfiguration() const override;
    TestConfiguration *debugConfiguration() const override;
    QList<TestConfiguration *> getAllTestConfigurations() const override;
    QList<TestConfiguration *> getSelectedTestConfigurations() const override;
    TestTreeItem *find(const TestParseResult *result) override;
    bool modify(const TestParseResult *result) override;

    void setStates(TestStates states) { m_state = states; }
    void setState(TestState state) { m_state |= state; }
    TestStates state() const { return m_state; }
    bool modifyTestSetContent(const GTestParseResult *result);
    TestTreeItem *findChildByNameStateAndFile(const QString &name,
                                              GTestTreeItem::TestStates state,
                                              const QString &proFile) const;
    QString nameSuffix() const;

private:
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
