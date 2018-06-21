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

class QtTestTreeItem : public TestTreeItem
{
public:
    explicit QtTestTreeItem(const QString &name = QString(), const QString &filePath = QString(),
                            Type type = Root);

    TestTreeItem *copyWithoutChildren() override;
    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;
    bool canProvideTestConfiguration() const override;
    bool canProvideDebugConfiguration() const override;
    TestConfiguration *testConfiguration() const override;
    TestConfiguration *debugConfiguration() const override;
    QList<TestConfiguration *> getAllTestConfigurations() const override;
    QList<TestConfiguration *> getSelectedTestConfigurations() const override;
    QList<TestConfiguration *> getTestConfigurationsForFile(const Utils::FileName &fileName) const override;
    TestTreeItem *find(const TestParseResult *result) override;
    TestTreeItem *findChild(const TestTreeItem *other) override;
    bool modify(const TestParseResult *result) override;
    void setInherited(bool inherited) { m_inherited = inherited; }
    bool inherited() const { return m_inherited; }
    TestTreeItem *createParentGroupNode() const override;
private:
    TestTreeItem *findChildByNameAndInheritance(const QString &name, bool inherited) const;
    QString nameSuffix() const;
    bool m_inherited = false;
};

class QtTestCodeLocationAndType : public TestCodeLocationAndType
{
public:
    bool m_inherited = false;
};

typedef QVector<QtTestCodeLocationAndType> QtTestCodeLocationList;

} // namespace Internal
} // namespace Autotest
