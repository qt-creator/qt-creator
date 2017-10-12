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

class QuickTestTreeItem : public TestTreeItem
{
public:
    explicit QuickTestTreeItem(const QString &name = QString(), const QString &filePath = QString(),
                               Type type = Root) : TestTreeItem(name, filePath, type) {}

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;
    bool canProvideTestConfiguration() const override;
    bool canProvideDebugConfiguration() const override;
    TestConfiguration *testConfiguration() const override;
    TestConfiguration *debugConfiguration() const override;
    QList<TestConfiguration *> getAllTestConfigurations() const override;
    QList<TestConfiguration *> getSelectedTestConfigurations() const override;
    TestTreeItem *find(const TestParseResult *result) override;
    bool modify(const TestParseResult *result) override;
    bool lessThan(const TestTreeItem *other, SortMode mode) const override;
    QSet<QString> internalTargets() const override;
private:
    TestTreeItem *unnamedQuickTests() const;
};

} // namespace Internal
} // namespace Autotest
